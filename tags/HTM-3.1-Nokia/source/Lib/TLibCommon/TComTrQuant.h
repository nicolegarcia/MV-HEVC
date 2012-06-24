/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComTrQuant.h
    \brief    transform and quantization class (header)
*/

#ifndef __TCOMTRQUANT__
#define __TCOMTRQUANT__

#include "CommonDef.h"
#include "TComYuv.h"
#include "TComDataCU.h"
#include "ContextTables.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constants
// ====================================================================================================================

#define QP_BITS                 15

// ====================================================================================================================
// Type definition
// ====================================================================================================================

typedef struct
{
  Int significantCoeffGroupBits[NUM_SIG_CG_FLAG_CTX][2];
  Int significantBits[NUM_SIG_FLAG_CTX][2];
  Int lastXBits[32];
  Int lastYBits[32];
  Int m_greaterOneBits[NUM_ONE_FLAG_CTX][2];
  Int m_levelAbsBits[NUM_ABS_FLAG_CTX][2];

  Int blockCbpBits[3*NUM_QT_CBF_CTX][2];
  Int blockRootCbpBits[4][2];
  Int scanZigzag[2];            ///< flag for zigzag scan
  Int scanNonZigzag[2];         ///< flag for non zigzag scan
} estBitsSbacStruct;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// QP class
class QpParam
{
public:
  QpParam();
  
  Int m_iQP;
  Int m_iPer;
  Int m_iRem;
  
public:
  Int m_iBits;
    
#if H0736_AVC_STYLE_QP_RANGE
  Void setQpParam( Int qpScaled, Bool bLowpass, SliceType eSliceType )
  {
    m_iQP   = qpScaled;
    m_iPer  = qpScaled / 6;
    m_iRem  = qpScaled % 6;
    m_iBits = QP_BITS + m_iPer;
  }
#else
  Void setQpParam( Int iQP, Bool bLowpass, SliceType eSliceType )
  {
    assert ( iQP >= MIN_QP && iQP <= MAX_QP );
    m_iQP   = iQP;
    
    m_iPer  = (iQP + 6*g_uiBitIncrement)/6;
#if FULL_NBIT
    m_iPer += g_uiBitDepth - 8;
#endif
    m_iRem  = iQP % 6;
    
    m_iBits = QP_BITS + m_iPer;
  }
#endif
  
  Void clear()
  {
    m_iQP   = 0;
    m_iPer  = 0;
    m_iRem  = 0;
    m_iBits = 0;
  }
  
  
  Int per()   const { return m_iPer; }
  Int rem()   const { return m_iRem; }
  Int bits()  const { return m_iBits; }
  
  Int qp() {return m_iQP;}
}; // END CLASS DEFINITION QpParam

/// transform and quantization class
class TComTrQuant
{
public:
  TComTrQuant();
  ~TComTrQuant();
  
  // initialize class
  Void init                 ( UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxTrSize, Int iSymbolMode = 0, UInt *aTable4 = NULL, UInt *aTable8 = NULL, UInt *aTableLastPosVlcIndex=NULL, Bool bUseRDOQ = false,  Bool bEnc = false
#if ADAPTIVE_QP_SELECTION
                       , Bool bUseAdaptQpSelect = false
#endif    
    );
  
  // transform & inverse transform functions
  Void transformNxN( TComDataCU* pcCU, 
                     Pel*        pcResidual, 
                     UInt        uiStride, 
                     TCoeff*     rpcCoeff, 
#if ADAPTIVE_QP_SELECTION
                     Int*&       rpcArlCoeff, 
#endif
                     UInt        uiWidth, 
                     UInt        uiHeight, 
                     UInt&       uiAbsSum, 
                     TextType    eTType, 
                     UInt        uiAbsPartIdx );
#if LOSSLESS_CODING
  Void invtransformNxN( TComDataCU* pcCU, TextType eText, UInt uiMode,Pel* rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight,  Int scalingListType);
#else
  Void invtransformNxN(                   TextType eText, UInt uiMode,Pel*& rpcResidual, UInt uiStride, TCoeff*   pcCoeff, UInt uiWidth, UInt uiHeight, Int scalingListType);
#endif
  Void invRecurTransformNxN ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eTxt, Pel* rpcResidual, UInt uiAddr,   UInt uiStride, UInt uiWidth, UInt uiHeight,
                             UInt uiMaxTrMode,  UInt uiTrMode, TCoeff* rpcCoeff );
  
  // Misc functions
#if H0736_AVC_STYLE_QP_RANGE
  Void setQPforQuant( Int qpy, Bool bLowpass, SliceType eSliceType, TextType eTxtType, Int qpBdOffset, Int chromaQPOffset);
