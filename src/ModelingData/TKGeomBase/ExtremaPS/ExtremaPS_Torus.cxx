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

#include <ExtremaPS_Torus.hxx>

#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>

#include <cmath>

ExtremaPS_Torus::ExtremaPS_Torus(const gp_Torus& theTorus)
    : myTorus(theTorus),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_Torus::ExtremaPS_Torus(const gp_Torus& theTorus, const ExtremaPS::Domain2D& theDomain)
    : myTorus(theTorus),
      myDomain(isNaturalDomain(theDomain) ? std::nullopt
                                          : std::optional<ExtremaPS::Domain2D>(theDomain))
{
  initCache();
}

//==================================================================================================

bool ExtremaPS_Torus::isNaturalDomain(const ExtremaPS::Domain2D& theDomain)
{
  return theDomain.IsUFullPeriod(MathUtils::THE_2PI) && theDomain.IsVFullPeriod(MathUtils::THE_2PI);
}

//==================================================================================================

void ExtremaPS_Torus::initCache()
{
  const gp_Ax3& aPos    = myTorus.Position();
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

  myMajorRadius = myTorus.MajorRadius();
  myMinorRadius = myTorus.MinorRadius();
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Torus::Perform(
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

  const double aDx = theP.X() - myCenterX;
  const double aDy = theP.Y() - myCenterY;
  const double aDz = theP.Z() - myCenterZ;

  const double aHeight = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;

  const double aRadX         = aDx - aHeight * myAxisX;
  const double aRadY         = aDy - aHeight * myAxisY;
  const double aRadZ         = aDz - aHeight * myAxisZ;
  const double aRadialDistSq = aRadX * aRadX + aRadY * aRadY + aRadZ * aRadZ;

  if (aRadialDistSq < theTol * theTol)
  {
    const double aDistToCircle = std::sqrt(aHeight * aHeight + myMajorRadius * myMajorRadius);
    if (aDistToCircle < theTol)
    {
      myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = myMinorRadius * myMinorRadius;
      return myResult;
    }

    double aVMin = std::atan2(aHeight, -myMajorRadius);
    if (aVMin < 0.0)
    {
      aVMin += MathUtils::THE_2PI;
    }
    double aVMax = aVMin + MathUtils::THE_PI;
    if (aVMax >= MathUtils::THE_2PI)
    {
      aVMax -= MathUtils::THE_2PI;
    }

    const bool hasMinimum =
      !myDomain.has_value() || ExtremaPS::IsInPeriodicRange(aVMin, myDomain->V(), theTol);
    const bool hasMaximum =
      !myDomain.has_value() || ExtremaPS::IsInPeriodicRange(aVMax, myDomain->V(), theTol);
    const bool useMinimum = theMode != ExtremaPS::SearchMode::Max && hasMinimum;
    const bool useMaximum = !useMinimum && theMode != ExtremaPS::SearchMode::Min && hasMaximum;
    if (!useMinimum && !useMaximum)
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }

    const double aDistance =
      useMinimum ? std::abs(aDistToCircle - myMinorRadius) : aDistToCircle + myMinorRadius;
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = aDistance * aDistance;
    return myResult;
  }

  const double aRadialDist = std::sqrt(aRadialDistSq);
  const double aInvRadDist = 1.0 / aRadialDist;

  double aRadNormX = aRadX * aInvRadDist;
  double aRadNormY = aRadY * aInvRadDist;
  double aRadNormZ = aRadZ * aInvRadDist;

  const double aCosU = aRadNormX * myXDirX + aRadNormY * myXDirY + aRadNormZ * myXDirZ;
  const double aSinU = aRadNormX * myYDirX + aRadNormY * myYDirY + aRadNormZ * myYDirZ;
  double       aU    = std::atan2(aSinU, aCosU);
  if (aU < 0.0)
    aU += MathUtils::THE_2PI;

  if (myMajorRadius < myMinorRadius)
  {
    const double aCircleCenterX1 = myCenterX + myMajorRadius * aRadNormX;
    const double aCircleCenterY1 = myCenterY + myMajorRadius * aRadNormY;
    const double aCircleCenterZ1 = myCenterZ + myMajorRadius * aRadNormZ;

    const double aCircleCenterX2 = myCenterX - myMajorRadius * aRadNormX;
    const double aCircleCenterY2 = myCenterY - myMajorRadius * aRadNormY;
    const double aCircleCenterZ2 = myCenterZ - myMajorRadius * aRadNormZ;

    const double aDx1           = theP.X() - aCircleCenterX1;
    const double aDy1           = theP.Y() - aCircleCenterY1;
    const double aDz1           = theP.Z() - aCircleCenterZ1;
    const double aDistSq1       = aDx1 * aDx1 + aDy1 * aDy1 + aDz1 * aDz1;
    const double aMinorRadiusSq = myMinorRadius * myMinorRadius;
    const double aD1            = std::abs(aDistSq1 - aMinorRadiusSq);

    const double aDx2     = theP.X() - aCircleCenterX2;
    const double aDy2     = theP.Y() - aCircleCenterY2;
    const double aDz2     = theP.Z() - aCircleCenterZ2;
    const double aDistSq2 = aDx2 * aDx2 + aDy2 * aDy2 + aDz2 * aDz2;
    const double aD2      = std::abs(aDistSq2 - aMinorRadiusSq);

    if (aD2 < aD1)
    {
      aU += MathUtils::THE_PI;
      if (aU >= MathUtils::THE_2PI)
        aU -= MathUtils::THE_2PI;
      aRadNormX = -aRadNormX;
      aRadNormY = -aRadNormY;
      aRadNormZ = -aRadNormZ;
    }
  }

  if (myDomain.has_value())
  {
    aU = ElCLib::InPeriod(aU, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
  }

  double aUOpp = aU + MathUtils::THE_PI;
  if (myDomain.has_value())
  {
    aUOpp = ElCLib::InPeriod(aUOpp, myDomain->UMin, myDomain->UMin + MathUtils::THE_2PI);
  }
  else if (aUOpp >= MathUtils::THE_2PI)
  {
    aUOpp -= MathUtils::THE_2PI;
  }

  const double aCircleCenterX = myCenterX + myMajorRadius * aRadNormX;
  const double aCircleCenterY = myCenterY + myMajorRadius * aRadNormY;
  const double aCircleCenterZ = myCenterZ + myMajorRadius * aRadNormZ;

  const double aCircleDx = theP.X() - aCircleCenterX;
  const double aCircleDy = theP.Y() - aCircleCenterY;
  const double aCircleDz = theP.Z() - aCircleCenterZ;
  const double aCircleDistSq =
    aCircleDx * aCircleDx + aCircleDy * aCircleDy + aCircleDz * aCircleDz;
  const double aCircleDist = std::sqrt(aCircleDistSq);

  double aV = 0.0;
  double aCircleNormX, aCircleNormY, aCircleNormZ;
  if (aCircleDist < Precision::Confusion())
  {
    aCircleNormX = aRadNormX;
    aCircleNormY = aRadNormY;
    aCircleNormZ = aRadNormZ;
    if (myDomain.has_value())
    {
      aV = (myDomain->VMin + myDomain->VMax) * 0.5;
    }
    else
    {
      aV = 0.0;
    }
  }
  else
  {
    const double aInvCircleDist = 1.0 / aCircleDist;
    aCircleNormX                = aCircleDx * aInvCircleDist;
    aCircleNormY                = aCircleDy * aInvCircleDist;
    aCircleNormZ                = aCircleDz * aInvCircleDist;
    const double aCosV =
      aCircleNormX * aRadNormX + aCircleNormY * aRadNormY + aCircleNormZ * aRadNormZ;
    const double aSinV = aCircleNormX * myAxisX + aCircleNormY * myAxisY + aCircleNormZ * myAxisZ;
    aV                 = std::atan2(aSinV, aCosV);
    if (aV < 0.0)
      aV += MathUtils::THE_2PI;
  }

  if (myDomain.has_value())
  {
    aV = ElCLib::InPeriod(aV, myDomain->VMin, myDomain->VMin + MathUtils::THE_2PI);
  }

  double aVOpp = aV + MathUtils::THE_PI;
  if (myDomain.has_value())
  {
    aVOpp = ElCLib::InPeriod(aVOpp, myDomain->VMin, myDomain->VMin + MathUtils::THE_2PI);
  }
  else if (aVOpp >= MathUtils::THE_2PI)
  {
    aVOpp -= MathUtils::THE_2PI;
  }

  if (!myDomain.has_value())
  {
    if (theMode != ExtremaPS::SearchMode::Max)
    {
      const gp_Pnt aSurfPt(aCircleCenterX + myMinorRadius * aCircleNormX,
                           aCircleCenterY + myMinorRadius * aCircleNormY,
                           aCircleCenterZ + myMinorRadius * aCircleNormZ);
      const double aDistToSurf = aCircleDist - myMinorRadius;

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
      const gp_Pnt aSurfPt1(aCircleCenterX - myMinorRadius * aCircleNormX,
                            aCircleCenterY - myMinorRadius * aCircleNormY,
                            aCircleCenterZ - myMinorRadius * aCircleNormZ);
      const double aDistToSurf1 = aCircleDist + myMinorRadius;

      ExtremaPS::ExtremumResult anExt1;
      anExt1.U              = aU;
      anExt1.V              = aVOpp;
      anExt1.Point          = aSurfPt1;
      anExt1.SquareDistance = aDistToSurf1 * aDistToSurf1;
      anExt1.IsMinimum      = false;
      myResult.Extrema.Append(anExt1);

      const double aOppCircleCenterX = myCenterX - myMajorRadius * aRadNormX;
      const double aOppCircleCenterY = myCenterY - myMajorRadius * aRadNormY;
      const double aOppCircleCenterZ = myCenterZ - myMajorRadius * aRadNormZ;

      const double aOppCircleDx = theP.X() - aOppCircleCenterX;
      const double aOppCircleDy = theP.Y() - aOppCircleCenterY;
      const double aOppCircleDz = theP.Z() - aOppCircleCenterZ;
      const double aOppCircleDistSq =
        aOppCircleDx * aOppCircleDx + aOppCircleDy * aOppCircleDy + aOppCircleDz * aOppCircleDz;
      const double aOppCircleDist = std::sqrt(aOppCircleDistSq);

      if (aOppCircleDist > theTol)
      {
        const double aInvOppDist = 1.0 / aOppCircleDist;
        const double aOppNormX   = aOppCircleDx * aInvOppDist;
        const double aOppNormY   = aOppCircleDy * aInvOppDist;
        const double aOppNormZ   = aOppCircleDz * aInvOppDist;

        const double aOppCosV =
          aOppNormX * (-aRadNormX) + aOppNormY * (-aRadNormY) + aOppNormZ * (-aRadNormZ);
        const double aOppSinV = aOppNormX * myAxisX + aOppNormY * myAxisY + aOppNormZ * myAxisZ;
        double       aVOpp2   = std::atan2(aOppSinV, aOppCosV);
        if (aVOpp2 < 0.0)
          aVOpp2 += MathUtils::THE_2PI;

        const gp_Pnt aSurfPt2(aOppCircleCenterX + myMinorRadius * aOppNormX,
                              aOppCircleCenterY + myMinorRadius * aOppNormY,
                              aOppCircleCenterZ + myMinorRadius * aOppNormZ);
        const double aDistToSurf2 = aOppCircleDist - myMinorRadius;

        ExtremaPS::ExtremumResult anExt2;
        anExt2.U              = aUOpp;
        anExt2.V              = aVOpp2;
        anExt2.Point          = aSurfPt2;
        anExt2.SquareDistance = aDistToSurf2 * aDistToSurf2;
        anExt2.IsMinimum      = false;
        myResult.Extrema.Append(anExt2);

        double aVOpp3 = aVOpp2 + MathUtils::THE_PI;
        if (aVOpp3 >= MathUtils::THE_2PI)
          aVOpp3 -= MathUtils::THE_2PI;

        const gp_Pnt aSurfPt3(aOppCircleCenterX - myMinorRadius * aOppNormX,
                              aOppCircleCenterY - myMinorRadius * aOppNormY,
                              aOppCircleCenterZ - myMinorRadius * aOppNormZ);
        const double aDistToSurf3 = aOppCircleDist + myMinorRadius;

        ExtremaPS::ExtremumResult anExt3;
        anExt3.U              = aUOpp;
        anExt3.V              = aVOpp3;
        anExt3.Point          = aSurfPt3;
        anExt3.SquareDistance = aDistToSurf3 * aDistToSurf3;
        anExt3.IsMinimum      = false;
        myResult.Extrema.Append(anExt3);
      }
    }

    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const ExtremaPS::Domain2D& aDomain  = *myDomain;
  const bool                 aIsFullU = aDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);
  const bool                 aIsFullV = aDomain.IsVFullPeriod(MathUtils::THE_2PI, theTol);

  double aVOpp2 = 0.0;
  double aVOpp3 = MathUtils::THE_PI;
  {
    const double aOppCircleCenterX = myCenterX - myMajorRadius * aRadNormX;
    const double aOppCircleCenterY = myCenterY - myMajorRadius * aRadNormY;
    const double aOppCircleCenterZ = myCenterZ - myMajorRadius * aRadNormZ;

    const double aOppCircleDx = theP.X() - aOppCircleCenterX;
    const double aOppCircleDy = theP.Y() - aOppCircleCenterY;
    const double aOppCircleDz = theP.Z() - aOppCircleCenterZ;
    const double aOppCircleDistSq =
      aOppCircleDx * aOppCircleDx + aOppCircleDy * aOppCircleDy + aOppCircleDz * aOppCircleDz;
    const double aOppCircleDist = std::sqrt(aOppCircleDistSq);

    if (aOppCircleDist > theTol)
    {
      const double aInvOppDist = 1.0 / aOppCircleDist;
      const double aOppNormX   = aOppCircleDx * aInvOppDist;
      const double aOppNormY   = aOppCircleDy * aInvOppDist;
      const double aOppNormZ   = aOppCircleDz * aInvOppDist;

      const double aOppCosV =
        aOppNormX * (-aRadNormX) + aOppNormY * (-aRadNormY) + aOppNormZ * (-aRadNormZ);
      const double aOppSinV = aOppNormX * myAxisX + aOppNormY * myAxisY + aOppNormZ * myAxisZ;
      aVOpp2                = std::atan2(aOppSinV, aOppCosV);
      if (aVOpp2 < 0.0)
        aVOpp2 += MathUtils::THE_2PI;

      aVOpp3 = aVOpp2 + MathUtils::THE_PI;
      if (aVOpp3 >= MathUtils::THE_2PI)
        aVOpp3 -= MathUtils::THE_2PI;
    }

    aVOpp2 = ElCLib::InPeriod(aVOpp2, aDomain.VMin, aDomain.VMin + MathUtils::THE_2PI);
    aVOpp3 = ElCLib::InPeriod(aVOpp3, aDomain.VMin, aDomain.VMin + MathUtils::THE_2PI);
  }

  auto addExtremum = [&](double aExtU, double aExtV, bool aIsMin) {
    const bool aUInRange = aIsFullU || ExtremaPS::IsInPeriodicRange(aExtU, aDomain.U(), theTol);
    const bool aVInRange = aIsFullV || ExtremaPS::IsInPeriodicRange(aExtV, aDomain.V(), theTol);
    if (!aUInRange || !aVInRange)
      return;

    double aClampedU = aExtU;
    double aClampedV = aExtV;
    if (!aIsFullU)
    {
      aClampedU = aDomain.U().Clamp(
        ElCLib::InPeriod(aClampedU, aDomain.UMin, aDomain.UMin + MathUtils::THE_2PI));
    }
    if (!aIsFullV)
    {
      aClampedV = aDomain.V().Clamp(
        ElCLib::InPeriod(aClampedV, aDomain.VMin, aDomain.VMin + MathUtils::THE_2PI));
    }

    const gp_Pnt aSurfPt = ElSLib::Value(aClampedU, aClampedV, myTorus);
    const double aSqDist = theP.SquareDistance(aSurfPt);

    for (size_t i = 0; i < myResult.Extrema.Size(); ++i)
    {
      const ExtremaPS::ExtremumResult& anExisting = myResult.Extrema.Value(i);
      if (std::abs(anExisting.U - aClampedU) < Precision::PConfusion()
          && std::abs(anExisting.V - aClampedV) < Precision::PConfusion())
        return;
      if (anExisting.Point.SquareDistance(aSurfPt) < theTol * theTol)
        return;
    }

    ExtremaPS::ExtremumResult anExt;
    anExt.U              = aClampedU;
    anExt.V              = aClampedV;
    anExt.Point          = aSurfPt;
    anExt.SquareDistance = aSqDist;
    anExt.IsMinimum      = aIsMin;
    myResult.Extrema.Append(anExt);
  };

  if (theMode != ExtremaPS::SearchMode::Max)
  {
    addExtremum(aU, aV, true);
  }

  if (theMode != ExtremaPS::SearchMode::Min)
  {
    addExtremum(aU, aVOpp, false);
    addExtremum(aUOpp, aVOpp2, false);
    addExtremum(aUOpp, aVOpp3, false);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_Torus::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }

  const ExtremaPS::Domain2D& aDomain  = *myDomain;
  const bool                 aIsFullU = aDomain.IsUFullPeriod(MathUtils::THE_2PI, theTol);
  const bool                 aIsFullV = aDomain.IsVFullPeriod(MathUtils::THE_2PI, theTol);

  if (!aIsFullU || !aIsFullV)
  {
    ExtremaPS::AddBoundaryExtrema(myResult, theP, aDomain, myTorus, theTol, theMode);
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
