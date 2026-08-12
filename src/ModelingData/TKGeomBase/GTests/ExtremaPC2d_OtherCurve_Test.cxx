// Copyright (c) 2026 OPEN CASCADE SAS
// This file is part of Open CASCADE Technology software library.

#include <ExtremaPC2d_OtherCurve.hxx>

#include <Geom2dEval_SineWaveCurve.hxx>
#include <Geom2dAdaptor_Curve.hxx>

#include <gtest/gtest.h>

#include <cmath>

namespace
{
constexpr double THE_TOL = 1.0e-8;
}

TEST(ExtremaPC2d_OtherCurveTest, SineWaveFindsOriginMinimum)
{
  occ::handle<Geom2dEval_SineWaveCurve> aCurve = new Geom2dEval_SineWaveCurve(
    gp_Ax2d(gp_Pnt2d(), gp_Dir2d(1.0, 0.0)),
    1.0,
    1.0);
  Geom2dAdaptor_Curve anAdaptor(aCurve, -M_PI, M_PI);
  ExtremaPC2d_OtherCurve anEvaluator(anAdaptor, {-M_PI, M_PI});
  const ExtremaPC2d::Result& aResult = anEvaluator.Perform(gp_Pnt2d(-1.0, 1.0), THE_TOL,
                                                           ExtremaPC2d::SearchMode::Min);
  ASSERT_GE(aResult.NbExt(), 1);
  bool hasOrigin = false;
  for (int anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
  {
    hasOrigin = hasOrigin || std::abs(aResult[anIndex].Parameter) <= THE_TOL;
    EXPECT_EQ(aResult[anIndex].Point.Distance(aCurve->Value(aResult[anIndex].Parameter)), 0.0);
  }
  EXPECT_TRUE(hasOrigin);
}

TEST(ExtremaPC2d_OtherCurveTest, MultipleWavesReturnMultipleExtrema)
{
  occ::handle<Geom2dEval_SineWaveCurve> aCurve = new Geom2dEval_SineWaveCurve(
    gp_Ax2d(gp_Pnt2d(), gp_Dir2d(1.0, 0.0)),
    1.0,
    3.0);
  Geom2dAdaptor_Curve anAdaptor(aCurve, -2.0, 2.0);
  ExtremaPC2d_OtherCurve anEvaluator(anAdaptor, {-2.0, 2.0});
  const ExtremaPC2d::Result& aResult = anEvaluator.Perform(gp_Pnt2d(), THE_TOL);
  ASSERT_TRUE(aResult.IsDone());
  EXPECT_GE(aResult.NbExt(), 5);
  for (int anIndex = 0; anIndex < aResult.NbExt(); ++anIndex)
  {
    gp_Pnt2d aPoint;
    gp_Vec2d aDerivative;
    aCurve->D1(aResult[anIndex].Parameter, aPoint, aDerivative);
    EXPECT_NEAR(gp_Vec2d(gp_Pnt2d(), aPoint).Dot(aDerivative), 0.0, 1.0e-6);
  }
}

TEST(ExtremaPC2d_OtherCurveTest, EndpointsAndModesAreForwarded)
{
  occ::handle<Geom2dEval_SineWaveCurve> aCurve = new Geom2dEval_SineWaveCurve(
    gp_Ax2d(gp_Pnt2d(), gp_Dir2d(1.0, 0.0)),
    1.0,
    1.0);
  Geom2dAdaptor_Curve anAdaptor(aCurve, -1.0, 1.0);
  ExtremaPC2d_OtherCurve anEvaluator(anAdaptor, {-1.0, 1.0});
  const ExtremaPC2d::Result& aMin = anEvaluator.PerformWithEndpoints(
    gp_Pnt2d(3.0, 0.0), THE_TOL, ExtremaPC2d::SearchMode::Min);
  for (int anIndex = 0; anIndex < aMin.NbExt(); ++anIndex)
  {
    EXPECT_TRUE(aMin[anIndex].IsMinimum);
  }
  const ExtremaPC2d::Result& aMax = anEvaluator.PerformWithEndpoints(
    gp_Pnt2d(3.0, 0.0), THE_TOL, ExtremaPC2d::SearchMode::Max);
  for (int anIndex = 0; anIndex < aMax.NbExt(); ++anIndex)
  {
    EXPECT_TRUE(aMax[anIndex].IsMaximum);
  }
}

TEST(ExtremaPC2d_OtherCurveTest, NaturalAndInvalidDomains)
{
  occ::handle<Geom2dEval_SineWaveCurve> aCurve = new Geom2dEval_SineWaveCurve(
    gp_Ax2d(gp_Pnt2d(), gp_Dir2d(1.0, 0.0)),
    1.0,
    1.0);
  Geom2dAdaptor_Curve anAdaptor(aCurve);
  ExtremaPC2d_OtherCurve aNatural(anAdaptor);
  EXPECT_EQ(aNatural.Perform(gp_Pnt2d(), THE_TOL).Status,
            ExtremaPC2d::Status::InvalidInput);
  ExtremaPC2d_OtherCurve anInvalid(anAdaptor, {1.0, 0.0});
  EXPECT_EQ(anInvalid.Perform(gp_Pnt2d(), THE_TOL).Status,
            ExtremaPC2d::Status::InvalidInput);
}
