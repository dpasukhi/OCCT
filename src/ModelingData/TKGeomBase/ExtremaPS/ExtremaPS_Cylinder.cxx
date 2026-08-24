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

#include <ExtremaPS_Cylinder.hxx>

#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <MathUtils_Core.hxx>

#include <cmath>

ExtremaPS_Cylinder::ExtremaPS_Cylinder(const gp_Cylinder& theCylinder)
    : myCylinder(theCylinder),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_Cylinder::ExtremaPS_Cylinder(const gp_Cylinder&         theCylinder,
                                       const ExtremaPS::Domain2D& theDomain)
    : myCylinder(theCylinder),
      myDomain(isNaturalDomain(theDomain) ? std::nullopt
                                          : std::optional<ExtremaPS::Domain2D>(theDomain))
{
  initCache();
}

//==================================================================================================

bool ExtremaPS_Cylinder::isNaturalDomain(const ExtremaPS::Domain2D& theDomain)
{
  return theDomain.IsUFullPeriod(MathUtils::THE_2PI) && !theDomain.V().IsFinite();
}

//==================================================================================================

void ExtremaPS_Cylinder::initCache()
{
  const gp_Ax3& aPos  = myCylinder.Position();
  const gp_Pnt& aLoc  = aPos.Location();
  const gp_Dir& aAxis = aPos.Direction();
  const gp_Dir& aXDir = aPos.XDirection();
  const gp_Dir& aYDir = aPos.YDirection();

  myLocX   = aLoc.X();
  myLocY   = aLoc.Y();
  myLocZ   = aLoc.Z();
  myAxisX  = aAxis.X();
  myAxisY  = aAxis.Y();
  myAxisZ  = aAxis.Z();
  myXDirX  = aXDir.X();
  myXDirY  = aXDir.Y();
  myXDirZ  = aXDir.Z();
  myYDirX  = aYDir.X();
  myYDirY  = aYDir.Y();
  myYDirZ  = aYDir.Z();
  myRadius = myCylinder.Radius();
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Cylinder::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  myResult.Clear();
  if (!ExtremaPC::IsFinitePoint(theP) || !ExtremaPC::IsValidTolerance(theTol)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  const double aDx = theP.X() - myLocX;
  const double aDy = theP.Y() - myLocY;
  const double aDz = theP.Z() - myLocZ;

  const double aV = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;

  const double aRadX         = aDx - aV * myAxisX;
  const double aRadY         = aDy - aV * myAxisY;
  const double aRadZ         = aDz - aV * myAxisZ;
  const double aRadialDistSq = aRadX * aRadX + aRadY * aRadY + aRadZ * aRadZ;

  if (aRadialDistSq < theTol * theTol)
  {
    if (theMode == ExtremaPS::SearchMode::Max
        || (myDomain.has_value() && !myDomain->V().Contains(aV, theTol)))
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }

    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = myRadius * myRadius;
    return myResult;
  }

  const double aRadialDist = std::sqrt(aRadialDistSq);
  const double aInvRad     = 1.0 / aRadialDist;

  const double aRadNormX = aRadX * aInvRad;
  const double aRadNormY = aRadY * aInvRad;
  const double aRadNormZ = aRadZ * aInvRad;

  const double aCosU = aRadNormX * myXDirX + aRadNormY * myXDirY + aRadNormZ * myXDirZ;
  const double aSinU = aRadNormX * myYDirX + aRadNormY * myYDirY + aRadNormZ * myYDirZ;
  double       aU    = std::atan2(aSinU, aCosU);
  if (aU < 0.0)
    aU += MathUtils::THE_2PI;

  if (myDomain.has_value())
  {
    aU = ElCLib::InPeriod(aU, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
  }

  if (!myDomain.has_value())
  {
    const double aAxisOffX = myLocX + aV * myAxisX;
    const double aAxisOffY = myLocY + aV * myAxisY;
    const double aAxisOffZ = myLocZ + aV * myAxisZ;

    const double aRadCompX = myRadius * aRadNormX;
    const double aRadCompY = myRadius * aRadNormY;
    const double aRadCompZ = myRadius * aRadNormZ;

    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt(aAxisOffX + aRadCompX, aAxisOffY + aRadCompY, aAxisOffZ + aRadCompZ);
      const double aDistMinSq = (aRadialDist - myRadius) * (aRadialDist - myRadius);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aU;
      anExt.V              = aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistMinSq;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }

    if (theMode != ExtremaPS::SearchMode::Min)
    {
      const gp_Pnt aSurfPt(aAxisOffX - aRadCompX, aAxisOffY - aRadCompY, aAxisOffZ - aRadCompZ);
      const double aDistMaxSq = (aRadialDist + myRadius) * (aRadialDist + myRadius);

      double aUOpp = aU + MathUtils::THE_PI;
      if (aUOpp >= MathUtils::THE_2PI)
        aUOpp -= MathUtils::THE_2PI;

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aUOpp;
      anExt.V              = aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistMaxSq;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const ExtremaPS::Domain2D& theDomain = *myDomain;

  const bool aIsFullU = theDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);

  const bool aVInRange = theDomain.V().Contains(aV, theTol);

  if (aIsFullU)
  {
    if (!aVInRange)
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }
    const double aClampedV = theDomain.V().Clamp(aV);

    const double aAxisOffX = myLocX + aClampedV * myAxisX;
    const double aAxisOffY = myLocY + aClampedV * myAxisY;
    const double aAxisOffZ = myLocZ + aClampedV * myAxisZ;

    const double aRadCompX = myRadius * aRadNormX;
    const double aRadCompY = myRadius * aRadNormY;
    const double aRadCompZ = myRadius * aRadNormZ;

    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt(aAxisOffX + aRadCompX, aAxisOffY + aRadCompY, aAxisOffZ + aRadCompZ);
      const double aDistMinSq =
        (aRadialDist - myRadius) * (aRadialDist - myRadius) + (aV - aClampedV) * (aV - aClampedV);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aU;
      anExt.V              = aClampedV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistMinSq;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }

    if (theMode != ExtremaPS::SearchMode::Min)
    {
      const gp_Pnt aSurfPt(aAxisOffX - aRadCompX, aAxisOffY - aRadCompY, aAxisOffZ - aRadCompZ);
      const double aDistMaxSq =
        (aRadialDist + myRadius) * (aRadialDist + myRadius) + (aV - aClampedV) * (aV - aClampedV);

      double aUOpp = aU + MathUtils::THE_PI;
      aUOpp        = ElCLib::InPeriod(aUOpp, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aUOpp;
      anExt.V              = aClampedV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistMaxSq;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const double aClampedV = theDomain.V().Clamp(aV);

  if (theMode != ExtremaPS::SearchMode::Max
      && ExtremaPS::IsInPeriodicRange(aU, theDomain.U(), theTol) && aVInRange)
  {
    const double aNormalizedU =
      ElCLib::InPeriod(aU, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI);
    const double aClampedU = theDomain.U().Clamp(aNormalizedU);

    const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myCylinder);
    const double aSqDist = theP.SquareDistance(aSurfPt);

    ExtremaPS::ExtremumResult anExt;
    anExt.U              = aClampedU;
    anExt.V              = aClampedV;
    anExt.Point          = aSurfPt;
    anExt.SquareDistance = aSqDist;
    anExt.IsMinimum      = true;
    myResult.Extrema.Append(anExt);
  }

  double aUOpp = aU + MathUtils::THE_PI;
  aUOpp        = ElCLib::InPeriod(aUOpp, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI);

  if (theMode != ExtremaPS::SearchMode::Min
      && ExtremaPS::IsInPeriodicRange(aUOpp, theDomain.U(), theTol) && aVInRange)
  {
    const double aNormalizedU =
      ElCLib::InPeriod(aUOpp, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI);
    const double aClampedU = theDomain.U().Clamp(aNormalizedU);

    const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myCylinder);
    const double aSqDist = theP.SquareDistance(aSurfPt);

    ExtremaPS::ExtremumResult anExt;
    anExt.U              = aClampedU;
    anExt.V              = aClampedV;
    anExt.Point          = aSurfPt;
    anExt.SquareDistance = aSqDist;
    anExt.IsMinimum      = false;
    myResult.Extrema.Append(anExt);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Cylinder::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }

  const ExtremaPS::Domain2D& theDomain = *myDomain;
  const double               aDx       = theP.X() - myLocX;
  const double               aDy       = theP.Y() - myLocY;
  const double               aDz       = theP.Z() - myLocZ;
  const double               aV        = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;
  const double               aRadX     = aDx - aV * myAxisX;
  const double               aRadY     = aDy - aV * myAxisY;
  const double               aRadZ     = aDz - aV * myAxisZ;
  const double               aRadialSq = aRadX * aRadX + aRadY * aRadY + aRadZ * aRadZ;
  if (aRadialSq < theTol * theTol && theDomain.V().IsFinite())
  {
    double aBoundaryV = theDomain.V().Clamp(aV);
    if (theMode == ExtremaPS::SearchMode::Max)
    {
      aBoundaryV = std::abs(aV - theDomain.VMin) > std::abs(aV - theDomain.VMax) ? theDomain.VMin
                                                                                 : theDomain.VMax;
    }
    const double aDeltaV            = aV - aBoundaryV;
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = myRadius * myRadius + aDeltaV * aDeltaV;
    return myResult;
  }

  const bool aIsFullU     = theDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);
  const bool aVOutOfRange = !theDomain.V().Contains(aV, theTol);

  if (!aIsFullU || aVOutOfRange)
  {
    ExtremaPS::AddBoundaryExtrema(myResult, theP, theDomain, myCylinder, theTol, theMode);
  }

  if (myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
  }
  else
  {
    myResult.Status = ExtremaPS::Status::OK;
  }

  return myResult;
}
