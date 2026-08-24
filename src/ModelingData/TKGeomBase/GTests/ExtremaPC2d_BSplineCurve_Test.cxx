// Copyright (c) 2026 OPEN CASCADE SAS
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

#include <ExtremaPC2d_BSplineCurve.hxx>

#include <gtest/gtest.h>

#include <cmath>

namespace
{
constexpr double THE_TOL = 1.0e-8;

occ::handle<Geom2d_BSplineCurve> makeLinear()
{
  NCollection_Array1<gp_Pnt2d> aPoles(1, 2);
  aPoles(1) = gp_Pnt2d(0.0, 0.0);
  aPoles(2) = gp_Pnt2d(10.0, 0.0);
  NCollection_Array1<double> aKnots(1, 2);
  aKnots(1) = 0.0;
  aKnots(2) = 1.0;
  NCollection_Array1<int> aMults(1, 2);
  aMults(1) = 2;
  aMults(2) = 2;
  return new Geom2d_BSplineCurve(aPoles, aKnots, aMults, 1);
}

occ::handle<Geom2d_BSplineCurve> makeC1Curve()
{
  NCollection_Array1<gp_Pnt2d> aPoles(1, 4);
  aPoles(1) = gp_Pnt2d(0.0, 0.0);
  aPoles(2) = gp_Pnt2d(1.0, 1.0);
  aPoles(3) = gp_Pnt2d(2.0, 1.0);
  aPoles(4) = gp_Pnt2d(3.0, 0.0);
  NCollection_Array1<double> aKnots(1, 3);
  aKnots(1) = 0.0;
  aKnots(2) = 0.5;
  aKnots(3) = 1.0;
  NCollection_Array1<int> aMults(1, 3);
  aMults(1) = 3;
  aMults(2) = 1;
  aMults(3) = 3;
  return new Geom2d_BSplineCurve(aPoles, aKnots, aMults, 2);
}
} // namespace

TEST(ExtremaPC2d_BSplineCurveTest, LinearProjectionIsExact)
{
  ExtremaPC2d_BSplineCurve   anEvaluator(makeLinear());
  const ExtremaPC2d::Result& aResult = anEvaluator.Perform(gp_Pnt2d(6.0, 4.0), THE_TOL);
  ASSERT_EQ(aResult.NbExt(), 1);
  EXPECT_NEAR(aResult[0].Parameter, 0.6, THE_TOL);
  EXPECT_NEAR(aResult[0].SquareDistance, 16.0, THE_TOL);
}

TEST(ExtremaPC2d_BSplineCurveTest, PartialDomainUsesEndpointWhenRequested)
{
  ExtremaPC2d_BSplineCurve anEvaluator(makeLinear(), {0.0, 0.5});
  EXPECT_EQ(anEvaluator.Perform(gp_Pnt2d(8.0, 3.0), THE_TOL).NbExt(), 0);
  const ExtremaPC2d::Result& aResult =
    anEvaluator.PerformWithEndpoints(gp_Pnt2d(8.0, 3.0), THE_TOL, ExtremaPC2d::SearchMode::Min);
  ASSERT_EQ(aResult.NbExt(), 1);
  EXPECT_NEAR(aResult[0].Parameter, 0.5, THE_TOL);
}

TEST(ExtremaPC2d_BSplineCurveTest, C1JunctionPointIsFound)
{
  occ::handle<Geom2d_BSplineCurve> aCurve = makeC1Curve();
  ExtremaPC2d_BSplineCurve         anEvaluator(aCurve);
  const ExtremaPC2d::Result&       aResult =
    anEvaluator.Perform(aCurve->Value(0.5), THE_TOL, ExtremaPC2d::SearchMode::Min);
  ASSERT_GE(aResult.NbExt(), 1);
  EXPECT_NEAR(aResult.MinSquareDistance(), 0.0, THE_TOL);
  EXPECT_NEAR(aResult[aResult.MinIndex()].Parameter, 0.5, THE_TOL);
}

