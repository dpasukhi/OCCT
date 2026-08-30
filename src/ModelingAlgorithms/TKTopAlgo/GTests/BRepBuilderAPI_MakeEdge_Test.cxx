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

#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeEdge2d.hxx>
#include <BRepGProp.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Pln.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <StdFail_NotDone.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <gtest/gtest.h>

TEST(BRepBuilderAPI_MakeEdgeTest, LinearEdge_TwoPoints)
{
  gp_Pnt aP1(0.0, 0.0, 0.0);
  gp_Pnt aP2(10.0, 0.0, 0.0);

  BRepBuilderAPI_MakeEdge aMakeEdge(aP1, aP2);
  ASSERT_TRUE(aMakeEdge.IsDone()) << "Linear edge creation failed";

  TopoDS_Edge anEdge = aMakeEdge.Edge();
  ASSERT_FALSE(anEdge.IsNull()) << "Resulting edge is null";

  // Verify edge length
  GProp_GProps aProps;
  BRepGProp::LinearProperties(anEdge, aProps);
  EXPECT_NEAR(aProps.Mass(), 10.0, Precision::Confusion()) << "Edge length should be 10";
}

TEST(BRepBuilderAPI_MakeEdgeTest, CircularEdge_Full)
{
  gp_Ax2                   anAxis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(anAxis, 5.0);

  BRepBuilderAPI_MakeEdge aMakeEdge(aCircle);
  ASSERT_TRUE(aMakeEdge.IsDone()) << "Full circular edge creation failed";

  TopoDS_Edge anEdge = aMakeEdge.Edge();
  ASSERT_FALSE(anEdge.IsNull());

  // Full circle length = 2*PI*R
  GProp_GProps aProps;
  BRepGProp::LinearProperties(anEdge, aProps);
  EXPECT_NEAR(aProps.Mass(), 2.0 * M_PI * 5.0, Precision::Confusion());
}

TEST(BRepBuilderAPI_MakeEdgeTest, CircularEdge_Trimmed)
{
  gp_Ax2                   anAxis(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(0.0, 0.0, 1.0));
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(anAxis, 5.0);

  // Half circle: 0 to PI
  BRepBuilderAPI_MakeEdge aMakeEdge(aCircle, 0.0, M_PI);
  ASSERT_TRUE(aMakeEdge.IsDone()) << "Trimmed circular edge creation failed";

  TopoDS_Edge anEdge = aMakeEdge.Edge();
  ASSERT_FALSE(anEdge.IsNull());

  // Half circle length = PI*R
  GProp_GProps aProps;
  BRepGProp::LinearProperties(anEdge, aProps);
  EXPECT_NEAR(aProps.Mass(), M_PI * 5.0, Precision::Confusion());
}

TEST(BRepBuilderAPI_MakeEdgeTest, EdgeFromLine_WithBounds)
{
  occ::handle<Geom_Line> aLine = new Geom_Line(gp_Pnt(0.0, 0.0, 0.0), gp_Dir(1.0, 0.0, 0.0));

  BRepBuilderAPI_MakeEdge aMakeEdge(aLine, 0.0, 7.0);
  ASSERT_TRUE(aMakeEdge.IsDone()) << "Edge from line with bounds failed";

  TopoDS_Edge anEdge = aMakeEdge.Edge();
  ASSERT_FALSE(anEdge.IsNull());

  GProp_GProps aProps;
  BRepGProp::LinearProperties(anEdge, aProps);
  EXPECT_NEAR(aProps.Mass(), 7.0, Precision::Confusion());
}

TEST(BRepBuilderAPI_MakeEdgeTest, VertexExtraction)
{
  gp_Pnt aP1(0.0, 0.0, 0.0);
  gp_Pnt aP2(10.0, 0.0, 0.0);

  BRepBuilderAPI_MakeEdge aMakeEdge(aP1, aP2);
  ASSERT_TRUE(aMakeEdge.IsDone());

  TopoDS_Edge   anEdge = aMakeEdge.Edge();
  TopoDS_Vertex aV1, aV2;
  TopExp::Vertices(anEdge, aV1, aV2);

  ASSERT_FALSE(aV1.IsNull()) << "First vertex is null";
  ASSERT_FALSE(aV2.IsNull()) << "Last vertex is null";

  gp_Pnt aFirstPnt = BRep_Tool::Pnt(aV1);
  gp_Pnt aLastPnt  = BRep_Tool::Pnt(aV2);

  EXPECT_TRUE(aFirstPnt.IsEqual(aP1, Precision::Confusion()));
  EXPECT_TRUE(aLastPnt.IsEqual(aP2, Precision::Confusion()));
}

