/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
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

/** \file     TAppEncTop.cpp
    \brief    Encoder application class
*/

#include <list>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppEncTop.h"
#include "TLibEncoder/AnnexBwrite.h"

using namespace std;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncTop::TAppEncTop()
{
#if !H_MV
  m_iFrameRcvd = 0;
#endif
  m_totalBytes = 0;
  m_essentialBytes = 0;
}

TAppEncTop::~TAppEncTop()
{
}

Void TAppEncTop::xInitLibCfg()
{
#if H_MV
  TComVPS& vps = m_vps;   
#else
  TComVPS vps;
#endif
  
#if H_MV
  Int maxTempLayer = -1; 
  for (Int j = 0; j < m_numberOfLayers; j++)
  {
    maxTempLayer = max( m_maxTempLayerMvc[ j ], maxTempLayer ); 
  }

  vps.setMaxTLayers                       ( maxTempLayer );
  if ( maxTempLayer )
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayersMinus1( m_numberOfLayers - 1);
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    Int maxNumReOrderPics  = 0; 
    Int maxDecPicBuffering = 0;
    for (Int j = 0; j < m_numberOfLayers; j++)
    {
      maxNumReOrderPics  = max( maxNumReOrderPics,  m_numReorderPicsMvc    [ j ][ i ] );     
      maxDecPicBuffering = max( maxDecPicBuffering, m_maxDecPicBufferingMvc[ j ][ i ] );     
    }

    vps.setNumReorderPics                 ( maxNumReOrderPics  ,i );
    vps.setMaxDecPicBuffering             ( maxDecPicBuffering ,i );
  }
#else
  vps.setMaxTLayers                       ( m_maxTempLayer );
  if (m_maxTempLayer == 1)
  {
    vps.setTemporalNestingFlag(true);
  }
  vps.setMaxLayers                        ( 1 );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    vps.setNumReorderPics                 ( m_numReorderPics[i], i );
    vps.setMaxDecPicBuffering             ( m_maxDecPicBuffering[i], i );
  }
#endif
#if H_MV
  xSetLayerIds             ( vps );   
  xSetDimensionIdAndLength ( vps );
  xSetDependencies( vps );
  xSetProfileTierLevel     ( vps ); 
  xSetRepFormat            ( vps ); 
  xSetLayerSets            ( vps ); 
  xSetVPSVUI               ( vps ); 
#if H_3D
  xSetVPSExtension2        ( vps ); 
  m_ivPicLists.setVPS      ( &vps ); 
#endif


  for(Int layerIdInVps = 0; layerIdInVps < m_numberOfLayers; layerIdInVps++)
  {
    m_frameRcvd                 .push_back(0);
    m_acTEncTopList             .push_back(new TEncTop); 
    m_acTVideoIOYuvInputFileList.push_back(new TVideoIOYuv);
    m_acTVideoIOYuvReconFileList.push_back(new TVideoIOYuv);
    m_picYuvRec                 .push_back(new TComList<TComPicYuv*>) ;
    m_ivPicLists.push_back( m_acTEncTopList[ layerIdInVps ]->getListPic()  ); 
    TEncTop& m_cTEncTop = *m_acTEncTopList[ layerIdInVps ];  // It is not a member, but this name helps avoiding code duplication !!!

    Int layerId = vps.getLayerIdInNuh( layerIdInVps );
    m_cTEncTop.setLayerIdInVps                 ( layerIdInVps ); 
    m_cTEncTop.setLayerId                      ( layerId );    
    m_cTEncTop.setViewId                       ( vps.getViewId      (  layerId ) );
    m_cTEncTop.setViewIndex                    ( vps.getViewIndex   (  layerId ) );
#if H_3D
    Bool isDepth = ( vps.getDepthId     ( layerId ) != 0 ) ;
    m_cTEncTop.setIsDepth                      ( isDepth );
    //====== Camera Parameters =========
    m_cTEncTop.setCameraParameters             ( &m_cCameraData );     
#if QC_DEPTH_IV_MRG_F0125
    m_cTEncTop.setCamParPrecision              ( m_cCameraData.getCamParsCodedPrecision  () );
    m_cTEncTop.setCamParInSliceHeader          ( m_cCameraData.getVaryingCameraParameters() );
    m_cTEncTop.setCodedScale                   ( m_cCameraData.getCodedScale             () );
    m_cTEncTop.setCodedOffset                  ( m_cCameraData.getCodedOffset            () );
#else
    m_cTEncTop.setCamParPrecision              ( isDepth ? false : m_cCameraData.getCamParsCodedPrecision  () );
    m_cTEncTop.setCamParInSliceHeader          ( isDepth ? 0     : m_cCameraData.getVaryingCameraParameters() );
    m_cTEncTop.setCodedScale                   ( isDepth ? 0     : m_cCameraData.getCodedScale             () );
    m_cTEncTop.setCodedOffset                  ( isDepth ? 0     : m_cCameraData.getCodedOffset            () );
#endif
#if H_3D_VSO
    //====== VSO =========
    m_cTEncTop.setRenderModelParameters        ( &m_cRenModStrParser ); 
    m_cTEncTop.setForceLambdaScaleVSO          ( isDepth ? m_bForceLambdaScaleVSO : false );
    m_cTEncTop.setLambdaScaleVSO               ( isDepth ? m_dLambdaScaleVSO      : 1     );
    m_cTEncTop.setVSOMode                      ( isDepth ? m_uiVSOMode            : 0     );

    m_cTEncTop.setAllowNegDist                 ( isDepth ? m_bAllowNegDist        : false );

    // SAIT_VSO_EST_A0033
    m_cTEncTop.setUseEstimatedVSD              ( isDepth ? m_bUseEstimatedVSD     : false );

    // LGE_WVSO_A0119
    m_cTEncTop.setUseWVSO                      ( isDepth ? m_bUseWVSO             : false );   
    m_cTEncTop.setVSOWeight                    ( isDepth ? m_iVSOWeight           : 0     );
    m_cTEncTop.setVSDWeight                    ( isDepth ? m_iVSDWeight           : 0     );
    m_cTEncTop.setDWeight                      ( isDepth ? m_iDWeight             : 0     );
#endif // H_3D_VSO
#if H_3D_ARP
    //====== Advanced Inter-view Residual Prediction =========
    m_cTEncTop.setUseAdvRP                     ( ( isDepth || 0==layerIdInVps ) ? 0 : m_uiUseAdvResPred );
    m_cTEncTop.setARPStepNum                   ( ( isDepth || 0==layerIdInVps ) ? 1 : H_3D_ARP_WFNR     );
#endif
#if H_3D_IC
    m_cTEncTop.setUseIC                        ( vps.getViewIndex( layerId ) == 0 ? false : m_abUseIC[isDepth ? 1 : 0] );
#endif
  //========== Depth intra modes ==========
#if H_3D_DIM
    m_cTEncTop.setUseDMM                       ( isDepth ? m_useDMM               : false );
    m_cTEncTop.setUseRBC                       ( isDepth ? m_useRBC               : false );
    m_cTEncTop.setUseSDC                       ( isDepth ? m_useSDC               : false );
    m_cTEncTop.setUseDLT                       ( isDepth ? m_useDLT               : false );
#endif
#if H_3D_QTLPC
    m_cTEncTop.setUseQTL                       ( isDepth ? m_bUseQTL               : false );
    m_cTEncTop.setUsePC                        ( isDepth ? m_bUsePC                : false );
#endif
    //====== Depth Inter SDC =========
#if H_3D_INTER_SDC
    m_cTEncTop.setInterSDCEnable               ( isDepth ? m_bDepthInterSDCFlag    : false );
#endif
#if SEC_MPI_ENABLING_MERGE_F0150
    m_cTEncTop.setUseMPI               ( isDepth ? m_bMPIFlag    : false );
#endif
#endif // H_3D

    m_cTEncTop.setIvPicLists                   ( &m_ivPicLists ); 
#endif // H_MV
  m_cTEncTop.setVPS(&vps);

  m_cTEncTop.setProfile(m_profile);
  m_cTEncTop.setLevel(m_levelTier, m_level);
  m_cTEncTop.setProgressiveSourceFlag(m_progressiveSourceFlag);
  m_cTEncTop.setInterlacedSourceFlag(m_interlacedSourceFlag);
  m_cTEncTop.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
  m_cTEncTop.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);
  
  m_cTEncTop.setFrameRate                    ( m_iFrameRate );
  m_cTEncTop.setFrameSkip                    ( m_FrameSkip );
  m_cTEncTop.setSourceWidth                  ( m_iSourceWidth );
  m_cTEncTop.setSourceHeight                 ( m_iSourceHeight );
  m_cTEncTop.setConformanceWindow            ( m_confLeft, m_confRight, m_confTop, m_confBottom );
  m_cTEncTop.setFramesToBeEncoded            ( m_framesToBeEncoded );
  
  //====== Coding Structure ========
  m_cTEncTop.setIntraPeriod                  ( m_iIntraPeriod );
  m_cTEncTop.setDecodingRefreshType          ( m_iDecodingRefreshType );
  m_cTEncTop.setGOPSize                      ( m_iGOPSize );
