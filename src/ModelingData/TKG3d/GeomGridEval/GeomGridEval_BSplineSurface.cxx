// Copyright (c) 2025 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <GeomGridEval_BSplineSurface.hxx>

#include <BSplCLib.hxx>
#include <BSplSLib.hxx>
#include <gp_Pnt.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>
#include <NCollection_LocalArray.hxx>

namespace
{

//! Helper structure holding extracted B-spline surface data.
struct SurfaceData
{
  const NCollection_Array1<double>* UFlatKnots;
  const NCollection_Array1<double>* VFlatKnots;
  const NCollection_Array2<gp_Pnt>* Poles;
  const NCollection_Array2<double>* Weights;
  int                               UDegree;
  int                               VDegree;
  bool                              IsUPeriodic;
  bool                              IsVPeriodic;
  bool                              IsURational;
  bool                              IsVRational;
};

//! Extract surface data from geometry. Returns false if data is invalid.
bool extractSurfaceData(const occ::handle<Geom_BSplineSurface>& theGeom, SurfaceData& theData)
{
  if (theGeom.IsNull())
  {
    return false;
  }

  theData.UFlatKnots  = &theGeom->UKnotSequence();
  theData.VFlatKnots  = &theGeom->VKnotSequence();
  theData.Poles       = &theGeom->Poles();
  theData.Weights     = theGeom->Weights();
  theData.UDegree     = theGeom->UDegree();
  theData.VDegree     = theGeom->VDegree();
  theData.IsUPeriodic = theGeom->IsUPeriodic();
  theData.IsVPeriodic = theGeom->IsVPeriodic();
  theData.IsURational = theGeom->IsURational();
  theData.IsVRational = theGeom->IsVRational();
  return true;
}

//! Locate parameter and compute span index.
inline void locateSpan(double                            theParam,
                       const NCollection_Array1<double>& theFlatKnots,
                       int                               theDegree,
                       bool                              theIsPeriodic,
                       double&                           theAdjustedParam,
                       int&                              theSpanIdx)
{
  theAdjustedParam = theParam;
  BSplCLib::LocateParameter(theDegree,
                            theFlatKnots,
                            BSplCLib::NoMults(),
                            theParam,
                            theIsPeriodic,
                            theSpanIdx,
                            theAdjustedParam);
}

//! Count consecutive parameters belonging to the same span.
//! Non-periodic parameters are classified by the already known span bounds.
inline size_t countSpanSize(const NCollection_Array1<double>& theParams,
                            const NCollection_Array1<double>& theFlatKnots,
                            const int                         theDegree,
                            const bool                        theIsPeriodic,
                            const size_t                      theStartIdx,
                            const int                         theTargetSpan)
{
  const size_t aNb    = theParams.Size();
  size_t       aCount = 1;

  if (!theIsPeriodic)
  {
    if (theTargetSpan + 1 > theFlatKnots.Upper())
    {
      return aCount;
    }
    const double aSpanStart = theFlatKnots.Value(theTargetSpan);
    const double aNextKnot  = theFlatKnots.Value(theTargetSpan + 1);
    for (size_t anIndex = theStartIdx + 1; anIndex < aNb; ++anIndex)
    {
      const double aParameter = theParams.At(anIndex);
      if (aParameter < aSpanStart || aParameter >= aNextKnot)
      {
        break;
      }
      ++aCount;
    }
    return aCount;
  }

  // Periodic fallback: span assignment requires full LocateParameter.
  for (size_t anIndex = theStartIdx + 1; anIndex < aNb; ++anIndex)
  {
    double aParam    = theParams.At(anIndex);
    double aAdjusted = aParam;
    int    aSpan     = 0;
    BSplCLib::LocateParameter(theDegree,
                              theFlatKnots,
                              BSplCLib::NoMults(),
                              aParam,
                              theIsPeriodic,
                              aSpan,
                              aAdjusted);
    if (aSpan == theTargetSpan)
    {
      ++aCount;
    }
    else
    {
      break;
    }
  }
  return aCount;
}

//! Compute span local parameter coefficients (mid and half-length).
inline void computeSpanCoeffs(const NCollection_Array1<double>& theFlatKnots,
                              int                               theSpan,
                              double&                           theMid,
                              double&                           theHalfLen)
{
  const double aStart = theFlatKnots.Value(theSpan);
  theHalfLen          = 0.5 * (theFlatKnots.Value(theSpan + 1) - aStart);
  theMid              = aStart + theHalfLen;
}

struct SpanBlock
{
  size_t Start;
  size_t Count;
  int    Span;
};

size_t prepareSpanBlocks(const NCollection_Array1<double>& theParams,
                         const NCollection_Array1<double>& theFlatKnots,
                         const int                         theDegree,
                         const bool                        theIsPeriodic,
                         NCollection_LocalArray<double>&    theLocalParams,
                         NCollection_LocalArray<SpanBlock>& theBlocks)
{
  const size_t aNbParams = theParams.Size();
  size_t       aNbBlocks = 0;
  for (size_t aStart = 0; aStart < aNbParams;)
  {
    double anAdjusted = theParams.At(aStart);
    int    aSpan      = 0;
    locateSpan(anAdjusted, theFlatKnots, theDegree, theIsPeriodic, anAdjusted, aSpan);
    const size_t aCount = countSpanSize(
      theParams, theFlatKnots, theDegree, theIsPeriodic, aStart, aSpan);
    double aMid     = 0.0;
    double aHalfLen = 0.0;
    computeSpanCoeffs(theFlatKnots, aSpan, aMid, aHalfLen);
    const double anInvHalfLen = 1.0 / aHalfLen;
    for (size_t anOffset = 0; anOffset < aCount; ++anOffset)
    {
      double aParameter = theParams.At(aStart + anOffset);
      if (theIsPeriodic)
      {
        int aLocatedSpan = 0;
        locateSpan(aParameter, theFlatKnots, theDegree, true, aParameter, aLocatedSpan);
      }
      theLocalParams[aStart + anOffset] = (aParameter - aMid) * anInvHalfLen;
    }
    theBlocks[aNbBlocks++] = {aStart, aCount, aSpan};
    aStart += aCount;
  }
  return aNbBlocks;
}

//! Visits Cartesian parameter blocks sharing one U and V knot span.
template <typename BlockEvaluator>
void evaluateSpanBlocks(const SurfaceData&                theData,
                        const NCollection_Array1<double>& theUParams,
                        const NCollection_Array1<double>& theVParams,
                        BlockEvaluator&&                  theEvaluator)
{
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  const occ::handle<BSplSLib_Cache> aCache = new BSplSLib_Cache(theData.UDegree,
                                                                theData.IsUPeriodic,
                                                                *theData.UFlatKnots,
                                                                theData.VDegree,
                                                                theData.IsVPeriodic,
                                                                *theData.VFlatKnots,
                                                                theData.Weights);
  NCollection_LocalArray<double>    aLocalU(aNbU);
  NCollection_LocalArray<double>    aLocalV(aNbV);
  NCollection_LocalArray<SpanBlock> aUBlocks(aNbU);
  NCollection_LocalArray<SpanBlock> aVBlocks(aNbV);
  const size_t aNbUBlocks = prepareSpanBlocks(theUParams,
                                              *theData.UFlatKnots,
                                              theData.UDegree,
                                              theData.IsUPeriodic,
                                              aLocalU,
                                              aUBlocks);
  const size_t aNbVBlocks = prepareSpanBlocks(theVParams,
                                              *theData.VFlatKnots,
                                              theData.VDegree,
                                              theData.IsVPeriodic,
                                              aLocalV,
                                              aVBlocks);

  for (size_t aUBlockIndex = 0; aUBlockIndex < aNbUBlocks; ++aUBlockIndex)
  {
    const SpanBlock& aUBlock = aUBlocks[aUBlockIndex];
    for (size_t aVBlockIndex = 0; aVBlockIndex < aNbVBlocks; ++aVBlockIndex)
    {
      const SpanBlock& aVBlock = aVBlocks[aVBlockIndex];
      aCache->BuildCacheForSpan(aUBlock.Span,
                                aVBlock.Span,
                                *theData.UFlatKnots,
                                *theData.VFlatKnots,
                                *theData.Poles,
                                theData.Weights);
      theEvaluator(*aCache,
                   aUBlock.Start,
                   aVBlock.Start,
                   aUBlock.Count,
                   aVBlock.Count,
                   static_cast<const double*>(aLocalU) + aUBlock.Start,
                   static_cast<const double*>(aLocalV) + aVBlock.Start);
    }
  }
}

//! Template helper for direct-only grid evaluation (D3, DN).
//! @tparam ResultT result type
//! @tparam EvalF functor: (data, adjU, adjV, uSpan, vSpan, result) -> void
template <typename ResultT, typename EvalF>
NCollection_Array2<ResultT> evaluateGridDirect(const SurfaceData&                theData,
                                               const NCollection_Array1<double>& theUParams,
                                               const NCollection_Array1<double>& theVParams,
                                               EvalF                             theEval)
{
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();

  NCollection_Array2<ResultT> aGrid(1,
                                    static_cast<int>(aNbU),
                                    1,
                                    static_cast<int>(aNbV));

  int    aPrevUSpan = -1;
  double aUSpanStart = 0.0;
  double aUSpanEnd  = 0.0;

  for (size_t aUIndex = 0; aUIndex < aNbU; ++aUIndex)
  {
    const double aU     = theUParams.At(aUIndex);
    double       aAdjU  = aU;
    int          aUSpan = 0;

    if (!theData.IsUPeriodic && aPrevUSpan >= 0 && aU >= aUSpanStart && aU < aUSpanEnd)
    {
      aAdjU  = aU;
      aUSpan = aPrevUSpan;
    }
    else
    {
      locateSpan(aU, *theData.UFlatKnots, theData.UDegree, theData.IsUPeriodic, aAdjU, aUSpan);
    }
    if (aUSpan != aPrevUSpan)
    {
      aPrevUSpan = aUSpan;
      aUSpanStart = theData.UFlatKnots->Value(aUSpan);
      aUSpanEnd  = theData.UFlatKnots->Value(aUSpan + 1);
    }

    int    aPrevVSpan = -1;
    double aVSpanStart = 0.0;
    double aVSpanEnd  = 0.0;

    for (size_t aVIndex = 0; aVIndex < aNbV; ++aVIndex)
    {
      const double aV     = theVParams.At(aVIndex);
      double       aAdjV  = aV;
      int          aVSpan = 0;

      if (!theData.IsVPeriodic && aPrevVSpan >= 0 && aV >= aVSpanStart && aV < aVSpanEnd)
      {
        aAdjV  = aV;
        aVSpan = aPrevVSpan;
      }
      else
      {
        locateSpan(aV, *theData.VFlatKnots, theData.VDegree, theData.IsVPeriodic, aAdjV, aVSpan);
      }
      if (aVSpan != aPrevVSpan)
      {
        aPrevVSpan = aVSpan;
        aVSpanStart = theData.VFlatKnots->Value(aVSpan);
        aVSpanEnd  = theData.VFlatKnots->Value(aVSpan + 1);
      }

      ResultT aResult;
      theEval(theData, aAdjU, aAdjV, aUSpan, aVSpan, aResult);
      aGrid.ChangeAt(aUIndex * aNbV + aVIndex) = aResult;
    }
  }

  return aGrid;
}

} // namespace