#else
  Void setQPforQuant( Int iQP, Bool bLowpass, SliceType eSliceType, TextType eTxtType, Int Shift);
#endif

#if RDOQ_CHROMA_LAMBDA 
  Void setLambda(Double dLambdaLuma, Double dLambdaChroma) { m_dLambdaLuma = dLambdaLuma; m_dLambdaChroma = dLambdaChroma; }
  Void selectLambda(TextType eTType) { m_dLambda = (eTType == TEXT_LUMA) ? m_dLambdaLuma : m_dLambdaChroma; }
#else
  Void setLambda(Double dLambda) { m_dLambda = dLambda;}
#endif
  Void setRDOQOffset( UInt uiRDOQOffset ) { m_uiRDOQOffset = uiRDOQOffset; }
  
  estBitsSbacStruct* m_pcEstBitsSbac;
  
  static Int      getSigCtxInc     ( TCoeff*                         pcCoeff,
                                     Int                             posX,
                                     Int                             posY,
                                     Int                             blockType,
                                     Int                             width
                                    ,Int                             height
                                    ,TextType                        textureType
                                    );
  static UInt getSigCoeffGroupCtxInc  ( const UInt*                   uiSigCoeffGroupFlag,
                                       const UInt                       uiCGPosX,
                                       const UInt                       uiCGPosY,
#if MULTILEVEL_SIGMAP_EXT
                                       const UInt                     scanIdx,
#endif
                                       Int width, Int height);
#if !REMOVE_INFER_SIGGRP  
  static Bool bothCGNeighboursOne  ( const UInt*                      uiSigCoeffGroupFlag,
                                    const UInt                       uiCGPosX,
                                    const UInt                       uiCGPosY,
#if MULTILEVEL_SIGMAP_EXT
                                    const UInt                       scanIdx,
#endif
                                    Int width, Int height);
#endif
  Void initScalingList                      ();
  Void destroyScalingList                   ();
  Void setErrScaleCoeff    ( UInt list, UInt size, UInt qp, UInt dir);
  double* getErrScaleCoeff ( UInt list, UInt size, UInt qp, UInt dir) {return m_errScale[size][list][qp][dir];};    //!< get Error Scale Coefficent
  Int* getQuantCoeff       ( UInt list, UInt qp, UInt size, UInt dir) {return m_quantCoef[size][list][qp][dir];};   //!< get Quant Coefficent
  Int* getDequantCoeff     ( UInt list, UInt qp, UInt size, UInt dir) {return m_dequantCoef[size][list][qp][dir];}; //!< get DeQuant Coefficent
  Void setUseScalingList   ( Bool bUseScalingList){ m_scalingListEnabledFlag = bUseScalingList; };
  Bool getUseScalingList   (){ return m_scalingListEnabledFlag; };
  Void setFlatScalingList  ();
  Void xsetFlatScalingList ( UInt list, UInt size, UInt qp);
  Void xSetScalingListEnc  ( TComScalingList *scalingList, UInt list, UInt size, UInt qp);
  Void xSetScalingListDec  ( TComScalingList *scalingList, UInt list, UInt size, UInt qp);
  Void setScalingList      ( TComScalingList *scalingList);
  Void setScalingListDec   ( TComScalingList *scalingList);
  Void processScalingListEnc( Int *coeff, Int *quantcoeff, Int quantScales, UInt height, UInt width, UInt ratio, Int sizuNum, UInt dc);
  Void processScalingListDec( Int *coeff, Int *dequantcoeff, Int invQuantScales, UInt height, UInt width, UInt ratio, Int sizuNum, UInt dc);
#if ADAPTIVE_QP_SELECTION
  Void    initSliceQpDelta() ;
  Void    storeSliceQpNext(TComSlice* pcSlice);
  Void    clearSliceARLCnt();
  Int     getQpDelta(Int qp) { return m_qpDelta[qp]; } 
  Int*    getSliceNSamples(){ return m_sliceNsamples ;} 
  Double* getSliceSumC()    { return m_sliceSumC; }
#endif
protected:
#if ADAPTIVE_QP_SELECTION
  Int     m_qpDelta[MAX_QP+1]; 
  Int     m_sliceNsamples[LEVEL_RANGE+1];  
  Double  m_sliceSumC[LEVEL_RANGE+1] ;  
#endif
  Int*    m_plTempCoeff;
  
  QpParam  m_cQP;
#if RDOQ_CHROMA_LAMBDA
  Double   m_dLambdaLuma;
  Double   m_dLambdaChroma;
