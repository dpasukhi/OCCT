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

#include <gtest/gtest.h>

#include <ExtremaPS.hxx>
#include <ExtremaPS_BSplineSurface.hxx>
#include <ExtremaPS_Surface.hxx>

#include <Geom_BSplineSurface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <NCollection_LinearVector.hxx>

#include <cmath>
#include <vector>

namespace
{
const double THE_TOLERANCE = 1.0e-6;

//! Create a flat BSpline surface (plane-like)
occ::handle<Geom_BSplineSurface> MakeFlatBSpline()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 4, 1, 4);
  for (int i = 1; i <= 4; ++i)
  {
    for (int j = 1; j <= 4; ++j)
    {
      double aX = (i - 1) * 10.0 / 3.0;
      double aY = (j - 1) * 10.0 / 3.0;
      aPoles.SetValue(i, j, gp_Pnt(aX, aY, 0.0));
    }
  }

  NCollection_Array1<double> aUKnots(1, 2), aVKnots(1, 2);
  NCollection_Array1<int>    aUMults(1, 2), aVMults(1, 2);
  aUKnots(1) = 0.0;
  aUKnots(2) = 1.0;
  aVKnots(1) = 0.0;
  aVKnots(2) = 1.0;
  aUMults(1) = 4;
  aUMults(2) = 4;
  aVMults(1) = 4;
  aVMults(2) = 4;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 3, 3);
}

//! Create a curved BSpline surface (dome-like)
//! For 5 poles with degree 2: sum of multiplicities = 5 + 2 + 1 = 8
occ::handle<Geom_BSplineSurface> MakeDomeBSpline()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 5, 1, 5);
  for (int i = 1; i <= 5; ++i)
  {
    for (int j = 1; j <= 5; ++j)
    {
      double aX = (i - 1) * 2.5;
      double aY = (j - 1) * 2.5;
      double aR = std::sqrt((aX - 5.0) * (aX - 5.0) + (aY - 5.0) * (aY - 5.0));
      double aZ = std::max(0.0, 5.0 - aR * 0.5);
      aPoles.SetValue(i, j, gp_Pnt(aX, aY, aZ));
    }
  }

  NCollection_Array1<double> aUKnots(1, 3), aVKnots(1, 3);
  NCollection_Array1<int>    aUMults(1, 3), aVMults(1, 3);
  aUKnots(1) = 0.0;
  aUKnots(2) = 0.5;
  aUKnots(3) = 1.0;
  aVKnots(1) = 0.0;
  aVKnots(2) = 0.5;
  aVKnots(3) = 1.0;
  // For 5 poles, deg 2: need 5+2+1 = 8 total multiplicity
  aUMults(1) = 3; // degree + 1 at start
  aUMults(2) = 2; // 8 - 3 - 3 = 2
  aUMults(3) = 3; // degree + 1 at end
  aVMults(1) = 3;
  aVMults(2) = 2;
  aVMults(3) = 3;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 2, 2);
}

//! Create a wavy BSpline surface
//! For 7 poles with degree 3: sum of multiplicities = 7 + 3 + 1 = 11
occ::handle<Geom_BSplineSurface> MakeWavyBSpline()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 7, 1, 7);
  for (int i = 1; i <= 7; ++i)
  {
    for (int j = 1; j <= 7; ++j)
    {
      double aX = (i - 1) * 10.0 / 6.0;
      double aY = (j - 1) * 10.0 / 6.0;
      double aZ = std::sin(aX * 0.6) * std::cos(aY * 0.6) * 2.0;
      aPoles.SetValue(i, j, gp_Pnt(aX, aY, aZ));
    }
  }

  NCollection_Array1<double> aUKnots(1, 4), aVKnots(1, 4);
  NCollection_Array1<int>    aUMults(1, 4), aVMults(1, 4);
  aUKnots(1) = 0.0;
  aUKnots(2) = 0.33;
  aUKnots(3) = 0.67;
  aUKnots(4) = 1.0;
  aVKnots(1) = 0.0;
  aVKnots(2) = 0.33;
  aVKnots(3) = 0.67;
  aVKnots(4) = 1.0;
  // For 7 poles, deg 3: need 7+3+1 = 11 total multiplicity
  aUMults(1) = 4; // degree + 1 at start
  aUMults(2) = 2; // 11 - 4 - 4 - 1 = 2
  aUMults(3) = 1;
  aUMults(4) = 4; // degree + 1 at end
  aVMults(1) = 4;
  aVMults(2) = 2;
  aVMults(3) = 1;
  aVMults(4) = 4;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 3, 3);
}