//=================================================================================================

NCollection_Array2<gp_Pnt> GeomGridEval_BSplineSurface::EvaluateGrid(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return NCollection_Array2<gp_Pnt>();
  }

  const size_t aNbV = theVParams.Size();
  NCollection_Array2<gp_Pnt> aGrid(1,
                                   static_cast<int>(theUParams.Size()),
                                   1,
                                   static_cast<int>(aNbV));
  evaluateSpanBlocks(aData,
                     theUParams,
                     theVParams,
                     [&](const BSplSLib_Cache& theCache,
                         const size_t           theUStart,
                         const size_t           theVStart,
                         const size_t           theNbU,
                         const size_t           theNbV,
                         const double*          theLocalU,
                         const double*          theLocalV) {
                       gp_Pnt* const aBlock = &aGrid.ChangeAt(theUStart * aNbV + theVStart);
                       theCache.D0GridLocal(
                         theLocalU, theNbU, theLocalV, theNbV, aBlock, aNbV);
                     });
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD1> GeomGridEval_BSplineSurface::EvaluateGridD1(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return {};
  }
  NCollection_Array2<GeomGridEval::SurfD1> aGrid(1,
                                                static_cast<int>(theUParams.Size()),
                                                1,
                                                static_cast<int>(theVParams.Size()));
  const size_t aNbV = theVParams.Size();
  evaluateSpanBlocks(aData,
                     theUParams,
                     theVParams,
                     [&](const BSplSLib_Cache& theCache,
                         const size_t           theUStart,
                         const size_t           theVStart,
                         const size_t           theNbU,
                         const size_t           theNbV,
                         const double*          theLocalU,
                         const double*          theLocalV) {
                       for (size_t aUIndex = 0; aUIndex < theNbU; ++aUIndex)
                       {
                         for (size_t aVIndex = 0; aVIndex < theNbV; ++aVIndex)
                         {
                           GeomGridEval::SurfD1& aResult = aGrid.ChangeAt(
                             (theUStart + aUIndex) * aNbV + theVStart + aVIndex);
                           theCache.D1Local(theLocalU[aUIndex],
                                            theLocalV[aVIndex],
                                            aResult.Point,
                                            aResult.D1U,
                                            aResult.D1V);
                         }
                       }
                     });
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD1Coords>
  GeomGridEval_BSplineSurface::EvaluateGridD1Coords(
    const NCollection_Array1<double>& theUParams,
    const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return {};
  }

  const size_t aNbV = theVParams.Size();
  NCollection_Array2<GeomGridEval::SurfD1Coords> aGrid(theUParams.Size(), aNbV);
  evaluateSpanBlocks(aData,
                     theUParams,
                     theVParams,
                     [&](const BSplSLib_Cache& theCache,
                         const size_t           theUStart,
                         const size_t           theVStart,
                         const size_t           theNbU,
                         const size_t           theNbV,
                         const double*          theLocalU,
                         const double*          theLocalV) {
                       GeomGridEval::SurfD1Coords* const aBlock =
                         &aGrid.ChangeAt(theUStart * aNbV + theVStart);
                       theCache.D1GridLocal(theLocalU,
                                            theNbU,
                                            theLocalV,
                                            theNbV,
                                            reinterpret_cast<double*>(aBlock),
                                            aNbV * 9);
                     });
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD2> GeomGridEval_BSplineSurface::EvaluateGridD2(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return {};
  }
  NCollection_Array2<GeomGridEval::SurfD2> aGrid(1,
                                                static_cast<int>(theUParams.Size()),
                                                1,
                                                static_cast<int>(theVParams.Size()));
  const size_t aNbV = theVParams.Size();
  evaluateSpanBlocks(aData,
                     theUParams,
                     theVParams,
                     [&](const BSplSLib_Cache& theCache,
                         const size_t           theUStart,
                         const size_t           theVStart,
                         const size_t           theNbU,
                         const size_t           theNbV,
                         const double*          theLocalU,
                         const double*          theLocalV) {
                       for (size_t aUIndex = 0; aUIndex < theNbU; ++aUIndex)
                       {
                         for (size_t aVIndex = 0; aVIndex < theNbV; ++aVIndex)
                         {
                           GeomGridEval::SurfD2& aResult = aGrid.ChangeAt(
                             (theUStart + aUIndex) * aNbV + theVStart + aVIndex);
                           theCache.D2Local(theLocalU[aUIndex],
                                            theLocalV[aVIndex],
                                            aResult.Point,
                                            aResult.D1U,
                                            aResult.D1V,
                                            aResult.D2U,
                                            aResult.D2V,
                                            aResult.D2UV);
                         }
                       }
                     });
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD2Coords>
  GeomGridEval_BSplineSurface::EvaluateGridD2Coords(
    const NCollection_Array1<double>& theUParams,
    const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return {};
  }

  const size_t aNbV = theVParams.Size();
  NCollection_Array2<GeomGridEval::SurfD2Coords> aGrid(theUParams.Size(), aNbV);
  evaluateSpanBlocks(aData,
                     theUParams,
                     theVParams,
                     [&](const BSplSLib_Cache& theCache,
                         const size_t           theUStart,
                         const size_t           theVStart,
                         const size_t           theNbU,
                         const size_t           theNbV,
                         const double*          theLocalU,
                         const double*          theLocalV) {
                       GeomGridEval::SurfD2Coords* const aBlock =
                         &aGrid.ChangeAt(theUStart * aNbV + theVStart);
                       theCache.D2GridLocal(theLocalU,
                                            theNbU,
                                            theLocalV,
                                            theNbV,
                                            reinterpret_cast<double*>(aBlock),
                                            aNbV * 18);
                     });
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD3> GeomGridEval_BSplineSurface::EvaluateGridD3(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || !extractSurfaceData(myGeom, aData))
  {
    return NCollection_Array2<GeomGridEval::SurfD3>();
  }

  return evaluateGridDirect<GeomGridEval::SurfD3>(aData,
                                                  theUParams,
                                                  theVParams,
                                                  [](const SurfaceData&    theData,
                                                     double                theU,
                                                     double                theV,
                                                     int                   theUSpan,
                                                     int                   theVSpan,
                                                     GeomGridEval::SurfD3& theResult) {
                                                    gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV;
                                                    BSplSLib::D3(theU,
                                                                 theV,
                                                                 theUSpan,
                                                                 theVSpan,
                                                                 *theData.Poles,
                                                                 theData.Weights,
                                                                 *theData.UFlatKnots,
                                                                 *theData.VFlatKnots,
                                                                 nullptr,
                                                                 nullptr,
                                                                 theData.UDegree,
                                                                 theData.VDegree,
                                                                 theData.IsURational,
                                                                 theData.IsVRational,
                                                                 theData.IsUPeriodic,
                                                                 theData.IsVPeriodic,
                                                                 theResult.Point,
                                                                 aD1U,
                                                                 aD1V,
                                                                 aD2U,
                                                                 aD2V,
                                                                 aD2UV,
                                                                 theResult.D3U,
                                                                 theResult.D3V,
                                                                 theResult.D3UUV,
                                                                 theResult.D3UVV);
                                                    theResult.D1U  = aD1U;
                                                    theResult.D1V  = aD1V;
                                                    theResult.D2U  = aD2U;
                                                    theResult.D2V  = aD2V;
                                                    theResult.D2UV = aD2UV;
                                                  });
}