#if H_MV
m_cTEncTop.setGopList                      ( m_GOPListMvc[layerIdInVps] );
  m_cTEncTop.setExtraRPSs                    ( m_extraRPSsMvc[layerIdInVps] );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics             ( m_numReorderPicsMvc[layerIdInVps][i], i );
    m_cTEncTop.setMaxDecPicBuffering         ( m_maxDecPicBufferingMvc[layerIdInVps][i], i );
  }
#else
  m_cTEncTop.setGopList                      ( m_GOPList );
  m_cTEncTop.setExtraRPSs                    ( m_extraRPSs );
  for(Int i = 0; i < MAX_TLAYER; i++)
  {
    m_cTEncTop.setNumReorderPics             ( m_numReorderPics[i], i );
    m_cTEncTop.setMaxDecPicBuffering         ( m_maxDecPicBuffering[i], i );
  }
#endif
  for( UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop )
  {
    m_cTEncTop.setLambdaModifier( uiLoop, m_adLambdaModifier[ uiLoop ] );
  }
#if H_MV
  m_cTEncTop.setQP                           ( m_iQP[layerIdInVps] );
#else
  m_cTEncTop.setQP                           ( m_iQP );
#endif

  m_cTEncTop.setPad                          ( m_aiPad );

#if H_MV
  m_cTEncTop.setMaxTempLayer                 ( m_maxTempLayerMvc[layerIdInVps] );
#else
  m_cTEncTop.setMaxTempLayer                 ( m_maxTempLayer );
#endif
  m_cTEncTop.setUseAMP( m_enableAMP );
  
  //===== Slice ========
  
  //====== Loop/Deblock Filter ========
#if H_MV
  m_cTEncTop.setLoopFilterDisable            ( m_bLoopFilterDisable[layerIdInVps]);
#else
  m_cTEncTop.setLoopFilterDisable            ( m_bLoopFilterDisable       );
#endif
  m_cTEncTop.setLoopFilterOffsetInPPS        ( m_loopFilterOffsetInPPS );
  m_cTEncTop.setLoopFilterBetaOffset         ( m_loopFilterBetaOffsetDiv2  );
  m_cTEncTop.setLoopFilterTcOffset           ( m_loopFilterTcOffsetDiv2    );
  m_cTEncTop.setDeblockingFilterControlPresent( m_DeblockingFilterControlPresent);
  m_cTEncTop.setDeblockingFilterMetric       ( m_DeblockingFilterMetric );

  //====== Motion search ========
  m_cTEncTop.setFastSearch                   ( m_iFastSearch  );
  m_cTEncTop.setSearchRange                  ( m_iSearchRange );
  m_cTEncTop.setBipredSearchRange            ( m_bipredSearchRange );

  //====== Quality control ========
  m_cTEncTop.setMaxDeltaQP                   ( m_iMaxDeltaQP  );
  m_cTEncTop.setMaxCuDQPDepth                ( m_iMaxCuDQPDepth  );

  m_cTEncTop.setChromaCbQpOffset               ( m_cbQpOffset     );
  m_cTEncTop.setChromaCrQpOffset            ( m_crQpOffset  );

#if ADAPTIVE_QP_SELECTION
  m_cTEncTop.setUseAdaptQpSelect             ( m_bUseAdaptQpSelect   );
#endif

  Int lowestQP;
  lowestQP =  - 6*(g_bitDepthY - 8); // XXX: check

#if H_MV
  if ((m_iMaxDeltaQP == 0 ) && (m_iQP[layerIdInVps] == lowestQP) && (m_useLossless == true))
#else
  if ((m_iMaxDeltaQP == 0 ) && (m_iQP == lowestQP) && (m_useLossless == true))
#endif
  {
    m_bUseAdaptiveQP = false;
  }
  m_cTEncTop.setUseAdaptiveQP                ( m_bUseAdaptiveQP  );
  m_cTEncTop.setQPAdaptationRange            ( m_iQPAdaptationRange );
  
  //====== Tool list ========
  m_cTEncTop.setUseSBACRD                    ( m_bUseSBACRD   );
  m_cTEncTop.setDeltaQpRD                    ( m_uiDeltaQpRD  );
  m_cTEncTop.setUseASR                       ( m_bUseASR      );
  m_cTEncTop.setUseHADME                     ( m_bUseHADME    );
  m_cTEncTop.setUseLossless                  ( m_useLossless );
#if H_MV
  m_cTEncTop.setdQPs                         ( m_aidQP[layerIdInVps]   );
#else
  m_cTEncTop.setdQPs                         ( m_aidQP        );
#endif
  m_cTEncTop.setUseRDOQ                      ( m_useRDOQ     );
  m_cTEncTop.setUseRDOQTS                    ( m_useRDOQTS   );
  m_cTEncTop.setRDpenalty                 ( m_rdPenalty );
  m_cTEncTop.setQuadtreeTULog2MaxSize        ( m_uiQuadtreeTULog2MaxSize );
  m_cTEncTop.setQuadtreeTULog2MinSize        ( m_uiQuadtreeTULog2MinSize );
  m_cTEncTop.setQuadtreeTUMaxDepthInter      ( m_uiQuadtreeTUMaxDepthInter );
  m_cTEncTop.setQuadtreeTUMaxDepthIntra      ( m_uiQuadtreeTUMaxDepthIntra );
  m_cTEncTop.setUseFastEnc                   ( m_bUseFastEnc  );
  m_cTEncTop.setUseEarlyCU                   ( m_bUseEarlyCU  ); 
  m_cTEncTop.setUseFastDecisionForMerge      ( m_useFastDecisionForMerge  );
  m_cTEncTop.setUseCbfFastMode            ( m_bUseCbfFastMode  );
  m_cTEncTop.setUseEarlySkipDetection            ( m_useEarlySkipDetection );

  m_cTEncTop.setUseTransformSkip             ( m_useTransformSkip      );
  m_cTEncTop.setUseTransformSkipFast         ( m_useTransformSkipFast  );
  m_cTEncTop.setUseConstrainedIntraPred      ( m_bUseConstrainedIntraPred );
  m_cTEncTop.setPCMLog2MinSize          ( m_uiPCMLog2MinSize);
  m_cTEncTop.setUsePCM                       ( m_usePCM );
  m_cTEncTop.setPCMLog2MaxSize               ( m_pcmLog2MaxSize);
  m_cTEncTop.setMaxNumMergeCand              ( m_maxNumMergeCand );
  

  //====== Weighted Prediction ========
  m_cTEncTop.setUseWP                   ( m_useWeightedPred      );
  m_cTEncTop.setWPBiPred                ( m_useWeightedBiPred   );
  //====== Parallel Merge Estimation ========
  m_cTEncTop.setLog2ParallelMergeLevelMinus2 ( m_log2ParallelMergeLevel - 2 );

  //====== Slice ========
  m_cTEncTop.setSliceMode               ( m_sliceMode                );
  m_cTEncTop.setSliceArgument           ( m_sliceArgument            );

  //====== Dependent Slice ========
  m_cTEncTop.setSliceSegmentMode        ( m_sliceSegmentMode         );
  m_cTEncTop.setSliceSegmentArgument    ( m_sliceSegmentArgument     );
  Int iNumPartInCU = 1<<(m_uiMaxCUDepth<<1);
  if(m_sliceSegmentMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceSegmentArgument ( m_sliceSegmentArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_LCU)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument * iNumPartInCU );
  }
  if(m_sliceMode==FIXED_NUMBER_OF_TILES)
  {
    m_cTEncTop.setSliceArgument ( m_sliceArgument );
  }
  
  if(m_sliceMode == 0 )
  {
    m_bLFCrossSliceBoundaryFlag = true;
  }
  m_cTEncTop.setLFCrossSliceBoundaryFlag( m_bLFCrossSliceBoundaryFlag );
#if H_MV
  m_cTEncTop.setUseSAO ( m_bUseSAO[layerIdInVps] );
#else
  m_cTEncTop.setUseSAO ( m_bUseSAO );
