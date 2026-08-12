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

#include <ExtremaPC_BezierCurve.hxx>

//==================================================================================================

ExtremaPC_BezierCurve::ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve)
    : myCurve(theCurve),
      myDomain{0.0, 1.0},
      myNbSamples(ExtremaPC::THE_BEZIER_MIN_SAMPLES)
{
  if (myCurve.IsNull())
  {
    return;
  }
  myAdaptor.Load(myCurve);
  myDomain.Min = myCurve->FirstParameter();
  myDomain.Max = myCurve->LastParameter();
  myNbSamples  = std::max(ExtremaPC::THE_BEZIER_MIN_SAMPLES,
                         ExtremaPC::THE_BEZIER_DEGREE_MULTIPLIER * (myCurve->Degree() + 1));
  buildParams();
}

//==================================================================================================

ExtremaPC_BezierCurve::ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve,
                                             const ExtremaPC::Domain1D&           theDomain)
    : myCurve(theCurve),
      myDomain(theDomain),
      myNbSamples(ExtremaPC::THE_BEZIER_MIN_SAMPLES)
{
  if (myCurve.IsNull())
  {
    return;
  }
  myAdaptor.Load(myCurve);
  myNbSamples = std::max(ExtremaPC::THE_BEZIER_MIN_SAMPLES,
                         ExtremaPC::THE_BEZIER_DEGREE_MULTIPLIER * (myCurve->Degree() + 1));
  buildParams();
}

//==================================================================================================

void ExtremaPC_BezierCurve::buildParams()
{
  if (myCurve.IsNull() || !myDomain.IsValid() || !myDomain.IsFinite())
  {
    return;
  }

  math_Vector aParams =
    ExtremaPC_GridEvaluator::BuildUniformParams(myDomain.Min, myDomain.Max, myNbSamples);

  myEvaluator.SetParams(aParams);
}

//==================================================================================================

gp_Pnt ExtremaPC_BezierCurve::Value(double theU) const
{
  return myCurve->Value(theU);
}

//==================================================================================================

const ExtremaPC::Result& ExtremaPC_BezierCurve::Perform(const gp_Pnt&         theP,
                                                        double                theTol,
                                                        ExtremaPC::SearchMode theMode) const
{
  if (myCurve.IsNull())
  {
    myEvaluator.Result().Clear();
    myEvaluator.Result().Status = ExtremaPC::Status::NotDone;
    return myEvaluator.Result();
  }

  return myEvaluator.Perform(myAdaptor, theP, myDomain, theTol, theMode);
}

//==================================================================================================

const ExtremaPC::Result& ExtremaPC_BezierCurve::PerformWithEndpoints(
  const gp_Pnt&         theP,
  double                theTol,
  ExtremaPC::SearchMode theMode) const
{
  (void)Perform(theP, theTol, theMode);

  // Add endpoints to the result
  ExtremaPC::Result& aResult = myEvaluator.Result();
  if (aResult.Status == ExtremaPC::Status::OK || aResult.Status == ExtremaPC::Status::NoSolution)
  {
    const bool isClosed = ExtremaPC_GridEvaluator::IsClosedDomain(myAdaptor, myDomain);
    ExtremaPC::AddEndpointExtrema(aResult, theP, myDomain, *this, theMode, isClosed);

    // Update status if we found any extrema (including endpoints)
    if (!aResult.Extrema.IsEmpty())
    {
      aResult.Status = ExtremaPC::Status::OK;
    }
  }

  return aResult;
}
