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

#include <ExtremaPS_Plane.hxx>

#include <ElSLib.hxx>

ExtremaPS_Plane::ExtremaPS_Plane(const gp_Pln& thePlane)
    : myPlane(thePlane),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_Plane::ExtremaPS_Plane(const gp_Pln& thePlane, const ExtremaPS::Domain2D& theDomain)
    : myPlane(thePlane),
      myDomain(theDomain.IsValid() && !theDomain.U().IsFinite() && !theDomain.V().IsFinite()
                 ? std::nullopt
                 : std::optional<ExtremaPS::Domain2D>(theDomain))
{
  initCache();
}

//==================================================================================================

void ExtremaPS_Plane::initCache()
{
  const gp_Ax3& aPos = myPlane.Position();
  const gp_Pnt& aLoc = aPos.Location();
  myLocX             = aLoc.X();
  myLocY             = aLoc.Y();
  myLocZ             = aLoc.Z();

  const gp_Dir& aNorm = aPos.Direction();
  myNormX             = aNorm.X();
  myNormY             = aNorm.Y();
  myNormZ             = aNorm.Z();

  const gp_Dir& aXDir = aPos.XDirection();
  myXDirX             = aXDir.X();
  myXDirY             = aXDir.Y();
  myXDirZ             = aXDir.Z();

  const gp_Dir& aYDir = aPos.YDirection();
  myYDirX             = aYDir.X();
  myYDirY             = aYDir.Y();
  myYDirZ             = aYDir.Z();
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Plane::Perform(
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

  if (theMode == ExtremaPS::SearchMode::Max)
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
    return myResult;
  }

  const double aDx = theP.X() - myLocX;
  const double aDy = theP.Y() - myLocY;
  const double aDz = theP.Z() - myLocZ;

  const double aSignedDist = aDx * myNormX + aDy * myNormY + aDz * myNormZ;
  const double aU          = aDx * myXDirX + aDy * myXDirY + aDz * myXDirZ;
  const double aV          = aDx * myYDirX + aDy * myYDirY + aDz * myYDirZ;

  if (!myDomain.has_value())
  {
    myResult.Status = ExtremaPS::Status::OK;
    ExtremaPS::ExtremumResult anExt;
    anExt.U              = aU;
    anExt.V              = aV;
    anExt.Point          = gp_Pnt(theP.X() - aSignedDist * myNormX,
                                  theP.Y() - aSignedDist * myNormY,
                                  theP.Z() - aSignedDist * myNormZ);
    anExt.SquareDistance = aSignedDist * aSignedDist;
    anExt.IsMinimum      = true;
    myResult.Extrema.Append(anExt);
    return myResult;
  }

  const double aUMin = myDomain->UMin - theTol;
  const double aUMax = myDomain->UMax + theTol;
  const double aVMin = myDomain->VMin - theTol;
  const double aVMax = myDomain->VMax + theTol;

  if (aU < aUMin || aU > aUMax || aV < aVMin || aV > aVMax)
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
    return myResult;
  }

  double aClampedU = aU;
  double aClampedV = aV;
  myDomain->Clamp(aClampedU, aClampedV);

  myResult.Status = ExtremaPS::Status::OK;
  ExtremaPS::ExtremumResult anExt;
  anExt.U              = aClampedU;
  anExt.V              = aClampedV;
  const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myPlane);
  anExt.Point          = aSurfPt;
  anExt.SquareDistance = theP.SquareDistance(aSurfPt);
  anExt.IsMinimum      = true;
  myResult.Extrema.Append(anExt);
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Plane::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  if (!ExtremaPC::IsFinitePoint(theP) || !ExtremaPC::IsValidTolerance(theTol)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Clear();
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  if (theMode != ExtremaPS::SearchMode::Max)
  {
    const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
    if (!anInteriorResult.IsDone())
    {
      return anInteriorResult;
    }
  }
  else
  {
    myResult.Clear();
    myResult.Status = ExtremaPS::Status::OK;
  }

  if (myDomain.has_value())
  {
    ExtremaPS::SearchMode aBoundaryMode = theMode;
    if (!myResult.Extrema.IsEmpty())
    {
      if (theMode == ExtremaPS::SearchMode::Min)
      {
        return myResult;
      }
      aBoundaryMode = ExtremaPS::SearchMode::Max;
    }
    ExtremaPS::AddBoundaryExtrema(myResult, theP, *myDomain, myPlane, theTol, aBoundaryMode);
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
