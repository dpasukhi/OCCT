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

#include <gtest/gtest.h>

#include <cmath>
#include <csignal>
#include <cstdlib>
#include <limits>
#include <OSD.hxx>

#include <Adaptor3d_CurveOnSurface.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Line.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomLib_CheckCurveOnSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Standard_ConstructionError.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Precision.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

namespace
{
// Circle of radius theRadius at height theHeight on a cylinder of radius
// theCylRadius around OZ; the pcurve is the corresponding iso line in UV.
GeomLib_CheckCurveOnSurface makeCircleOnCylinderCheck(const double theCircRadius,
                                                      const double theCylRadius,
                                                      const double theHeight)
{
  const double aFirst = 0.0, aLast = 2.0 * M_PI;

  const occ::handle<Geom_Circle> aCirc = new Geom_Circle(
    gp_Ax2(gp_Pnt(0.0, 0.0, theHeight), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)),
    theCircRadius);
  const occ::handle<GeomAdaptor_Curve> aC3d = new GeomAdaptor_Curve(aCirc, aFirst, aLast);

  const occ::handle<Geom_CylindricalSurface> aCyl = new Geom_CylindricalSurface(
    gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)),
    theCylRadius);
  const occ::handle<Geom2d_Line> aPLine =
    new Geom2d_Line(gp_Pnt2d(0.0, theHeight), gp_Dir2d(1.0, 0.0));

  const occ::handle<Geom2dAdaptor_Curve>      aC2d = new Geom2dAdaptor_Curve(aPLine, aFirst, aLast);
  const occ::handle<GeomAdaptor_Surface>      aSurf = new GeomAdaptor_Surface(aCyl);
  const occ::handle<Adaptor3d_CurveOnSurface> aCoS  = new Adaptor3d_CurveOnSurface(aC2d, aSurf);

  GeomLib_CheckCurveOnSurface aCheck;
  aCheck.Init(aC3d, Precision::PConfusion());
  aCheck.Perform(aCoS);
  return aCheck;
}

GeomLib_CheckCurveOnSurface makeLineOnPlaneCheck(const gp_Dir&   theCurveDirection,
                                                 const gp_Dir2d& thePCurveDirection,
                                                 const double    theFirst,
                                                 const double    theLast)
{
  const occ::handle<Geom_Line> aLine = new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), theCurveDirection);
  const occ::handle<GeomAdaptor_Curve> aC3d = new GeomAdaptor_Curve(aLine, theFirst, theLast);

  const occ::handle<Geom_Plane> aPlane =
    new Geom_Plane(gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)));
  const occ::handle<Geom2d_Line> aPLine = new Geom2d_Line(gp_Pnt2d(0.0, 0.0), thePCurveDirection);
  const occ::handle<Geom2dAdaptor_Curve> aC2d  = new Geom2dAdaptor_Curve(aPLine, theFirst, theLast);
  const occ::handle<GeomAdaptor_Surface> aSurf = new GeomAdaptor_Surface(aPlane);
  const occ::handle<Adaptor3d_CurveOnSurface> aCoS = new Adaptor3d_CurveOnSurface(aC2d, aSurf);

  GeomLib_CheckCurveOnSurface aCheck;
  aCheck.Init(aC3d, Precision::PConfusion());
  aCheck.Perform(aCoS);
  return aCheck;
}

GeomLib_CheckCurveOnSurface makeCircleAndEllipseCheck(const double theMajorRadius,
                                                      const double theMinorRadius,
                                                      const double theFirst,
                                                      const double theLast)
{
  const occ::handle<Geom_Circle> aCircle =
    new Geom_Circle(gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)),
                    2.0);
  const occ::handle<GeomAdaptor_Curve> aC3d = new GeomAdaptor_Curve(aCircle, theFirst, theLast);

  const occ::handle<Geom2d_Ellipse> anEllipse =
    new Geom2d_Ellipse(gp_Ax2d(gp_Pnt2d(0.0, 0.0), gp_Dir2d(1.0, 0.0)),
                       theMajorRadius,
                       theMinorRadius);
  const occ::handle<Geom2dAdaptor_Curve> aC2d =
    new Geom2dAdaptor_Curve(anEllipse, theFirst, theLast);
  const occ::handle<Geom_Plane> aPlane =
    new Geom_Plane(gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)));
  const occ::handle<GeomAdaptor_Surface>      aSurf = new GeomAdaptor_Surface(aPlane);
  const occ::handle<Adaptor3d_CurveOnSurface> aCoS  = new Adaptor3d_CurveOnSurface(aC2d, aSurf);

  GeomLib_CheckCurveOnSurface aCheck;
  aCheck.Init(aC3d, Precision::PConfusion());
  aCheck.Perform(aCoS);
  return aCheck;
}

