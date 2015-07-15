/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
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

/** \file     TComDataCU.h
    \brief    CU data structure (header)
    \todo     not all entities are documented
*/

#ifndef __TCOMDATACU__
#define __TCOMDATACU__

#include <algorithm>
#include <vector>

// Include files
#include "CommonDef.h"
#include "TComMotionInfo.h"
#include "TComSlice.h"
#include "TComRdCost.h"
#include "TComPattern.h"

#if H_3D
#include <algorithm>
#include <vector>
#endif

//! \ingroup TLibCommon
//! \{

class TComTU; // forward declaration

static const UInt NUM_MOST_PROBABLE_MODES=3;

#if NH_3D_DBBP
typedef struct _DBBPTmpData
{
  TComMv      acMvd[2][2];          // for two segments and two lists
  TComMvField acMvField[2][2];      // for two segments and two lists
  Int         aiMvpNum[2][2];       // for two segments and two lists
  Int         aiMvpIdx[2][2];       // for two segments and two lists
  UChar       auhInterDir[2];       // for two segments
  Bool        abMergeFlag[2];       // for two segments
  UChar       auhMergeIndex[2];     // for two segments
  PartSize    eVirtualPartSize;
  UInt        uiVirtualPartIndex;
} DbbpTmpData;
#endif


// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU data structure class
class TComDataCU
{
private:

  // -------------------------------------------------------------------------------------------------------------------
  // class pointers
  // -------------------------------------------------------------------------------------------------------------------

  TComPic*      m_pcPic;              ///< picture class pointer
  TComSlice*    m_pcSlice;            ///< slice header pointer

  // -------------------------------------------------------------------------------------------------------------------
  // CU description
  // -------------------------------------------------------------------------------------------------------------------

  UInt          m_ctuRsAddr;          ///< CTU (also known as LCU) address in a slice (Raster-scan address, as opposed to tile-scan/encoding order).
  UInt          m_absZIdxInCtu;       ///< absolute address in a CTU. It's Z scan order
  UInt          m_uiCUPelX;           ///< CU position in a pixel (X)
  UInt          m_uiCUPelY;           ///< CU position in a pixel (Y)
  UInt          m_uiNumPartition;     ///< total number of minimum partitions in a CU
  UChar*        m_puhWidth;           ///< array of widths
  UChar*        m_puhHeight;          ///< array of heights
  UChar*        m_puhDepth;           ///< array of depths
  Int           m_unitSize;           ///< size of a "minimum partition"

  // -------------------------------------------------------------------------------------------------------------------
  // CU data
  // -------------------------------------------------------------------------------------------------------------------

  Bool*          m_skipFlag;           ///< array of skip flags
#if NH_3D_DIS
  Bool*          m_bDISFlag;         
  UChar*         m_ucDISType;
#endif
  Char*          m_pePartSize;         ///< array of partition sizes
  Char*          m_pePredMode;         ///< array of prediction modes
  Char*          m_crossComponentPredictionAlpha[MAX_NUM_COMPONENT]; ///< array of cross-component prediction alpha values
  Bool*          m_CUTransquantBypass;   ///< array of cu_transquant_bypass flags
  Char*          m_phQP;               ///< array of QP values
  UChar*         m_ChromaQpAdj;        ///< array of chroma QP adjustments (indexed). when value = 0, cu_chroma_qp_offset_flag=0; when value>0, indicates cu_chroma_qp_offset_flag=1 and cu_chroma_qp_offset_idx=value-1
  UInt           m_codedChromaQpAdj;
  UChar*         m_puhTrIdx;           ///< array of transform indices
  UChar*         m_puhTransformSkip[MAX_NUM_COMPONENT];///< array of transform skipping flags
  UChar*         m_puhCbf[MAX_NUM_COMPONENT];          ///< array of coded block flags (CBF)
  TComCUMvField  m_acCUMvField[NUM_REF_PIC_LIST_01];    ///< array of motion vectors.
  TCoeff*        m_pcTrCoeff[MAX_NUM_COMPONENT];       ///< array of transform coefficient buffers (0->Y, 1->Cb, 2->Cr)
#if ADAPTIVE_QP_SELECTION
  TCoeff*        m_pcArlCoeff[MAX_NUM_COMPONENT];  // ARL coefficient buffer (0->Y, 1->Cb, 2->Cr)
  Bool           m_ArlCoeffIsAliasedAllocation;  ///< ARL coefficient buffer is an alias of the global buffer and must not be free()'d
#endif

  Pel*           m_pcIPCMSample[MAX_NUM_COMPONENT];    ///< PCM sample buffer (0->Y, 1->Cb, 2->Cr)

  // -------------------------------------------------------------------------------------------------------------------
  // neighbour access variables
  // -------------------------------------------------------------------------------------------------------------------

  TComDataCU*   m_pCtuAboveLeft;      ///< pointer of above-left CTU.
  TComDataCU*   m_pCtuAboveRight;     ///< pointer of above-right CTU.
  TComDataCU*   m_pCtuAbove;          ///< pointer of above CTU.
  TComDataCU*   m_pCtuLeft;           ///< pointer of left CTU
  TComDataCU*   m_apcCUColocated[NUM_REF_PIC_LIST_01];  ///< pointer of temporally colocated CU's for both directions
  TComMvField   m_cMvFieldA;          ///< motion vector of position A
  TComMvField   m_cMvFieldB;          ///< motion vector of position B
  TComMvField   m_cMvFieldC;          ///< motion vector of position C
  TComMv        m_cMvPred;            ///< motion vector predictor

  // -------------------------------------------------------------------------------------------------------------------
  // coding tool information
  // -------------------------------------------------------------------------------------------------------------------