TEST(ExtremaPC2d_BSplineCurveTest, HighDegreeRationalFindsAllOscillatoryExtrema)
{
  constexpr int    aDegree = 9;
  constexpr double aNumerators[] =
    {-1.0, 17.0, -85.0, 221.0, -2431.0 / 7.0, 2431.0 / 7.0, -221.0, 85.0, -17.0, 1.0};
  NCollection_Array1<gp_Pnt2d> aPoles(1, aDegree + 1);
  NCollection_Array1<double>   aWeights(1, aDegree + 1);
  for (size_t anIndex = 0; anIndex < aPoles.Size(); ++anIndex)
  {
    const double aWeight       = anIndex % 2 == 0 ? 0.8 : 1.2;
    aWeights.ChangeAt(anIndex) = aWeight;
    aPoles.ChangeAt(anIndex)   = gp_Pnt2d(0.0, aNumerators[anIndex] / aWeight);
  }

  NCollection_Array1<double> aKnots(1, 2);
  aKnots(1) = 0.0;
  aKnots(2) = 1.0;
  NCollection_Array1<int> aMultiplicities(1, 2);
  aMultiplicities(1) = aDegree + 1;
  aMultiplicities(2) = aDegree + 1;

  occ::handle<Geom2d_BSplineCurve> aCurve =
    new Geom2d_BSplineCurve(aPoles, aWeights, aKnots, aMultiplicities, aDegree);
  const gp_Pnt2d             aPoint(0.0, 0.0);
  ExtremaPC2d_BSplineCurve   anEvaluator(aCurve);
  const ExtremaPC2d::Result& aResult = anEvaluator.Perform(aPoint, THE_TOL);

  ASSERT_TRUE(aResult.IsDone());
  ASSERT_EQ(aResult.NbExt(), static_cast<size_t>(2 * aDegree - 1));
  size_t aNbMinima = 0;
  size_t aNbMaxima = 0;
  for (size_t anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
  {
    const ExtremaPC2d::ExtremumResult& anExtremum = aResult[anIndex];
    const double                       aDelta     = 1.0e-6;
    const double                       aLeftDistance =
      aPoint.SquareDistance(aCurve->EvalD0(anExtremum.Parameter - aDelta));
    const double aRightDistance =
      aPoint.SquareDistance(aCurve->EvalD0(anExtremum.Parameter + aDelta));
    if (anExtremum.IsMinimum)
    {
      ++aNbMinima;
      EXPECT_LE(anExtremum.SquareDistance, aLeftDistance + THE_TOL);
      EXPECT_LE(anExtremum.SquareDistance, aRightDistance + THE_TOL);
    }
    else
    {
      ++aNbMaxima;
      EXPECT_GE(anExtremum.SquareDistance + THE_TOL, aLeftDistance);
      EXPECT_GE(anExtremum.SquareDistance + THE_TOL, aRightDistance);
    }
  }
  EXPECT_EQ(aNbMinima, static_cast<size_t>(aDegree));
  EXPECT_EQ(aNbMaxima, static_cast<size_t>(aDegree - 1));

  for (int aRootIndex = 1; aRootIndex <= aDegree; ++aRootIndex)
  {
    const double anExpected =
      0.5 * (1.0 + std::cos((2.0 * aRootIndex - 1.0) * M_PI / (2.0 * aDegree)));
    bool hasMinimum = false;
    for (size_t anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
    {
      hasMinimum = hasMinimum
                   || (aResult[anIndex].IsMinimum
                       && std::abs(aResult[anIndex].Parameter - anExpected) <= 1.0e-6);
    }
    EXPECT_TRUE(hasMinimum) << "missing Chebyshev zero at " << anExpected;
  }
}

TEST(ExtremaPC2d_BSplineCurveTest, DegenerateCurveHasInfiniteSolutions)
{
  occ::handle<Geom2d_BSplineCurve> aCurve = makeLinear();
  aCurve->SetPole(1, gp_Pnt2d(2.0, -1.0));
  aCurve->SetPole(2, gp_Pnt2d(2.0, -1.0));
  ExtremaPC2d_BSplineCurve   anEvaluator(aCurve);
  const ExtremaPC2d::Result& aResult = anEvaluator.Perform(gp_Pnt2d(5.0, 3.0), THE_TOL);
  EXPECT_TRUE(aResult.IsInfinite());
  EXPECT_NEAR(aResult.InfiniteSquareDistance, 25.0, THE_TOL);
}

TEST(ExtremaPC2d_BSplineCurveTest, RecreateEvaluatorAfterGeometryChange)
{
  occ::handle<Geom2d_BSplineCurve> aCurve = makeC1Curve();
  ExtremaPC2d_BSplineCurve         anInitialEvaluator(aCurve);
  const double                     aBefore =
    anInitialEvaluator.Perform(gp_Pnt2d(1.5, 2.0), THE_TOL).MinSquareDistance();
  aCurve->SetPole(2, gp_Pnt2d(1.0, -3.0));
  ExtremaPC2d_BSplineCurve   anUpdatedEvaluator(aCurve);
  const ExtremaPC2d::Result& anAfter = anUpdatedEvaluator.Perform(gp_Pnt2d(1.5, 2.0), THE_TOL);
  ASSERT_TRUE(anAfter.IsDone());
  EXPECT_GT(std::abs(anAfter.MinSquareDistance() - aBefore), 1.0e-4);
  EXPECT_NEAR(anAfter[anAfter.MinIndex()].Point.Distance(
                aCurve->Value(anAfter[anAfter.MinIndex()].Parameter)),
              0.0,
              THE_TOL);
}

TEST(ExtremaPC2d_BSplineCurveTest, SearchModesFilterExtrema)
{
  ExtremaPC2d_BSplineCurve   anEvaluator(makeC1Curve());
  const ExtremaPC2d::Result& aMin =
    anEvaluator.Perform(gp_Pnt2d(1.5, 2.0), THE_TOL, ExtremaPC2d::SearchMode::Min);
  for (size_t anIndex = 0; anIndex < aMin.NbExt(); ++anIndex)
  {
    EXPECT_TRUE(aMin[anIndex].IsMinimum);
  }
  const ExtremaPC2d::Result& aMax =
    anEvaluator.Perform(gp_Pnt2d(1.5, 2.0), THE_TOL, ExtremaPC2d::SearchMode::Max);
  for (size_t anIndex = 0; anIndex < aMax.NbExt(); ++anIndex)
  {
    EXPECT_TRUE(aMax[anIndex].IsMaximum);
  }
}

TEST(ExtremaPC2d_BSplineCurveTest, SingletonInvalidNullAndOwnership)
{
  ExtremaPC2d_BSplineCurve aSingleton(makeLinear(), {0.4, 0.4});
  EXPECT_EQ(aSingleton.Perform(gp_Pnt2d(), THE_TOL).NbExt(), 0);
  EXPECT_EQ(aSingleton.PerformWithEndpoints(gp_Pnt2d(), THE_TOL).NbExt(), 1);
  ExtremaPC2d_BSplineCurve anInvalid(makeLinear(), {1.0, 0.0});
  EXPECT_EQ(anInvalid.Perform(gp_Pnt2d(), THE_TOL).Status, ExtremaPC2d::Status::InvalidInput);
  occ::handle<Geom2d_BSplineCurve> aNull;
  ExtremaPC2d_BSplineCurve         aNullEvaluator(aNull);
  EXPECT_EQ(aNullEvaluator.Perform(gp_Pnt2d(), THE_TOL).Status, ExtremaPC2d::Status::NotDone);
  occ::handle<Geom2d_BSplineCurve> aCurve = makeLinear();
  ExtremaPC2d_BSplineCurve         anOwned(aCurve);
  aCurve.Nullify();
  EXPECT_FALSE(anOwned.Curve().IsNull());
}