// Two intervals: one evaluates normally, the other simulates an evaluator failure.
// Successful intervals must not conceal a missing part of the deviation search.
class FailingCurve : public Adaptor3d_Curve
{
public:
  explicit FailingCurve(const bool theDerivatives)
      : myDerivatives(theDerivatives)
  {
    NCollection_Array1<gp_Pnt> aPoles(1, 3);
    aPoles(1) = gp_Pnt(0, 0, 0);
    aPoles(2) = gp_Pnt(0.5, 0, 0);
    aPoles(3) = gp_Pnt(1, 0, 0);
    NCollection_Array1<double> aKnots(1, 3);
    aKnots(1) = 0;
    aKnots(2) = 0.5;
    aKnots(3) = 1;
    NCollection_Array1<int> aMults(1, 3);
    aMults(1) = 2;
    aMults(2) = 1;
    aMults(3) = 2;
    myCurve.Load(new Geom_BSplineCurve(aPoles, aKnots, aMults, 1));
  }

  occ::handle<Adaptor3d_Curve> ShallowCopy() const override
  {
    return new FailingCurve(myDerivatives);
  }

  double FirstParameter() const override { return 0; }

  double LastParameter() const override { return 1; }

  GeomAbs_CurveType GetType() const override { return GeomAbs_BSplineCurve; }

  occ::handle<Geom_BSplineCurve> BSpline() const override { return myCurve.BSpline(); }

  gp_Pnt EvalD0(const double theU) const override
  {
    if (!myDerivatives && theU > 0.5)
    {
      throw Standard_ConstructionError("test point evaluation failure");
    }
    return myCurve.EvalD0(theU);
  }

  Geom_Curve::ResD1 EvalD1(const double theU) const override
  {
    if (myDerivatives && theU > 0.5)
    {
      throw Standard_ConstructionError("test derivative evaluation failure");
    }
    return myCurve.EvalD1(theU);
  }

  Geom_Curve::ResD2 EvalD2(const double theU) const override
  {
    if (myDerivatives && theU > 0.5)
    {
      throw Standard_ConstructionError("test derivative evaluation failure");
    }
    return myCurve.EvalD2(theU);
  }

private:
  bool              myDerivatives;
  GeomAdaptor_Curve myCurve;
};
} // namespace

TEST(GeomLib_CheckCurveOnSurfaceTest, AnalyticCoincident_ReportsZeroDeviation)
{
  const GeomLib_CheckCurveOnSurface aCheck = makeCircleOnCylinderCheck(2.0, 2.0, 5.0);

  EXPECT_TRUE(aCheck.IsDone());
  EXPECT_LE(aCheck.MaxDistance(), Precision::Confusion());
}

TEST(GeomLib_CheckCurveOnSurfaceTest, AnalyticDeviating_ReportsActualDeviation)
{
  // The 3D circle is 0.05 smaller than the cylinder carrying the pcurve: the
  // deviation is constant and equal to the radius difference. This guards the
  // constant-distance shortcut against swallowing real deviations.
  const GeomLib_CheckCurveOnSurface aCheck = makeCircleOnCylinderCheck(1.95, 2.0, 5.0);

  EXPECT_TRUE(aCheck.IsDone());
  EXPECT_NEAR(aCheck.MaxDistance(), 0.05, 1.0e-9);
}

TEST(GeomLib_CheckCurveOnSurfaceTest, LineOnPlane_ReportsZeroDeviation)
{
  const GeomLib_CheckCurveOnSurface aCheck =
    makeLineOnPlaneCheck(gp_Dir(1.0, 0.0, 0.0), gp_Dir2d(1.0, 0.0), -10.0, 10.0);

  EXPECT_TRUE(aCheck.IsDone());
  EXPECT_LE(aCheck.MaxDistance(), Precision::Confusion());
}