#endif
  m_cTEncTop.setMaxNumOffsetsPerPic (m_maxNumOffsetsPerPic);

  m_cTEncTop.setSaoLcuBoundary (m_saoLcuBoundary);
  m_cTEncTop.setSaoLcuBasedOptimization (m_saoLcuBasedOptimization);
  m_cTEncTop.setPCMInputBitDepthFlag  ( m_bPCMInputBitDepthFlag); 
  m_cTEncTop.setPCMFilterDisableFlag  ( m_bPCMFilterDisableFlag); 

  m_cTEncTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
  m_cTEncTop.setRecoveryPointSEIEnabled( m_recoveryPointSEIEnabled );
  m_cTEncTop.setBufferingPeriodSEIEnabled( m_bufferingPeriodSEIEnabled );
  m_cTEncTop.setPictureTimingSEIEnabled( m_pictureTimingSEIEnabled );
  m_cTEncTop.setToneMappingInfoSEIEnabled                 ( m_toneMappingInfoSEIEnabled );
  m_cTEncTop.setTMISEIToneMapId                           ( m_toneMapId );
  m_cTEncTop.setTMISEIToneMapCancelFlag                   ( m_toneMapCancelFlag );
  m_cTEncTop.setTMISEIToneMapPersistenceFlag              ( m_toneMapPersistenceFlag );
  m_cTEncTop.setTMISEICodedDataBitDepth                   ( m_toneMapCodedDataBitDepth );
  m_cTEncTop.setTMISEITargetBitDepth                      ( m_toneMapTargetBitDepth );
  m_cTEncTop.setTMISEIModelID                             ( m_toneMapModelId );
  m_cTEncTop.setTMISEIMinValue                            ( m_toneMapMinValue );
  m_cTEncTop.setTMISEIMaxValue                            ( m_toneMapMaxValue );
  m_cTEncTop.setTMISEISigmoidMidpoint                     ( m_sigmoidMidpoint );
  m_cTEncTop.setTMISEISigmoidWidth                        ( m_sigmoidWidth );
  m_cTEncTop.setTMISEIStartOfCodedInterva                 ( m_startOfCodedInterval );
  m_cTEncTop.setTMISEINumPivots                           ( m_numPivots );
  m_cTEncTop.setTMISEICodedPivotValue                     ( m_codedPivotValue );
  m_cTEncTop.setTMISEITargetPivotValue                    ( m_targetPivotValue );
  m_cTEncTop.setTMISEICameraIsoSpeedIdc                   ( m_cameraIsoSpeedIdc );
  m_cTEncTop.setTMISEICameraIsoSpeedValue                 ( m_cameraIsoSpeedValue );
  m_cTEncTop.setTMISEIExposureCompensationValueSignFlag   ( m_exposureCompensationValueSignFlag );
  m_cTEncTop.setTMISEIExposureCompensationValueNumerator  ( m_exposureCompensationValueNumerator );
  m_cTEncTop.setTMISEIExposureCompensationValueDenomIdc   ( m_exposureCompensationValueDenomIdc );
  m_cTEncTop.setTMISEIRefScreenLuminanceWhite             ( m_refScreenLuminanceWhite );
  m_cTEncTop.setTMISEIExtendedRangeWhiteLevel             ( m_extendedRangeWhiteLevel );
  m_cTEncTop.setTMISEINominalBlackLevelLumaCodeValue      ( m_nominalBlackLevelLumaCodeValue );
  m_cTEncTop.setTMISEINominalWhiteLevelLumaCodeValue      ( m_nominalWhiteLevelLumaCodeValue );
  m_cTEncTop.setTMISEIExtendedWhiteLevelLumaCodeValue     ( m_extendedWhiteLevelLumaCodeValue );
  m_cTEncTop.setFramePackingArrangementSEIEnabled( m_framePackingSEIEnabled );
  m_cTEncTop.setFramePackingArrangementSEIType( m_framePackingSEIType );
  m_cTEncTop.setFramePackingArrangementSEIId( m_framePackingSEIId );
  m_cTEncTop.setFramePackingArrangementSEIQuincunx( m_framePackingSEIQuincunx );
  m_cTEncTop.setFramePackingArrangementSEIInterpretation( m_framePackingSEIInterpretation );
  m_cTEncTop.setDisplayOrientationSEIAngle( m_displayOrientationSEIAngle );
  m_cTEncTop.setTemporalLevel0IndexSEIEnabled( m_temporalLevel0IndexSEIEnabled );
  m_cTEncTop.setGradualDecodingRefreshInfoEnabled( m_gradualDecodingRefreshInfoEnabled );
  m_cTEncTop.setDecodingUnitInfoSEIEnabled( m_decodingUnitInfoSEIEnabled );
  m_cTEncTop.setSOPDescriptionSEIEnabled( m_SOPDescriptionSEIEnabled );
  m_cTEncTop.setScalableNestingSEIEnabled( m_scalableNestingSEIEnabled );
  m_cTEncTop.setUniformSpacingIdr          ( m_iUniformSpacingIdr );
  m_cTEncTop.setNumColumnsMinus1           ( m_iNumColumnsMinus1 );
  m_cTEncTop.setNumRowsMinus1              ( m_iNumRowsMinus1 );
  if(m_iUniformSpacingIdr==0)
  {
    m_cTEncTop.setColumnWidth              ( m_pColumnWidth );
    m_cTEncTop.setRowHeight                ( m_pRowHeight );
  }
  m_cTEncTop.xCheckGSParameters();
  Int uiTilesCount          = (m_iNumRowsMinus1+1) * (m_iNumColumnsMinus1+1);
  if(uiTilesCount == 1)
  {
    m_bLFCrossTileBoundaryFlag = true; 
  }
  m_cTEncTop.setLFCrossTileBoundaryFlag( m_bLFCrossTileBoundaryFlag );
  m_cTEncTop.setWaveFrontSynchro           ( m_iWaveFrontSynchro );
  m_cTEncTop.setWaveFrontSubstreams        ( m_iWaveFrontSubstreams );
  m_cTEncTop.setTMVPModeId ( m_TMVPModeId );
  m_cTEncTop.setUseScalingListId           ( m_useScalingListId  );
  m_cTEncTop.setScalingListFile            ( m_scalingListFile   );
  m_cTEncTop.setSignHideFlag(m_signHideFlag);
#if RATE_CONTROL_LAMBDA_DOMAIN
#if KWU_RC_VIEWRC_E0227 || KWU_RC_MADPRED_E0227
  if(!m_cTEncTop.getIsDepth())    //only for texture
  {
    m_cTEncTop.setUseRateCtrl         ( m_RCEnableRateControl );
  }
  else
  {
    m_cTEncTop.setUseRateCtrl         ( 0 );
  }
#else
    m_cTEncTop.setUseRateCtrl         ( m_RCEnableRateControl );
#endif
#if !KWU_RC_VIEWRC_E0227
  m_cTEncTop.setTargetBitrate       ( m_RCTargetBitrate );
#endif
  m_cTEncTop.setKeepHierBit         ( m_RCKeepHierarchicalBit );
  m_cTEncTop.setLCULevelRC          ( m_RCLCULevelRC );
  m_cTEncTop.setUseLCUSeparateModel ( m_RCUseLCUSeparateModel );
  m_cTEncTop.setInitialQP           ( m_RCInitialQP );
  m_cTEncTop.setForceIntraQP        ( m_RCForceIntraQP );

#if KWU_RC_MADPRED_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseDepthMADPred(layerIdInVps ? m_depthMADPred       : 0);

    if(m_cTEncTop.getUseDepthMADPred())
    {
      m_cTEncTop.setCamParam(&m_cCameraData);
    }
  }
#endif
#if KWU_RC_VIEWRC_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseViewWiseRateCtrl(m_viewWiseRateCtrl);

    if(m_iNumberOfViews == 1)
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        m_cTEncTop.setTargetBitrate       ( m_RCTargetBitrate );
      }
    }
    else
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        if(m_iNumberOfViews == 2)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*80)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*20)/100 );
          }
        }
        else if(m_iNumberOfViews == 3)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*66)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*17)/100 );
          }
          else if(m_cTEncTop.getViewId() == 2)
          {
            m_cTEncTop.setTargetBitrate              ( (m_RCTargetBitrate*17)/100 );
          }
        }
        else
        {
          m_cTEncTop.setTargetBitrate              ( m_RCTargetBitrate );
        }
      }
    }
  }
#endif
#else
#if KWU_RC_VIEWRC_E0227 || KWU_RC_MADPRED_E0227
  if(!m_cTEncTop.getIsDepth())    //only for texture
  {
    m_cTEncTop.setUseRateCtrl         ( m_enableRateCtrl );
    m_cTEncTop.setTargetBitrate       ( m_targetBitrate );
    m_cTEncTop.setNumLCUInUnit        ( m_numLCUInUnit);
  }
  else
  {
    m_cTEncTop.setUseRateCtrl         ( 0 );
  }
