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

#include <ExtremaPS_BSplineSurface.hxx>

#include <MathRoot_Multiple.hxx>
#include <NCollection_Array1.hxx>
#include <Precision.hxx>

#include <utility>

namespace
{
//! Builds a degree-aware sampling grid aligned with BSpline knot spans.
NCollection_Array1<double> BuildKnotAwareParams(const NCollection_Array1<double>& theKnots,
                                                const int                         theDegree,
                                                const bool                        theIsRational,
                                                const double                      theParMin,
                                                const double                      theParMax)
{
  size_t aNbSpans = 0;
  for (size_t anIndex = 0; anIndex + 1 < theKnots.Size(); ++anIndex)
  {
    const double aKnotLo = theKnots.At(anIndex);
    const double aKnotHi = theKnots.At(anIndex + 1);
    if (aKnotHi >= theParMin + Precision::PConfusion()
        && aKnotLo <= theParMax - Precision::PConfusion())
    {
      ++aNbSpans;
    }
  }
  aNbSpans = std::max<size_t>(1, aNbSpans);

  const size_t aDegreeRootBound  = (theIsRational ? 3 : 2) * static_cast<size_t>(theDegree);
  const size_t aBaseIntervals    = MathRoot::NbIntervalsForRootBound(aDegreeRootBound);
  const size_t aDefaultIntervals = ExtremaPS::THE_DEFAULT_NB_SAMPLES - 1;
  const size_t aMinIntervals     = (aDefaultIntervals + aNbSpans - 1) / aNbSpans;
  const size_t aNbIntervals      = std::max(aBaseIntervals, aMinIntervals);

  const auto forEachParameter = [&](auto&& theAppend) {
    double aPrevious = theParMin;
    theAppend(theParMin);
    for (size_t anIndex = 0; anIndex + 1 < theKnots.Size(); ++anIndex)
    {
      double aKnotLo = theKnots.At(anIndex);
      double aKnotHi = theKnots.At(anIndex + 1);
      if (aKnotHi < theParMin + Precision::PConfusion())
      {
        continue;
      }
      if (aKnotLo > theParMax - Precision::PConfusion())
      {
        break;
      }
      aKnotLo           = std::max(aKnotLo, theParMin);
      aKnotHi           = std::min(aKnotHi, theParMax);
      const double aStep = (aKnotHi - aKnotLo) / static_cast<double>(aNbIntervals);
      for (size_t anInterval = 0; anInterval <= aNbIntervals; ++anInterval)
      {
        const double aParameter = aKnotLo + static_cast<double>(anInterval) * aStep;
        if (aParameter > theParMax - Precision::PConfusion())
        {
          break;
        }
        if (aParameter > aPrevious + Precision::PConfusion())
        {
          theAppend(aParameter);
          aPrevious = aParameter;
        }
      }
    }
    if (theParMax > aPrevious + Precision::PConfusion())
    {
      theAppend(theParMax);
    }
  };

  size_t aNbParameters = 0;
  forEachParameter([&](const double) { ++aNbParameters; });
  NCollection_Array1<double> aResult(aNbParameters);
  size_t                     anOutputIndex = 0;
  forEachParameter(
    [&](const double theParameter) { aResult.ChangeAt(anOutputIndex++) = theParameter; });

  return aResult;
}
} // namespace

//==================================================================================================

ExtremaPS_BSplineSurface::ExtremaPS_BSplineSurface(
  const occ::handle<Geom_BSplineSurface>& theSurface)
    : mySurface(theSurface)
{
  if (mySurface.IsNull())
  {
    return;
  }
  myAdaptor.Load(mySurface);

  // Get bounds from surface
  double aU1, aU2, aV1, aV2;
  theSurface->Bounds(aU1, aU2, aV1, aV2);
  myDomain = ExtremaPS::Domain2D(aU1, aU2, aV1, aV2);

  buildGrid();
}

//==================================================================================================

ExtremaPS_BSplineSurface::ExtremaPS_BSplineSurface(
  const occ::handle<Geom_BSplineSurface>& theSurface,
  const ExtremaPS::Domain2D&              theDomain)
    : mySurface(theSurface),
      myDomain(theDomain)
{
  if (mySurface.IsNull())
  {
    return;
  }
  myAdaptor.Load(mySurface);

  buildGrid();
}

//==================================================================================================

void ExtremaPS_BSplineSurface::buildGrid()
{
  if (mySurface.IsNull())
  {
    return;
  }

  const bool  isRational = mySurface->IsURational() || mySurface->IsVRational();
  NCollection_Array1<double> aUParams = BuildKnotAwareParams(mySurface->UKnots(),
                                                             mySurface->UDegree(),
                                                             isRational,
                                                             myDomain.UMin,
                                                             myDomain.UMax);
  NCollection_Array1<double> aVParams = BuildKnotAwareParams(mySurface->VKnots(),
                                                             mySurface->VDegree(),
                                                             isRational,
                                                             myDomain.VMin,
                                                             myDomain.VMax);

  const GeomGridEval_BSplineSurface anEval(mySurface);
  myEvaluator.SetGrid(anEval.EvaluateGridD1Coords(aUParams, aVParams),
                      std::move(aUParams),
                      std::move(aVParams));
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_BSplineSurface::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  if (mySurface.IsNull())
  {
    myEvaluator.ChangeResult().Clear();
    myEvaluator.ChangeResult().Status = ExtremaPS::Status::NotDone;
    return myEvaluator.ChangeResult();
  }
  return myEvaluator.Perform(myAdaptor, theP, myDomain, theTol, theMode);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_BSplineSurface::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone())
  {
    return anInteriorResult;
  }
  ExtremaPS::Result& aResult = myEvaluator.ChangeResult();
  ExtremaPS::AddBoundaryExtrema(aResult, theP, myDomain, myAdaptor, theTol, theMode);

  if (!aResult.Extrema.IsEmpty())
  {
    aResult.Status = ExtremaPS::Status::OK;
  }

  return aResult;
}