TEST(GeomLib_CheckCurveOnSurfaceTest, DeviatingLines_ReportsEndpointMaximum)
{
  const GeomLib_CheckCurveOnSurface aCheck =
    makeLineOnPlaneCheck(gp_Dir(1.0, 0.0, 0.0), gp_Dir2d(0.0, 1.0), -2.0, 3.0);

  ASSERT_TRUE(aCheck.IsDone());
  EXPECT_NEAR(aCheck.MaxDistance(), std::sqrt(18.0), Precision::Confusion());
  EXPECT_DOUBLE_EQ(aCheck.MaxParameter(), 3.0);
}

TEST(GeomLib_CheckCurveOnSurfaceTest, CircleAndEllipse_ReportsAnalyticMaximum)
{
  const GeomLib_CheckCurveOnSurface aCheck = makeCircleAndEllipseCheck(4.0, 1.0, 0.0, 2.0 * M_PI);

  ASSERT_TRUE(aCheck.IsDone());
  EXPECT_NEAR(aCheck.MaxDistance(), 2.0, Precision::Confusion());
}

TEST(GeomLib_CheckCurveOnSurfaceTest, NegativeTrim_ReportsInteriorMaximum)
{
  const GeomLib_CheckCurveOnSurface aCheck = makeCircleAndEllipseCheck(2.0, 1.0, -2.0, -1.0);

  ASSERT_TRUE(aCheck.IsDone());
  EXPECT_NEAR(aCheck.MaxDistance(), 1.0, Precision::Confusion());
  EXPECT_NEAR(aCheck.MaxParameter(), -0.5 * M_PI, Precision::PConfusion());
}

TEST(GeomLib_CheckCurveOnSurfaceTest, AnalyticAliasing_ReportsActualDeviation)
{
  const double aFirst = 0.0, aLast = 80.0 * M_PI;

  const occ::handle<Geom_Circle> aCircle =
    new Geom_Circle(gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)),
                    4.0);
  const occ::handle<GeomAdaptor_Curve> aC3d = new GeomAdaptor_Curve(aCircle, aFirst, aLast);

  const occ::handle<Geom_ToroidalSurface> aTorus = new Geom_ToroidalSurface(
    gp_Ax3(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0), gp_Dir(1.0, 0.0, 0.0)),
    3.0,
    1.0);
  const occ::handle<Geom2d_Line> aPLine = new Geom2d_Line(gp_Pnt2d(0.0, 0.0), gp_Dir2d(3.0, 4.0));
  const occ::handle<Geom2dAdaptor_Curve>      aC2d = new Geom2dAdaptor_Curve(aPLine, aFirst, aLast);
  const occ::handle<GeomAdaptor_Surface>      aSurf = new GeomAdaptor_Surface(aTorus);
  const occ::handle<Adaptor3d_CurveOnSurface> aCoS  = new Adaptor3d_CurveOnSurface(aC2d, aSurf);

  GeomLib_CheckCurveOnSurface aCheck;
  aCheck.Init(aC3d, Precision::PConfusion());
  aCheck.Perform(aCoS);

  ASSERT_TRUE(aCheck.IsDone());
  EXPECT_NEAR(aCheck.MaxDistance(), 8.0, Precision::Confusion());
}

TEST(GeomLib_CheckCurveOnSurfaceTest, FailedIntervalIsNotHiddenBySuccessfulIntervals)
{
  const occ::handle<Geom2dAdaptor_Curve> aPCurve =
    new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(0, 1), gp_Dir2d(1, 0)), 0, 1);
  const occ::handle<GeomAdaptor_Surface> aSurface =
    new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
  const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
    new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
  for (const bool isParallel : {false, true})
  {
    for (const bool failDerivatives : {false, true})
    {
      SCOPED_TRACE(isParallel);
      SCOPED_TRACE(failDerivatives);
      GeomLib_CheckCurveOnSurface aCheck(new FailingCurve(failDerivatives));
      aCheck.SetParallel(isParallel);
      EXPECT_NO_THROW(aCheck.Perform(aCurveOnSurface));
      // Derivatives are not required by the deviation search.
      EXPECT_EQ(aCheck.IsDone(), failDerivatives);
      EXPECT_EQ(aCheck.ErrorStatus(), failDerivatives ? 0 : 3);
    }
  }
}