//=================================================================================================

NCollection_Array2<gp_Vec> GeomGridEval_BSplineSurface::EvaluateGridDN(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams,
  int                               theNU,
  int                               theNV) const
{
  SurfaceData aData;
  if (theUParams.IsEmpty() || theVParams.IsEmpty() || theNU < 0 || theNV < 0 || (theNU + theNV) < 1
      || !extractSurfaceData(myGeom, aData))
  {
    return NCollection_Array2<gp_Vec>();
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();

  // Derivatives beyond degree are zero
  if (theNU > aData.UDegree || theNV > aData.VDegree)
  {
    NCollection_Array2<gp_Vec> aGrid(1,
                                     static_cast<int>(aNbU),
                                     1,
                                     static_cast<int>(aNbV));
    const gp_Vec               aZero(0.0, 0.0, 0.0);
    for (size_t anIndex = 0; anIndex < aGrid.Size(); ++anIndex)
    {
      aGrid.ChangeAt(anIndex) = aZero;
    }
    return aGrid;
  }

  return evaluateGridDirect<gp_Vec>(aData,
                                    theUParams,
                                    theVParams,
                                    [theNU, theNV](const SurfaceData& theData,
                                                   double             theU,
                                                   double             theV,
                                                   int                theUSpan,
                                                   int                theVSpan,
                                                   gp_Vec&            theResult) {
                                      BSplSLib::DN(theU,
                                                   theV,
                                                   theNU,
                                                   theNV,
                                                   theUSpan,
                                                   theVSpan,
                                                   *theData.Poles,
                                                   theData.Weights,
                                                   *theData.UFlatKnots,
                                                   *theData.VFlatKnots,
                                                   nullptr,
                                                   nullptr,
                                                   theData.UDegree,
                                                   theData.VDegree,
                                                   theData.IsURational,
                                                   theData.IsVRational,
                                                   theData.IsUPeriodic,
                                                   theData.IsVPeriodic,
                                                   theResult);
                                    });
}
