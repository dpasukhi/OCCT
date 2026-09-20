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

#include <GCPnts_AbscissaPoint.hxx>
#include <GeomProjLib.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Plane.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

namespace
{
occ::handle<Geom_Plane> makeOCC31661Plane()
{
  return new Geom_Plane(gp_Pln(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0)));
}

gp_Ax2 makeOCC31661CurveAxis()
{
  return gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 1.0, 1.0), gp_Dir(2.0, 0.0, -2.0));
}
} // namespace

// Migrated from tests/bugs/moddata_3/bug31661_1. Projection of a parabola onto
// the XY plane must preserve its conic type and the trimmed parameter range.
TEST(GeomProjLibTest, OCC31661_1_ProjectParabolaOnPlane)
{
  const occ::handle<Geom_Parabola> aParabola = new Geom_Parabola(makeOCC31661CurveAxis(), 10.0);
  const occ::handle<Geom_Plane>    aPlane    = makeOCC31661Plane();

  const occ::handle<Geom_Curve> aProjected =
    GeomProjLib::ProjectOnPlane(aParabola, aPlane, gp_Dir(0.0, 0.0, 1.0), false);
  ASSERT_FALSE(aProjected.IsNull());
  EXPECT_STREQ(aProjected->DynamicType()->Name(), "Geom_Parabola");

  const occ::handle<Geom_TrimmedCurve> aTrimmed = new Geom_TrimmedCurve(aParabola, -100.0, 100.0);
  const occ::handle<Geom_Curve>        aProjectedTrimmed =
    GeomProjLib::ProjectOnPlane(aTrimmed, aPlane, gp_Dir(0.0, 0.0, 1.0), false);
  ASSERT_FALSE(aProjectedTrimmed.IsNull());
  EXPECT_STREQ(aProjectedTrimmed->DynamicType()->Name(), "Geom_TrimmedCurve");

  const GeomAdaptor_Curve anAdaptor(aProjectedTrimmed);
  EXPECT_NEAR(GCPnts_AbscissaPoint::Length(anAdaptor, Precision::Confusion()),
              408.40363195229503,
              2.0e-5);
  // The parameter checks retain DRAW's references; the wider bound reflects
  // the relative numerical tolerance used by its checkreal command.
  EXPECT_NEAR(aProjectedTrimmed->FirstParameter(), -91.077748943768597, 2.0e-6);
  EXPECT_NEAR(aProjectedTrimmed->LastParameter(), 72.221567418462357, 2.0e-6);
}

// Migrated from tests/bugs/moddata_3/bug31661_2. Projection of a hyperbola
// must not crash and must retain the hyperbolic conic type after trimming.
TEST(GeomProjLibTest, OCC31661_2_ProjectHyperbolaOnPlane)
{
  const occ::handle<Geom_Hyperbola> aHyperbola =
    new Geom_Hyperbola(makeOCC31661CurveAxis(), 10.0, 10.0);
  const occ::handle<Geom_Plane> aPlane = makeOCC31661Plane();

  const occ::handle<Geom_Curve> aProjected =
    GeomProjLib::ProjectOnPlane(aHyperbola, aPlane, gp_Dir(0.0, 0.0, 1.0), false);
  ASSERT_FALSE(aProjected.IsNull());
  EXPECT_STREQ(aProjected->DynamicType()->Name(), "Geom_Hyperbola");

  const occ::handle<Geom_TrimmedCurve> aTrimmed = new Geom_TrimmedCurve(aHyperbola, -5.0, 5.0);
  const occ::handle<Geom_Curve>        aProjectedTrimmed =
    GeomProjLib::ProjectOnPlane(aTrimmed, aPlane, gp_Dir(0.0, 0.0, 1.0), false);
  ASSERT_FALSE(aProjectedTrimmed.IsNull());
  EXPECT_STREQ(aProjectedTrimmed->DynamicType()->Name(), "Geom_TrimmedCurve");

  const GeomAdaptor_Curve anAdaptor(aProjectedTrimmed);
  EXPECT_NEAR(GCPnts_AbscissaPoint::Length(anAdaptor, Precision::Confusion()),
              1664.3732976598988,
              2.0e-5);
  EXPECT_NEAR(aProjectedTrimmed->FirstParameter(), -5.23179933356147, 1.0e-7);
  EXPECT_NEAR(aProjectedTrimmed->LastParameter(), 4.76820064934972, 1.0e-7);
}

