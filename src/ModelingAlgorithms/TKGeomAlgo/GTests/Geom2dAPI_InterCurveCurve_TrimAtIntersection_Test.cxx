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

#include <Geom2dAPI_InterCurveCurve.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt2d.hxx>

#include <gtest/gtest.h>

namespace
{
occ::handle<Geom2d_TrimmedCurve> makeSegment(const gp_Pnt2d& theStart, const gp_Pnt2d& theEnd)
{
  const gp_Vec2d aDir(theStart, theEnd);
  const double   aLength = aDir.Magnitude();
  occ::handle<Geom2d_Line> aLine = new Geom2d_Line(theStart, gp_Dir2d(aDir));
  return new Geom2d_TrimmedCurve(aLine, 0.0, aLength);
}
} // namespace

TEST(Geom2dAPI_InterCurveCurve_Test, TrimAtIntersection_LineSegmentEnd)
{
  const gp_Pnt2d aLongStart(0.0, 0.0);
  const gp_Pnt2d aLongEnd(10.0, 0.0);

  const gp_Pnt2d aCutStart(5.0, 0.0);
  const gp_Pnt2d aCutEnd(5.1, 0.1);

  const occ::handle<Geom2d_TrimmedCurve> aLong = makeSegment(aLongStart, aLongEnd);
  const occ::handle<Geom2d_TrimmedCurve> aCut  = makeSegment(aCutStart, aCutEnd);

  Geom2dAPI_InterCurveCurve anIntersector;
  anIntersector.Init(aLong, aCut, Precision::Confusion());

  ASSERT_EQ(anIntersector.NbPoints(), 1);

  const gp_Pnt2d aHit = anIntersector.Point(1);
  EXPECT_NEAR(aHit.X(), 5.0, Precision::Confusion());
  EXPECT_NEAR(aHit.Y(), 0.0, Precision::Confusion());

  Geom2dAPI_ProjectPointOnCurve aProjector(aHit, aLong);
  ASSERT_GT(aProjector.NbPoints(), 0);
  EXPECT_NEAR(aProjector.LowerDistanceParameter(), 5.0, Precision::Confusion());
}