//! Create a saddle-like BSpline surface
//! For 5 poles with degree 2: sum of multiplicities = 5 + 2 + 1 = 8
occ::handle<Geom_BSplineSurface> MakeSaddleBSpline()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 5, 1, 5);
  for (int i = 1; i <= 5; ++i)
  {
    for (int j = 1; j <= 5; ++j)
    {
      double aX = (i - 1) * 2.5;
      double aY = (j - 1) * 2.5;
      double aZ = ((aX - 5.0) * (aX - 5.0) - (aY - 5.0) * (aY - 5.0)) * 0.1;
      aPoles.SetValue(i, j, gp_Pnt(aX, aY, aZ));
    }
  }

  NCollection_Array1<double> aUKnots(1, 3), aVKnots(1, 3);
  NCollection_Array1<int>    aUMults(1, 3), aVMults(1, 3);
  aUKnots(1) = 0.0;
  aUKnots(2) = 0.5;
  aUKnots(3) = 1.0;
  aVKnots(1) = 0.0;
  aVKnots(2) = 0.5;
  aVKnots(3) = 1.0;
  // For 5 poles, deg 2: need 5+2+1 = 8 total multiplicity
  aUMults(1) = 3; // degree + 1 at start
  aUMults(2) = 2; // 8 - 3 - 3 = 2
  aUMults(3) = 3; // degree + 1 at end
  aVMults(1) = 3;
  aVMults(2) = 2;
  aVMults(3) = 3;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 2, 2);
}

//! Create a high-degree BSpline surface
occ::handle<Geom_BSplineSurface> MakeHighDegreeBSpline()
{
  NCollection_Array2<gp_Pnt> aPoles(1, 6, 1, 6);
  for (int i = 1; i <= 6; ++i)
  {
    for (int j = 1; j <= 6; ++j)
    {
      double aX = (i - 1) * 2.0;
      double aY = (j - 1) * 2.0;
      double aZ = std::sin(aX * 0.5) * std::sin(aY * 0.5) * 3.0;
      aPoles.SetValue(i, j, gp_Pnt(aX, aY, aZ));
    }
  }

  NCollection_Array1<double> aUKnots(1, 2), aVKnots(1, 2);
  NCollection_Array1<int>    aUMults(1, 2), aVMults(1, 2);
  aUKnots(1) = 0.0;
  aUKnots(2) = 1.0;
  aVKnots(1) = 0.0;
  aVKnots(2) = 1.0;
  aUMults(1) = 6;
  aUMults(2) = 6;
  aVMults(1) = 6;
  aVMults(2) = 6;

  return new Geom_BSplineSurface(aPoles, aUKnots, aVKnots, aUMults, aVMults, 5, 5);
}

