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

#include <ExtremaPC_BSplineCurve.hxx>

#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_BSplineCurve.hxx>
#include <ProjLib.hxx>

#include <math_Vector.hxx>
#include <NCollection_LocalArray.hxx>
#include <Standard_Integer.hxx>
#include <Standard_RangeError.hxx>

#include <cmath>

//==================================================================================================

ExtremaPC_BSplineCurve::ExtremaPC_BSplineCurve(const occ::handle<Geom_BSplineCurve>& theCurve)
    : myCurve(theCurve),
      myDomain{0.0, 1.0}
{
  if (myCurve.IsNull())
  {
    return;
  }
  myAdaptor.Load(myCurve);
  myDomain.Min = myCurve->FirstParameter();
  myDomain.Max = myCurve->LastParameter();
  buildParams();
  initPlanar();
}

//==================================================================================================

ExtremaPC_BSplineCurve::ExtremaPC_BSplineCurve(const occ::handle<Geom_BSplineCurve>& theCurve,
                                               const ExtremaPC::Domain1D&            theDomain)
    : myCurve(theCurve),
      myDomain(theDomain)
{
  if (myCurve.IsNull())
  {
    return;
  }
  myAdaptor.Load(myCurve);
  buildParams();
  initPlanar();
}

//==================================================================================================

math_Vector ExtremaPC_BSplineCurve::buildKnotAwareParams() const
{
  if (myCurve.IsNull())
  {
    // Fallback to uniform sampling
    return ExtremaPC_GridEvaluator::BuildUniformParams(myDomain.Min,
                                                       myDomain.Max,
                                                       ExtremaPC::THE_BSPLINE_FALLBACK_SAMPLES);
  }

  const double theUMin = myDomain.Min;
  const double theUMax = myDomain.Max;

  const NCollection_Array1<double>& aKnots = myCurve->Knots();

  const bool   isPeriodic      = myCurve->IsPeriodic();
  const double aPeriod         = isPeriodic ? myCurve->Period() : 0.0;
  const double aFirstShiftReal = isPeriodic ? std::floor((theUMin - aKnots.Last()) / aPeriod) : 0.0;
  const double aLastShiftReal  = isPeriodic ? std::ceil((theUMax - aKnots.First()) / aPeriod) : 0.0;
  const double aNbShifts       = aLastShiftReal - aFirstShiftReal + 1.0;
  const double aMaxParams      = 2.0 + aNbShifts * static_cast<double>(aKnots.Size() - 1);
  Standard_RangeError_Raise_if(aFirstShiftReal < IntegerFirst() || aFirstShiftReal > IntegerLast()
                                 || aLastShiftReal < IntegerFirst()
                                 || aLastShiftReal > IntegerLast() || aNbShifts < 1.0
                                 || aMaxParams > IntegerLast(),
                               "ExtremaPC_BSplineCurve: parameter grid is too large");
  const int                           aFirstShift = static_cast<int>(aFirstShiftReal);
  const int                           aLastShift  = static_cast<int>(aLastShiftReal);
  NCollection_LocalArray<double, 128> aParams(static_cast<size_t>(aMaxParams));
  size_t                              aNbParams = 0;
  aParams[aNbParams++]                          = theUMin;
  for (int aShiftIndex = aFirstShift;; ++aShiftIndex)
  {
    const double aShift = aShiftIndex * aPeriod;
    for (size_t aKnotIndex = 0; aKnotIndex + 1 < aKnots.Size(); ++aKnotIndex)
    {
      const double aKnotLo = aKnots.At(aKnotIndex) + aShift;
      const double aKnotHi = aKnots.At(aKnotIndex + 1) + aShift;
      const double aSpanLo = std::max(aKnotLo, theUMin);
      const double aSpanHi = std::min(aKnotHi, theUMax);
      if (aSpanHi <= aSpanLo)
      {
        continue;
      }

      if (aKnotHi > theUMin && aKnotHi < theUMax)
      {
        aParams[aNbParams++] = aKnotHi;
      }
    }
    if (aShiftIndex == aLastShift)
    {
      break;
    }
  }
  aParams[aNbParams++] = theUMax;

  // Convert to math_Vector for the numerical evaluator.
  math_Vector aResult(aNbParams);
  for (size_t anIndex = 0; anIndex < aNbParams; ++anIndex)
  {
    aResult.ChangeAt(anIndex) = aParams[anIndex];
  }

  return aResult;
}

