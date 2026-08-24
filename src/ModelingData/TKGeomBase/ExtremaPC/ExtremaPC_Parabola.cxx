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

#include <ExtremaPC_Parabola.hxx>

#include <ElCLib.hxx>
#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_Parabola.hxx>
#include <gp_Vec.hxx>
#include <MathPoly_Cubic.hxx>
#include <Precision.hxx>
#include <ProjLib.hxx>

#include <cmath>

ExtremaPC_Parabola::ExtremaPC_Parabola(const gp_Parab& theParabola)
    : myParabola(theParabola),
      myDomain(std::nullopt)
{
  cacheGeometry();
}

//==================================================================================================

ExtremaPC_Parabola::ExtremaPC_Parabola(const gp_Parab&            theParabola,
                                       const ExtremaPC::Domain1D& theDomain)
    : myParabola(theParabola),
      myDomain(theDomain)
{
  cacheGeometry();
}

//==================================================================================================

gp_Pnt ExtremaPC_Parabola::EvalD0(const double theU) const
{
  if (myFocal <= gp::Resolution())
  {
    return myParabola.Location().Translated(theU * gp_Vec(myParabola.XAxis().Direction()));
  }

  const double aX = theU * theU * my1Over4F;
  return gp_Pnt(myVertexX + aX * myXDirX + theU * myYDirX,
                myVertexY + aX * myXDirY + theU * myYDirY,
                myVertexZ + aX * myXDirZ + theU * myYDirZ);
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Parabola::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  const gp_Pln aPlane(gp_Ax3(myParabola.Position()));
  if (ExtremaPC::IsValidTolerance(theTol) && ExtremaPC::IsFinitePoint(theP)
      && (!myDomain.has_value() || myDomain->IsValid())
      && ExtremaPC::PerformPlanar<ExtremaPC2d_Parabola>(ProjLib::Project(aPlane, myParabola),
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

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Parabola::PerformWithEndpoints(
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

void ExtremaPC_Parabola::cacheGeometry()
{
  const gp_Pnt& aVertex = myParabola.Location();
  myVertexX             = aVertex.X();
  myVertexY             = aVertex.Y();
  myVertexZ             = aVertex.Z();

  const gp_Dir aXDir = myParabola.XAxis().Direction();
  myXDirX            = aXDir.X();
  myXDirY            = aXDir.Y();
  myXDirZ            = aXDir.Z();

  const gp_Dir aYDir = myParabola.YAxis().Direction();
  myYDirX            = aYDir.X();
  myYDirY            = aYDir.Y();
  myYDirZ            = aYDir.Z();

  const gp_Dir& aAxis = myParabola.Axis().Direction();
  myAxisX             = aAxis.X();
  myAxisY             = aAxis.Y();
  myAxisZ             = aAxis.Z();

  myFocal   = myParabola.Focal();
  my1Over4F = myFocal > gp::Resolution() ? 1.0 / (4.0 * myFocal) : 0.0;
  my2F      = 2.0 * myFocal;
}

//==================================================================================================

MathPoly::PolyResult ExtremaPC_Parabola::solveCubic(const gp_Pnt& theP) const
{
  // Vector from vertex to point
  const double aDx = theP.X() - myVertexX;
  const double aDy = theP.Y() - myVertexY;
  const double aDz = theP.Z() - myVertexZ;

  // Project point P onto the parabola plane
  // Height = (P - Vertex) . Axis
  const double aHeight = aDx * myAxisX + aDy * myAxisY + aDz * myAxisZ;

  // Projected point Pp = P - Height * Axis
  const double aPpX = theP.X() - aHeight * myAxisX;
  const double aPpY = theP.Y() - aHeight * myAxisY;
  const double aPpZ = theP.Z() - aHeight * myAxisZ;

  // Vector from vertex to projected point
  const double aOPpX = aPpX - myVertexX;
  const double aOPpY = aPpY - myVertexY;
  const double aOPpZ = aPpZ - myVertexZ;

  // Local coordinates in parabola frame
  const double aX = aOPpX * myXDirX + aOPpY * myXDirY + aOPpZ * myXDirZ;
  const double aY = aOPpX * myYDirX + aOPpY * myYDirY + aOPpZ * myYDirZ;

  // Solve cubic equation: (1/(4F)) * u^3 + (2F - X) * u - 2F*Y = 0
  return MathPoly::Cubic(my1Over4F, 0.0, my2F - aX, -my2F * aY);
}

//==================================================================================================

void ExtremaPC_Parabola::performCore(const gp_Pnt&                             theP,
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

  if (myFocal <= gp::Resolution())
  {
    if (theMode == ExtremaPC::SearchMode::Max)
    {
      myResult.Status = ExtremaPC::Status::NoSolution;
      return;
    }

    const gp_Pnt& aVertex = myParabola.Location();
    const gp_Vec  aDirection(myParabola.XAxis().Direction());
    double        aU = gp_Vec(aVertex, theP).Dot(aDirection);
    if (theDomain.has_value() && !theDomain->Contains(aU, theTol))
    {
      myResult.Status = ExtremaPC::Status::NoSolution;
      return;
    }
    if (theDomain.has_value())
    {
      aU = theDomain->Clamp(aU);
    }

    const gp_Pnt              aCurvePt = EvalD0(aU);
    ExtremaPC::ExtremumResult anExt;
    anExt.Parameter      = aU;
    anExt.Point          = aCurvePt;
    anExt.SquareDistance = theP.SquareDistance(aCurvePt);
    anExt.IsMinimum      = true;
    anExt.IsMaximum      = false;
    myResult.Extrema.Append(anExt);
    myResult.Status = ExtremaPC::Status::OK;
    return;
  }

  MathPoly::PolyResult aPolyRes = solveCubic(theP);

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

  // Process all roots
  for (size_t i = 0; i < aPolyRes.NbRoots; ++i)
  {
    double aU = aPolyRes.Roots[i];

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

    // The stationary polynomial has a positive leading coefficient. Its sign to the right
    // of this root is determined by the odd-multiplicity roots that follow it.
    double aSqDist          = theP.SquareDistance(aCurvePt);
    size_t aNbOddRootsAfter = 0;
    for (size_t aNext = i + 1; aNext < aPolyRes.NbRoots; ++aNext)
    {
      aNbOddRootsAfter += aPolyRes.Multiplicities[aNext] % 2;
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