#else
  m_cTEncTop.setUseRateCtrl         ( m_enableRateCtrl );
  m_cTEncTop.setTargetBitrate       ( m_targetBitrate );
  m_cTEncTop.setNumLCUInUnit        ( m_numLCUInUnit);
#endif

  
#if KWU_RC_MADPRED_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseDepthMADPred(layerIdInVps ? m_depthMADPred       : 0);

    if(m_cTEncTop.getUseDepthMADPred())
    {
      m_cTEncTop.setCamParam(&m_cCameraData);
    }
  }
#endif

#if KWU_RC_VIEWRC_E0227
  if(m_cTEncTop.getUseRateCtrl() && !m_cTEncTop.getIsDepth())
  {
    m_cTEncTop.setUseViewWiseRateCtrl(m_viewWiseRateCtrl);
    if(m_iNumberOfViews == 1)
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        m_cTEncTop.setTargetBitrate       ( m_targetBitrate );
      }
    }
    else
    {
      if(m_viewWiseRateCtrl)
      {
        m_cTEncTop.setTargetBitrate(m_viewTargetBits[layerIdInVps>>1]);
      }
      else
      {
        if(m_iNumberOfViews == 2)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_targetBitrate*80)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_targetBitrate*20)/100 );
          }
        }
        else if(m_iNumberOfViews == 3)
        {
          if(m_cTEncTop.getViewId() == 0)
          {
            m_cTEncTop.setTargetBitrate              ( (m_targetBitrate*66)/100 );
          }
          else if(m_cTEncTop.getViewId() == 1)
          {
            m_cTEncTop.setTargetBitrate              ( (m_targetBitrate*17)/100 );
          }
          else if(m_cTEncTop.getViewId() == 2)
          {
            m_cTEncTop.setTargetBitrate              ( (m_targetBitrate*17)/100 );
          }
        }
        else
        {
          m_cTEncTop.setTargetBitrate              ( m_targetBitrate );
        }
      }
    }
  }
#endif
#endif
  m_cTEncTop.setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
  m_cTEncTop.setCUTransquantBypassFlagValue(m_CUTransquantBypassFlagValue);
  m_cTEncTop.setUseRecalculateQPAccordingToLambda( m_recalculateQPAccordingToLambda );
  m_cTEncTop.setUseStrongIntraSmoothing( m_useStrongIntraSmoothing );
  m_cTEncTop.setActiveParameterSetsSEIEnabled ( m_activeParameterSetsSEIEnabled ); 
  m_cTEncTop.setVuiParametersPresentFlag( m_vuiParametersPresentFlag );
  m_cTEncTop.setAspectRatioIdc( m_aspectRatioIdc );
  m_cTEncTop.setSarWidth( m_sarWidth );
  m_cTEncTop.setSarHeight( m_sarHeight );
  m_cTEncTop.setOverscanInfoPresentFlag( m_overscanInfoPresentFlag );
  m_cTEncTop.setOverscanAppropriateFlag( m_overscanAppropriateFlag );
  m_cTEncTop.setVideoSignalTypePresentFlag( m_videoSignalTypePresentFlag );
  m_cTEncTop.setVideoFormat( m_videoFormat );
  m_cTEncTop.setVideoFullRangeFlag( m_videoFullRangeFlag );
  m_cTEncTop.setColourDescriptionPresentFlag( m_colourDescriptionPresentFlag );
  m_cTEncTop.setColourPrimaries( m_colourPrimaries );
  m_cTEncTop.setTransferCharacteristics( m_transferCharacteristics );
  m_cTEncTop.setMatrixCoefficients( m_matrixCoefficients );
  m_cTEncTop.setChromaLocInfoPresentFlag( m_chromaLocInfoPresentFlag );
  m_cTEncTop.setChromaSampleLocTypeTopField( m_chromaSampleLocTypeTopField );
  m_cTEncTop.setChromaSampleLocTypeBottomField( m_chromaSampleLocTypeBottomField );
  m_cTEncTop.setNeutralChromaIndicationFlag( m_neutralChromaIndicationFlag );
  m_cTEncTop.setDefaultDisplayWindow( m_defDispWinLeftOffset, m_defDispWinRightOffset, m_defDispWinTopOffset, m_defDispWinBottomOffset );
  m_cTEncTop.setFrameFieldInfoPresentFlag( m_frameFieldInfoPresentFlag );
  m_cTEncTop.setPocProportionalToTimingFlag( m_pocProportionalToTimingFlag );
  m_cTEncTop.setNumTicksPocDiffOneMinus1   ( m_numTicksPocDiffOneMinus1    );
  m_cTEncTop.setBitstreamRestrictionFlag( m_bitstreamRestrictionFlag );
  m_cTEncTop.setTilesFixedStructureFlag( m_tilesFixedStructureFlag );
  m_cTEncTop.setMotionVectorsOverPicBoundariesFlag( m_motionVectorsOverPicBoundariesFlag );
  m_cTEncTop.setMinSpatialSegmentationIdc( m_minSpatialSegmentationIdc );
  m_cTEncTop.setMaxBytesPerPicDenom( m_maxBytesPerPicDenom );
  m_cTEncTop.setMaxBitsPerMinCuDenom( m_maxBitsPerMinCuDenom );
  m_cTEncTop.setLog2MaxMvLengthHorizontal( m_log2MaxMvLengthHorizontal );
  m_cTEncTop.setLog2MaxMvLengthVertical( m_log2MaxMvLengthVertical );
#if H_MV
  }
#endif
#if H_3D_VSO
  if ( m_bUseVSO )
  {
    if ( m_uiVSOMode == 4 )
    {
#if H_3D_VSO_EARLY_SKIP
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, g_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0, m_bVSOEarlySkip );
#else
      m_cRendererModel.create( m_cRenModStrParser.getNumOfBaseViews(), m_cRenModStrParser.getNumOfModels(), m_iSourceWidth, g_uiMaxCUHeight , LOG2_DISP_PREC_LUT, 0 );
#endif
      for ( Int layer = 0; layer < m_numberOfLayers ; layer++ )
      {
        TEncTop* pcEncTop =  m_acTEncTopList[ layer ]; 
        Int iViewNum      = pcEncTop->getViewIndex(); 
        Int iContent      = pcEncTop->getIsDepth() ? 1 : 0; 
        Int iNumOfModels  = m_cRenModStrParser.getNumOfModelsForView(iViewNum, iContent);

        Bool bUseVSO      = (iNumOfModels != 0);

        pcEncTop->setUseVSO( bUseVSO );
        pcEncTop->getRdCost()->setRenModel( bUseVSO ? &m_cRendererModel : NULL );

        for (Int iCurModel = 0; iCurModel < iNumOfModels; iCurModel++ )
        {
          Int iModelNum; Int iLeftViewNum; Int iRightViewNum; Int iDump; Int iOrgRefNum; Int iBlendMode;

          m_cRenModStrParser.getSingleModelData  ( iViewNum, iContent, iCurModel, iModelNum, iBlendMode, iLeftViewNum, iRightViewNum, iOrgRefNum, iDump ) ;
          m_cRendererModel  .createSingleModel   ( iViewNum, iContent, iModelNum, iLeftViewNum, iRightViewNum, (iOrgRefNum != -1), iBlendMode );
        }            
      }
    }
    else
    {
      AOT(true);
    }
  }
#endif
}