//==================================================================================================

void ExtremaPC_BSplineCurve::buildParams()
{
  if (myCurve.IsNull() || !myDomain.IsValid() || !myDomain.IsFinite())
  {
    return;
  }

  math_Vector  aParams                = buildKnotAwareParams();
  const size_t aDegree                = static_cast<size_t>(myCurve->Degree());
  const size_t aStationarityRootBound = (myCurve->IsRational() ? 3 : 2) * aDegree;
  const size_t aNbRootIntervals =
    std::max(MathRoot::THE_MIN_NB_INTERVALS,
             MathRoot::NbIntervalsForRootBound(aStationarityRootBound));
  myEvaluator.SetParams(aParams, aNbRootIntervals);
}

//==================================================================================================

void ExtremaPC_BSplineCurve::initPlanar()
{
  occ::handle<Geom2d_BSplineCurve> aCurve2d;
  if (ExtremaPC::ProjectBSpline(myCurve, myPlanarPlane, aCurve2d))
  {
    myPlanarEvaluator.emplace(aCurve2d, ExtremaPC2d::Domain1D(myDomain.Min, myDomain.Max));
  }
}

//==================================================================================================

bool ExtremaPC_BSplineCurve::performPlanar(const gp_Pnt&               theP,
                                           const double                theTol,
                                           const ExtremaPC::SearchMode theMode,
                                           const bool                  theIncludeEndpoints) const
{
  if (!myPlanarEvaluator.has_value())
  {
    return false;
  }

  const ExtremaPC2d::SearchMode aMode    = ExtremaPC::ToSearchMode2d(theMode);
  const gp_Pnt2d                aPoint2d = ProjLib::Project(myPlanarPlane, theP);
  const ExtremaPC2d::Result&    aResult2d =
    theIncludeEndpoints ? myPlanarEvaluator->PerformWithEndpoints(aPoint2d, theTol, aMode)
                        : myPlanarEvaluator->Perform(aPoint2d, theTol, aMode);
  const Adaptor3d_Curve& anAdaptor = myAdaptor;
  return ExtremaPC::ConvertPlanarResult(aResult2d,
                                        theP,
                                        myPlanarPlane,
                                        anAdaptor,
                                        myEvaluator.ChangeResult());
}

//==================================================================================================

const ExtremaPC::Result& ExtremaPC_BSplineCurve::Perform(const gp_Pnt&               theP,
                                                         const double                theTol,
                                                         const ExtremaPC::SearchMode theMode) const
{
  if (myCurve.IsNull())
  {
    myEvaluator.ChangeResult().Clear();
    myEvaluator.ChangeResult().Status = ExtremaPC::Status::NotDone;
    return myEvaluator.ChangeResult();
  }

  if (ExtremaPC::IsValidTolerance(theTol) && myDomain.IsValid()
      && performPlanar(theP, theTol, theMode, false))
  {
    return myEvaluator.ChangeResult();
  }

  return myEvaluator.Perform(myAdaptor, theP, myDomain, theTol, theMode);
}

//==================================================================================================

const ExtremaPC::Result& ExtremaPC_BSplineCurve::PerformWithEndpoints(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  if (myCurve.IsNull())
  {
    myEvaluator.ChangeResult().Clear();
    myEvaluator.ChangeResult().Status = ExtremaPC::Status::NotDone;
    return myEvaluator.ChangeResult();
  }
  if (ExtremaPC::IsValidTolerance(theTol) && myDomain.IsValid()
      && performPlanar(theP, theTol, theMode, true))
  {
    return myEvaluator.ChangeResult();
  }

  const ExtremaPC::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone())
  {
    return anInteriorResult;
  }
  ExtremaPC::Result&     aResult   = myEvaluator.ChangeResult();
  const bool             isClosed  = ExtremaPC_GridEvaluator::IsClosedDomain(myAdaptor, myDomain);
  const Adaptor3d_Curve& anAdaptor = myAdaptor;
  ExtremaPC::AddEndpointExtrema(aResult, theP, myDomain, anAdaptor, theMode, isClosed);
  if (!aResult.Extrema.IsEmpty())
  {
    aResult.Status = ExtremaPC::Status::OK;
  }

  return aResult;
}
