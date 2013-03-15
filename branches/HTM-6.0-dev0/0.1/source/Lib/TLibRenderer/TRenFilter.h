


#ifndef __TRENFILTER__
#define __TRENFILTER__

#include "../TLibCommon/CommonDef.h"
#include "TRenImage.h"
#include "../TLibCommon/TComPredFilter.h"

typedef Int (TComPredFilter::*FpChromaIntFilt) ( Pel*, Int );

class TRenFilter
{
public:

  /////////// Helpers ////////
  static Void setSubPelShiftLUT ( Int iLutPrec, Int** piSubPelShiftLUT, Int iShift );
  static Void setupZLUT         ( Bool bBlendUseDistWeight, Int iBlendZThresPerc, Int iRelDistToLeft, Int** ppiBaseShiftLUTLeft, Int** ppiBaseShiftLUTRight, Int& riBlendZThres, Int& riBlendDistWeight, Int* piInvZLUTLeft, Int* piInvZLUTRight );
  static Void filledToUsedPelMap( PelImage* pcFilledImage, PelImage* pcUsedPelsImage, Int iUsedPelMapMarExt  );

  /////////// Copy ///////////
  static Void copy( Pel* pcInputPlaneData, Int iInputStride, Int iWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride);

  /////////// Horizontal Mirroring ///////////
  template <typename T> static Void mirrorHor(        TRenImage<T> *pcInputImage );
  //Plane
  template <typename T> static Void mirrorHor(   TRenImagePlane<T> *pcImagePlane );

  /////////// Comparison ///////////

  static Int64                          SSE  ( PelImagePlane*     pcInputPlane1, PelImagePlane*      pcInputPlane2 );
  static Int64                          SSE  ( Pel* piSrc1,       Int iSrcStride1, Int iWidth, Int iHeight,  Pel* piSrc2, Int iSrcStride2 );

  template <typename T> static Bool compare  (TRenImage<T> *pcInputImage1     , TRenImage<T> *pcInputImage2);
  //Plane
  template <typename T> static Bool compare  (TRenImagePlane<T>* pcInputPlane1, TRenImagePlane<T>* pcInputPlane2 );

  /////////// other Filters ///////////
  static Void binominal  ( PelImage*      pcInputImage,  PelImage*      pcOutputPlane, UInt uiSize);
  static Void binominal  ( PelImagePlane* pcInputPlane,  PelImagePlane* pcOutputPlane, UInt uiSize );

  static Void lineMedian3( PelImage*      pcImage );
  static Void convRect   ( PelImage*      pcImage,       UInt uiSize);
  static Void laplace    ( DoubleImage*   pcInputImage,  DoubleImage* pcOutputImage);
  static Void diffHorSym ( PelImage*      pcInputImage,  IntImage* pcOutputImage);

  //Plane
  static Void diffHorSym (PelImagePlane* pcInputPlane, IntImagePlane* pcOutputPlane);

  ///////////  Convolution ///////////
  static Void conv (PelImage* pcImage, DoubleImage* pcKernel);

  /////////// InterPolation ///////////
  static Pel interpCHSpline(Double dX, Double dS0, Double dS1, Int iQ0, Int iQ1, Int iQ2, Int iQ3);


  /////////// HEVC/ binomial Up and Down sampling ///////////
  //// Down sampling (binomial)
  // Horizontally
  static Void sampleHorDown     (Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight     , Pel* pcOutputPlaneData, Int iOutputStride );
  static Void sampleCHorDown    (Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iInputHeight, Pel* pcOutputPlaneData, Int iOutputStride );
  // 444->420 and horizontally
  static Void sampleCDownHorDown(Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iInputHeight, Pel* pcOutputPlaneData, Int iOutputStride );

  //// Up sampling (HEVC 8/4 tap)
  // Horizontally
  static Void sampleHorUp       (Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight,      Pel* pcOutputPlaneData, Int iOutputStride );
  static Void sampleCHorUp      (Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight,      Pel* pcOutputPlaneData, Int iOutputStride );
  // 420->444 and horizontally
  static Void sampleCUpHorUp    (Int iLog2HorSampFac, Pel* pcInputPlaneData, Int iInputStride, Int iInputWidth, Int iHeight,      Pel* pcOutputPlaneData, Int iOutputStride );

  //// Down sampling (13 tap)

  static Void sampleDown2Tap13   ( Pel* pcInputPlaneData, Int iInputStride, Int iWidth, Int iHeight, Pel* pcOutputPlaneData, Int iOutputStride );
  // Plane
  static Void sampleHorDown2Tap13(PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane, Int uiPad);
  static Void sampleVerDown2Tap13(PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane, Int uiPad);
  static Void sampleDown2Tap13   (PelImagePlane* pcInputPlane, PelImagePlane* pcOutputPlane);
  // Image
  static Void sampleDown2Tap13   (PelImage*      pcInputImage, PelImage*      pcOutputImage);
  static Void sampleUp2Tap13     (PelImage*      pcInputImage, PelImage*      pcOutImage   );
private:

  // Helper Functions
  static inline Pel   xMedian3 (Pel* pcData);
  static Void         xDilate (Pel* piSrc, Int iSrcStride, Int    iWidth, Int iHeight,    Pel* piDst, Int iDstStride, Int iSize, Bool bVerticalDir, Bool bToTopOrLeft );

  // Down sampling (binomial)
  static Void xSampleDownHor2 (Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight,    Pel* piDst, Int iDstStride );
  static Void xSampleDownHor4 (Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight,    Pel* piDst, Int iDstStride );
  static Void xSampleDownHor8 (Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iHeight,    Pel* piDst, Int iDstStride );

  static Void xSampleDownVer2 (Pel* piSrc, Int iSrcStride, Int iSrcWidth, Int iSrcHeight, Pel* piDst, Int iDstStride );

  // Up sampling (8/4-Tap HEVC)
  static Void xInterpVerChroma(Pel* piSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* piDst, Int iDstStride, Int iDstStepX, Int iDstStepY, FpChromaIntFilt fpFilter);
  static Void xInterpHorChroma(Pel* piSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* piDst, Int iDstStride, Int iDstStepX, Int iDstStepY, FpChromaIntFilt fpFilter);
  static Void xDistributeArray(Pel* pcSrc, Int iSrcStride, Int iSrcStepX, Int iSrcStepY, Int iWidth, Int iHeight, Pel* pcDst, Int iDstStride, Int iDstStepX, Int iDstStepY );

  // Binominal Filtering
  static Pel  xFiltBinom3     (Pel* pcInputData, Int iStride );
  static Pel  xFiltBinom5     (Pel* pcInputData, Int iStride );
  static Pel  xFiltBinom7     (Pel* pcInputData, Int iStride );
  static Pel  xFiltBinom9     (Pel* pcInputData, Int iStride );
};


#endif //__TRENFILTER__
