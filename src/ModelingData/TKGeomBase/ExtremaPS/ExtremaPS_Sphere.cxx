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

#include <ExtremaPS_Sphere.hxx>

#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>

#include <cmath>

ExtremaPS_Sphere::ExtremaPS_Sphere(const gp_Sphere& theSphere)
    : mySphere(theSphere),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_Sphere::ExtremaPS_Sphere(const gp_Sphere& theSphere, const ExtremaPS::Domain2D& theDomain)
    : mySphere(theSphere),
      myDomain(isNaturalDomain(theDomain) ? std::nullopt
                                          : std::optional<ExtremaPS::Domain2D>(theDomain))
{
  initCache();
}

//==================================================================================================

bool ExtremaPS_Sphere::isNaturalDomain(const ExtremaPS::Domain2D& theDomain)
{
  return theDomain.IsUFullPeriod(MathUtils::THE_2PI)
         && theDomain.VMin <= -(0.5 * MathUtils::THE_PI) + Precision::PConfusion()
         && theDomain.VMax >= (0.5 * MathUtils::THE_PI) - Precision::PConfusion();
}

//==================================================================================================

void ExtremaPS_Sphere::initCache()
{
  const gp_Ax3& aPos    = mySphere.Position();
  const gp_Pnt& aCenter = aPos.Location();
  myCenterX             = aCenter.X();
  myCenterY             = aCenter.Y();
  myCenterZ             = aCenter.Z();

  const gp_Dir& aAxis = aPos.Direction();
  myAxisX             = aAxis.X();
  myAxisY             = aAxis.Y();
  myAxisZ             = aAxis.Z();

  const gp_Dir& aXDir = aPos.XDirection();
  myXDirX             = aXDir.X();
  myXDirY             = aXDir.Y();
  myXDirZ             = aXDir.Z();

  const gp_Dir& aYDir = aPos.YDirection();
  myYDirX             = aYDir.X();
  myYDirY             = aYDir.Y();
  myYDirZ             = aYDir.Z();

  myRadius = mySphere.Radius();
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Sphere::Perform(
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

  const double aDx     = theP.X() - myCenterX;
  const double aDy     = theP.Y() - myCenterY;
  const double aDz     = theP.Z() - myCenterZ;
  const double aDistSq = aDx * aDx + aDy * aDy + aDz * aDz;

  if (aDistSq < theTol * theTol)
  {
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = myRadius * myRadius;
    return myResult;
  }

  const double aDist    = std::sqrt(aDistSq);
  const double aInvDist = 1.0 / aDist;

  const double aDirX = aDx * aInvDist;
  const double aDirY = aDy * aInvDist;
  const double aDirZ = aDz * aInvDist;

  double aSinV    = aDirX * myAxisX + aDirY * myAxisY + aDirZ * myAxisZ;
  aSinV           = aSinV < -1.0 ? -1.0 : (aSinV > 1.0 ? 1.0 : aSinV);
  const double aV = std::asin(aSinV);

  const double aCosV = std::cos(aV);
  double       aU    = 0.0;

  const bool aIsAtPole = (std::abs(aCosV) < Precision::PConfusion());
  if (!aIsAtPole)
  {
    const double aInvCosV = 1.0 / aCosV;
    const double aCosU    = (aDirX * myXDirX + aDirY * myXDirY + aDirZ * myXDirZ) * aInvCosV;
    const double aSinU    = (aDirX * myYDirX + aDirY * myYDirY + aDirZ * myYDirZ) * aInvCosV;
    aU                    = std::atan2(aSinU, aCosU);
    if (aU < 0.0)
      aU += MathUtils::THE_2PI;

    if (myDomain.has_value())
    {
      aU = ElCLib::InPeriod(aU, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
    }
  }
  else if (myDomain.has_value())
  {
    aU = (myDomain->UMin + myDomain->UMax) * 0.5;
  }

  if (!myDomain.has_value())
  {
    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt(myCenterX + myRadius * aDirX,
                           myCenterY + myRadius * aDirY,
                           myCenterZ + myRadius * aDirZ);
      const double aDistToSurf = aDist - myRadius;

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aU;
      anExt.V              = aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistToSurf * aDistToSurf;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }

    if (theMode != ExtremaPS::SearchMode::Min)
    {
      const gp_Pnt aSurfPt(myCenterX - myRadius * aDirX,
                           myCenterY - myRadius * aDirY,
                           myCenterZ - myRadius * aDirZ);
      const double aDistToSurf = aDist + myRadius;

      double aUOpp = aU + MathUtils::THE_PI;
      if (aUOpp >= MathUtils::THE_2PI)
        aUOpp -= MathUtils::THE_2PI;

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aUOpp;
      anExt.V              = -aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistToSurf * aDistToSurf;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const ExtremaPS::Domain2D& theDomain = *myDomain;

  const bool aIsFullU = theDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);
  const bool aIsFullV = (theDomain.VMin <= -0.5 * MathUtils::THE_PI + theTol
                         && theDomain.VMax >= 0.5 * MathUtils::THE_PI - theTol);

  if (aIsFullU && aIsFullV)
  {
    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt(myCenterX + myRadius * aDirX,
                           myCenterY + myRadius * aDirY,
                           myCenterZ + myRadius * aDirZ);
      const double aDistToSurf = aDist - myRadius;

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aU;
      anExt.V              = aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistToSurf * aDistToSurf;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }

    if (theMode != ExtremaPS::SearchMode::Min)
    {
      const gp_Pnt aSurfPt(myCenterX - myRadius * aDirX,
                           myCenterY - myRadius * aDirY,
                           myCenterZ - myRadius * aDirZ);
      const double aDistToSurf = aDist + myRadius;

      double aUOpp = aU + MathUtils::THE_PI;
      if (aUOpp >= MathUtils::THE_2PI)
        aUOpp -= MathUtils::THE_2PI;

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aUOpp;
      anExt.V              = -aV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aDistToSurf * aDistToSurf;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  auto checkUInRange = [&](double aTestU) -> bool {
    return aIsFullU || ExtremaPS::IsInPeriodicRange(aTestU, theDomain.U(), theTol);
  };

  if (theMode != ExtremaPS::SearchMode::Max)
  {
    bool aAddMin = aIsAtPole ? theDomain.V().Contains(aV, theTol)
                             : (checkUInRange(aU) && theDomain.V().Contains(aV, theTol));

    if (aAddMin)
    {
      double aClampedV = theDomain.V().Clamp(aV);
      double aClampedU = aU;
      if (!aIsAtPole && !aIsFullU)
      {
        aClampedU = theDomain.U().Clamp(
          ElCLib::InPeriod(aClampedU, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI));
      }

      const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, mySphere);
      const double aSqDist = theP.SquareDistance(aSurfPt);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aClampedU;
      anExt.V              = aClampedV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aSqDist;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }
  }

  double aUOpp       = aU + MathUtils::THE_PI;
  aUOpp              = ElCLib::InPeriod(aUOpp, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI);
  const double aVOpp = -aV;
  const bool   aIsOppAtPole =
    (std::abs(std::abs(aVOpp) - 0.5 * MathUtils::THE_PI) < Precision::PConfusion());

  if (theMode != ExtremaPS::SearchMode::Min)
  {
    bool aAddMax = aIsOppAtPole ? theDomain.V().Contains(aVOpp, theTol)
                                : (checkUInRange(aUOpp) && theDomain.V().Contains(aVOpp, theTol));

    if (aAddMax)
    {
      double aClampedV = theDomain.V().Clamp(aVOpp);
      double aClampedU = aUOpp;
      if (!aIsOppAtPole && !aIsFullU)
      {
        aClampedU = theDomain.U().Clamp(
          ElCLib::InPeriod(aClampedU, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI));
      }

      const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, mySphere);
      const double aSqDist = theP.SquareDistance(aSurfPt);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aClampedU;
      anExt.V              = aClampedV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aSqDist;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Sphere::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }

  ExtremaPS::AddBoundaryExtrema(myResult, theP, *myDomain, mySphere, theTol, theMode);

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