  Bool*         m_pbMergeFlag;        ///< array of merge flags
  UChar*        m_puhMergeIndex;      ///< array of merge candidate indices
#if AMP_MRG
  Bool          m_bIsMergeAMP;
#endif
  UChar*        m_puhIntraDir[MAX_NUM_CHANNEL_TYPE]; // 0-> Luma, 1-> Chroma
  UChar*        m_puhInterDir;        ///< array of inter directions
  Char*         m_apiMVPIdx[NUM_REF_PIC_LIST_01];       ///< array of motion vector predictor candidates
  Char*         m_apiMVPNum[NUM_REF_PIC_LIST_01];       ///< array of number of possible motion vectors predictors
  Bool*         m_pbIPCMFlag;         ///< array of intra_pcm flags
#if NH_3D_NBDV
  DisInfo*      m_pDvInfo;
#endif
#if NH_3D_VSP
  Char*         m_piVSPFlag;          ///< array of VSP flags to indicate whehter a block uses VSP or not  ///< 0: non-VSP; 1: VSP
#endif
#if NH_3D_SPIVMP
  Bool*         m_pbSPIVMPFlag;       ///< array of sub-PU IVMP flags to indicate whehter a block uses sub-PU IVMP ///< 0: non-SPIVMP; 1: SPIVMP
#endif
#if NH_3D_ARP
  UChar*        m_puhARPW;
#endif
#if NH_3D_IC
  Bool*         m_pbICFlag;           ///< array of IC flags
#endif
#if NH_3D_DMM
  Pel*          m_dmmDeltaDC[NUM_DMM][2];
  UInt*         m_dmm1WedgeTabIdx;
#endif
#if NH_3D_SDC_INTRA
  Bool*         m_pbSDCFlag;
  Pel*          m_apSegmentDCOffset[2];
#endif
#if NH_3D_DBBP
  Bool*         m_pbDBBPFlag;        ///< array of DBBP flags
  DbbpTmpData   m_sDBBPTmpData;
#endif
#if NH_3D_MLC
  Bool          m_bAvailableFlagA1;    ///< A1 available flag
  Bool          m_bAvailableFlagB1;    ///< B1 available flag
  Bool          m_bAvailableFlagB0;    ///< B0 available flag
  Bool          m_bAvailableFlagA0;    ///< A0 available flag
  Bool          m_bAvailableFlagB2;    ///< B2 available flag
#endif


  // -------------------------------------------------------------------------------------------------------------------
  // misc. variables
  // -------------------------------------------------------------------------------------------------------------------

  Bool          m_bDecSubCu;          ///< indicates decoder-mode
  Double        m_dTotalCost;         ///< sum of partition RD costs
#if NH_3D_VSO
  Dist          m_uiTotalDistortion;  ///< sum of partition distortion
#else
  Distortion    m_uiTotalDistortion;  ///< sum of partition distortion
#endif
  UInt          m_uiTotalBits;        ///< sum of partition bits
  UInt          m_uiTotalBins;        ///< sum of partition bins
  Char          m_codedQP;
#if NH_3D_MLC
  DisInfo         m_cDefaultDisInfo;    ///< Default disparity information for initializing
  TComMotionCand  m_mergCands[MRG_IVSHIFT+1];   ///< Motion candidates for merge mode
  Int             m_numSpatialCands;
#endif

  UChar*        m_explicitRdpcmMode[MAX_NUM_COMPONENT]; ///< Stores the explicit RDPCM mode for all TUs belonging to this CU

protected:

  /// add possible motion vector predictor candidates
  Bool          xAddMVPCand           ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
  Bool          xAddMVPCandOrder      ( AMVPInfo* pInfo, RefPicList eRefPicList, Int iRefIdx, UInt uiPartUnitIdx, MVP_DIR eDir );
#if NH_3D_VSP
  Bool          xAddVspCand( Int mrgCandIdx, DisInfo* pDInfo, Int& iCount);
#endif
#if NH_3D_IV_MERGE
  Bool          xAddIvMRGCand( Int mrgCandIdx, Int& iCount, Int*   ivCandDir, TComMv* ivCandMv, Int* ivCandRefIdx ); 
#endif

  Void          deriveRightBottomIdx        ( UInt uiPartIdx, UInt& ruiPartIdxRB );
  Bool          xGetColMVP( RefPicList eRefPicList, Int ctuRsAddr, Int uiPartUnitIdx, TComMv& rcMv, Int& riRefIdx
#if NH_3D_TMVP
  ,  Bool bMRG = true
#endif
 );


  /// compute scaling factor from POC difference
#if !NH_3D_ARP
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
#endif

  Void xDeriveCenterIdx( UInt uiPartIdx, UInt& ruiPartIdxCenter );

#if NH_3D_VSP
  Void xSetMvFieldForVSP  ( TComDataCU *cu, TComPicYuv *picRefDepth, TComMv *dv, UInt partAddr, Int width, Int height, Int *shiftLUT, RefPicList refPicList, Int refIdx, Bool isDepth, Int &vspSize );
#endif

public:
  TComDataCU();
  virtual ~TComDataCU();

  // -------------------------------------------------------------------------------------------------------------------
  // create / destroy / initialize / copy
  // -------------------------------------------------------------------------------------------------------------------
#if NH_3D_ARP
  /// compute scaling factor from POC difference
  Int           xGetDistScaleFactor   ( Int iCurrPOC, Int iCurrRefPOC, Int iColPOC, Int iColRefPOC );
#endif 
  Void          create                ( ChromaFormat chromaFormatIDC, UInt uiNumPartition, UInt uiWidth, UInt uiHeight, Bool bDecSubCu, Int unitSize
#if ADAPTIVE_QP_SELECTION
    , TCoeff *pParentARLBuffer = 0
#endif
    );
  Void          destroy               ();

  Void          initCtu               ( TComPic* pcPic, UInt ctuRsAddr );
  Void          initEstData           ( const UInt uiDepth, const Int qp, const Bool bTransquantBypass );
  Void          initSubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth, Int qp );
  Void          setOutsideCUPart      ( UInt uiAbsPartIdx, UInt uiDepth );