Void TAppEncTop::xCreateLib()
{
#if H_MV
  // initialize global variables
  initROM();
#if H_3D_DIM_DMM
  initWedgeLists( true );
#endif

  for( Int layer=0; layer < m_numberOfLayers; layer++)
  {
    m_acTVideoIOYuvInputFileList[layer]->open( m_pchInputFileList[layer],     false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
    m_acTVideoIOYuvInputFileList[layer]->skipFrames( m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

    if (m_pchReconFileList[layer])
    {
      m_acTVideoIOYuvReconFileList[layer]->open( m_pchReconFileList[layer], true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC);  // write mode
    }
    m_acTEncTopList[layer]->create();
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.open( m_pchInputFile,     false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
  m_cTVideoIOYuvInputFile.skipFrames(m_FrameSkip, m_iSourceWidth - m_aiPad[0], m_iSourceHeight - m_aiPad[1]);

  if (m_pchReconFile)
    m_cTVideoIOYuvReconFile.open(m_pchReconFile, true, m_outputBitDepthY, m_outputBitDepthC, m_internalBitDepthY, m_internalBitDepthC);  // write mode
  
  // Neo Decoder
  m_cTEncTop.create();
#endif
}

Void TAppEncTop::xDestroyLib()
{
#if H_MV
  // destroy ROM
  destroyROM();

  for(Int layer=0; layer<m_numberOfLayers; layer++)
  {
    m_acTVideoIOYuvInputFileList[layer]->close();
    m_acTVideoIOYuvReconFileList[layer]->close();
    delete m_acTVideoIOYuvInputFileList[layer] ; 
    m_acTVideoIOYuvInputFileList[layer] = NULL;
    delete m_acTVideoIOYuvReconFileList[layer] ; 
    m_acTVideoIOYuvReconFileList[layer] = NULL;
    m_acTEncTopList[layer]->deletePicBuffer();
    m_acTEncTopList[layer]->destroy();
    delete m_acTEncTopList[layer] ; 
    m_acTEncTopList[layer] = NULL;
    delete m_picYuvRec[layer] ; 
    m_picYuvRec[layer] = NULL;
  }
#else
  // Video I/O
  m_cTVideoIOYuvInputFile.close();
  m_cTVideoIOYuvReconFile.close();
  
  // Neo Decoder
  m_cTEncTop.destroy();
#endif
}

Void TAppEncTop::xInitLib(Bool isFieldCoding)
{
#if H_MV
  for(Int layer=0; layer<m_numberOfLayers; layer++)
  {
#if KWU_RC_MADPRED_E0227
    m_acTEncTopList[layer]->init( isFieldCoding, this );
#else
    m_acTEncTopList[layer]->init( isFieldCoding );
#endif
  }
#else
  m_cTEncTop.init( isFieldCoding );
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal variable
 - until the end of input YUV file, call encoding function in TEncTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppEncTop::encode()
{
  fstream bitstreamFile(m_pchBitstreamFile, fstream::binary | fstream::out);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  TComPicYuv*       pcPicYuvOrg = new TComPicYuv;
  TComPicYuv*       pcPicYuvRec = NULL;
  
  // initialize internal class & member variables
  xInitLibCfg();
  xCreateLib();
  xInitLib(m_isField);
  
  // main encoder loop
#if H_MV
  Bool  allEos = false;
  std::vector<Bool>  eos ;
  std::vector<Bool>  flush ;  
  
  Int gopSize    = 1;
  Int maxGopSize = 0;
  maxGopSize = (std::max)(maxGopSize, m_acTEncTopList[0]->getGOPSize());  
  
  for(Int layer=0; layer < m_numberOfLayers; layer++ )
  {
    eos  .push_back( false );
    flush.push_back( false );
  }
#else
  Int   iNumEncoded = 0;
  Bool  bEos = false;
#endif
 
  list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process

  // allocate original YUV buffer
  if( m_isField )
  {
    pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeightOrg, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
  else
  {
  pcPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  }
  
#if H_MV
  while ( !allEos )
  {
    for(Int layer=0; layer < m_numberOfLayers; layer++ )
    {
      Int frmCnt = 0;
      while ( !eos[layer] && !(frmCnt == gopSize))
      {
        // get buffers
        xGetBuffer(pcPicYuvRec, layer);

        // read input YUV file
        m_acTVideoIOYuvInputFileList[layer]->read      ( pcPicYuvOrg, m_aiPad );
        m_acTEncTopList             [layer]->initNewPic( pcPicYuvOrg );

        // increase number of received frames
        m_frameRcvd[layer]++;
        
        frmCnt++;

        eos[layer] = (m_frameRcvd[layer] == m_framesToBeEncoded);
        allEos = allEos||eos[layer];

        // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
        if (m_acTVideoIOYuvInputFileList[layer]->isEof())
        {
          flush          [layer] = true;
          eos            [layer] = true;
          m_frameRcvd    [layer]--;
          m_acTEncTopList[layer]->setFramesToBeEncoded(m_frameRcvd[layer]);
        }
      }
    }
    for ( Int gopId=0; gopId < gopSize; gopId++ )
    {
#if H_3D
      UInt iNextPoc = m_acTEncTopList[0] -> getFrameId( gopId );
      if ( iNextPoc < m_framesToBeEncoded )
      {
        m_cCameraData.update( iNextPoc );
      }
#endif
      for(Int layer=0; layer < m_numberOfLayers; layer++ )
      {
#if H_3D_VSO        
          if( m_bUseVSO && m_bUseEstimatedVSD && iNextPoc < m_framesToBeEncoded )
          {
            m_cCameraData.setDispCoeff( iNextPoc, m_acTEncTopList[layer]->getViewIndex() );
            m_acTEncTopList[layer]  ->setDispCoeff( m_cCameraData.getDispCoeff() );
          }
#endif
        Int   iNumEncoded = 0;

        // call encoding function for one frame          
        m_acTEncTopList[layer]->encode( eos[layer], flush[layer] ? 0 : pcPicYuvOrg, *m_picYuvRec[layer], outputAccessUnits, iNumEncoded, gopId );        
        xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits, layer);
        outputAccessUnits.clear();
      }
    }

#if !RATE_CONTROL_LAMBDA_DOMAIN && KWU_FIX_URQ
    for(Int layer=0; layer < m_numberOfLayers; layer++ )
    {
      if(m_acTEncTopList[layer]->getUseRateCtrl()  && !m_acTEncTopList[layer]->getIsDepth())
      {
        m_acTEncTopList[layer]->getRateCtrl()->updateRCGOPStatus();
      }
    }
#endif

    gopSize = maxGopSize;
  }
  for(Int layer=0; layer < m_numberOfLayers; layer++ )
  {
    m_acTEncTopList[layer]->printSummary( m_acTEncTopList[layer]->getNumAllPicCoded(), m_isField );
  }
#else
  while ( !bEos )
  {
    // get buffers
    xGetBuffer(pcPicYuvRec);

    // read input YUV file
    m_cTVideoIOYuvInputFile.read( pcPicYuvOrg, m_aiPad );

    // increase number of received frames
    m_iFrameRcvd++;

    bEos = (m_isField && (m_iFrameRcvd == (m_framesToBeEncoded >> 1) )) || ( !m_isField && (m_iFrameRcvd == m_framesToBeEncoded) );

    Bool flush = 0;
    // if end of file (which is only detected on a read failure) flush the encoder of any queued pictures
    if (m_cTVideoIOYuvInputFile.isEof())
    {
      flush = true;
      bEos = true;
      m_iFrameRcvd--;
      m_cTEncTop.setFramesToBeEncoded(m_iFrameRcvd);
    }

    // call encoding function for one frame
    if ( m_isField )
    {
      m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, m_cListPicYuvRec, outputAccessUnits, iNumEncoded, m_isTopFieldFirst);
    }
    else
    {
    m_cTEncTop.encode( bEos, flush ? 0 : pcPicYuvOrg, m_cListPicYuvRec, outputAccessUnits, iNumEncoded );
    }
    
    // write bistream to file if necessary
    if ( iNumEncoded > 0 )
    {
      xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits);
      outputAccessUnits.clear();
    }
  }

  m_cTEncTop.printSummary(m_isField);
#endif

  // delete original YUV buffer
  pcPicYuvOrg->destroy();
  delete pcPicYuvOrg;
  pcPicYuvOrg = NULL;
  
#if !H_MV
  // delete used buffers in encoder class
  m_cTEncTop.deletePicBuffer();
#endif

  // delete buffers & classes
  xDeleteBuffer();
  xDestroyLib();
  
  printRateSummary();

#if H_3D_REN_MAX_DEV_OUT
  Double dMaxDispDiff = m_cCameraData.getMaxShiftDeviation(); 

  if ( !(dMaxDispDiff < 0) )
  {  
    printf("\n Max. possible shift error: %12.3f samples.\n", dMaxDispDiff );
  }
#endif

  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - application has picture buffer list with size of GOP
 - picture buffer list acts as ring buffer
 - end of the list has the latest picture
 .
 */
#if H_MV
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec, UInt layer)
#else
Void TAppEncTop::xGetBuffer( TComPicYuv*& rpcPicYuvRec)
#endif
{
  assert( m_iGOPSize > 0 );
  
  // org. buffer
#if H_MV
  if ( m_picYuvRec[layer]->size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_picYuvRec[layer]->popFront();
#else
  if ( m_cListPicYuvRec.size() == (UInt)m_iGOPSize )
  {
    rpcPicYuvRec = m_cListPicYuvRec.popFront();
#endif

  }
  else
  {
    rpcPicYuvRec = new TComPicYuv;
    
    rpcPicYuvRec->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );

  }
#if H_MV
  m_picYuvRec[layer]->pushBack( rpcPicYuvRec );
#else
  m_cListPicYuvRec.pushBack( rpcPicYuvRec );
#endif
}

Void TAppEncTop::xDeleteBuffer( )
{
#if H_MV
  for(Int layer=0; layer<m_picYuvRec.size(); layer++)
  {
    if(m_picYuvRec[layer])
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_picYuvRec[layer]->begin();
      Int iSize = Int( m_picYuvRec[layer]->size() );
#else
  TComList<TComPicYuv*>::iterator iterPicYuvRec  = m_cListPicYuvRec.begin();
  
  Int iSize = Int( m_cListPicYuvRec.size() );
#endif

  for ( Int i = 0; i < iSize; i++ )
  {
    TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
    pcPicYuvRec->destroy();
    delete pcPicYuvRec; pcPicYuvRec = NULL;
  }

#if H_MV
    }
  }
#endif  
}

/** \param iNumEncoded  number of encoded frames
 */
#if H_MV
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, std::list<AccessUnit>& accessUnits, UInt layerId)
#else
Void TAppEncTop::xWriteOutput(std::ostream& bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
#endif
{
  if (m_isField)
  {
    //Reinterlace fields
    Int i;
#if H_MV
    if( iNumEncoded > 0 )
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec = m_picYuvRec[layerId]->end();
#else
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();
#endif

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded/2; i++ )
    {
      TComPicYuv*  pcPicYuvRecTop  = *(iterPicYuvRec++);
      TComPicYuv*  pcPicYuvRecBottom  = *(iterPicYuvRec++);

#if H_MV
      if (m_pchReconFileList[layerId])
      {
        m_acTVideoIOYuvReconFileList[layerId]->write( pcPicYuvRecTop, pcPicYuvRecBottom, m_confLeft, m_confRight, m_confTop, m_confBottom, m_isTopFieldFirst );
      }
    }
  }

  if( ! accessUnits.empty() )
  {
    list<AccessUnit>::iterator aUIter;
    for( aUIter = accessUnits.begin(); aUIter != accessUnits.end(); aUIter++ )
    {
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, *aUIter);
      rateStatsAccum(*aUIter, stats);
    }
  }
#else
      if (m_pchReconFile)
      {
        m_cTVideoIOYuvReconFile.write( pcPicYuvRecTop, pcPicYuvRecBottom, m_confLeft, m_confRight, m_confTop, m_confBottom, m_isTopFieldFirst );
      }

      const AccessUnit& auTop = *(iterBitstream++);
      const vector<UInt>& statsTop = writeAnnexB(bitstreamFile, auTop);
      rateStatsAccum(auTop, statsTop);

      const AccessUnit& auBottom = *(iterBitstream++);
      const vector<UInt>& statsBottom = writeAnnexB(bitstreamFile, auBottom);
      rateStatsAccum(auBottom, statsBottom);
    }