occ::handle<Geom_BSplineSurface> MakeOscillatoryBSpline(const bool theIsRational)
{
  constexpr int    aUDegree = 9;
  constexpr int    aVDegree = 2;
  constexpr double aUNumerators[] =
    {-1.0, 17.0, -85.0, 221.0, -2431.0 / 7.0, 2431.0 / 7.0, -221.0, 85.0, -17.0, 1.0};
  constexpr double           aVPoles[] = {1.0, -3.0, 1.0};
  NCollection_Array2<gp_Pnt> aPoles(1, aUDegree + 1, 1, aVDegree + 1);
  NCollection_Array2<double> aWeights(1, aUDegree + 1, 1, aVDegree + 1);
  for (size_t anIndexU = 0; anIndexU < aPoles.RowSize(); ++anIndexU)
  {
    const double aWeight = theIsRational ? (anIndexU % 2 == 0 ? 0.8 : 1.2) : 1.0;
    for (size_t anIndexV = 0; anIndexV < aPoles.ColSize(); ++anIndexV)
    {
      aPoles.ChangeAt(anIndexU, anIndexV) =
        gp_Pnt(0.0, aUNumerators[anIndexU] / aWeight, aVPoles[anIndexV]);
      aWeights.ChangeAt(anIndexU, anIndexV) = aWeight;
    }
  }

  NCollection_Array1<double> aUKnots(1, 2), aVKnots(1, 2);
  NCollection_Array1<int>    aUMultiplicities(1, 2), aVMultiplicities(1, 2);
  aUKnots(1)          = 0.0;
  aUKnots(2)          = 1.0;
  aVKnots(1)          = 0.0;
  aVKnots(2)          = 1.0;
  aUMultiplicities(1) = aUDegree + 1;
  aUMultiplicities(2) = aUDegree + 1;
  aVMultiplicities(1) = aVDegree + 1;
  aVMultiplicities(2) = aVDegree + 1;
  return new Geom_BSplineSurface(aPoles,
                                 aWeights,
                                 aUKnots,
                                 aVKnots,
                                 aUMultiplicities,
                                 aVMultiplicities,
                                 aUDegree,
                                 aVDegree);
}
} // namespace

//==================================================================================================
// Test fixture for ExtremaPS_BSplineSurface tests
//==================================================================================================
class ExtremaPS_BSplineSurfaceTest : public testing::Test
{
protected:
  void SetUp() override { myFlatSurface = MakeFlatBSpline(); }

  occ::handle<Geom_BSplineSurface> myFlatSurface;
};