#if NH_3D_NBDV
  Void          copyDVInfoFrom        (TComDataCU* pcCU, UInt uiAbsPartIdx);
#endif

  Void          copySubCU             ( TComDataCU* pcCU, UInt uiPartUnitIdx );
  Void          copyInterPredInfoFrom ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList 
#if NH_3D_NBDV
  , Bool bNBDV = false
#endif
);
  Void          copyPartFrom          ( TComDataCU* pcCU, UInt uiPartUnitIdx, UInt uiDepth );

  Void          copyToPic             ( UChar uiDepth );

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU description
  // -------------------------------------------------------------------------------------------------------------------

  TComPic*        getPic              ()                        { return m_pcPic;           }
  const TComPic*  getPic              () const                  { return m_pcPic;           }
  TComSlice*       getSlice           ()                        { return m_pcSlice;         }
  const TComSlice* getSlice           () const                  { return m_pcSlice;         }
  UInt&         getCtuRsAddr          ()                        { return m_ctuRsAddr;       }
  UInt          getCtuRsAddr          () const                  { return m_ctuRsAddr;       }
  UInt          getZorderIdxInCtu     () const                  { return m_absZIdxInCtu;    }
  UInt          getCUPelX             () const                  { return m_uiCUPelX;        }
  UInt          getCUPelY             () const                  { return m_uiCUPelY;        }

  UChar*        getDepth              ()                        { return m_puhDepth;        }
  UChar         getDepth              ( UInt uiIdx ) const      { return m_puhDepth[uiIdx]; }
  Void          setDepth              ( UInt uiIdx, UChar  uh ) { m_puhDepth[uiIdx] = uh;   }

  Void          setDepthSubParts      ( UInt uiDepth, UInt uiAbsPartIdx );
#if NH_3D_VSO
  Void          getPosInPic           ( UInt uiAbsPartIndex, Int& riPosX, Int& riPosY ) const;
#endif

#if NH_3D_ARP
  Void          setSlice              ( TComSlice* pcSlice)     { m_pcSlice = pcSlice;       }
  Void          setPic                ( TComDataCU* pcCU  )     { m_pcPic              = pcCU->getPic(); }
#endif
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for CU data
  // -------------------------------------------------------------------------------------------------------------------

  Char*         getPartitionSize      ()                        { return m_pePartSize;        }
  PartSize      getPartitionSize      ( UInt uiIdx )            { return static_cast<PartSize>( m_pePartSize[uiIdx] ); }
  Void          setPartitionSize      ( UInt uiIdx, PartSize uh){ m_pePartSize[uiIdx] = uh;   }
  Void          setPartSizeSubParts   ( PartSize eMode, UInt uiAbsPartIdx, UInt uiDepth );
  Void          setCUTransquantBypassSubParts( Bool flag, UInt uiAbsPartIdx, UInt uiDepth );

#if NH_3D_DBBP
  Pel*          getVirtualDepthBlock(UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt& uiDepthStride);
#endif
  
  Bool*         getSkipFlag            ()                        { return m_skipFlag;          }
  Bool          getSkipFlag            (UInt idx)                { return m_skipFlag[idx];     }
  Void          setSkipFlag           ( UInt idx, Bool skip)     { m_skipFlag[idx] = skip;   }
  Void          setSkipFlagSubParts   ( Bool skip, UInt absPartIdx, UInt depth );