#endif
  }
  else
  {
    Int i;
#if H_MV
    if( iNumEncoded > 0 )
    {
      TComList<TComPicYuv*>::iterator iterPicYuvRec = m_picYuvRec[layerId]->end();
#else
    TComList<TComPicYuv*>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();
#endif

    for ( i = 0; i < iNumEncoded; i++ )
    {
      --iterPicYuvRec;
    }

    for ( i = 0; i < iNumEncoded; i++ )
    {
      TComPicYuv*  pcPicYuvRec  = *(iterPicYuvRec++);
#if H_MV
      if (m_pchReconFileList[layerId])
      {
        m_acTVideoIOYuvReconFileList[layerId]->write( pcPicYuvRec, m_confLeft, m_confRight, m_confTop, m_confBottom );
      }    
    }
  }
  if( ! accessUnits.empty() )
  {
    list<AccessUnit>::iterator aUIter;
    for( aUIter = accessUnits.begin(); aUIter != accessUnits.end(); aUIter++ )
    {
      const vector<unsigned>& stats = writeAnnexB(bitstreamFile, *aUIter);
      rateStatsAccum(*aUIter, stats);
    }
  }
#else
      if (m_pchReconFile)
      {
        m_cTVideoIOYuvReconFile.write( pcPicYuvRec, m_confLeft, m_confRight, m_confTop, m_confBottom );
      }

      const AccessUnit& au = *(iterBitstream++);
      const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
      rateStatsAccum(au, stats);   
    }
#endif    
  }
}
  
/**
 *
 */
void TAppEncTop::rateStatsAccum(const AccessUnit& au, const std::vector<UInt>& annexBsizes)
{
  AccessUnit::const_iterator it_au = au.begin();
  vector<UInt>::const_iterator it_stats = annexBsizes.begin();

  for (; it_au != au.end(); it_au++, it_stats++)
  {
    switch ((*it_au)->m_nalUnitType)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:
    case NAL_UNIT_CODED_SLICE_TRAIL_N:
    case NAL_UNIT_CODED_SLICE_TLA_R:
    case NAL_UNIT_CODED_SLICE_TSA_N:
    case NAL_UNIT_CODED_SLICE_STSA_R:
    case NAL_UNIT_CODED_SLICE_STSA_N:
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL_N:
    case NAL_UNIT_CODED_SLICE_RADL_R:
    case NAL_UNIT_CODED_SLICE_RASL_N:
    case NAL_UNIT_CODED_SLICE_RASL_R:
    case NAL_UNIT_VPS:
    case NAL_UNIT_SPS:
    case NAL_UNIT_PPS:
      m_essentialBytes += *it_stats;
      break;
    default:
      break;
    }

    m_totalBytes += *it_stats;
  }
}

void TAppEncTop::printRateSummary()
{
#if H_MV
  Double time = (Double) m_frameRcvd[0] / m_iFrameRate;
  printf("\n");
#else
  Double time = (Double) m_iFrameRcvd / m_iFrameRate;
#endif
  printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
#if VERBOSE_RATE
  printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
#endif
}

#if H_3D_DIM_DLT
Void TAppEncTop::xAnalyzeInputBaseDepth(UInt layer, UInt uiNumFrames, TComVPS* vps)
{
  TComPicYuv*       pcDepthPicYuvOrg = new TComPicYuv;
  // allocate original YUV buffer
  pcDepthPicYuvOrg->create( m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth );
  
  TVideoIOYuv* depthVideoFile = new TVideoIOYuv;
  
  UInt uiMaxDepthValue = ((1 << g_bitDepthY)-1);
  
  Bool abValidDepths[256];
  
  depthVideoFile->open( m_pchInputFileList[layer], false, m_inputBitDepthY, m_inputBitDepthC, m_internalBitDepthY, m_internalBitDepthC );  // read  mode
  
  // initialize boolean array
  for(Int p=0; p<=uiMaxDepthValue; p++)
    abValidDepths[p] = false;
  
  Int iHeight   = pcDepthPicYuvOrg->getHeight();
  Int iWidth    = pcDepthPicYuvOrg->getWidth();
  Int iStride   = pcDepthPicYuvOrg->getStride();
  
  Pel* pInDM    = pcDepthPicYuvOrg->getLumaAddr();
  
  for(Int uiFrame=0; uiFrame < uiNumFrames; uiFrame++ )
  {
    depthVideoFile->read( pcDepthPicYuvOrg, m_aiPad );
    
    // check all pixel values
    for (Int i=0; i<iHeight; i++)
    {
      Int rowOffset = i*iStride;
      
      for (Int j=0; j<iWidth; j++)
      {
        Pel depthValue = pInDM[rowOffset+j];
        abValidDepths[depthValue] = true;
      }
    }
  }
  
  depthVideoFile->close();
  delete depthVideoFile; 
  
  pcDepthPicYuvOrg->destroy();
  delete pcDepthPicYuvOrg;
  
  // convert boolean array to idx2Depth LUT
  Int* aiIdx2DepthValue = (Int*) calloc(uiMaxDepthValue, sizeof(Int));
  Int iNumDepthValues = 0;
  for(Int p=0; p<=uiMaxDepthValue; p++)
  {
    if( abValidDepths[p] == true)
    {
      aiIdx2DepthValue[iNumDepthValues++] = p;
    }
  }
  
  if( uiNumFrames == 0 || numBitsForValue(iNumDepthValues) == g_bitDepthY )
  {
    // don't use DLT
    vps->setUseDLTFlag(layer, false);
  }
  
  // assign LUT
  if( vps->getUseDLTFlag(layer) )
    vps->setDepthLUTs(layer, aiIdx2DepthValue, iNumDepthValues);
  
  // free temporary memory
  free(aiIdx2DepthValue);
}
#endif

