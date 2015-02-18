/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
* Copyright (c) 2010-2014, ITU/ISO/IEC
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

/** \file     TDecCAVLC.h
    \brief    CAVLC decoder class (header)
*/

#ifndef __TDECCAVLC__
#define __TDECCAVLC__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TDecEntropy.h"
#include "SyntaxElementParser.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CAVLC decoder class
class TDecCavlc : public SyntaxElementParser, public TDecEntropyIf
{
public:
  TDecCavlc();
  virtual ~TDecCavlc();
  
protected:
  void  parseShortTermRefPicSet            (TComSPS* pcSPS, TComReferencePictureSet* pcRPS, Int idx);
  
#if H_3D
  Int**    m_aaiTempScale;
  Int**    m_aaiTempOffset;
#endif

public:

  /// rest entropy coder by intial QP and IDC in CABAC
  Void  resetEntropy        ( TComSlice* /*pcSlice*/  )     { assert(0); };
  Void  setBitstream        ( TComInputBitstream* p )   { m_pcBitstream = p; }
  Void  parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void  parseQtCbf          ( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiDepth );
  Void  parseQtRootCbf      ( UInt uiAbsPartIdx, UInt& uiQtRootCbf );
  Void  parseVPS            ( TComVPS* pcVPS );
#if H_MV
  Void  parseVPSExtension   ( TComVPS* pcVPS ); 
  Void  parseRepFormat      ( Int i, TComRepFormat* curRepFormat, TComRepFormat* prevRepFormat );
  Void  parseVPSVUI         ( TComVPS* pcVPS );
  Void  parseVideoSignalInfo( TComVideoSignalInfo* pcVideoSignalInfo ); 
  Void  parseDpbSize        ( TComVPS* pcVPS ); 
  Void  parseVpsVuiBspHrdParameters( TComVPS* pcVPS ); 
#endif

#if H_MV
  Void  parseSPSExtension   ( TComSPS* pcSPS );  
#endif
#if H_3D
  Void  parseVPS3dExtension  ( TComVPS* pcVPS ); 
  Void  parseSPS3dExtension ( TComSPS* pcSPS );
  Void  parseSPS            ( TComSPS* pcSPS );
#else
  Void  parseSPS            ( TComSPS* pcSPS );
#endif

#if H_MV
  Void  parsePPSMultilayerExtension( TComPPS* pcPPS );
#endif


#if H_3D
  Void  parsePPS            ( TComPPS* pcPPS, TComVPS* pcVPS );
  Void  parsePPSExtension   ( TComPPS* pcPPS, TComVPS* pcVPS );
#else
  Void  parsePPS            ( TComPPS* pcPPS);
#endif

  Void  parseVUI            ( TComVUI* pcVUI, TComSPS* pcSPS );
  Void  parseSEI            ( SEIMessages& );
  Void  parsePTL            ( TComPTL *rpcPTL, Bool profilePresentFlag, Int maxNumSubLayersMinus1 );
  Void  parseProfileTier    (ProfileTierLevel *ptl);
  Void  parseHrdParameters  (TComHRD *hrd, Bool cprms_present_flag, UInt tempLevelHigh);
#if H_MV
  Void  parseSliceHeader    ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager, Int targetOlsIdx);
#else
  Void  parseSliceHeader    ( TComSlice*& rpcSlice, ParameterSetManagerDecoder *parameterSetManager);
#endif
  Void  parseTerminatingBit ( UInt& ruiBit );
  
  Void  parseMVPIdx         ( Int& riMVPIdx );
  
  Void  parseSkipFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if H_3D_SINGLE_DEPTH
  Void  parseSingleDepthMode        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif  
  Void  parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex      ( TComDataCU* pcCU, UInt& ruiMergeIndex );
#if H_3D_ARP 
  Void parseARPW            ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_IC
  Void  parseICFlag         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_INTER_SDC
  Void  parseDeltaDC        ( TComDataCU* pcCU, UInt absPartIdx, UInt depth );
  Void  parseSDCFlag        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
#if H_3D_DBBP
  Void  parseDBBPFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parseSplitFlag       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePartSize        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePredMode        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirLumaAng ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseIntraDirChroma  ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  
  Void parseInterDir        ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx );
  Void parseRefFrmIdx       ( TComDataCU* pcCU, Int& riRefFrmIdx,  RefPicList eRefList );
  Void parseMvd             ( TComDataCU* pcCU, UInt uiAbsPartAddr,UInt uiPartIdx,    UInt uiDepth, RefPicList eRefList );
  
  Void parseDeltaQP         ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseCoeffNxN        ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType );
  Void parseTransformSkipFlags ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt width, UInt height, UInt uiDepth, TextType eTType);

  Void parseIPCMInfo        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void updateContextTables  ( SliceType /*eSliceType*/, Int /*iQp*/ ) { return; }

  Void xParsePredWeightTable ( TComSlice* pcSlice );
  Void  parseScalingList               ( TComScalingList* scalingList );
  Void xDecodeScalingList    ( TComScalingList *scalingList, UInt sizeId, UInt listId);
protected:
  Bool  xMoreRbspData();
};

//! \}

#endif // !defined(AFX_TDECCAVLC_H__9732DD64_59B0_4A41_B29E_1A5B18821EAD__INCLUDED_)

