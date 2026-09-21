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

#include <Approx_MCurvesToBSpCurve.hxx>

#include <AppParCurves_MultiCurve.hxx>
#include <AppParCurves_MultiPoint.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Sequence.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

#include <gtest/gtest.h>

namespace
{
AppParCurves_MultiCurve makeSegment(const gp_Pnt&   theP1,
                                    const gp_Pnt&   theP2,
                                    const gp_Pnt&   theP3,
                                    const gp_Pnt2d& theP2d1,
                                    const gp_Pnt2d& theP2d2,
                                    const gp_Pnt2d& theP2d3)
{
  NCollection_Array1<AppParCurves_MultiPoint> aPoints(1, 3);
  const gp_Pnt                                aP3d[] = {theP1, theP2, theP3};
  const gp_Pnt2d                              aP2d[] = {theP2d1, theP2d2, theP2d3};
  for (int anIndex = 1; anIndex <= 3; ++anIndex)
  {
    AppParCurves_MultiPoint aPoint(1, 1);
    aPoint.SetPoint(1, aP3d[anIndex - 1]);
    aPoint.SetPoint2d(2, aP2d[anIndex - 1]);
    aPoints(anIndex) = aPoint;
  }
  return AppParCurves_MultiCurve(aPoints);
}
} // namespace


TEST(Approx_MCurvesToBSpCurveTest, SingleSegmentKeepsBezier)
{
  NCollection_Sequence<AppParCurves_MultiCurve> aSegments;
  aSegments.Append(makeSegment(gp_Pnt(0, 0, 0),
                               gp_Pnt(0.5, 1, 0),
                               gp_Pnt(1, 0, 0),
                               gp_Pnt2d(0, 0),
                               gp_Pnt2d(0.5, 1),
                               gp_Pnt2d(1, 0)));

  Approx_MCurvesToBSpCurve aBuilder;
  aBuilder.Perform(aSegments);

  ASSERT_TRUE(aBuilder.IsDone());
  const AppParCurves_MultiBSpCurve& aResult = aBuilder.Value();
  EXPECT_EQ(aResult.Degree(), 2);
  EXPECT_EQ(aResult.Knots().Length(), 2);
  EXPECT_EQ(aResult.Multiplicities()(1), 3);
  EXPECT_EQ(aResult.Multiplicities()(2), 3);
}

TEST(Approx_MCurvesToBSpCurveTest, PreservesCommonC1Parameterization)
{
  NCollection_Sequence<AppParCurves_MultiCurve> aSegments;
  aSegments.Append(makeSegment(gp_Pnt(0, 0, 0),
                               gp_Pnt(0.5, 0, 0),
                               gp_Pnt(1, 0, 0),
                               gp_Pnt2d(0, 0),
                               gp_Pnt2d(0.5, 0),
                               gp_Pnt2d(1, 0)));
  aSegments.Append(makeSegment(gp_Pnt(1, 0, 0),
                               gp_Pnt(1.5, 0, 0),
                               gp_Pnt(2, 0, 0),
                               gp_Pnt2d(1, 0),
                               gp_Pnt2d(1.5, 0),
                               gp_Pnt2d(2, 0)));

  Approx_MCurvesToBSpCurve aBuilder;
  aBuilder.Perform(aSegments);

  ASSERT_TRUE(aBuilder.IsDone());
  const AppParCurves_MultiBSpCurve& aResult = aBuilder.Value();
  ASSERT_EQ(aResult.Degree(), 2);
  ASSERT_EQ(aResult.Multiplicities().Length(), 3);
  EXPECT_EQ(aResult.Multiplicities()(2), 1);
}

TEST(Approx_MCurvesToBSpCurveTest, FallsBackToC0ForDifferentComponentParameterization)
{
  const AppParCurves_MultiCurve aFirst = makeSegment(gp_Pnt(0, 0, 0),
                                                      gp_Pnt(0.5, 0, 0),
                                                      gp_Pnt(1, 0, 0),
                                                      gp_Pnt2d(0, 0),
                                                      gp_Pnt2d(0.5, 0),
                                                      gp_Pnt2d(1, 0));
  const AppParCurves_MultiCurve aSecond = makeSegment(gp_Pnt(1, 0, 0),
                                                       gp_Pnt(1.5, 0, 0),
                                                       gp_Pnt(2, 0, 0),
                                                       gp_Pnt2d(1, 0),
                                                       gp_Pnt2d(2, 0),
                                                       gp_Pnt2d(3, 0));
  NCollection_Sequence<AppParCurves_MultiCurve> aSegments;
  aSegments.Append(aFirst);
  aSegments.Append(aSecond);

  Approx_MCurvesToBSpCurve aBuilder;
  aBuilder.Perform(aSegments);

  ASSERT_TRUE(aBuilder.IsDone());
  const AppParCurves_MultiBSpCurve& aResult = aBuilder.Value();
  ASSERT_EQ(aResult.Degree(), 2);
  ASSERT_EQ(aResult.Multiplicities().Length(), 3);
  EXPECT_EQ(aResult.Multiplicities()(2), 2);

  gp_Pnt   aExpected3d;
  gp_Pnt   anActual3d;
  gp_Pnt2d aExpected2d;
  gp_Pnt2d anActual2d;
  aFirst.Value(1, 0.5, aExpected3d);
  aResult.Value(1, 0.25, anActual3d);
  aFirst.Value(2, 0.5, aExpected2d);
  aResult.Value(2, 0.25, anActual2d);
  EXPECT_LE(aExpected3d.Distance(anActual3d), Precision::Confusion());
  EXPECT_LE(aExpected2d.Distance(anActual2d), Precision::PConfusion());

  aSecond.Value(1, 0.5, aExpected3d);
  aResult.Value(1, 0.75, anActual3d);
  aSecond.Value(2, 0.5, aExpected2d);
  aResult.Value(2, 0.75, anActual2d);
  EXPECT_LE(aExpected3d.Distance(anActual3d), Precision::Confusion());
  EXPECT_LE(aExpected2d.Distance(anActual2d), Precision::PConfusion());
}

TEST(Approx_MCurvesToBSpCurveTest, RejectsDisconnectedComponent)
{
  NCollection_Sequence<AppParCurves_MultiCurve> aSegments;
  aSegments.Append(makeSegment(gp_Pnt(0, 0, 0),
                               gp_Pnt(0.5, 0, 0),
                               gp_Pnt(1, 0, 0),
                               gp_Pnt2d(0, 0),
                               gp_Pnt2d(0.5, 0),
                               gp_Pnt2d(1, 0)));
  aSegments.Append(makeSegment(gp_Pnt(1, 0, 0),
                               gp_Pnt(1.5, 0, 0),
                               gp_Pnt(2, 0, 0),
                               gp_Pnt2d(2, 0),
                               gp_Pnt2d(2.5, 0),
                               gp_Pnt2d(3, 0)));

  Approx_MCurvesToBSpCurve aBuilder;
  aBuilder.Perform(aSegments);
  EXPECT_FALSE(aBuilder.IsDone());
}

TEST(Approx_MCurvesToBSpCurveTest, EmptySequenceIsNotDone)
{
  Approx_MCurvesToBSpCurve                      aBuilder;
  NCollection_Sequence<AppParCurves_MultiCurve> aSegments;
  aBuilder.Perform(aSegments);
  EXPECT_FALSE(aBuilder.IsDone());
}