#endif
  Double   m_dLambda;
  UInt     m_uiRDOQOffset;
  UInt     m_uiMaxTrSize;
  Bool     m_bEnc;
  Bool     m_bUseRDOQ;
#if ADAPTIVE_QP_SELECTION
  Bool     m_bUseAdaptQpSelect;
#endif

  Bool     m_scalingListEnabledFlag;
  Int      *m_quantCoef      [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 4x4
  Int      *m_dequantCoef    [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of dequantization matrix coefficient 4x4
  double   *m_errScale       [SCALING_LIST_SIZE_NUM][SCALING_LIST_NUM][SCALING_LIST_REM_NUM][SCALING_LIST_DIR_NUM]; ///< array of quantization matrix coefficient 4x4
private:
  // forward Transform
  Void xT   ( UInt uiMode,Pel* pResidual, UInt uiStride, Int* plCoeff, Int iWidth, Int iHeight );
  
#if MULTIBITS_DATA_HIDING
  Void signBitHidingHDQ( TComDataCU* pcCU, TCoeff* pQCoef, TCoeff* pCoef, UInt const *scan, Int* deltaU, Int width, Int height );
#endif

  // quantization
  Void xQuant( TComDataCU* pcCU, 
               Int*        pSrc, 
               TCoeff*     pDes, 
#if ADAPTIVE_QP_SELECTION
               Int*&       pArlDes,
#endif
               Int         iWidth, 
               Int         iHeight, 
               UInt&       uiAcSum, 
               TextType    eTType, 
               UInt        uiAbsPartIdx );

  // RDOQ functions
  
  Void           xRateDistOptQuant ( TComDataCU*                     pcCU,
                                     Int*                            plSrcCoeff,
                                     TCoeff*                         piDstCoeff,
#if ADAPTIVE_QP_SELECTION
                                     Int*&                           piArlDstCoeff,
#endif
                                     UInt                            uiWidth,
                                     UInt                            uiHeight,
                                     UInt&                           uiAbsSum,
                                     TextType                        eTType,
                                     UInt                            uiAbsPartIdx );
__inline UInt              xGetCodedLevel  ( Double&                         rd64CodedCost,
                                             Double&                         rd64CodedCost0,
                                             Double&                         rd64CodedCostSig,
                                             Int                             lLevelDouble,
                                             UInt                            uiMaxAbsLevel,
                                             UShort                          ui16CtxNumSig,
                                             UShort                          ui16CtxNumOne,
                                             UShort                          ui16CtxNumAbs,
                                             UShort                          ui16AbsGoRice,
#if RESTRICT_GR1GR2FLAG_NUMBER
                                             UInt                            c1Idx,  
                                             UInt                            c2Idx,  
#endif
                                             Int                             iQBits,
                                             Double                          dTemp,
                                             Bool                            bLast        ) const;
  __inline Double xGetICRateCost   ( UInt                            uiAbsLevel,
                                     UShort                          ui16CtxNumOne,
                                     UShort                          ui16CtxNumAbs,
                                     UShort                          ui16AbsGoRice 
#if RESTRICT_GR1GR2FLAG_NUMBER
                                   , UInt                            c1Idx,
                                     UInt                            c2Idx
#endif
                                     ) const;
#if MULTIBITS_DATA_HIDING
__inline Int xGetICRate  ( UInt                            uiAbsLevel,
                           UShort                          ui16CtxNumOne,
                           UShort                          ui16CtxNumAbs,
                           UShort                          ui16AbsGoRice
#if RESTRICT_GR1GR2FLAG_NUMBER
                         , UInt                            c1Idx,
                           UInt                            c2Idx
#endif
                         ) const;
#endif
  __inline Double xGetRateLast     ( const UInt                      uiPosX,
                                     const UInt                      uiPosY,
                                     const UInt                      uiBlkWdth     ) const;
  __inline Double xGetRateSigCoeffGroup (  UShort                    uiSignificanceCoeffGroup,
                                     UShort                          ui16CtxNumSig ) const;
  __inline Double xGetRateSigCoef (  UShort                          uiSignificance,
                                     UShort                          ui16CtxNumSig ) const;
  __inline Double xGetICost        ( Double                          dRate         ) const; 
  __inline Double xGetIEPRate      (                                               ) const;
  
  
  // dequantization
  Void xDeQuant( const TCoeff* pSrc, Int* pDes, Int iWidth, Int iHeight, Int scalingListType );
  
  // inverse transform
  Void xIT    ( UInt uiMode, Int* plCoef, Pel* pResidual, UInt uiStride, Int iWidth, Int iHeight );
  
};// END CLASS DEFINITION TComTrQuant

//! \}

#endif // __TCOMTRQUANT__