#if H_MV
Void TAppEncTop::xSetDimensionIdAndLength( TComVPS& vps )
{   
  vps.setScalabilityMaskFlag( m_scalabilityMask ); 
  for( Int dim = 0; dim < m_dimIds.size(); dim++ )
  {
    vps.setDimensionIdLen( dim, m_dimensionIdLen[ dim ] );
    for( Int layer = 0; layer <= vps.getMaxLayersMinus1(); layer++ )

    {        
      vps.setDimensionId( layer, dim, m_dimIds[ dim ][ layer ] );        
    }  
  }

  Int maxViewId = xGetMax( m_viewId ); 

  Int viewIdLenMinus1 = std::max( gCeilLog2( maxViewId + 1 ) - 1, 0 ) ; 
  const Int maxViewIdLenMinus1 = ( 1 << 4 ) - 1; 
  assert( viewIdLenMinus1 <= maxViewIdLenMinus1  ); 
  vps.setViewIdLenMinus1( viewIdLenMinus1 ); 
  for (Int i = 0; i < m_iNumberOfViews; i++)
  {
    vps.setViewIdVal( i, m_viewId[ i] ); 
  }

  assert( m_iNumberOfViews == vps.getNumViews() ); 
}

Void TAppEncTop::xSetDependencies( TComVPS& vps )
{
  // Direct dependency flags + dependency types
  for( Int depLayer = 1; depLayer < MAX_NUM_LAYERS; depLayer++ )
  {
    for( Int refLayer = 0; refLayer < MAX_NUM_LAYERS; refLayer++ )
    {
      vps.setDirectDependencyFlag( depLayer, refLayer, false); 
      vps.setDirectDependencyType( depLayer, refLayer,    -1 ); 
    }
    }
  for( Int depLayer = 1; depLayer < m_numberOfLayers; depLayer++ )
  {
    Int numRefLayers = (Int) m_directRefLayers[depLayer].size(); 
    assert(  numRefLayers == (Int) m_dependencyTypes[depLayer].size() ); 
    for( Int i = 0; i < numRefLayers; i++ )
    {
      Int refLayer = m_directRefLayers[depLayer][i]; 
      vps.setDirectDependencyFlag( depLayer, refLayer, true); 
      vps.setDirectDependencyType( depLayer, refLayer,m_dependencyTypes[depLayer][i]); 
    }
  }

  // Max temporal id for inter layer reference pictures + presence flag
  Bool maxTidRefPresentFlag = false; 
  for ( Int refLayerIdInVps = 0; refLayerIdInVps < m_numberOfLayers; refLayerIdInVps++)
    {
    Int maxTid = -1; 
    for ( Int curLayerIdInVps = 1; curLayerIdInVps < m_numberOfLayers; curLayerIdInVps++)
      {
      for( Int i = 0; i < getGOPSize(); i++ ) 
      {        
        GOPEntry geCur =  m_GOPListMvc[curLayerIdInVps][i];
        GOPEntry geRef =  m_GOPListMvc[refLayerIdInVps][i];
        
        for (Int j = 0; j < geCur.m_numActiveRefLayerPics; j++)
        {        
          if ( m_directRefLayers[ curLayerIdInVps ][ geCur.m_interLayerPredLayerIdc[ j ]] == refLayerIdInVps )
          {
            maxTid = std::max( maxTid, geRef.m_temporalId ); 
          }
        }
      }
    }
    vps.setMaxTidIlRefPicPlus1( refLayerIdInVps, maxTid + 1 );
    maxTidRefPresentFlag = maxTidRefPresentFlag || ( maxTid != 6 );    
  }

  vps.setMaxTidRefPresentFlag( maxTidRefPresentFlag );
  // Max one active ref layer flag
  Bool maxOneActiveRefLayerFlag = true;  
  for ( Int layerIdInVps = 1; layerIdInVps < m_numberOfLayers && maxOneActiveRefLayerFlag; layerIdInVps++)
  {
    for( Int i = 0; i < ( getGOPSize() + 1) && maxOneActiveRefLayerFlag; i++ ) 
    {        
      GOPEntry ge =  m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]; 
      maxOneActiveRefLayerFlag =  maxOneActiveRefLayerFlag && (ge.m_numActiveRefLayerPics <= 1); 
    }            
}

  vps.setMaxOneActiveRefLayerFlag( maxOneActiveRefLayerFlag );
  
  
  // All Ref layers active flag
  Bool allRefLayersActiveFlag = true; 
  for ( Int layerIdInVps = 1; layerIdInVps < m_numberOfLayers && allRefLayersActiveFlag; layerIdInVps++)
  {    
    for( Int i = 0; i < ( getGOPSize() + 1) && allRefLayersActiveFlag; i++ ) 
    {        
      GOPEntry ge =  m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]; 
      allRefLayersActiveFlag = allRefLayersActiveFlag && (ge.m_numActiveRefLayerPics == m_directRefLayers[ layerIdInVps ].size() ); 
    }            
  }

  vps.setAllRefLayersActiveFlag( allRefLayersActiveFlag );

  // Currently cross layer irap aligned is always true.   
  vps.setCrossLayerIrapAlignedFlag( true ); 
  vps.setRefLayers(); 
}; 

GOPEntry* TAppEncTop::xGetGopEntry( Int layerIdInVps, Int poc )
{
  GOPEntry* geFound = NULL; 
  for( Int i = 0; i < ( getGOPSize() + 1) && geFound == NULL ; i++ ) 
  {
    GOPEntry* ge = &(m_GOPListMvc[layerIdInVps][ ( i < getGOPSize()  ? i : MAX_GOP ) ]);
    if ( ge->m_POC == poc )
    {
      geFound = ge;       
    }
  }
  assert( geFound != NULL ); 
  return geFound; 
}

Void TAppEncTop::xSetLayerIds( TComVPS& vps )
{
  vps.setSplittingFlag     ( m_splittingFlag );

  Bool nuhLayerIdPresentFlag = !( m_layerIdInNuh.size() == 1 ); 
  Int  maxNuhLayerId = nuhLayerIdPresentFlag ? xGetMax( m_layerIdInNuh ) : ( m_numberOfLayers - 1 ) ; 

  vps.setVpsMaxLayerId( maxNuhLayerId ); 
  vps.setVpsNuhLayerIdPresentFlag( nuhLayerIdPresentFlag ); 

  for (Int layer = 0; layer < m_numberOfLayers; layer++ )
  {
    vps.setLayerIdInNuh( layer, nuhLayerIdPresentFlag ? m_layerIdInNuh[ layer ] : layer ); 
    vps.setLayerIdInVps( vps.getLayerIdInNuh( layer ), layer ); 
  }
}

Int TAppEncTop::xGetMax( std::vector<Int>& vec )
{
  Int maxVec = 0; 
  for ( Int i = 0; i < vec.size(); i++)    
  {
    maxVec = max( vec[i], maxVec ); 
  }
  return maxVec;
}

Void TAppEncTop::xSetProfileTierLevel( TComVPS& vps )
{ 
  const Int vpsNumProfileTierLevelMinus1 = 0; //TBD
  vps.setVpsNumProfileTierLevelMinus1( vpsNumProfileTierLevelMinus1 ); 
  
  for (Int i = 0; i <= vps.getVpsNumProfileTierLevelMinus1(); i++ )
  {
    vps.setVpsProfilePresentFlag( i, true ); 
  }
}


Void TAppEncTop::xSetRepFormat( TComVPS& vps )
{
  vps.setRepFormatIdxPresentFlag( true ); 
  vps.setVpsNumRepFormatsMinus1 ( 0    ); 

  TComRepFormat* repFormat = new TComRepFormat; 

  repFormat->setBitDepthVpsChromaMinus8   ( g_bitDepthC - 8 ); 
  repFormat->setBitDepthVpsLumaMinus8     ( g_bitDepthY - 8 );
  repFormat->setChromaFormatVpsIdc        ( CHROMA_420      );
  repFormat->setPicHeightVpsInLumaSamples ( m_iSourceHeight );
  repFormat->setPicWidthVpsInLumaSamples  ( m_iSourceWidth  );    
  // ToDo not supported yet. 
  //repFormat->setSeparateColourPlaneVpsFlag( );

  assert( vps.getRepFormat( 0 ) == NULL ); 
  vps.setRepFormat( 0 , repFormat );

  for(Int i = 0; i <= vps.getMaxLayersMinus1(); i++ )
  {
    vps.setVpsRepFormatIdx( i , 0 ); 
  }
}