TEST(GeomLib_CheckCurveOnSurfaceTest, SmallKnotIntervalRetainsSpatialMaximum)
{
  // Parameter scale says nothing about spatial variation. The only nonzero
  // deviation lies inside a span smaller than Precision::PConfusion().
  for (const double aKnot : {1.e-12, 1.e-20, 1.e-40})
  {
    SCOPED_TRACE(aKnot);
    NCollection_Array1<gp_Pnt> aPoles(1, 5);
    aPoles(1) = gp_Pnt(0, 0, 0);
    aPoles(2) = gp_Pnt(aKnot / 2, 10, 0);
    aPoles(3) = gp_Pnt(aKnot, 0, 0);
    aPoles(4) = gp_Pnt((1 + aKnot) / 2, 0, 0);
    aPoles(5) = gp_Pnt(1, 0, 0);
    NCollection_Array1<double> aKnots(1, 3);
    aKnots(1) = 0;
    aKnots(2) = aKnot;
    aKnots(3) = 1;
    NCollection_Array1<int> aMults(1, 3);
    aMults(1) = 3;
    aMults(2) = 2;
    aMults(3) = 3;
    const occ::handle<GeomAdaptor_Curve> aCurve =
      new GeomAdaptor_Curve(new Geom_BSplineCurve(aPoles, aKnots, aMults, 2));
    const occ::handle<Geom2dAdaptor_Curve> aPCurve =
      new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(), gp_Dir2d(1, 0)), 0, 1);
    const occ::handle<GeomAdaptor_Surface> aSurface =
      new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
    const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
      new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
    for (const bool isParallel : {false, true})
    {
      GeomLib_CheckCurveOnSurface aCheck(aCurve);
      aCheck.SetParallel(isParallel);
      aCheck.Perform(aCurveOnSurface);
      ASSERT_TRUE(aCheck.IsDone());
      EXPECT_NEAR(aCheck.MaxDistance(), 5, Precision::Confusion());
      EXPECT_NEAR(aCheck.MaxParameter() / aKnot, 0.5, 1.e-6);
    }
  }
}

#if defined(OCC_CONVERT_SIGNALS) && GTEST_HAS_DEATH_TEST
namespace
{
class SignalCurve : public FailingCurve
{
public:
  SignalCurve()
      : FailingCurve(false)
  {
  }

  occ::handle<Adaptor3d_Curve> ShallowCopy() const override { return new SignalCurve; }

  gp_Pnt EvalD0(const double) const override
  {
    std::raise(SIGFPE);
    return {};
  }
};
} // namespace

TEST(GeomLib_CheckCurveOnSurfaceTest, IntervalGuardHandlesSignalsOnSerialAndParallelWorkers)
{
  // Change process signal handlers only in an isolated, freshly executed child.
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  for (const bool isParallel : {false, true})
  {
    EXPECT_EXIT(
      {
        OSD::SetSignal(OSD_SignalMode_Set, false);
        const occ::handle<Geom2dAdaptor_Curve> aPCurve =
          new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(), gp_Dir2d(1, 0)), 0, 1);
        const occ::handle<GeomAdaptor_Surface> aSurface =
          new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
        const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
          new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
        GeomLib_CheckCurveOnSurface aCheck(new SignalCurve);
        aCheck.SetParallel(isParallel);
        aCheck.Perform(aCurveOnSurface);
        std::_Exit(!aCheck.IsDone() && aCheck.ErrorStatus() == 3 ? 0 : 1);
      },
      ::testing::ExitedWithCode(0),
      "");
  }
}
#endif

namespace
{
class NonFiniteCurve : public FailingCurve
{
public:
  NonFiniteCurve()
      : FailingCurve(false)
  {
  }

  occ::handle<Adaptor3d_Curve> ShallowCopy() const override { return new NonFiniteCurve; }

