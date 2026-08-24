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

#include <ExtremaPC_Circle.hxx>

#include <ElCLib.hxx>
#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_Circle.hxx>
#include <gp_Vec.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>
#include <ProjLib.hxx>

#include <cmath>

ExtremaPC_Circle::ExtremaPC_Circle(const gp_Circ& theCircle)
    : myCircle(theCircle),
      myDomain(std::nullopt)
{
}

//==================================================================================================

ExtremaPC_Circle::ExtremaPC_Circle(const gp_Circ& theCircle, const ExtremaPC::Domain1D& theDomain)
    : myCircle(theCircle),
      myDomain(theDomain)
{
}

//==================================================================================================

gp_Pnt ExtremaPC_Circle::EvalD0(const double theU) const
{
  return ElCLib::Value(theU, myCircle);
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Circle::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return myResult;
  }

  const gp_Pln aPlane(gp_Ax3(myCircle.Position()));
  if (ExtremaPC::PerformPlanar<ExtremaPC2d_Circle>(ProjLib::Project(aPlane, myCircle),
                                                   myDomain,
                                                   ProjLib::Project(aPlane, theP),
                                                   theP,
                                                   aPlane,
                                                   *this,
                                                   theTol,
                                                   theMode,
                                                   false,
                                                   myResult))
  {
    return myResult;
  }

  if (myCircle.Radius() <= gp::Resolution())
  {
    myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = theP.SquareDistance(myCircle.Location());
    return myResult;
  }

  // Step 1: Project point P onto the circle plane
  const gp_Pnt& aCenter = myCircle.Location();
  const gp_Dir& aAxis   = myCircle.Axis().Direction();
  gp_Vec        aToP(aCenter, theP);
  double        aHeight = aToP.Dot(gp_Vec(aAxis));
  gp_Vec        aTrsl   = gp_Vec(aAxis) * (-aHeight);
  gp_Pnt        aPp     = theP.Translated(aTrsl);

  // Step 2: Check for degenerate case - point projects to center
  gp_Vec aOPp(aCenter, aPp);
  double aOPpMag = aOPp.Magnitude();

  if (aOPpMag <= gp::Resolution())
  {
    // Point is on the circle axis - all points on circle are equidistant
    myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
    double aRadius                  = myCircle.Radius();
    myResult.InfiniteSquareDistance = aRadius * aRadius + aHeight * aHeight;
    return myResult;
  }

  // Step 3: Compute the angle of the closest point
  // Us1 corresponds to minimum distance (closest point)
  double aUs1 = myCircle.XAxis().Direction().AngleWithRef(aOPp, aAxis);

  // Handle angle boundaries
  if (aUs1 + MathUtils::THE_PI < Precision::Angular())
  {
    aUs1 = -MathUtils::THE_PI;
  }
  else if (aUs1 - MathUtils::THE_PI > -Precision::Angular())
  {
    aUs1 = MathUtils::THE_PI;
  }

  // Us2 = Us1 + PI corresponds to maximum distance (farthest point)
  double aUs2 = aUs1 + MathUtils::THE_PI;

  // Step 4: For bounded case, lift each canonical solution into the requested domain.
  double aTolU = Precision::Angular();
  if (myDomain.has_value())
  {
    const double theUMin = myDomain->Min;

    double aRadius = myCircle.Radius();
    if (aRadius > gp::Resolution())
    {
      aTolU = theTol / aRadius;
    }

    const double aLiftTolerance = Precision::Angular();
    aUs1 +=
      std::ceil((theUMin - aUs1 - aLiftTolerance) / (MathUtils::THE_2PI)) * (MathUtils::THE_2PI);
    aUs2 +=
      std::ceil((theUMin - aUs2 - aLiftTolerance) / (MathUtils::THE_2PI)) * (MathUtils::THE_2PI);
    if (std::abs(aUs1 - theUMin) <= aLiftTolerance)
      aUs1 = theUMin;
    if (std::abs(aUs2 - theUMin) <= aLiftTolerance)
      aUs2 = theUMin;
  }

  // Step 5: Add extrema (with bounds check if domain specified)
  // Skip based on search mode: i=0 is minimum, i=1 is maximum
  const size_t aStart = (theMode == ExtremaPC::SearchMode::Max) ? 1 : 0;
  const size_t aEnd   = (theMode == ExtremaPC::SearchMode::Min) ? 1 : 2;

  const double aSolutions[2] = {aUs1, aUs2};

  for (size_t anIndex = aStart; anIndex < aEnd; ++anIndex)
  {
    const double aU = aSolutions[anIndex];

    // Check bounds only if domain is specified
    if (myDomain.has_value())
    {
      if (aU >= myDomain->Min - aTolU && aU <= myDomain->Max + aTolU)
      {
        gp_Pnt aCurvePt = ElCLib::Value(aU, myCircle);

        ExtremaPC::ExtremumResult anExt;
        anExt.Parameter      = aU;
        anExt.Point          = aCurvePt;
        anExt.SquareDistance = theP.SquareDistance(aCurvePt);
        anExt.IsMinimum      = (anIndex == 0);
        anExt.IsMaximum      = (anIndex == 1);

        myResult.Extrema.Append(anExt);
      }
      continue;
    }

    gp_Pnt aCurvePt = ElCLib::Value(aU, myCircle);

    ExtremaPC::ExtremumResult anExt;
    anExt.Parameter      = aU;
    anExt.Point          = aCurvePt;
    anExt.SquareDistance = theP.SquareDistance(aCurvePt);
    anExt.IsMinimum      = (anIndex == 0); // First solution is minimum, second is maximum
    anExt.IsMaximum      = (anIndex == 1);

    myResult.Extrema.Append(anExt);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution : ExtremaPC::Status::OK;
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Circle::PerformWithEndpoints(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Clear();
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return myResult;
  }

  if (myDomain.has_value() && myDomain->IsValid() && myDomain->Min == myDomain->Max)
  {
    myResult.Clear();
    ExtremaPC::AddEndpointExtrema(myResult, theP, *myDomain, *this, theMode, false);
    myResult.Status =
      myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution : ExtremaPC::Status::OK;
    return myResult;
  }

  const ExtremaPC::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }
  const bool isClosed =
    ExtremaPC::IsClosedPeriodicDomain(*myDomain, MathUtils::THE_2PI, Precision::Angular());
  ExtremaPC::AddEndpointExtrema(myResult, theP, *myDomain, *this, theMode, isClosed);
  if (!myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPC::Status::OK;
  }

  return myResult;
}