Void TAppEncTop::xSetLayerSets( TComVPS& vps )
{   
  // Layer sets
  vps.setVpsNumLayerSetsMinus1   ( m_vpsNumLayerSets - 1 ); 
  vps.setVpsNumberLayerSetsMinus1( vps.getVpsNumLayerSetsMinus1() ); 
    
  for (Int lsIdx = 0; lsIdx < m_vpsNumLayerSets; lsIdx++ )
  {
    for( Int layerId = 0; layerId < MAX_NUM_LAYER_IDS; layerId++ )
    {
      vps.setLayerIdIncludedFlag( false, lsIdx, layerId ); 
    }
    for ( Int i = 0; i < m_layerIdsInSets[lsIdx].size(); i++)
    {       
      vps.setLayerIdIncludedFlag( true, lsIdx, vps.getLayerIdInNuh( m_layerIdsInSets[lsIdx][i] ) ); 
    } 
  }

  Int numAddOuputLayerSets = (Int) m_outputLayerSetIdx.size(); 
  // Additional output layer sets + profileLevelTierIdx
  vps.setDefaultOneTargetOutputLayerFlag   ( m_defaultOneTargetOutputLayerFlag ); 
  vps.setMoreOutputLayerSetsThanDefaultFlag( numAddOuputLayerSets       != 0 );   
  vps.setNumAddOutputLayerSetsMinus1       ( numAddOuputLayerSets - 1        ); 

  for (Int lsIdx = 1; lsIdx < m_vpsNumLayerSets; lsIdx++)
  {
    vps.setProfileLevelTierIdx( lsIdx, m_profileLevelTierIdx[ lsIdx ] ); 
  }

  for (Int addOutLs = 0; addOutLs < numAddOuputLayerSets; addOutLs++ )
  {
    vps.setProfileLevelTierIdx( m_vpsNumLayerSets + addOutLs, m_profileLevelTierIdx[ addOutLs ] ); 

    Int refLayerSetIdx = m_outputLayerSetIdx[ addOutLs ];     
    vps.setOutputLayerSetIdxMinus1( m_vpsNumLayerSets + addOutLs, refLayerSetIdx - 1 ); 

    for (Int i = 0; i < m_layerIdsInSets[ refLayerSetIdx].size(); i++ )
    {
      Bool outputLayerFlag = false; 
      for (Int j = 0; j < m_layerIdsInAddOutputLayerSet[ addOutLs ].size(); j++ )
      {
        if (  m_layerIdsInAddOutputLayerSet[addOutLs][ j ] == m_layerIdsInSets[ refLayerSetIdx][ i ] )
        {
          outputLayerFlag = true; 
          break; 
        }
      }
      vps.setOutputLayerFlag( m_vpsNumLayerSets + addOutLs, i, outputLayerFlag );       
    }
  }
}

Void TAppEncTop::xSetVPSVUI( TComVPS& vps )
{
  vps.setVpsVuiPresentFlag( m_vpsVuiPresentFlag ); 

  if ( m_vpsVuiPresentFlag )
  {
    TComVPSVUI* pcVPSVUI = vps.getVPSVUI(  ); 

    assert( pcVPSVUI ); 

    pcVPSVUI->setBitRatePresentVpsFlag( m_bitRatePresentVpsFlag );
    pcVPSVUI->setPicRatePresentVpsFlag( m_picRatePresentVpsFlag );

    if( pcVPSVUI->getBitRatePresentVpsFlag( )  ||  pcVPSVUI->getPicRatePresentVpsFlag( ) )
    {
      for( Int i = 0; i  <=  vps.getVpsNumberLayerSetsMinus1(); i++ )
      {
        for( Int j = 0; j  <=  vps.getMaxTLayers(); j++ ) 
        {
          if( pcVPSVUI->getBitRatePresentVpsFlag( ) && m_bitRatePresentFlag[i].size() > j )
          {
            pcVPSVUI->setBitRatePresentFlag( i, j, m_bitRatePresentFlag[i][j] );            
          }
          if( pcVPSVUI->getPicRatePresentVpsFlag( ) && m_picRatePresentFlag[i].size() > j   )
          {
            pcVPSVUI->setPicRatePresentFlag( i, j, m_picRatePresentFlag[i][j] );
          }
          if( pcVPSVUI->getBitRatePresentFlag( i, j )  && m_avgBitRate[i].size() > j )
          {
            pcVPSVUI->setAvgBitRate( i, j, m_avgBitRate[i][j] );          
          }
          if( pcVPSVUI->getBitRatePresentFlag( i, j )  && m_maxBitRate[i].size() > j )
          {
            pcVPSVUI->setMaxBitRate( i, j, m_maxBitRate[i][j] );
          }
          if( pcVPSVUI->getPicRatePresentFlag( i, j ) && m_constantPicRateIdc[i].size() > j )
          {
            pcVPSVUI->setConstantPicRateIdc( i, j, m_constantPicRateIdc[i][j] );
          }
          if( pcVPSVUI->getPicRatePresentFlag( i, j ) && m_avgPicRate[i].size() > j )
          {
            pcVPSVUI->setAvgPicRate( i, j, m_avgPicRate[i][j] );
          }
        }
      }
    }

    for( Int i = 1; i  <=  vps.getMaxLayersMinus1(); i++ )
    {
      for( Int  j = 0; j < vps.getNumDirectRefLayers( vps.getLayerIdInNuh( i ) ); j++ ) 
      {
        if ( m_tileBoundariesAlignedFlag[i].size() > j )
        {
          pcVPSVUI->setTileBoundariesAlignedFlag( i, j, m_tileBoundariesAlignedFlag[i][j]);
        }
      }
    }

    pcVPSVUI->setIlpRestrictedRefLayersFlag( m_ilpRestrictedRefLayersFlag );

    if( pcVPSVUI->getIlpRestrictedRefLayersFlag( ) )
    {
      for( Int i = 1; i  <=  vps.getMaxLayersMinus1(); i++ )
      {
        for( Int j = 0; j < vps.getNumDirectRefLayers( vps.getLayerIdInNuh( i ) ); j++ )
        {
          if ( m_minSpatialSegmentOffsetPlus1[i].size() > j )
          {        
            pcVPSVUI->setMinSpatialSegmentOffsetPlus1( i, j, m_minSpatialSegmentOffsetPlus1[i][j] );
          }
          if( pcVPSVUI->getMinSpatialSegmentOffsetPlus1( i, j ) > 0 )
          {
            if ( m_ctuBasedOffsetEnabledFlag[i].size() > j )
            {        
              pcVPSVUI->setCtuBasedOffsetEnabledFlag( i, j, m_ctuBasedOffsetEnabledFlag[i][j] );
            }
            if( pcVPSVUI->getCtuBasedOffsetEnabledFlag( i, j ) )
            {
              if ( m_minHorizontalCtuOffsetPlus1[i].size() > j )
              {
                pcVPSVUI->setMinHorizontalCtuOffsetPlus1( i, j, m_minHorizontalCtuOffsetPlus1[i][j] );
              }
            }
          }
        }
      }
    }      
  }
}
#endif
#if H_3D
Void TAppEncTop::xSetVPSExtension2( TComVPS& vps )
{

  for ( Int layer = 0; layer <= vps.getMaxLayersMinus1(); layer++ )
  {
    Bool isDepth      = ( vps.getDepthId( layer ) == 1 ) ;
    Bool isLayerZero  = ( layer == 0 ); 

#if H_3D_ARP
    vps.setUseAdvRP        ( layer, ( isDepth || isLayerZero ) ? 0 : m_uiUseAdvResPred );
    vps.setARPStepNum      ( layer, ( isDepth || isLayerZero ) ? 1 : H_3D_ARP_WFNR     );
#endif  

#if H_3D_DIM
    vps.setVpsDepthModesFlag( layer, isDepth && !isLayerZero && (m_useDMM || m_useRBC || m_useSDC || m_useDLT ) );
#if H_3D_DIM_DLT
    vps.setUseDLTFlag( layer , isDepth && m_useDLT );
    if( vps.getUseDLTFlag( layer ) )
    {
      xAnalyzeInputBaseDepth(layer, max(m_iIntraPeriod, 24), &vps);
    }
#endif
#endif

#if H_3D_IV_MERGE
#if QC_DEPTH_IV_MRG_F0125
    if( isDepth )
    {
      vps.setIvMvPredFlag         ( layer, (layer != 1) && m_ivMvPredFlag[1] ); 
    }
    else
    {
      vps.setIvMvPredFlag         ( layer, !isLayerZero && m_ivMvPredFlag[0] ); 
    }
#else
    vps.setIvMvPredFlag         ( layer, !isLayerZero && !isDepth && m_ivMvPredFlag ); 
#endif
#endif
#if H_3D_NBDV_REF
    vps.setDepthRefinementFlag  ( layer, !isLayerZero && !isDepth && m_depthRefinementFlag );         
#endif
#if H_3D_VSP
    vps.setViewSynthesisPredFlag( layer, !isLayerZero && !isDepth && m_viewSynthesisPredFlag );         
#endif      
#if H_3D_INTER_SDC
    vps.setInterSDCFlag( layer, !isLayerZero && isDepth && m_bDepthInterSDCFlag );
#endif
#if SEC_MPI_ENABLING_MERGE_F0150
    vps.setMPIFlag( layer, !isLayerZero && isDepth && m_bMPIFlag );
#endif
  }  
#if H_3D
  vps.setIvMvScalingFlag( m_ivMvScalingFlag );   
#endif
}
#endif
//! \}