#if NH_3D_DIS
  Bool*        getDISFlag            ()                         { return m_bDISFlag;          }
  Bool         getDISFlag            ( UInt idx)                { return m_bDISFlag[idx];     }
  Void         setDISFlag            ( UInt idx, Bool bDIS)     { m_bDISFlag[idx] = bDIS;   }
  Void         setDISFlagSubParts    ( Bool bDIS, UInt uiAbsPartIdx, UInt uiDepth );

  UChar*       getDISType            ()                         { return m_ucDISType; }
  UChar        getDISType            ( UInt idx)                { return m_ucDISType[idx];     }
  Void         getDISType            ( UInt idx, UChar ucDISType)     { m_ucDISType[idx] = ucDISType;   }
  Void         setDISTypeSubParts    ( UChar ucDISType, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Char*         getPredictionMode     ()                        { return m_pePredMode;        }
  PredMode      getPredictionMode     ( UInt uiIdx )            { return static_cast<PredMode>( m_pePredMode[uiIdx] ); }
  Void          setPredictionMode     ( UInt uiIdx, PredMode uh){ m_pePredMode[uiIdx] = uh;   }
  Void          setPredModeSubParts   ( PredMode eMode, UInt uiAbsPartIdx, UInt uiDepth );

#if NH_3D_DBBP
  Bool*         getDBBPFlag           ()                        { return m_pbDBBPFlag;               }
  Bool          getDBBPFlag           ( UInt uiIdx )            { return m_pbDBBPFlag[uiIdx];        }
  Void          setDBBPFlag           ( UInt uiIdx, Bool b )    { m_pbDBBPFlag[uiIdx] = b;           }
  Void          setDBBPFlagSubParts   ( Bool bDBBPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  DbbpTmpData*  getDBBPTmpData        () { return &m_sDBBPTmpData; }
#endif

  Char*         getCrossComponentPredictionAlpha( ComponentID compID )             { return m_crossComponentPredictionAlpha[compID];         }
  Char          getCrossComponentPredictionAlpha( UInt uiIdx, ComponentID compID ) { return m_crossComponentPredictionAlpha[compID][uiIdx];  }

  Bool*         getCUTransquantBypass ()                        { return m_CUTransquantBypass;        }
  Bool          getCUTransquantBypass( UInt uiIdx )             { return m_CUTransquantBypass[uiIdx]; }

  UChar*        getWidth              ()                        { return m_puhWidth;          }
  UChar         getWidth              ( UInt uiIdx )            { return m_puhWidth[uiIdx];   }
  Void          setWidth              ( UInt uiIdx, UChar  uh ) { m_puhWidth[uiIdx] = uh;     }

  UChar*        getHeight             ()                        { return m_puhHeight;         }
  UChar         getHeight             ( UInt uiIdx )            { return m_puhHeight[uiIdx];  }
  Void          setHeight             ( UInt uiIdx, UChar  uh ) { m_puhHeight[uiIdx] = uh;    }

  Void          setSizeSubParts       ( UInt uiWidth, UInt uiHeight, UInt uiAbsPartIdx, UInt uiDepth );

  Char*         getQP                 ()                        { return m_phQP;              }
  Char          getQP                 ( UInt uiIdx ) const      { return m_phQP[uiIdx];       }
  Void          setQP                 ( UInt uiIdx, Char value ){ m_phQP[uiIdx] =  value;     }
  Void          setQPSubParts         ( Int qp,   UInt uiAbsPartIdx, UInt uiDepth );
  Int           getLastValidPartIdx   ( Int iAbsPartIdx );
  Char          getLastCodedQP        ( UInt uiAbsPartIdx );
  Void          setQPSubCUs           ( Int qp, UInt absPartIdx, UInt depth, Bool &foundNonZeroCbf );
  Void          setCodedQP            ( Char qp )               { m_codedQP = qp;             }
  Char          getCodedQP            ()                        { return m_codedQP;           }

  UChar*        getChromaQpAdj        ()                        { return m_ChromaQpAdj;       } ///< array of chroma QP adjustments (indexed). when value = 0, cu_chroma_qp_offset_flag=0; when value>0, indicates cu_chroma_qp_offset_flag=1 and cu_chroma_qp_offset_idx=value-1
  UChar         getChromaQpAdj        (Int idx)           const { return m_ChromaQpAdj[idx];  } ///< When value = 0, cu_chroma_qp_offset_flag=0; when value>0, indicates cu_chroma_qp_offset_flag=1 and cu_chroma_qp_offset_idx=value-1
  Void          setChromaQpAdj        (Int idx, UChar val)      { m_ChromaQpAdj[idx] = val;   } ///< When val = 0,   cu_chroma_qp_offset_flag=0; when val>0,   indicates cu_chroma_qp_offset_flag=1 and cu_chroma_qp_offset_idx=val-1
  Void          setChromaQpAdjSubParts( UChar val, Int absPartIdx, Int depth );
  Void          setCodedChromaQpAdj   ( Char qp )               { m_codedChromaQpAdj = qp;    }
  Char          getCodedChromaQpAdj   ()                        { return m_codedChromaQpAdj;  }

  Bool          isLosslessCoded       ( UInt absPartIdx );

  UChar*        getTransformIdx       ()                        { return m_puhTrIdx;          }
  UChar         getTransformIdx       ( UInt uiIdx )            { return m_puhTrIdx[uiIdx];   }
  Void          setTrIdxSubParts      ( UInt uiTrIdx, UInt uiAbsPartIdx, UInt uiDepth );

  UChar*        getTransformSkip      ( ComponentID compID )    { return m_puhTransformSkip[compID];}
  UChar         getTransformSkip      ( UInt uiIdx, ComponentID compID)    { return m_puhTransformSkip[compID][uiIdx];}
  Void          setTransformSkipSubParts  ( UInt useTransformSkip, ComponentID compID, UInt uiAbsPartIdx, UInt uiDepth);
  Void          setTransformSkipSubParts  ( const UInt useTransformSkip[MAX_NUM_COMPONENT], UInt uiAbsPartIdx, UInt uiDepth );

  UChar*        getExplicitRdpcmMode      ( ComponentID component ) { return m_explicitRdpcmMode[component]; }
  UChar         getExplicitRdpcmMode      ( ComponentID component, UInt partIdx ) {return m_explicitRdpcmMode[component][partIdx]; }
  Void          setExplicitRdpcmModePartRange ( UInt rdpcmMode, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes );

  Bool          isRDPCMEnabled         ( UInt uiAbsPartIdx )  { return getSlice()->getSPS()->getSpsRangeExtension().getRdpcmEnabledFlag(isIntra(uiAbsPartIdx) ? RDPCM_SIGNAL_IMPLICIT : RDPCM_SIGNAL_EXPLICIT); }

  Void          setCrossComponentPredictionAlphaPartRange    ( Char alphaValue, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes );
  Void          setTransformSkipPartRange                    ( UInt useTransformSkip, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes );

  UInt          getQuadtreeTULog2MinSizeInCU( UInt uiIdx );

  TComCUMvField* getCUMvField         ( RefPicList e )          { return  &m_acCUMvField[e];  }

  TCoeff*       getCoeff              (ComponentID component)   { return m_pcTrCoeff[component]; }

#if ADAPTIVE_QP_SELECTION
  TCoeff*       getArlCoeff           ( ComponentID component ) { return m_pcArlCoeff[component]; }
#endif
  Pel*          getPCMSample          ( ComponentID component ) { return m_pcIPCMSample[component]; }

  UChar         getCbf    ( UInt uiIdx, ComponentID eType )                  { return m_puhCbf[eType][uiIdx];  }
  UChar*        getCbf    ( ComponentID eType )                              { return m_puhCbf[eType];         }
  UChar         getCbf    ( UInt uiIdx, ComponentID eType, UInt uiTrDepth )  { return ( ( getCbf( uiIdx, eType ) >> uiTrDepth ) & 0x1 ); }
  Void          setCbf    ( UInt uiIdx, ComponentID eType, UChar uh )        { m_puhCbf[eType][uiIdx] = uh;    }
  Void          clearCbf  ( UInt uiIdx, ComponentID eType, UInt uiNumParts );
  UChar         getQtRootCbf          ( UInt uiIdx );

  Void          setCbfSubParts        ( const UInt uiCbf[MAX_NUM_COMPONENT],  UInt uiAbsPartIdx, UInt uiDepth           );
  Void          setCbfSubParts        ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiDepth                    );
  Void          setCbfSubParts        ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth    );

  Void          setCbfPartRange       ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes      );
  Void          bitwiseOrCbfPartRange ( UInt uiCbf, ComponentID compID, UInt uiAbsPartIdx, UInt uiCoveredPartIdxes      );

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for coding tool information
  // -------------------------------------------------------------------------------------------------------------------

  Bool*         getMergeFlag          ()                        { return m_pbMergeFlag;               }
  Bool          getMergeFlag          ( UInt uiIdx )            { return m_pbMergeFlag[uiIdx];        }
  Void          setMergeFlag          ( UInt uiIdx, Bool b )    { m_pbMergeFlag[uiIdx] = b;           }
  Void          setMergeFlagSubParts  ( Bool bMergeFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );

  UChar*        getMergeIndex         ()                        { return m_puhMergeIndex;                         }
  UChar         getMergeIndex         ( UInt uiIdx )            { return m_puhMergeIndex[uiIdx];                  }
  Void          setMergeIndex         ( UInt uiIdx, UInt uiMergeIndex ) { m_puhMergeIndex[uiIdx] = uiMergeIndex;  }
  Void          setMergeIndexSubParts ( UInt uiMergeIndex, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  template <typename T>
  Void          setSubPart            ( T bParameter, T* pbBaseCtu, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );
#if H_3D_VSP || NH_3D_DBBP
  template<typename T>
  Void          setSubPartT           ( T uiParameter, T* puhBaseLCU, UInt uiCUAddr, UInt uiCUDepth, UInt uiPUIdx );
#endif

#if AMP_MRG
  Void          setMergeAMP( Bool b )      { m_bIsMergeAMP = b; }
  Bool          getMergeAMP( )             { return m_bIsMergeAMP; }
#endif

  UChar*        getIntraDir         ( const ChannelType channelType )                   const { return m_puhIntraDir[channelType];         }
  UChar         getIntraDir         ( const ChannelType channelType, const UInt uiIdx ) const { return m_puhIntraDir[channelType][uiIdx];  }

  Void          setIntraDirSubParts ( const ChannelType channelType,
                                      const UInt uiDir,
                                      const UInt uiAbsPartIdx,
                                      const UInt uiDepth );

  UChar*        getInterDir           ()                        { return m_puhInterDir;               }
  UChar         getInterDir           ( UInt uiIdx )            { return m_puhInterDir[uiIdx];        }
  Void          setInterDir           ( UInt uiIdx, UChar  uh ) { m_puhInterDir[uiIdx] = uh;          }
  Void          setInterDirSubParts   ( UInt uiDir,  UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Bool*         getIPCMFlag           ()                        { return m_pbIPCMFlag;               }
  Bool          getIPCMFlag           (UInt uiIdx )             { return m_pbIPCMFlag[uiIdx];        }
  Void          setIPCMFlag           (UInt uiIdx, Bool b )     { m_pbIPCMFlag[uiIdx] = b;           }
  Void          setIPCMFlagSubParts   (Bool bIpcmFlag, UInt uiAbsPartIdx, UInt uiDepth);
#if NH_3D_NBDV
  Void          setDvInfoSubParts     ( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiDepth );
#if H_3D_VSP || NH_3D_DBBP
  Void          setDvInfoSubParts     ( DisInfo cDvInfo, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth);
#endif
  DisInfo*      getDvInfo             ()                        { return m_pDvInfo;                 }
  DisInfo       getDvInfo             (UInt uiIdx)              { return m_pDvInfo[uiIdx];          }
#endif
#if NH_3D_NBDV
  Void          xDeriveRightBottomNbIdx(Int &uiLCUIdxRBNb, Int &uiPartIdxRBNb );
  Bool          xCheckSpatialNBDV (TComDataCU* pcTmpCU, UInt uiIdx, DisInfo* pNbDvInfo, Bool bSearchForMvpDv, IDVInfo* paMvpDvInfo,
                                   UInt uiMvpDvPos
#if NH_3D_NBDV_REF
  , Bool bDepthRefine = false
#endif
  );
  Bool          xGetColDisMV      ( Int currCandPic, RefPicList eRefPicList, Int refidx, Int uiCUAddr, Int uiPartUnitIdx, TComMv& rcMv, Int & iTargetViewIdx, Int & iStartViewIdx );
  Void          getDisMvpCandNBDV ( DisInfo* pDInfo
#if NH_3D_NBDV_REF
   , Bool bDepthRefine = false
#endif
   ); 
   
#if NH_3D_IV_MERGE
  Void          getDispforDepth  ( UInt uiPartIdx, UInt uiPartAddr, DisInfo* cDisp);
#endif

#if H_3D
  Void          getDispforDepth  ( UInt uiPartIdx, UInt uiPartAddr, DisInfo* cDisp);
  Bool          getDispMvPredCan(UInt uiPartIdx, RefPicList eRefPicList, Int iRefIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDis, Int* iPdm );
#endif
#if NH_3D_NBDV_REF
  Pel           getMcpFromDM(TComPicYuv* pcBaseViewDepthPicYuv, TComMv* mv, Int iBlkX, Int iBlkY, Int iWidth, Int iHeight, Int* aiShiftLUT );
  Void          estimateDVFromDM(Int refViewIdx, UInt uiPartIdx, TComPic* picDepth, UInt uiPartAddr, TComMv* cMvPred );
#endif //NH_3D_NBDV_REF
#endif
#if NH_3D_DIS
   Bool          getNeighDepth (UInt uiPartIdx, UInt uiPartAddr, Pel* pNeighDepth, Int index);
#endif
#if  H_3D_FAST_TEXTURE_ENCODING
  Void          getIVNStatus       ( UInt uiPartIdx,  DisInfo* pDInfo, Bool& bIVFMerge,  Int& iIVFMaxD);
#endif
#if NH_3D_SPIVMP
  Void          getSPPara(Int iPUWidth, Int iPUHeight, Int& iNumSP, Int& iNumSPInOneLine, Int& iSPWidth, Int& iSPHeight);
  Void          getSPAbsPartIdx(UInt uiBaseAbsPartIdx, Int iWidth, Int iHeight, Int iPartIdx, Int iNumPartLine, UInt& ruiPartAddr );
  Void          setInterDirSP( UInt uiDir, UInt uiAbsPartIdx, Int iWidth, Int iHeight );
#endif
#if NH_3D_IV_MERGE
  Bool          getInterViewMergeCands          ( UInt uiPartIdx, Int* paiPdmRefIdx, TComMv* pacPdmMv, DisInfo* pDInfo, Int* availableMcDc, Bool bIsDepth           

#if NH_3D_SPIVMP
    , TComMvField* pcMFieldSP, UChar* puhInterDirSP
#endif    
    , Bool bICFlag
    );   
#endif
#if NH_3D_ARP
  UChar*        getARPW            ()                        { return m_puhARPW;               }
  UChar         getARPW            ( UInt uiIdx )            { return m_puhARPW[uiIdx];        }
  Void          setARPW            ( UInt uiIdx, UChar w )   { m_puhARPW[uiIdx] = w;           }
  Void          setARPWSubParts    ( UChar w, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if NH_3D_IC
  Bool*         getICFlag          ()                        { return m_pbICFlag;               }
  Bool          getICFlag          ( UInt uiIdx )            { return m_pbICFlag[uiIdx];        }
  Void          setICFlag          ( UInt uiIdx, Bool  uh )  { m_pbICFlag[uiIdx] = uh;          }
  Void          setICFlagSubParts  ( Bool bICFlag,  UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Bool          isICFlagRequired   ( UInt uiAbsPartIdx );
  Void          getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx = 0, Bool bLCU = false);
#elif NH_3D_VSP
  Void          getPartIndexAndSize( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight, UInt uiAbsPartIdx = 0, Bool bLCU = false);
#else
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for accessing partition information
  // -------------------------------------------------------------------------------------------------------------------

  Void          getPartIndexAndSize   ( UInt uiPartIdx, UInt& ruiPartAddr, Int& riWidth, Int& riHeight ); // This is for use by a leaf/sub CU object only, with no additional AbsPartIdx
#endif
  UChar         getNumPartitions      ( const UInt uiAbsPartIdx = 0 );
  Bool          isFirstAbsZorderIdxInDepth (UInt uiAbsPartIdx, UInt uiDepth);

#if NH_3D_DMM
  Pel*  getDmmDeltaDC                 ( DmmID dmmType, UInt segId )                      { return m_dmmDeltaDC[dmmType][segId];        } 
  Pel   getDmmDeltaDC                 ( DmmID dmmType, UInt segId, UInt uiIdx )          { return m_dmmDeltaDC[dmmType][segId][uiIdx]; } 
  Void  setDmmDeltaDC                 ( DmmID dmmType, UInt segId, UInt uiIdx, Pel val ) { m_dmmDeltaDC[dmmType][segId][uiIdx] = val;  }

  UInt* getDmm1WedgeTabIdx            ()                                                { return m_dmm1WedgeTabIdx;          }        
  UInt  getDmm1WedgeTabIdx            ( UInt uiIdx )                                    { return m_dmm1WedgeTabIdx[uiIdx];   }
  Void  setDmm1WedgeTabIdx            ( UInt uiIdx, UInt tabIdx )                       { m_dmm1WedgeTabIdx[uiIdx] = tabIdx; }
  Void  setDmm1WedgeTabIdxSubParts    ( UInt tabIdx, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if NH_3D_SDC_INTRA
  Bool*         getSDCFlag          ()                        { return m_pbSDCFlag;               }
  Bool          getSDCFlag          ( UInt uiIdx )            { return m_pbSDCFlag[uiIdx];        }
  Void          setSDCFlagSubParts  ( Bool bSDCFlag, UInt uiAbsPartIdx, UInt uiDepth );
  
  Bool          getSDCAvailable             ( UInt uiAbsPartIdx );
  
  Pel*          getSDCSegmentDCOffset( UInt uiSeg ) { return m_apSegmentDCOffset[uiSeg]; }
  Pel           getSDCSegmentDCOffset( UInt uiSeg, UInt uiPartIdx ) { return m_apSegmentDCOffset[uiSeg][uiPartIdx]; }
  Void          setSDCSegmentDCOffset( Pel pOffset, UInt uiSeg, UInt uiPartIdx) { m_apSegmentDCOffset[uiSeg][uiPartIdx] = pOffset; }
#endif
  
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for motion vector
  // -------------------------------------------------------------------------------------------------------------------

  Void          getMvField            ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefPicList, TComMvField& rcMvField );

  Void          fillMvpCand           ( UInt uiPartIdx, UInt uiPartAddr, RefPicList eRefPicList, Int iRefIdx, AMVPInfo* pInfo );
  Bool          isDiffMER             ( Int xN, Int yN, Int xP, Int yP);
  Void          getPartPosition       ( UInt partIdx, Int& xP, Int& yP, Int& nPSW, Int& nPSH);

  Void          setMVPIdx             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPIdx)  { m_apiMVPIdx[eRefPicList][uiIdx] = iMVPIdx;  }
  Int           getMVPIdx             ( RefPicList eRefPicList, UInt uiIdx)               { return m_apiMVPIdx[eRefPicList][uiIdx];     }
  Char*         getMVPIdx             ( RefPicList eRefPicList )                          { return m_apiMVPIdx[eRefPicList];            }

  Void          setMVPNum             ( RefPicList eRefPicList, UInt uiIdx, Int iMVPNum ) { m_apiMVPNum[eRefPicList][uiIdx] = iMVPNum;  }
  Int           getMVPNum             ( RefPicList eRefPicList, UInt uiIdx )              { return m_apiMVPNum[eRefPicList][uiIdx];     }
  Char*         getMVPNum             ( RefPicList eRefPicList )                          { return m_apiMVPNum[eRefPicList];            }

  Void          setMVPIdxSubParts     ( Int iMVPIdx, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMVPNumSubParts     ( Int iMVPNum, RefPicList eRefPicList, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );

  Void          clipMv                ( TComMv&     rcMv     );
#if NH_MV
  Void          checkMvVertRest (TComMv&  rcMv,  RefPicList eRefPicList, int iRefIdx );
#endif
  Void          getMvPredLeft         ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldA.getMv(); }
  Void          getMvPredAbove        ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldB.getMv(); }
  Void          getMvPredAboveRight   ( TComMv&     rcMvPred )   { rcMvPred = m_cMvFieldC.getMv(); }
#if NH_3D
  Void          compressMV            ( Int scale );
#else            
  Void          compressMV            ();
#endif  
  // -------------------------------------------------------------------------------------------------------------------
  // utility functions for neighbouring information
  // -------------------------------------------------------------------------------------------------------------------

  TComDataCU*   getCtuLeft                  () { return m_pCtuLeft;       }
  TComDataCU*   getCtuAbove                 () { return m_pCtuAbove;      }
  TComDataCU*   getCtuAboveLeft             () { return m_pCtuAboveLeft;  }
  TComDataCU*   getCtuAboveRight            () { return m_pCtuAboveRight; }
  TComDataCU*   getCUColocated              ( RefPicList eRefPicList ) { return m_apcCUColocated[eRefPicList]; }
  Bool          CUIsFromSameSlice           ( const TComDataCU *pCU /* Can be NULL */) const { return ( pCU!=NULL && pCU->getSlice()->getSliceCurStartCtuTsAddr() == getSlice()->getSliceCurStartCtuTsAddr() ); }
  Bool          CUIsFromSameTile            ( const TComDataCU *pCU /* Can be NULL */) const;
  Bool          CUIsFromSameSliceAndTile    ( const TComDataCU *pCU /* Can be NULL */) const;
  Bool          CUIsFromSameSliceTileAndWavefrontRow( const TComDataCU *pCU /* Can be NULL */) const;
  Bool          isLastSubCUOfCtu(const UInt absPartIdx);


  TComDataCU*   getPULeft                   ( UInt&  uiLPartUnitIdx,
                                              UInt uiCurrPartUnitIdx,
                                              Bool bEnforceSliceRestriction=true,
                                              Bool bEnforceTileRestriction=true );
  TComDataCU*   getPUAbove                  ( UInt&  uiAPartUnitIdx,
                                              UInt uiCurrPartUnitIdx,
                                              Bool bEnforceSliceRestriction=true,
                                              Bool planarAtCTUBoundary = false,
                                              Bool bEnforceTileRestriction=true );
  TComDataCU*   getPUAboveLeft              ( UInt&  uiALPartUnitIdx, UInt uiCurrPartUnitIdx, Bool bEnforceSliceRestriction=true );

  TComDataCU*   getQpMinCuLeft              ( UInt&  uiLPartUnitIdx , UInt uiCurrAbsIdxInCtu );
  TComDataCU*   getQpMinCuAbove             ( UInt&  uiAPartUnitIdx , UInt uiCurrAbsIdxInCtu );
  Char          getRefQP                    ( UInt   uiCurrAbsIdxInCtu                       );

  /// returns CU and part index of the PU above the top row of the current uiCurrPartUnitIdx of the CU, at a horizontal offset (to the right) of uiPartUnitOffset (in parts)
  TComDataCU*   getPUAboveRight             ( UInt&  uiARPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true );
  /// returns CU and part index of the PU left of the lefthand column of the current uiCurrPartUnitIdx of the CU, at a vertical offset (below) of uiPartUnitOffset (in parts)
  TComDataCU*   getPUBelowLeft              ( UInt&  uiBLPartUnitIdx, UInt uiCurrPartUnitIdx, UInt uiPartUnitOffset = 1, Bool bEnforceSliceRestriction=true );

  Void          deriveLeftRightTopIdx       ( UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdx         ( UInt uiPartIdx, UInt& ruiPartIdxLB );

  Bool          hasEqualMotion              ( UInt uiAbsPartIdx, TComDataCU* pcCandCU, UInt uiCandAbsPartIdx );

#if NH_3D_MLC
  Bool          getAvailableFlagA1() { return m_bAvailableFlagA1;   }
  Bool          getAvailableFlagB1() { return m_bAvailableFlagB1;   }
  Bool          getAvailableFlagB0() { return m_bAvailableFlagB0;   }
  Bool          getAvailableFlagA0() { return m_bAvailableFlagA0;   }
  Bool          getAvailableFlagB2() { return m_bAvailableFlagB2;   }
  Void          initAvailableFlags() { m_bAvailableFlagA1 = m_bAvailableFlagB1 = m_bAvailableFlagB0 = m_bAvailableFlagA0 = m_bAvailableFlagB2 = 0;  }
  Void          buildMCL(TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#if NH_3D_VSP
    , Int* vspFlag
#endif
#if NH_3D_SPIVMP
    , Bool* pbSPIVMPFlag
#endif
    , Int& numValidMergeCand
    );
  Void          xGetInterMergeCandidates      ( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours
#if NH_3D_SPIVMP
  , TComMvField* pcMvFieldSP, UChar* puhInterDirSP
#endif
  , Int& numValidMergeCand, Int mrgCandIdx = -1 );
#endif
  Void          getInterMergeCandidates       ( UInt uiAbsPartIdx, UInt uiPUIdx, TComMvField* pcMFieldNeighbours, UChar* puhInterDirNeighbours, Int& numValidMergeCand, Int mrgCandIdx = -1 );

#if NH_3D_VSP
#if NH_3D_SPIVMP
  Bool*         getSPIVMPFlag        ()                        { return m_pbSPIVMPFlag;          }
  Bool          getSPIVMPFlag        ( UInt uiIdx )            { return m_pbSPIVMPFlag[uiIdx];   }
  Void          setSPIVMPFlag        ( UInt uiIdx, Bool n )     { m_pbSPIVMPFlag[uiIdx] = n;      }
  Void          setSPIVMPFlagSubParts( Bool bSPIVMPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
#endif

  Char*         getVSPFlag        ()                        { return m_piVSPFlag;          }
  Char          getVSPFlag        ( UInt uiIdx )            { return m_piVSPFlag[uiIdx];   }
  Void          setVSPFlag        ( UInt uiIdx, Int n )     { m_piVSPFlag[uiIdx] = n;      }
  Void          setVSPFlagSubParts( Char iVSPFlag, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void          setMvFieldPUForVSP    ( TComDataCU* cu, UInt partAddr, Int width, Int height, RefPicList refPicList, Int refIdx, Int &vspSize );
#endif
  Void          deriveLeftRightTopIdxGeneral  ( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLT, UInt& ruiPartIdxRT );
  Void          deriveLeftBottomIdxGeneral    ( UInt uiAbsPartIdx, UInt uiPartIdx, UInt& ruiPartIdxLB );

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for modes
  // -------------------------------------------------------------------------------------------------------------------

  Bool          isIntra            ( UInt uiPartIdx )  const { return m_pePredMode[ uiPartIdx ] == MODE_INTRA;                                              }
  Bool          isInter            ( UInt uiPartIdx )  const { return m_pePredMode[ uiPartIdx ] == MODE_INTER;                                              }
  Bool          isSkipped          ( UInt uiPartIdx );                                                     ///< returns true, if the partiton is skipped
  Bool          isBipredRestriction( UInt puIdx );

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for symbol prediction (most probable / mode conversion)
  // -------------------------------------------------------------------------------------------------------------------

  UInt          getIntraSizeIdx                 ( UInt uiAbsPartIdx                                       );

  Void          getAllowedChromaDir             ( UInt uiAbsPartIdx, UInt* uiModeList );
  Void          getIntraDirPredictor            ( UInt uiAbsPartIdx, Int uiIntraDirPred[NUM_MOST_PROBABLE_MODES], const ComponentID compID, Int* piMode = NULL );

  // -------------------------------------------------------------------------------------------------------------------
  // member functions for SBAC context
  // -------------------------------------------------------------------------------------------------------------------

  UInt          getCtxSplitFlag                 ( UInt   uiAbsPartIdx, UInt uiDepth                   );
  UInt          getCtxQtCbf                     ( TComTU &rTu, const ChannelType chType );

  UInt          getCtxSkipFlag                  ( UInt   uiAbsPartIdx                                 );
  UInt          getCtxInterDir                  ( UInt   uiAbsPartIdx                                 );
#if NH_3D_ARP
  UInt          getCTXARPWFlag                  ( UInt   uiAbsPartIdx                                 );
#endif  

  UInt&         getTotalBins            ()                            { return m_uiTotalBins;                              }
  // -------------------------------------------------------------------------------------------------------------------
  // member functions for RD cost storage
  // -------------------------------------------------------------------------------------------------------------------

  Double&       getTotalCost()                  { return m_dTotalCost;        }
#if NH_3D_VSO
  Dist&         getTotalDistortion()            { return m_uiTotalDistortion; }
#else
  Distortion&   getTotalDistortion()            { return m_uiTotalDistortion; }
#endif
  UInt&         getTotalBits()                  { return m_uiTotalBits;       }
  UInt&         getTotalNumPart()               { return m_uiNumPartition;    }

  UInt          getCoefScanIdx(const UInt uiAbsPartIdx, const UInt uiWidth, const UInt uiHeight, const ComponentID compID) const ;
};

namespace RasterAddress
{
  /** Check whether 2 addresses point to the same column
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualCol( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    // addrA % numUnitsPerRow == addrB % numUnitsPerRow
    return (( addrA ^ addrB ) &  ( numUnitsPerRow - 1 ) ) == 0;
  }

  /** Check whether 2 addresses point to the same row
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualRow( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    // addrA / numUnitsPerRow == addrB / numUnitsPerRow
    return (( addrA ^ addrB ) &~ ( numUnitsPerRow - 1 ) ) == 0;
  }

  /** Check whether 2 addresses point to the same row or column
   * \param addrA          First address in raster scan order
   * \param addrB          Second address in raters scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isEqualRowOrCol( Int addrA, Int addrB, Int numUnitsPerRow )
  {
    return isEqualCol( addrA, addrB, numUnitsPerRow ) | isEqualRow( addrA, addrB, numUnitsPerRow );
  }

  /** Check whether one address points to the first column
   * \param addr           Address in raster scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isZeroCol( Int addr, Int numUnitsPerRow )
  {
    // addr % numUnitsPerRow == 0
    return ( addr & ( numUnitsPerRow - 1 ) ) == 0;
  }

  /** Check whether one address points to the first row
   * \param addr           Address in raster scan order
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool isZeroRow( Int addr, Int numUnitsPerRow )
  {
    // addr / numUnitsPerRow == 0
    return ( addr &~ ( numUnitsPerRow - 1 ) ) == 0;
  }

  /** Check whether one address points to a column whose index is smaller than a given value
   * \param addr           Address in raster scan order
   * \param val            Given column index value
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool lessThanCol( Int addr, Int val, Int numUnitsPerRow )
  {
    // addr % numUnitsPerRow < val
    return ( addr & ( numUnitsPerRow - 1 ) ) < val;
  }

  /** Check whether one address points to a row whose index is smaller than a given value
   * \param addr           Address in raster scan order
   * \param val            Given row index value
   * \param numUnitsPerRow Number of units in a row
   * \return Result of test
   */
  static inline Bool lessThanRow( Int addr, Int val, Int numUnitsPerRow )
  {
    // addr / numUnitsPerRow < val
    return addr < val * numUnitsPerRow;
  }
}

//! \}

#endif
