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

#include <ExtremaPC_Hyperbola.hxx>

#include <ElCLib.hxx>
#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_Hyperbola.hxx>
#include <gp_Vec.hxx>
#include <MathPoly_Quartic.hxx>
#include <Precision.hxx>
#include <ProjLib.hxx>

#include <cmath>

ExtremaPC_Hyperbola::ExtremaPC_Hyperbola(const gp_Hypr& theHyperbola)
    : myHyperbola(theHyperbola),
      myDomain(std::nullopt)
{
  cacheGeometry();
}

//==================================================================================================

ExtremaPC_Hyperbola::ExtremaPC_Hyperbola(const gp_Hypr&             theHyperbola,
                                         const ExtremaPC::Domain1D& theDomain)
    : myHyperbola(theHyperbola),
      myDomain(theDomain)
{
  cacheGeometry();
}

//==================================================================================================

gp_Pnt ExtremaPC_Hyperbola::EvalD0(const double theU) const
{
  const double aCosh  = std::cosh(theU);
  const double aSinh  = std::sinh(theU);
  const double aRCosh = myMajorR * aCosh;
  const double arSinh = myMinorR * aSinh;
  return gp_Pnt(myCenterX + aRCosh * myXDirX + arSinh * myYDirX,
                myCenterY + aRCosh * myXDirY + arSinh * myYDirY,
                myCenterZ + aRCosh * myXDirZ + arSinh * myYDirZ);
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Hyperbola::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  const gp_Pln aPlane(gp_Ax3(myHyperbola.Position()));
  if (ExtremaPC::IsValidTolerance(theTol) && ExtremaPC::IsFinitePoint(theP)
      && (!myDomain.has_value() || myDomain->IsValid())
      && ExtremaPC::PerformPlanar<ExtremaPC2d_Hyperbola>(ProjLib::Project(aPlane, myHyperbola),
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
  performCore(theP, myDomain, theTol, theMode);
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Hyperbola::PerformWithEndpoints(
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
  ExtremaPC::AddEndpointExtrema(myResult, theP, *myDomain, *this, theMode, false);
  if (!myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPC::Status::OK;
  }

  return myResult;
}

//==================================================================================================

void ExtremaPC_Hyperbola::cacheGeometry()
{
  const gp_Pnt& aCenter = myHyperbola.Location();
  myCenterX             = aCenter.X();
  myCenterY             = aCenter.Y();
  myCenterZ             = aCenter.Z();

  const gp_Dir aXDir = myHyperbola.XAxis().Direction();
  myXDirX            = aXDir.X();
  myXDirY            = aXDir.Y();
  myXDirZ            = aXDir.Z();

  const gp_Dir aYDir = myHyperbola.YAxis().Direction();
  myYDirX            = aYDir.X();
  myYDirY            = aYDir.Y();
  myYDirZ            = aYDir.Z();

  const gp_Dir& aAxis = myHyperbola.Axis().Direction();
  myAxisX             = aAxis.X();
  myAxisY             = aAxis.Y();
  myAxisZ             = aAxis.Z();

  myMajorR        = myHyperbola.MajorRadius();
  myMinorR        = myHyperbola.MinorRadius();
  myR2PlusR2Over4 = (myMajorR * myMajorR + myMinorR * myMinorR) / 4.0;
}

//==================================================================================================

MathPoly::PolyResult ExtremaPC_Hyperbola::solveQuartic(const gp_Pnt& theP) const
{
  // Vector from center to point
  const double aDx = theP.X() - myCenterX;
  const double aDy = theP.Y() - myCenterY;
  const double aDz = theP.Z() - myCenterZ;

  // Project point P onto the hyperbola plane
  // Height = (P - Center) . Axis
  const double aHeight = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;

  // Projected point Pp = P - Height * Axis
  const double aPpX = theP.X() - aHeight * myAxisX;
  const double aPpY = theP.Y() - aHeight * myAxisY;
  const double aPpZ = theP.Z() - aHeight * myAxisZ;

  // Vector from center to projected point
  const double aOPpX = aPpX - myCenterX;
  const double aOPpY = aPpY - myCenterY;
  const double aOPpZ = aPpZ - myCenterZ;

  // Local coordinates in hyperbola frame
  const double aX = aOPpX * myXDirX + aOPpY * myXDirY + aOPpZ * myXDirZ;
  const double aY = aOPpX * myYDirX + aOPpY * myYDirY + aOPpZ * myYDirZ;

  // Solve quartic equation in v = e^u
  const double aC1 = myR2PlusR2Over4;
  const double aC2 = -(aX * myMajorR + aY * myMinorR) / 2.0;
  const double aC3 = 0.0;
  const double aC4 = (aX * myMajorR - aY * myMinorR) / 2.0;
  const double aC5 = -myR2PlusR2Over4;

  return MathPoly::Quartic(aC1, aC2, aC3, aC4, aC5);
}

//==================================================================================================

void ExtremaPC_Hyperbola::performCore(const gp_Pnt&                             theP,
                                      const std::optional<ExtremaPC::Domain1D>& theDomain,
                                      const double                              theTol,
                                      const ExtremaPC::SearchMode               theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || (theDomain.has_value() && !theDomain->IsValid()))
  {
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return;
  }

  MathPoly::PolyResult aPolyRes = solveQuartic(theP);

  if (!aPolyRes.IsDone())
  {
    if (aPolyRes.Status == MathUtils::Status::InfiniteSolutions)
    {
      myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
      gp_Pnt aPtOnCurve               = EvalD0(0.0);
      myResult.InfiniteSquareDistance = theP.SquareDistance(aPtOnCurve);
    }
    else
    {
      myResult.Status = ExtremaPC::Status::NumericalError;
    }
    return;
  }

  // Process all positive roots (v > 0 required for u = ln(v))
  for (size_t i = 0; i < aPolyRes.NbRoots; ++i)
  {
    double aV = aPolyRes.Roots[i];
    if (aV <= 0.0)
      continue;

    double aU = std::log(aV);

    // Check bounds if domain is specified
    if (theDomain.has_value())
    {
      if (!theDomain->Contains(aU, Precision::PConfusion()))
      {
        continue;
      }
      aU = theDomain->Clamp(aU);
    }

    gp_Pnt aCurvePt = EvalD0(aU);

    // Check for duplicates
    bool aDuplicate = false;
    for (const ExtremaPC::ExtremumResult& anExtremum : myResult.Extrema)
    {
      if (std::abs(aU - anExtremum.Parameter) < Precision::PConfusion())
      {
        aDuplicate = true;
        break;
      }
    }
    if (aDuplicate)
      continue;

    const size_t aMultiplicity = aPolyRes.Multiplicities[i];
    if (aMultiplicity % 2 == 0)
    {
      continue;
    }

    // The quartic is v^2 times the stationary function and has a positive leading
    // coefficient. Since v = exp(u) is increasing, odd roots that follow determine whether
    // the stationary function crosses from negative to positive at this root.
    double aSqDist          = theP.SquareDistance(aCurvePt);
    size_t aNbOddRootsAfter = 0;
    for (size_t aNext = i + 1; aNext < aPolyRes.NbRoots; ++aNext)
    {
      if (aPolyRes.Roots[aNext] > 0.0)
      {
        aNbOddRootsAfter += aPolyRes.Multiplicities[aNext] % 2;
      }
    }
    const bool aIsMin = aNbOddRootsAfter % 2 == 0;

    // Filter by search mode
    if (theMode == ExtremaPC::SearchMode::Min && !aIsMin)
      continue;
    if (theMode == ExtremaPC::SearchMode::Max && aIsMin)
      continue;

    ExtremaPC::ExtremumResult anExt;
    anExt.Parameter      = aU;
    anExt.Point          = aCurvePt;
    anExt.SquareDistance = aSqDist;
    anExt.IsMinimum      = aIsMin;
    anExt.IsMaximum      = !aIsMin;

    myResult.Extrema.Append(anExt);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution : ExtremaPC::Status::OK;
}