//==================================================================================================
// Basic Projection Tests on Flat Surface
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, FlatSurface_PointAbove)
{
  gp_Pnt                   aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 10.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, FlatSurface_PointOnSurface)
{
  gp_Pnt                   aP(5.0, 5.0, 0.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(aMinDist, 0.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, FlatSurface_PointBelow)
{
  gp_Pnt                   aP(5.0, 5.0, -5.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 5.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, FlatSurface_PointAtCorner)
{
  gp_Pnt                   aP(0.0, 0.0, 3.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 3.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, FlatSurface_PointOutsideDomain)
{
  gp_Pnt                   aP(-5.0, 5.0, 0.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 5.0, THE_TOLERANCE);
}

//==================================================================================================
// Dome Surface Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, DomeSurface_PointAbovePeak)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  gp_Pnt                           aP(5.0, 5.0, 15.0);
  ExtremaPS_BSplineSurface         anEval(aDome, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  // Minimum distance to dome from point 15 units above center should be reasonable
  // The dome peak is approximately at Z=5, so minimum distance should be around 10
  double aMinDist = std::sqrt(aResult.MinSquareDistance());
  EXPECT_GT(aMinDist, 5.0);  // At least 5 units away
  EXPECT_LT(aMinDist, 15.0); // At most 15 units away (from corners)
}

TEST_F(ExtremaPS_BSplineSurfaceTest, DomeSurface_PointAtEdge)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  gp_Pnt                           aP(0.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface         anEval(aDome, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, DomeSurface_PointUnderDome)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  gp_Pnt                           aP(5.0, 5.0, -5.0);
  ExtremaPS_BSplineSurface         anEval(aDome, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

//==================================================================================================
// Wavy Surface Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, WavySurface_PointAbovePeak)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  gp_Pnt                           aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface         anEval(aWavy, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, WavySurface_PointInValley)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  gp_Pnt                           aP(5.0, 0.0, -5.0);
  ExtremaPS_BSplineSurface         anEval(aWavy, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, WavySurface_MultipleExtrema)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  gp_Pnt                           aP(5.0, 5.0, 0.0);
  ExtremaPS_BSplineSurface         anEval(aWavy, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult =
    anEval.PerformWithBoundary(aP, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  EXPECT_GE(aResult.Extrema.Size(), 1);
}

//==================================================================================================
// Saddle Surface Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, SaddleSurface_PointAboveCenter)
{
  occ::handle<Geom_BSplineSurface> aSaddle = MakeSaddleBSpline();
  gp_Pnt                           aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface         anEval(aSaddle, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, SaddleSurface_PointAtSaddlePoint)
{
  occ::handle<Geom_BSplineSurface> aSaddle = MakeSaddleBSpline();
  gp_Pnt                           aP(5.0, 5.0, 5.0);
  ExtremaPS_BSplineSurface         anEval(aSaddle, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

//==================================================================================================
// High-Degree Surface Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, HighDegree_BasicProjection)
{
  occ::handle<Geom_BSplineSurface> aHighDeg = MakeHighDegreeBSpline();
  gp_Pnt                           aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface         anEval(aHighDeg, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, HighDegree_MultiplePoints)
{
  occ::handle<Geom_BSplineSurface> aHighDeg = MakeHighDegreeBSpline();
  ExtremaPS_BSplineSurface         anEval(aHighDeg, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));

  std::vector<gp_Pnt> aPoints = {gp_Pnt(2.0, 2.0, 5.0),
                                 gp_Pnt(7.0, 3.0, 3.0),
                                 gp_Pnt(5.0, 8.0, -2.0)};

  for (const auto& aP : aPoints)
  {
    const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);
    EXPECT_EQ(aResult.Status, ExtremaPS::Status::OK);
    EXPECT_GE(aResult.Extrema.Size(), 1);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, RationalOscillatorySurfaceFindsEveryInteriorMinimum)
{
  occ::handle<Geom_BSplineSurface> aSurface = MakeOscillatoryBSpline(true);
  ExtremaPS_BSplineSurface         anEvaluator(aSurface);
  const ExtremaPS::Result&         aResult =
    anEvaluator.Perform(gp_Pnt(0.0, 0.0, 0.0), THE_TOLERANCE, ExtremaPS::SearchMode::Min);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_EQ(aResult.NbExt(), static_cast<size_t>(18));
  for (size_t anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
  {
    const ExtremaPS::ExtremumResult& anExtremum = aResult[anIndex];
    EXPECT_TRUE(anExtremum.IsMinimum);
    EXPECT_NEAR(anExtremum.SquareDistance, 0.0, THE_TOLERANCE);

    const Geom_Surface::ResD1 aD1 = aSurface->EvalD1(anExtremum.U, anExtremum.V);
    const gp_Vec              aResidual(gp_Pnt(0.0, 0.0, 0.0), aD1.Point);
    EXPECT_NEAR(aResidual.Dot(aD1.D1U), 0.0, THE_TOLERANCE);
    EXPECT_NEAR(aResidual.Dot(aD1.D1V), 0.0, THE_TOLERANCE);
  }

  const double aVRoots[] = {0.5 * (1.0 - 1.0 / std::sqrt(2.0)), 0.5 * (1.0 + 1.0 / std::sqrt(2.0))};
  for (int aRootIndexU = 1; aRootIndexU <= 9; ++aRootIndexU)
  {
    const double anExpectedU = 0.5 * (1.0 + std::cos((2.0 * aRootIndexU - 1.0) * M_PI / 18.0));
    for (const double anExpectedV : aVRoots)
    {
      bool hasMinimum = false;
      for (size_t anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
      {
        hasMinimum = hasMinimum
                     || (std::abs(aResult[anIndex].U - anExpectedU) <= 1.0e-6
                         && std::abs(aResult[anIndex].V - anExpectedV) <= 1.0e-6);
      }
      EXPECT_TRUE(hasMinimum) << "missing minimum at (" << anExpectedU << ", " << anExpectedV
                              << ")";
    }
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, PolynomialOscillatorySurfaceFindsEveryInteriorMinimum)
{
  occ::handle<Geom_BSplineSurface> aSurface = MakeOscillatoryBSpline(false);
  ExtremaPS_BSplineSurface         anEvaluator(aSurface);
  const ExtremaPS::Result&         aResult =
    anEvaluator.Perform(gp_Pnt(0.0, 0.0, 0.0), THE_TOLERANCE, ExtremaPS::SearchMode::Min);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_EQ(aResult.NbExt(), static_cast<size_t>(18));
  const double aVRoots[] = {0.5 * (1.0 - 1.0 / std::sqrt(2.0)), 0.5 * (1.0 + 1.0 / std::sqrt(2.0))};
  for (int aRootIndexU = 1; aRootIndexU <= 9; ++aRootIndexU)
  {
    const double anExpectedU = 0.5 * (1.0 + std::cos((2.0 * aRootIndexU - 1.0) * M_PI / 18.0));
    for (const double anExpectedV : aVRoots)
    {
      bool hasMinimum = false;
      for (size_t anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
      {
        hasMinimum = hasMinimum
                     || (std::abs(aResult[anIndex].U - anExpectedU) <= 1.0e-6
                         && std::abs(aResult[anIndex].V - anExpectedV) <= 1.0e-6);
      }
      EXPECT_TRUE(hasMinimum) << "missing minimum at (" << anExpectedU << ", " << anExpectedV
                              << ")";
    }
  }
}

//==================================================================================================
// Partial Domain Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, PartialDomain_HalfU)
{
  gp_Pnt                   aP(2.5, 5.0, 5.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 0.5, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    EXPECT_LE(aResult.Extrema.Value(i).U, 0.5 + THE_TOLERANCE);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, PartialDomain_HalfV)
{
  gp_Pnt                   aP(5.0, 2.5, 5.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 0.5));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    EXPECT_LE(aResult.Extrema.Value(i).V, 0.5 + THE_TOLERANCE);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, PartialDomain_SmallPatch)
{
  gp_Pnt                   aP(5.0, 5.0, 5.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.4, 0.6, 0.4, 0.6));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, PartialDomain_SingleKnotSpan)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  gp_Pnt                           aP(3.0, 3.0, 5.0);
  ExtremaPS_BSplineSurface         anEval(aWavy, ExtremaPS::Domain2D(0.0, 0.33, 0.0, 0.33));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
}

//==================================================================================================
// SearchMode Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, SearchMode_MinOnly)
{
  gp_Pnt                   aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult =
    anEval.PerformWithBoundary(aP, THE_TOLERANCE, ExtremaPS::SearchMode::Min);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    EXPECT_TRUE(aResult.Extrema.Value(i).IsMinimum);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, SearchMode_MaxOnly)
{
  gp_Pnt                   aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult =
    anEval.PerformWithBoundary(aP, THE_TOLERANCE, ExtremaPS::SearchMode::Max);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, SearchMode_DomeMinMax)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  gp_Pnt                           aP(5.0, 5.0, 10.0);
  ExtremaPS_BSplineSurface         anEval(aDome, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult =
    anEval.PerformWithBoundary(aP, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  EXPECT_GE(aResult.Extrema.Size(), 1);
}

//==================================================================================================
// Aggregator Integration Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, Aggregator_FlatSurface)
{
  GeomAdaptor_Surface anAdaptor(myFlatSurface);
  ExtremaPS_Surface   anExtPS(anAdaptor);

  gp_Pnt                   aP(5.0, 5.0, 7.0);
  const ExtremaPS::Result& aResult = anExtPS.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 7.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, Aggregator_DomeSurface)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  GeomAdaptor_Surface              anAdaptor(aDome);
  ExtremaPS_Surface                anExtPS(anAdaptor);

  gp_Pnt                   aP(5.0, 5.0, 12.0);
  const ExtremaPS::Result& aResult = anExtPS.Perform(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, Aggregator_WithSearchMode)
{
  GeomAdaptor_Surface anAdaptor(myFlatSurface);
  ExtremaPS_Surface   anExtPS(anAdaptor);

  gp_Pnt                   aP(5.0, 5.0, 7.0);
  const ExtremaPS::Result& aResult =
    anExtPS.PerformWithBoundary(aP, THE_TOLERANCE, ExtremaPS::SearchMode::Min);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    EXPECT_TRUE(aResult.Extrema.Value(i).IsMinimum);
  }
}

//==================================================================================================
// Edge Cases
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, EdgeCase_VeryClosePoint)
{
  gp_Pnt                   aP(5.0, 5.0, 1.0e-8);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(aMinDist, 0.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, EdgeCase_VeryFarPoint)
{
  gp_Pnt                   aP(5.0, 5.0, 1000.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 1000.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, EdgeCase_PointOnEdge)
{
  gp_Pnt                   aP(5.0, 0.0, 3.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), 3.0, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, EdgeCase_DiagonalPoint)
{
  gp_Pnt                   aP(15.0, 15.0, 0.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  double aMinDist = aResult.MinSquareDistance();
  EXPECT_NEAR(std::sqrt(aMinDist), std::sqrt(50.0), THE_TOLERANCE);
}

//==================================================================================================
// Result Verification Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, Verify_PointOnSurface)
{
  occ::handle<Geom_BSplineSurface> aDome = MakeDomeBSpline();
  gp_Pnt                           aP(3.0, 7.0, 8.0);
  ExtremaPS_BSplineSurface         anEval(aDome, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result&         aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    const ExtremaPS::ExtremumResult& anExt   = aResult.Extrema.Value(i);
    gp_Pnt                           aSurfPt = aDome->Value(anExt.U, anExt.V);
    EXPECT_NEAR(anExt.Point.Distance(aSurfPt), 0.0, THE_TOLERANCE * 10);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, Verify_SquareDistanceConsistent)
{
  gp_Pnt                   aP(7.0, 3.0, 4.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    const ExtremaPS::ExtremumResult& anExt         = aResult.Extrema.Value(i);
    double                           aActualSqDist = aP.SquareDistance(anExt.Point);
    EXPECT_NEAR(anExt.SquareDistance, aActualSqDist, THE_TOLERANCE);
  }
}

TEST_F(ExtremaPS_BSplineSurfaceTest, Verify_ParametersInRange)
{
  gp_Pnt                   aP(5.0, 5.0, 5.0);
  ExtremaPS_BSplineSurface anEval(myFlatSurface, ExtremaPS::Domain2D(0.2, 0.8, 0.3, 0.7));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);

  for (size_t i = 0; i < aResult.Extrema.Size(); ++i)
  {
    const ExtremaPS::ExtremumResult& anExt = aResult.Extrema.Value(i);
    EXPECT_GE(anExt.U, 0.2 - THE_TOLERANCE);
    EXPECT_LE(anExt.U, 0.8 + THE_TOLERANCE);
    EXPECT_GE(anExt.V, 0.3 - THE_TOLERANCE);
    EXPECT_LE(anExt.V, 0.7 + THE_TOLERANCE);
  }
}

//==================================================================================================
// Knot-Specific Tests
//==================================================================================================

TEST_F(ExtremaPS_BSplineSurfaceTest, Knots_PointNearKnot)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  // Point near internal knot at U=0.33
  gp_Pnt                   aP(3.3, 5.0, 5.0);
  ExtremaPS_BSplineSurface anEval(aWavy, ExtremaPS::Domain2D(0.0, 1.0, 0.0, 1.0));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
  ASSERT_GE(aResult.Extrema.Size(), 1);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, Knots_CrossingKnotSpan)
{
  occ::handle<Geom_BSplineSurface> aWavy = MakeWavyBSpline();
  // Range that crosses knot at 0.33
  gp_Pnt                   aP(5.0, 5.0, 5.0);
  ExtremaPS_BSplineSurface anEval(aWavy, ExtremaPS::Domain2D(0.2, 0.5, 0.2, 0.5));
  const ExtremaPS::Result& aResult = anEval.PerformWithBoundary(aP, THE_TOLERANCE);

  ASSERT_EQ(aResult.Status, ExtremaPS::Status::OK);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, RepeatedMinMaxPreservesCompleteResult)
{
  occ::handle<Geom_BSplineSurface> aSurface = MakeWavyBSpline();
  ExtremaPS_BSplineSurface         anEval(aSurface);
  const gp_Pnt                     aQuery(5.0, 5.0, 0.0);

  const ExtremaPS::Result& aFirst =
    anEval.PerformWithBoundary(aQuery, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);
  ASSERT_EQ(aFirst.Status, ExtremaPS::Status::OK);
  const size_t aNbExtrema         = aFirst.NbExt();
  const double aMinSquareDistance = aFirst.MinSquareDistance();
  const double aMaxSquareDistance = aFirst.MaxSquareDistance();

  const ExtremaPS::Result& aSecond =
    anEval.PerformWithBoundary(aQuery, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);
  ASSERT_EQ(aSecond.Status, ExtremaPS::Status::OK);
  EXPECT_EQ(aSecond.NbExt(), aNbExtrema);
  EXPECT_NEAR(aSecond.MinSquareDistance(), aMinSquareDistance, THE_TOLERANCE);
  EXPECT_NEAR(aSecond.MaxSquareDistance(), aMaxSquareDistance, THE_TOLERANCE);
}

TEST_F(ExtremaPS_BSplineSurfaceTest, AffineTrajectoryMatchesIndependentEvaluation)
{
  const occ::handle<Geom_BSplineSurface> aSurface = MakeWavyBSpline();
  ExtremaPS_BSplineSurface               aReused(aSurface);
  NCollection_LinearVector<gp_Pnt>       aQueries;
  for (int anIndex = 0; anIndex < 20; ++anIndex)
  {
    const double aStep = static_cast<double>(anIndex);
    aQueries.Append(gp_Pnt(0.5 + 0.17 * aStep, 1.0 + 0.08 * aStep, 8.0 - 0.03 * aStep));
  }
  for (int anIndex = 19; anIndex >= 0; --anIndex)
  {
    aQueries.Append(aQueries.Value(static_cast<size_t>(anIndex)));
  }
  aQueries.Append(gp_Pnt(4.25, 2.75, 6.5));
  aQueries.Append(gp_Pnt(4.4, 2.9, 6.45));
  aQueries.Append(gp_Pnt(4.55, 3.05, 6.4));

  for (const gp_Pnt& aQuery : aQueries)
  {
    const ExtremaPS::Result& aReusedResult =
      aReused.Perform(aQuery, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);
    ExtremaPS_BSplineSurface aIndependent(aSurface);
    const ExtremaPS::Result& aExactResult =
      aIndependent.Perform(aQuery, THE_TOLERANCE, ExtremaPS::SearchMode::MinMax);

    ASSERT_EQ(aReusedResult.Status, aExactResult.Status);
    ASSERT_EQ(aReusedResult.NbExt(), aExactResult.NbExt());
    for (const ExtremaPS::ExtremumResult& anAffineExtremum : aReusedResult.Extrema)
    {
      bool hasMatch = false;
      for (const ExtremaPS::ExtremumResult& anExactExtremum : aExactResult.Extrema)
      {
        if (anAffineExtremum.IsMinimum == anExactExtremum.IsMinimum
            && std::abs(anAffineExtremum.U - anExactExtremum.U) <= THE_TOLERANCE
            && std::abs(anAffineExtremum.V - anExactExtremum.V) <= THE_TOLERANCE
            && std::abs(anAffineExtremum.SquareDistance - anExactExtremum.SquareDistance)
                 <= THE_TOLERANCE)
        {
          hasMatch = true;
          break;
        }
      }
      EXPECT_TRUE(hasMatch);
    }
  }
}
