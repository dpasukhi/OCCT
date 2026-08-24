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

#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_BezierCurve.hxx>
#include <ProjLib.hxx>

//==================================================================================================

ExtremaPC_BezierCurve::ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve)
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

ExtremaPC_BezierCurve::ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve,
                                             const ExtremaPC::Domain1D&           theDomain)
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

void ExtremaPC_BezierCurve::buildParams()
{
  if (myCurve.IsNull() || !myDomain.IsValid() || !myDomain.IsFinite())
  {
    return;
  }

  const size_t aNbSamples =
    std::max(ExtremaPC::THE_BEZIER_MIN_SAMPLES,
             ExtremaPC::THE_BEZIER_DEGREE_MULTIPLIER * static_cast<size_t>(myCurve->Degree() + 1));

  math_Vector aParams =
    ExtremaPC_GridEvaluator::BuildUniformParams(myDomain.Min, myDomain.Max, aNbSamples);

  myEvaluator.SetParams(aParams);
}

//==================================================================================================

void ExtremaPC_BezierCurve::initPlanar()
{
  occ::handle<Geom2d_BezierCurve> aCurve2d;
  if (ExtremaPC::ProjectBezier(myCurve, myPlanarPlane, aCurve2d))
  {
    myPlanarEvaluator.emplace(aCurve2d, ExtremaPC2d::Domain1D(myDomain.Min, myDomain.Max));
  }
}

//==================================================================================================

bool ExtremaPC_BezierCurve::performPlanar(const gp_Pnt&               theP,
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

const ExtremaPC::Result& ExtremaPC_BezierCurve::Perform(const gp_Pnt&               theP,
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

const ExtremaPC::Result& ExtremaPC_BezierCurve::PerformWithEndpoints(
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