TEST(BRepBuilderAPI_MakeEdgeTest, ToleranceCheck)
{
  gp_Pnt aP1(0.0, 0.0, 0.0);
  gp_Pnt aP2(10.0, 0.0, 0.0);

  BRepBuilderAPI_MakeEdge aMakeEdge(aP1, aP2);
  ASSERT_TRUE(aMakeEdge.IsDone());

  TopoDS_Edge anEdge = aMakeEdge.Edge();
  double      aTol   = BRep_Tool::Tolerance(anEdge);
  EXPECT_GT(aTol, 0.0) << "Edge tolerance should be positive";
  EXPECT_LE(aTol, Precision::Confusion()) << "Edge tolerance should not exceed confusion";
}

TEST(BRepBuilderAPI_MakeEdgeTest, Edge2dExplicitPlane)
{
  const gp_Pln aPlane(gp_Ax3(gp_Pnt(10.0, 20.0, 30.0),
                             gp_Dir(0.0, 1.0, 0.0),
                             gp_Dir(1.0, 0.0, 0.0)));
  BRepBuilderAPI_MakeEdge2d aMakeEdge(aPlane);
  aMakeEdge.Init(gp_Pnt2d(1.0, 2.0), gp_Pnt2d(4.0, 6.0));
  ASSERT_TRUE(aMakeEdge.IsDone());

  TopoDS_Vertex aV1, aV2;
  TopExp::Vertices(aMakeEdge.Edge(), aV1, aV2);
  ASSERT_FALSE(aV1.IsNull());
  ASSERT_FALSE(aV2.IsNull());

  EXPECT_TRUE(BRep_Tool::Pnt(aV1).IsEqual(gp_Pnt(11.0, 20.0, 28.0), Precision::Confusion()));
  EXPECT_TRUE(BRep_Tool::Pnt(aV2).IsEqual(gp_Pnt(14.0, 20.0, 24.0), Precision::Confusion()));
}

TEST(BRepBuilderAPI_MakeEdgeTest, Edge2dDefaultPlanesAreIndependent)
{
  BRepBuilderAPI_MakeEdge2d aFirstMaker(gp_Pnt2d(0.0, 0.0), gp_Pnt2d(1.0, 0.0));
  BRepBuilderAPI_MakeEdge2d aSecondMaker(gp_Pnt2d(0.0, 0.0), gp_Pnt2d(0.0, 1.0));
  ASSERT_TRUE(aFirstMaker.IsDone());
  ASSERT_TRUE(aSecondMaker.IsDone());

  occ::handle<Geom2d_Curve> aFirstCurve;
  occ::handle<Geom2d_Curve> aSecondCurve;
  occ::handle<Geom_Surface> aFirstSurface;
  occ::handle<Geom_Surface> aSecondSurface;
  TopLoc_Location           aFirstLocation;
  TopLoc_Location           aSecondLocation;
  double                    aFirstParameter = 0.0;
  double                    aLastParameter  = 0.0;
  BRep_Tool::CurveOnSurface(aFirstMaker.Edge(),
                            aFirstCurve,
                            aFirstSurface,
                            aFirstLocation,
                            aFirstParameter,
                            aLastParameter);
  BRep_Tool::CurveOnSurface(aSecondMaker.Edge(),
                            aSecondCurve,
                            aSecondSurface,
                            aSecondLocation,
                            aFirstParameter,
                            aLastParameter);

  ASSERT_FALSE(aFirstSurface.IsNull());
  ASSERT_FALSE(aSecondSurface.IsNull());
  EXPECT_NE(aFirstSurface.get(), aSecondSurface.get());
}

TEST(BRepBuilderAPI_MakeEdgeTest, Edge2dInitFailureClearsPreviousResult)
{
  BRepBuilderAPI_MakeEdge2d aMakeEdge{gp_Pln(gp::XOY())};
  aMakeEdge.Init(gp_Pnt2d(0.0, 0.0), gp_Pnt2d(1.0, 0.0));
  ASSERT_TRUE(aMakeEdge.IsDone());

  aMakeEdge.Init(gp_Pnt2d(2.0, 3.0), gp_Pnt2d(2.0, 3.0));
  EXPECT_FALSE(aMakeEdge.IsDone());
  EXPECT_EQ(aMakeEdge.Error(), BRepBuilderAPI_LineThroughIdenticPoints);
  EXPECT_THROW(aMakeEdge.Shape(), StdFail_NotDone);
}
