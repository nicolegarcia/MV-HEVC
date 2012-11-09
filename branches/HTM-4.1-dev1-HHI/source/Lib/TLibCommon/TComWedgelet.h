/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2011, ISO/IEC
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
 *  * Neither the name of the ISO/IEC nor the names of its contributors may
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



#ifndef __TCOMWEDGELET__
#define __TCOMWEDGELET__

// Include files
#include <assert.h>
#include "CommonDef.h"

#include <vector>

enum WedgeResolution
{
  DOUBLE_PEL,
  FULL_PEL,
  HALF_PEL
};

// ====================================================================================================================
// Class definition TComWedgelet
// ====================================================================================================================
class TComWedgelet
{
private:
  UChar           m_uhXs;                       // line start X pos
  UChar           m_uhYs;                       // line start Y pos
  UChar           m_uhXe;                       // line end   X pos
  UChar           m_uhYe;                       // line end   Y pos
  UChar           m_uhOri;                      // orientation index
  WedgeResolution m_eWedgeRes;                  // start/end pos resolution

  UInt  m_uiWidth;
  UInt  m_uiHeight;

  Bool* m_pbPattern;

  Void  xGenerateWedgePattern();
  Void  xDrawEdgeLine( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, Bool* pbPattern, Int iPatternStride );

public:
  TComWedgelet( UInt uiWidth, UInt uiHeight );
  TComWedgelet( const TComWedgelet &rcWedge );
  virtual ~TComWedgelet();

  Void  create ( UInt iWidth, UInt iHeight );   ///< create  wedgelet pattern
  Void  destroy();                              ///< destroy wedgelet pattern
  Void  clear  ();                              ///< clear   wedgelet pattern

  UInt            getWidth   () { return m_uiWidth; }
  UInt            getStride  () { return m_uiWidth; }
  UInt            getHeight  () { return m_uiHeight; }
  WedgeResolution getWedgeRes() { return m_eWedgeRes; }
  Bool*           getPattern () { return m_pbPattern; }
  UChar           getStartX  () { return m_uhXs; }
  UChar           getStartY  () { return m_uhYs; }
  UChar           getEndX    () { return m_uhXe; }
  UChar           getEndY    () { return m_uhYe; }
  UChar           getOri     () { return m_uhOri; }

  Void  setWedgelet( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UChar uhOri, WedgeResolution eWedgeRes );

  Bool  checkNotPlain();
  Bool  checkIdentical( Bool* pbRefPattern );
  Bool  checkInvIdentical( Bool* pbRefPattern );

#if HHI_DMM_WEDGE_INTRA
  Bool  checkPredDirAbovePossible( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffsett );
  Bool  checkPredDirLeftPossible ( UInt uiPredDirBlockSize, UInt uiPredDirBlockOffsett );

  Void  getPredDirStartEndAbove( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd );
  Void  getPredDirStartEndLeft ( UChar& ruhXs, UChar& ruhYs, UChar& ruhXe, UChar& ruhYe, UInt uiPredDirBlockSize, UInt uiPredDirBlockOffset, Int iDeltaEnd );
#endif
};  // END CLASS DEFINITION TComWedgelet

// type definition wedgelet pattern list
typedef std::vector<TComWedgelet> WedgeList;

// ====================================================================================================================
// Class definition TComWedgeRef
// ====================================================================================================================
class TComWedgeRef
{
private:
  UChar           m_uhXs;                       // line start X pos
  UChar           m_uhYs;                       // line start Y pos
  UChar           m_uhXe;                       // line end   X pos
  UChar           m_uhYe;                       // line end   Y pos
  UInt            m_uiRefIdx;                   // index of corresponding pattern of TComWedgelet object in wedge list

public:
  TComWedgeRef() {}
  virtual ~TComWedgeRef() {}

  UChar           getStartX  () { return m_uhXs; }
  UChar           getStartY  () { return m_uhYs; }
  UChar           getEndX    () { return m_uhXe; }
  UChar           getEndY    () { return m_uhYe; }
  UInt            getRefIdx  () { return m_uiRefIdx; }

  Void  setWedgeRef( UChar uhXs, UChar uhYs, UChar uhXe, UChar uhYe, UInt uiRefIdx ) { m_uhXs = uhXs; m_uhYs = uhYs; m_uhXe = uhXe; m_uhYe = uhYe; m_uiRefIdx = uiRefIdx; }
};  // END CLASS DEFINITION TComWedgeRef

// type definition wedgelet reference list
typedef std::vector<TComWedgeRef> WedgeRefList;

#if HHI_DMM_PRED_TEX
enum WedgeDist
{
  WedgeDist_SAD  = 0,
  WedgeDist_SSE  = 4,
};

class WedgeDistParam;
typedef UInt (*FpWedgeDistFunc) (WedgeDistParam*);

/// distortion parameter class
class WedgeDistParam
{
public:
  Pel*  pOrg;
  Pel*  pCur;
  Int   iStrideOrg;
  Int   iStrideCur;
  Int   iRows;
  Int   iCols;
  Int   iStep;
  FpWedgeDistFunc DistFunc;
  Int   iSubShift;

  WedgeDistParam()
  {
    pOrg = NULL;
    pCur = NULL;
    iStrideOrg = 0;
    iStrideCur = 0;
    iRows = 0;
    iCols = 0;
    iStep = 1;
    DistFunc = NULL;
    iSubShift = 0;
  }
};

// ====================================================================================================================
// Class definition TComWedgeDist
// ====================================================================================================================
class TComWedgeDist
{
private:
  Int                     m_iBlkWidth;
  Int                     m_iBlkHeight;
  FpWedgeDistFunc         m_afpDistortFunc[8];

public:
  TComWedgeDist();
  virtual ~TComWedgeDist();

  Void init();
  Void setDistParam( UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist, WedgeDistParam& rcDistParam );
  UInt getDistPart( Pel* piCur, Int iCurStride,  Pel* piOrg, Int iOrgStride, UInt uiBlkWidth, UInt uiBlkHeight, WedgeDist eWDist = WedgeDist_SAD );

private:
  static UInt xGetSAD4          ( WedgeDistParam* pcDtParam );
  static UInt xGetSAD8          ( WedgeDistParam* pcDtParam );
  static UInt xGetSAD16         ( WedgeDistParam* pcDtParam );
  static UInt xGetSAD32         ( WedgeDistParam* pcDtParam );

  static UInt xGetSSE4          ( WedgeDistParam* pcDtParam );
  static UInt xGetSSE8          ( WedgeDistParam* pcDtParam );
  static UInt xGetSSE16         ( WedgeDistParam* pcDtParam );
  static UInt xGetSSE32         ( WedgeDistParam* pcDtParam );

};// END CLASS DEFINITION TComWedgeDist
#endif

// ====================================================================================================================
// Function definition roftoi (mathematically correct rounding of float to int)
// ====================================================================================================================
__inline Int roftoi( Double value )
{
  (value < -0.5) ? (value -= 0.5) : (value += 0.5);
  return ( (Int)value );
}
#endif // __TCOMWEDGELET__