  gp_Pnt EvalD0(const double theU) const override
  {
    return gp_Pnt(theU > 0.5 ? std::numeric_limits<double>::quiet_NaN() : theU, 0, 0);
  }
};
} // namespace

TEST(GeomLib_CheckCurveOnSurfaceTest, NonFiniteDistanceIsAnEvaluationFailure)
{
  const occ::handle<Geom2dAdaptor_Curve> aPCurve =
    new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(), gp_Dir2d(1, 0)), 0, 1);
  const occ::handle<GeomAdaptor_Surface> aSurface =
    new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
  const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
    new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
  for (const bool isParallel : {false, true})
  {
    GeomLib_CheckCurveOnSurface aCheck(new NonFiniteCurve);
    aCheck.SetParallel(isParallel);
    aCheck.Perform(aCurveOnSurface);
    EXPECT_FALSE(aCheck.IsDone());
    EXPECT_EQ(aCheck.ErrorStatus(), 3);
  }
}

TEST(GeomLib_CheckCurveOnSurfaceTest, MultimodalBezierDeviationIsIndependentOfSpatialScale)
{
  const double               aHeights[] = {0, 1, 4, -8, 2, 9, -3, 1, 0};
  NCollection_Array1<gp_Pnt> aPoles(1, 9);
  for (int i = 1; i <= 9; ++i)
  {
    aPoles(i) = gp_Pnt(double(i - 1) / 8, aHeights[i - 1], 0);
  }
  const occ::handle<Geom_BezierCurve> aReference    = new Geom_BezierCurve(aPoles);
  double                              aDenseMaximum = 0;
  for (int i = 0; i <= 20000; ++i)
  {
    aDenseMaximum = std::max(aDenseMaximum, std::abs(aReference->Value(double(i) / 20000).Y()));
  }
  const occ::handle<Geom2dAdaptor_Curve> aPCurve =
    new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(), gp_Dir2d(1, 0)), 0, 1);
  const occ::handle<GeomAdaptor_Surface> aSurface =
    new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
  const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
    new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
  for (const double aScale : {1.e-8, 1.0, 1.e8})
  {
    SCOPED_TRACE(aScale);
    for (int i = 1; i <= 9; ++i)
    {
      aPoles(i).SetY(aScale * aHeights[i - 1]);
    }
    const occ::handle<GeomAdaptor_Curve> aCurve =
      new GeomAdaptor_Curve(new Geom_BezierCurve(aPoles));
    GeomLib_CheckCurveOnSurface aCheck(aCurve);
    aCheck.Perform(aCurveOnSurface);
    ASSERT_TRUE(aCheck.IsDone());
    EXPECT_NEAR(aCheck.MaxDistance() / aScale, aDenseMaximum, 1.e-7);
    EXPECT_GE(aCheck.MaxParameter(), 0);
    EXPECT_LE(aCheck.MaxParameter(), 1);
  }
}

TEST(GeomLib_CheckCurveOnSurfaceTest, InvalidSearchRangeIsRejectedBeforeEvaluation)
{
  class InvalidRange : public Adaptor3d_Curve
  {
  public:
    explicit InvalidRange(double theLast)
        : myLast(theLast)
    {
    }

    double FirstParameter() const override { return 0; }

    double LastParameter() const override { return myLast; }

  private:
    double myLast;
  };

  const occ::handle<Geom2dAdaptor_Curve> aPCurve =
    new Geom2dAdaptor_Curve(new Geom2d_Line(gp_Pnt2d(), gp_Dir2d(1, 0)), 0, 1);
  const occ::handle<GeomAdaptor_Surface> aSurface =
    new GeomAdaptor_Surface(new Geom_Plane(gp_Ax3()));
  const occ::handle<Adaptor3d_CurveOnSurface> aCurveOnSurface =
    new Adaptor3d_CurveOnSurface(aPCurve, aSurface);
  for (const double aLast : {0.0,
                             -1.0,
                             std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()})
  {
    GeomLib_CheckCurveOnSurface aCheck(new InvalidRange(aLast));
    EXPECT_NO_THROW(aCheck.Perform(aCurveOnSurface));
    EXPECT_FALSE(aCheck.IsDone());
    EXPECT_EQ(aCheck.ErrorStatus(), 2);
  }
}