//=================================================================================================

TEST(GeomProjLibTest, ConicProjectionPreservesShiftedTrimParameters)
{
  // Projection along Z from a parallel plane preserves the complete conic
  // parameterization, including trims outside its principal periodic range.
  const gp_Ax2                  anAxis(gp_Pnt(3, -2, 5), gp::DZ());
  const occ::handle<Geom_Plane> aPlane    = new Geom_Plane(gp::XOY());
  const occ::handle<Geom_Curve> aConics[] = {new Geom_Circle(anAxis, 2),
                                             new Geom_Ellipse(anAxis, 3, 1)};
  for (const occ::handle<Geom_Curve>& aConic : aConics)
  {
    const occ::handle<Geom_Curve> aTrim = new Geom_TrimmedCurve(aConic, 7.0, 11.0, true, false);
    const occ::handle<Geom_Curve> aProjection =
      GeomProjLib::ProjectOnPlane(aTrim, aPlane, gp::DZ(), true);
    ASSERT_FALSE(aProjection.IsNull());
    EXPECT_DOUBLE_EQ(aProjection->FirstParameter(), 7.0);
    EXPECT_DOUBLE_EQ(aProjection->LastParameter(), 11.0);
    for (int i = 0; i <= 16; ++i)
    {
      const double aParameter = 7.0 + i * 0.25;
      gp_Pnt       anExpected = aTrim->Value(aParameter);
      anExpected.SetZ(0);
      EXPECT_LT(anExpected.Distance(aProjection->Value(aParameter)), Precision::Confusion());
    }
  }
}

//=================================================================================================

TEST(GeomProjLibTest, ConicProjectionRecoversEndpointsWhenParametersChange)
{
  const occ::handle<Geom_Plane> aPlane = new Geom_Plane(gp::XOY());
  const occ::handle<Geom_Curve> aConic =
    new Geom_Ellipse(gp_Ax2(gp_Pnt(1, 2, 4), gp_Dir(1, 2, 3)), 3, 1);
  const occ::handle<Geom_Curve> aTrim = new Geom_TrimmedCurve(aConic, 0.3, 2.7);
  const occ::handle<Geom_Curve> aProjection =
    GeomProjLib::ProjectOnPlane(aTrim, aPlane, gp::DZ(), false);
  ASSERT_FALSE(aProjection.IsNull());
  gp_Pnt aFirst = aTrim->Value(aTrim->FirstParameter());
  gp_Pnt aLast  = aTrim->Value(aTrim->LastParameter());
  aFirst.SetZ(0);
  aLast.SetZ(0);
  EXPECT_LT(aFirst.Distance(aProjection->Value(aProjection->FirstParameter())),
            Precision::Confusion());
  EXPECT_LT(aLast.Distance(aProjection->Value(aProjection->LastParameter())),
            Precision::Confusion());
}

//=================================================================================================

TEST(GeomProjLibTest, LineProjectionRetainsSourceRangeWhenProjectedEndpointsCoincide)
{
  // X cannot represent the projected displacement at this location. The source
  // still has a nonzero parameter interval and a nonzero 3D displacement in Z.
  const occ::handle<Geom_Curve> aLine  = new Geom_Line(gp_Pnt(1.e16, 0, 0), gp_Dir(1, 0, 1));
  const occ::handle<Geom_Curve> aTrim  = new Geom_TrimmedCurve(aLine, 0, 1);
  const occ::handle<Geom_Plane> aPlane = new Geom_Plane(gp_Pln(gp::XOY()));
  occ::handle<Geom_Curve>       aProjected;
  ASSERT_NO_THROW(aProjected = GeomProjLib::ProjectOnPlane(aTrim, aPlane, gp::DZ(), true));
  ASSERT_FALSE(aProjected.IsNull());
  EXPECT_DOUBLE_EQ(aProjected->FirstParameter(), 0);
  EXPECT_DOUBLE_EQ(aProjected->LastParameter(), 1);
  const GeomAdaptor_Curve anAdaptor(aProjected);
  for (double t : {0.0, 0.5, 1.0})
  {
    EXPECT_DOUBLE_EQ(anAdaptor.EvalD0(t).X(), 1.e16);
    EXPECT_DOUBLE_EQ(anAdaptor.EvalD0(t).Y(), 0);
    EXPECT_DOUBLE_EQ(anAdaptor.EvalD0(t).Z(), 0);
  }
}
