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

#include <ExtremaPS_Cone.hxx>

#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <MathUtils_Core.hxx>

#include <cmath>

ExtremaPS_Cone::ExtremaPS_Cone(const gp_Cone& theCone)
    : myCone(theCone),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_Cone::ExtremaPS_Cone(const gp_Cone& theCone, const ExtremaPS::Domain2D& theDomain)
    : myCone(theCone),
      myDomain(isNaturalDomain(theDomain) ? std::nullopt
                                          : std::optional<ExtremaPS::Domain2D>(theDomain))
{
  initCache();
}

//==================================================================================================

bool ExtremaPS_Cone::isNaturalDomain(const ExtremaPS::Domain2D& theDomain)
{
  return theDomain.IsUFullPeriod(MathUtils::THE_2PI) && !theDomain.V().IsFinite();
}

//==================================================================================================

void ExtremaPS_Cone::initCache()
{
  const gp_Ax3& aPos = myCone.Position();
  const gp_Pnt& aLoc = aPos.Location();
  myLocX             = aLoc.X();
  myLocY             = aLoc.Y();
  myLocZ             = aLoc.Z();

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

  myRefRadius = myCone.RefRadius();
  mySemiAngle = myCone.SemiAngle();
  myTanAngle  = std::tan(mySemiAngle);
  myCosAngle  = std::cos(mySemiAngle);
  mySinAngle  = std::sin(mySemiAngle);
  myApexV     = std::abs(mySinAngle) > Precision::Angular() ? -myRefRadius / mySinAngle : 0.0;
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Cone::Perform(
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

  const double aZ = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;

  const double aRadX  = aDx - aZ * myAxisX;
  const double aRadY  = aDy - aZ * myAxisY;
  const double aRadZ  = aDz - aZ * myAxisZ;
  const double aRhoSq = aRadX * aRadX + aRadY * aRadY + aRadZ * aRadZ;
  const double aRho   = std::sqrt(aRhoSq);

  if (aRho < theTol)
  {
    if (theMode == ExtremaPS::SearchMode::Max)
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }

    const double aClosestV = aZ * myCosAngle - myRefRadius * mySinAngle;
    if (myDomain.has_value() && !myDomain->V().Contains(aClosestV, theTol))
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }

    const double aRadiusAtV         = myRefRadius + aClosestV * mySinAngle;
    const double aDeltaZ            = aZ - aClosestV * myCosAngle;
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = aRadiusAtV * aRadiusAtV + aDeltaZ * aDeltaZ;
    return myResult;
  }

  const double aInvRho   = 1.0 / aRho;
  const double aRadNormX = aRadX * aInvRho;
  const double aRadNormY = aRadY * aInvRho;
  const double aRadNormZ = aRadZ * aInvRho;

  const double aCosU = aRadNormX * myXDirX + aRadNormY * myXDirY + aRadNormZ * myXDirZ;
  const double aSinU = aRadNormX * myYDirX + aRadNormY * myYDirY + aRadNormZ * myYDirZ;
  double       aU    = std::atan2(aSinU, aCosU);
  if (aU < 0.0)
    aU += MathUtils::THE_2PI;

  if (myDomain.has_value())
  {
    aU = ElCLib::InPeriod(aU, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
  }

  const double aApexZ    = myApexV * myCosAngle; // Axial position of apex
  const double aLocalZ   = aZ - aApexZ;          // Point's axial distance from apex
  const double aGenDist  = aLocalZ * myCosAngle + aRho * mySinAngle;
  double       aClosestV = myApexV + aGenDist;

  double aUOpp = aU + MathUtils::THE_PI;
  if (myDomain.has_value())
  {
    aUOpp = ElCLib::InPeriod(aUOpp, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
  }
  else if (aUOpp >= MathUtils::THE_2PI)
  {
    aUOpp -= MathUtils::THE_2PI;
  }

  const double aGenDistOpp  = aLocalZ * myCosAngle - aRho * mySinAngle;
  double       aClosestVOpp = myApexV + aGenDistOpp;

  const double aR    = myRefRadius + aClosestV * mySinAngle;
  const double aROpp = myRefRadius + aClosestVOpp * mySinAngle;

  if (aR >= 0.0 && aROpp < 0.0)
  {
    const gp_Pnt aPt1   = ElSLib::Value(aU, aClosestV, myCone);
    const gp_Pnt aPt2   = ElSLib::Value(aUOpp, aClosestVOpp, myCone);
    const double aDist1 = theP.SquareDistance(aPt1);
    const double aDist2 = theP.SquareDistance(aPt2);
    if (aDist2 < aDist1)
    {
      std::swap(aU, aUOpp);
      std::swap(aClosestV, aClosestVOpp);
    }
  }
  else if (aR < 0.0 && aROpp >= 0.0)
  {
    const gp_Pnt aPt1   = ElSLib::Value(aU, aClosestV, myCone);
    const gp_Pnt aPt2   = ElSLib::Value(aUOpp, aClosestVOpp, myCone);
    const double aDist1 = theP.SquareDistance(aPt1);
    const double aDist2 = theP.SquareDistance(aPt2);
    if (aDist2 < aDist1)
    {
      std::swap(aU, aUOpp);
      std::swap(aClosestV, aClosestVOpp);
    }
  }
  else if (aR < 0.0 && aROpp < 0.0)
  {
    const gp_Pnt aPt1   = ElSLib::Value(aU, aClosestV, myCone);
    const gp_Pnt aPt2   = ElSLib::Value(aUOpp, aClosestVOpp, myCone);
    const double aDist1 = theP.SquareDistance(aPt1);
    const double aDist2 = theP.SquareDistance(aPt2);
    if (aDist2 < aDist1)
    {
      std::swap(aU, aUOpp);
      std::swap(aClosestV, aClosestVOpp);
    }
  }

  const double aClosestVMax = aClosestVOpp;

  if (!myDomain.has_value())
  {
    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt = ElSLib::Value(aU, aClosestV, myCone);
      const double aSqDist = theP.SquareDistance(aSurfPt);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aU;
      anExt.V              = aClosestV;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aSqDist;
      anExt.IsMinimum      = true;
      myResult.Extrema.Append(anExt);
    }

    if (theMode != ExtremaPS::SearchMode::Min)
    {
      const gp_Pnt aSurfPt = ElSLib::Value(aUOpp, aClosestVMax, myCone);
      const double aSqDist = theP.SquareDistance(aSurfPt);

      ExtremaPS::ExtremumResult anExt;
      anExt.U              = aUOpp;
      anExt.V              = aClosestVMax;
      anExt.Point          = aSurfPt;
      anExt.SquareDistance = aSqDist;
      anExt.IsMinimum      = false;
      myResult.Extrema.Append(anExt);
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const ExtremaPS::Domain2D& theDomain = *myDomain;

  const bool aIsFullU = theDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);

  if (theMode != ExtremaPS::SearchMode::Max)
  {
    if (theDomain.V().Contains(aClosestV, theTol)
        && (aIsFullU || ExtremaPS::IsInPeriodicRange(aU, theDomain.U(), theTol)))
    {
      double aClampedV = theDomain.V().Clamp(aClosestV);
      double aClampedU = aU;
      if (!aIsFullU)
      {
        aClampedU = theDomain.U().Clamp(
          ElCLib::InPeriod(aClampedU, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI));
      }

      const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myCone);
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

  if (theMode != ExtremaPS::SearchMode::Min)
  {
    if (theDomain.V().Contains(aClosestVMax, theTol)
        && (aIsFullU || ExtremaPS::IsInPeriodicRange(aUOpp, theDomain.U(), theTol)))
    {
      double aClampedV = theDomain.V().Clamp(aClosestVMax);
      double aClampedU = aUOpp;
      if (!aIsFullU)
      {
        aClampedU = theDomain.U().Clamp(
          ElCLib::InPeriod(aClampedU, theDomain.UMin, theDomain.UMin + MathUtils::THE_2PI));
      }

      const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myCone);
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

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Cone::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }

  const double aDx       = theP.X() - myLocX;
  const double aDy       = theP.Y() - myLocY;
  const double aDz       = theP.Z() - myLocZ;
  const double aZ        = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;
  const double aRadX     = aDx - aZ * myAxisX;
  const double aRadY     = aDy - aZ * myAxisY;
  const double aRadZ     = aDz - aZ * myAxisZ;
  const double aRadialSq = aRadX * aRadX + aRadY * aRadY + aRadZ * aRadZ;
  if (aRadialSq < theTol * theTol && myDomain->V().IsFinite())
  {
    const double aStationaryV = aZ * myCosAngle - myRefRadius * mySinAngle;
    double       aBoundaryV   = myDomain->V().Clamp(aStationaryV);
    if (theMode == ExtremaPS::SearchMode::Max)
    {
      const double aDeltaMin  = aZ - myDomain->VMin * myCosAngle;
      const double aRadiusMin = myRefRadius + myDomain->VMin * mySinAngle;
      const double aDistMinSq = aRadiusMin * aRadiusMin + aDeltaMin * aDeltaMin;
      const double aDeltaMax  = aZ - myDomain->VMax * myCosAngle;
      const double aRadiusMax = myRefRadius + myDomain->VMax * mySinAngle;
      const double aDistMaxSq = aRadiusMax * aRadiusMax + aDeltaMax * aDeltaMax;
      aBoundaryV              = aDistMinSq > aDistMaxSq ? myDomain->VMin : myDomain->VMax;
    }

    const double aRadiusAtV         = myRefRadius + aBoundaryV * mySinAngle;
    const double aDeltaZ            = aZ - aBoundaryV * myCosAngle;
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = aRadiusAtV * aRadiusAtV + aDeltaZ * aDeltaZ;
    return myResult;
  }

  ExtremaPS::AddBoundaryExtrema(myResult, theP, *myDomain, myCone, theTol, theMode);

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
