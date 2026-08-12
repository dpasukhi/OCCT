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

#include <ExtremaPC2d_BSplineCurve.hxx>

#include <math_Vector.hxx>
#include <NCollection_DynamicArray.hxx>

//==================================================================================================

ExtremaPC2d_BSplineCurve::ExtremaPC2d_BSplineCurve(const occ::handle<Geom2d_BSplineCurve>& theCurve)
    : myCurve(theCurve),
      myDomain{0.0, 1.0}
{
  if (myCurve.IsNull())
  {
    return;
  }
  myDomain.Min = myCurve->FirstParameter();
  myDomain.Max = myCurve->LastParameter();
  buildParams();
}

//==================================================================================================

ExtremaPC2d_BSplineCurve::ExtremaPC2d_BSplineCurve(const occ::handle<Geom2d_BSplineCurve>& theCurve,
                                               const ExtremaPC2d::Domain1D&            theDomain)
    : myCurve(theCurve),
      myDomain(theDomain)
{
  if (myCurve.IsNull())
  {
    return;
  }
  buildParams();
}

//==================================================================================================

math_Vector ExtremaPC2d_BSplineCurve::buildKnotAwareParams() const
{
  if (myCurve.IsNull())
  {
    // Fallback to uniform sampling
    return ExtremaPC2d_GridEvaluator::BuildUniformParams(myDomain.Min,
                                                       myDomain.Max,
                                                       ExtremaPC2d::THE_BSPLINE_FALLBACK_SAMPLES);
  }

  const double theUMin = myDomain.Min;
  const double theUMax = myDomain.Max;

  const int                         aDegree = myCurve->Degree();
  const NCollection_Array1<double>& aKnots  = myCurve->Knots();

  // Use multiplier*(degree+1) samples per span for accurate extrema detection.
  const int aSamplesPerSpan = ExtremaPC2d::THE_BSPLINE_SPAN_MULTIPLIER * (aDegree + 1);

  // Single-pass algorithm using dynamic vector
  NCollection_DynamicArray<double> aParams;
  aParams.Append(theUMin);

  const double aPeriod = myCurve->IsPeriodic() ? myCurve->Period() : 0.0;
  const int aFirstShift = myCurve->IsPeriodic()
                            ? static_cast<int>(std::floor((theUMin - aKnots.Last()) / aPeriod))
                            : 0;
  const int aLastShift = myCurve->IsPeriodic()
                           ? static_cast<int>(std::ceil((theUMax - aKnots.First()) / aPeriod))
                           : 0;
  for (int aShiftIndex = aFirstShift; aShiftIndex <= aLastShift; ++aShiftIndex)
  {
    const double aShift = aShiftIndex * aPeriod;
    for (int i = aKnots.Lower(); i < aKnots.Upper(); ++i)
    {
      const double aKnotLo = aKnots.Value(i) + aShift;
      const double aKnotHi = aKnots.Value(i + 1) + aShift;
      const double aSpanLo = std::max(aKnotLo, theUMin);
      const double aSpanHi = std::min(aKnotHi, theUMax);
      if (aSpanHi <= aSpanLo)
      {
        continue;
      }

      const double aStep = (aSpanHi - aSpanLo) / aSamplesPerSpan;
      for (int j = 1; j < aSamplesPerSpan; ++j)
      {
        const double aU = aSpanLo + j * aStep;
        if (aU > theUMin && aU < theUMax)
        {
          aParams.Append(aU);
        }
      }
      if (aKnotHi > theUMin && aKnotHi < theUMax)
      {
        aParams.Append(aKnotHi);
      }
    }
  }
  aParams.Append(theUMax);

  // Convert to math_Vector for the numerical evaluator.
  math_Vector aResult(1, aParams.Length());
  for (int i = 0; i < aParams.Length(); ++i)
  {
    aResult(i + 1) = aParams.Value(i);
  }

  return aResult;
}

//==================================================================================================

void ExtremaPC2d_BSplineCurve::buildParams() const
{
  if (myCurve.IsNull() || !myDomain.IsValid() || !myDomain.IsFinite())
  {
    return;
  }

  // Build knot-aware parameter partition.
  math_Vector aParams = buildKnotAwareParams();

  myEvaluator.SetParams(aParams);
}

//==================================================================================================

gp_Pnt2d ExtremaPC2d_BSplineCurve::Value(double theU) const
{
  return myCurve->Value(theU);
}

//==================================================================================================

const ExtremaPC2d::Result& ExtremaPC2d_BSplineCurve::Perform(const gp_Pnt2d&         theP,
                                                         double                theTol,
                                                         ExtremaPC2d::SearchMode theMode) const
{
  if (myCurve.IsNull())
  {
    myEvaluator.Result().Clear();
    myEvaluator.Result().Status = ExtremaPC2d::Status::NotDone;
    return myEvaluator.Result();
  }

  buildParams();
  Geom2dAdaptor_Curve anAdaptor(myCurve);
  return myEvaluator.Perform(anAdaptor, theP, myDomain, theTol, theMode);
}

//==================================================================================================

const ExtremaPC2d::Result& ExtremaPC2d_BSplineCurve::PerformWithEndpoints(
  const gp_Pnt2d&         theP,
  double                theTol,
  ExtremaPC2d::SearchMode theMode) const
{
  (void)Perform(theP, theTol, theMode);

  // Add endpoints to the result
  ExtremaPC2d::Result& aResult = myEvaluator.Result();
  if (aResult.Status == ExtremaPC2d::Status::OK || aResult.Status == ExtremaPC2d::Status::NoSolution)
  {
    Geom2dAdaptor_Curve anAdaptor(myCurve);
    const bool isClosed = ExtremaPC2d_GridEvaluator::IsClosedDomain(anAdaptor, myDomain);
    ExtremaPC2d::AddEndpointExtrema(aResult, theP, myDomain, *this, theMode, isClosed);

    // Update status if we found any extrema (including endpoints)
    if (!aResult.Extrema.IsEmpty())
    {
      aResult.Status = ExtremaPC2d::Status::OK;
    }
  }

  return aResult;
}
