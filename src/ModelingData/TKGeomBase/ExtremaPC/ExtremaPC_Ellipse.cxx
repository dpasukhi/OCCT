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

#include <ExtremaPC_Ellipse.hxx>

#include <ElCLib.hxx>
#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_Ellipse.hxx>
#include <gp_Vec.hxx>
#include <MathRoot_Trig.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>
#include <ProjLib.hxx>

#include <cmath>

ExtremaPC_Ellipse::ExtremaPC_Ellipse(const gp_Elips& theEllipse)
    : myEllipse(theEllipse),
      myDomain(std::nullopt)
{
}

//==================================================================================================

ExtremaPC_Ellipse::ExtremaPC_Ellipse(const gp_Elips&            theEllipse,
                                     const ExtremaPC::Domain1D& theDomain)
    : myEllipse(theEllipse),
      myDomain(theDomain)
{
}

//==================================================================================================

gp_Pnt ExtremaPC_Ellipse::EvalD0(const double theU) const
{
  return ElCLib::Value(theU, myEllipse);
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Ellipse::Perform(
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
    myResult.Status = ExtremaPC::Status::NoSolution;
    return myResult;
  }

  const gp_Pln aPlane(gp_Ax3(myEllipse.Position()));
  if (ExtremaPC::PerformPlanar<ExtremaPC2d_Ellipse>(ProjLib::Project(aPlane, myEllipse),
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

  // Use stored domain or full parameter range [0, 2*PI]
  ExtremaPC::Domain1D aDomain = myDomain.value_or(ExtremaPC::Domain1D{0.0, MathUtils::THE_2PI});
  performCore(theP, aDomain, theTol, theMode);
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Ellipse::PerformWithEndpoints(
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

//==================================================================================================

void ExtremaPC_Ellipse::performCore(const gp_Pnt&               theP,
                                    const ExtremaPC::Domain1D&  theDomain,
                                    const double                theTol,
                                    const ExtremaPC::SearchMode theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || !theDomain.IsValid())
  {
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return;
  }

  const double theUMin = theDomain.Min;
  const double theUMax = theDomain.Max;

  // Step 1: Project point P onto the ellipse plane
  const gp_Pnt& aCenter = myEllipse.Location();
  const gp_Dir& aAxis   = myEllipse.Axis().Direction();
  gp_Vec        aToP(aCenter, theP);
  double        aHeight = aToP.Dot(gp_Vec(aAxis));
  gp_Vec        aTrsl   = gp_Vec(aAxis) * (-aHeight);
  gp_Pnt        aPp     = theP.Translated(aTrsl);

  // Step 2: Get ellipse radii and compute local coordinates
  double aA = myEllipse.MajorRadius();
  double aB = myEllipse.MinorRadius();

  gp_Vec aOPp(aCenter, aPp);
  double aOPpMag = aOPp.Magnitude();

  // Check for degenerate case: point at center with circular ellipse
  if (aOPpMag <= gp::Resolution())
  {
    if (aA == aB)
    {
      // Point at center of a circle - infinite solutions
      myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = aA * aA + aHeight * aHeight;
      return;
    }
    // For non-circular ellipse at center, we still get valid extrema at semi-axes
  }

  // Local coordinates
  double aX = aOPp.Dot(gp_Vec(myEllipse.XAxis().Direction()));
  double aY = aOPp.Dot(gp_Vec(myEllipse.YAxis().Direction()));

  // Step 3: Solve trigonometric equation
  // (B^2 - A^2)*cos*sin - B*Y*cos + A*X*sin = 0
  // In MathRoot::Trigonometric form: a*cos^2 + 2*b*cos*sin + c*cos + d*sin + e = 0
  // a = 0, 2*b = (B^2 - A^2), c = -B*Y, d = A*X, e = 0
  double aKo2 = (aB * aB - aA * aA) / 2.0;
  double aKo3 = -aB * aY;
  double aKo4 = aA * aX;

  // MathRoot::Trigonometric handles all special cases including Y ~= 0
  MathRoot::TrigResult aTrigRes =
    MathRoot::Trigonometric(0.0, aKo2, aKo3, aKo4, 0.0, 0.0, MathUtils::THE_2PI);

  if (aTrigRes.InfiniteRoots)
  {
    myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
    const gp_Pnt aPtOnCurve         = ElCLib::Value(0.0, myEllipse);
    myResult.InfiniteSquareDistance = theP.SquareDistance(aPtOnCurve);
    return;
  }
  if (!aTrigRes.IsDone())
  {
    myResult.Status = ExtremaPC::Status::NumericalError;
    return;
  }

  // Step 4: Collect extrema
  auto stationarity = [&](const double theU) {
    const double aCos = std::cos(theU);
    const double aSin = std::sin(theU);
    return (aB * aB - aA * aA) * aCos * aSin - aB * aY * aCos + aA * aX * aSin;
  };

  auto addExtremum = [&](double aU, const bool theIsMin) {
    aU += std::ceil((theUMin - aU - Precision::PConfusion()) / (MathUtils::THE_2PI))
          * (MathUtils::THE_2PI);
    if (std::abs(aU - theUMin) <= Precision::PConfusion())
    {
      aU = theUMin;
    }
    if (aU < theUMin - Precision::PConfusion()
        || aU > theUMax + Precision::PConfusion())
    {
      return;
    }

    gp_Pnt aCurvePt = ElCLib::Value(aU, myEllipse);

    // Check for duplicates using parameter proximity (more robust)
    for (const ExtremaPC::ExtremumResult& anExtremum : myResult.Extrema)
    {
      if (std::abs(anExtremum.Parameter - aU) < Precision::PConfusion())
      {
        return;
      }
    }

    double aSqDist = theP.SquareDistance(aCurvePt);

    // Filter by search mode
    if (theMode == ExtremaPC::SearchMode::Min && !theIsMin)
    {
      return;
    }
    if (theMode == ExtremaPC::SearchMode::Max && theIsMin)
    {
      return;
    }

    ExtremaPC::ExtremumResult anExt;
    anExt.Parameter      = aU;
    anExt.Point          = aCurvePt;
    anExt.SquareDistance = aSqDist;
    anExt.IsMinimum      = theIsMin;
    anExt.IsMaximum      = !theIsMin;

    myResult.Extrema.Append(anExt);
  };

  std::array<double, 4> aCanonicalRoots   = {};
  size_t                aNbCanonicalRoots = 0;
  for (size_t aRootIndex = 0; aRootIndex < aTrigRes.NbRoots; ++aRootIndex)
  {
    double aRoot = std::fmod(aTrigRes.Roots[aRootIndex], MathUtils::THE_2PI);
    if (aRoot < 0.0)
    {
      aRoot += MathUtils::THE_2PI;
    }
    if (aRoot >= MathUtils::THE_2PI - Precision::PConfusion())
    {
      aRoot = 0.0;
    }

    bool isDuplicate = false;
    for (size_t anIndex = 0; anIndex < aNbCanonicalRoots; ++anIndex)
    {
      if (std::abs(aCanonicalRoots[anIndex] - aRoot) < Precision::PConfusion())
      {
        isDuplicate = true;
        break;
      }
    }
    if (!isDuplicate)
    {
      aCanonicalRoots[aNbCanonicalRoots++] = aRoot;
    }
  }
  std::sort(aCanonicalRoots.begin(), aCanonicalRoots.begin() + aNbCanonicalRoots);

  // Classify canonical roots and lift one representative into the requested domain.
  for (size_t i = 0; i < aNbCanonicalRoots; ++i)
  {
    const double aRoot = aCanonicalRoots[i];
    const double aPrev =
      i == 0 ? aCanonicalRoots[aNbCanonicalRoots - 1] - MathUtils::THE_2PI : aCanonicalRoots[i - 1];
    const double aNext =
      i + 1 == aNbCanonicalRoots ? aCanonicalRoots[0] + MathUtils::THE_2PI : aCanonicalRoots[i + 1];
    const double aLeftValue  = stationarity((aPrev + aRoot) * 0.5);
    const double aRightValue = stationarity((aRoot + aNext) * 0.5);
    const bool   isMinimum   = aLeftValue < 0.0 && aRightValue > 0.0;
    const bool   isMaximum   = aLeftValue > 0.0 && aRightValue < 0.0;
    if (isMinimum || isMaximum)
    {
      addExtremum(aRoot, isMinimum);
    }
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution : ExtremaPC::Status::OK;
}
