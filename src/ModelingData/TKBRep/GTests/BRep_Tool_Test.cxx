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

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRep_TVertex.hxx>
#include <BRep_TEdge.hxx>
#include <BRep_TFace.hxx>
#include <limits>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Poly_Polygon2D.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <NCollection_Array1.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <gtest/gtest.h>

TEST(BRep_Tool_Test, Pnt_FromVertex)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_VERTEX);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Vertex& aVertex = TopoDS::Vertex(anExp.Current());
  gp_Pnt               aPnt    = BRep_Tool::Pnt(aVertex);

  // The point coordinates should be within the box bounds [0,10] x [0,20] x [0,30]
  EXPECT_GE(aPnt.X(), -Precision::Confusion());
  EXPECT_LE(aPnt.X(), 10.0 + Precision::Confusion());
  EXPECT_GE(aPnt.Y(), -Precision::Confusion());
  EXPECT_LE(aPnt.Y(), 20.0 + Precision::Confusion());
  EXPECT_GE(aPnt.Z(), -Precision::Confusion());
  EXPECT_LE(aPnt.Z(), 30.0 + Precision::Confusion());
}

TEST(BRep_Tool_Test, Tolerance_Vertex)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_VERTEX);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Vertex& aVertex = TopoDS::Vertex(anExp.Current());
  double               aTol    = BRep_Tool::Tolerance(aVertex);
  EXPECT_GE(aTol, 0.0);
}

TEST(BRep_Tool_Test, Tolerance_Edge)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_EDGE);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Edge& anEdge = TopoDS::Edge(anExp.Current());
  double             aTol   = BRep_Tool::Tolerance(anEdge);
  EXPECT_GE(aTol, 0.0);
}

TEST(BRep_Tool_Test, Tolerance_Face)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_FACE);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Face& aFace = TopoDS::Face(anExp.Current());
  double             aTol  = BRep_Tool::Tolerance(aFace);
  EXPECT_GE(aTol, 0.0);
}

TEST(BRep_Tool_Test, Curve_FromEdge)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_EDGE);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Edge&      anEdge = TopoDS::Edge(anExp.Current());
  double                  aFirst = 0.0;
  double                  aLast  = 0.0;
  occ::handle<Geom_Curve> aCurve = BRep_Tool::Curve(anEdge, aFirst, aLast);

  EXPECT_FALSE(aCurve.IsNull()) << "Curve from a box edge should not be null";
  EXPECT_LT(aFirst, aLast) << "First parameter should be less than last parameter";
}

TEST(BRep_Tool_Test, Surface_FromFace)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_FACE);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Face&        aFace    = TopoDS::Face(anExp.Current());
  occ::handle<Geom_Surface> aSurface = BRep_Tool::Surface(aFace);

  EXPECT_FALSE(aSurface.IsNull()) << "Surface from a box face should not be null";
}

TEST(BRep_Tool_Test, IsClosed_CircleEdge)
{
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(gp_Ax2(gp_Pnt(0.0, 0.0, 0.0), gp::DZ()), 5.0);
  BRepBuilderAPI_MakeEdge  anEdgeMaker(aCircle);
  ASSERT_TRUE(anEdgeMaker.IsDone());

  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();
  EXPECT_TRUE(BRep_Tool::IsClosed(anEdge)) << "A full circle edge should be closed";
}

TEST(BRep_Tool_Test, IsClosed_LineEdge)
{
  BRepBuilderAPI_MakeEdge anEdgeMaker(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(10.0, 0.0, 0.0));
  ASSERT_TRUE(anEdgeMaker.IsDone());

  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();
  EXPECT_FALSE(BRep_Tool::IsClosed(anEdge)) << "A line segment edge should not be closed";
}

TEST(BRep_Tool_Test, Degenerated)
{
  BRepPrimAPI_MakeBox aBoxMaker(10.0, 20.0, 30.0);
  const TopoDS_Shape& aBox = aBoxMaker.Shape();
  ASSERT_TRUE(aBoxMaker.IsDone());

  TopExp_Explorer anExp(aBox, TopAbs_EDGE);
  ASSERT_TRUE(anExp.More());

  const TopoDS_Edge& anEdge = TopoDS::Edge(anExp.Current());
  EXPECT_FALSE(BRep_Tool::Degenerated(anEdge)) << "Box edges should not be degenerated";
}

TEST(BRep_Tool_Test, CurveOnPlane_RejectsEdgeRangeOutsideBoundedCurve)
{
  NCollection_Array1<gp_Pnt> aPoles(1, 3);
  aPoles.SetValue(1, gp_Pnt(0.0, 0.0, 0.0));
  aPoles.SetValue(2, gp_Pnt(0.5, 0.25, 0.0));
  aPoles.SetValue(3, gp_Pnt(1.0, 0.0, 0.0));
  occ::handle<Geom_BezierCurve> aCurve = new Geom_BezierCurve(aPoles);

  BRepBuilderAPI_MakeEdge anEdgeMaker(aCurve);
  ASSERT_TRUE(anEdgeMaker.IsDone());
  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();

  constexpr double THE_EDGE_FIRST = -0.000002673149646;
  constexpr double THE_EDGE_LAST  = 0.999997326850354;
  BRep_Builder().Range(anEdge, THE_EDGE_FIRST, THE_EDGE_LAST, true);

  occ::handle<Geom_Plane>   aPlane = new Geom_Plane(gp::XOY());
  occ::handle<Geom2d_Curve> aPCurve;
  double                    aFirst   = 0.0;
  double                    aLast    = 0.0;
  bool                      isStored = true;
  EXPECT_NO_THROW(
    aPCurve =
      BRep_Tool::CurveOnSurface(anEdge, aPlane, TopLoc_Location(), aFirst, aLast, &isStored));

  EXPECT_TRUE(aPCurve.IsNull());
  EXPECT_FALSE(isStored);
  EXPECT_DOUBLE_EQ(aFirst, THE_EDGE_FIRST);
  EXPECT_DOUBLE_EQ(aLast, THE_EDGE_LAST);
}

TEST(BRep_Tool_Test, CurveOnPlane_ProjectsShiftedPeriodicRange)
{
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(gp_Ax2(gp::Origin(), gp::DZ()), 1.0);
  BRepBuilderAPI_MakeEdge  anEdgeMaker(aCircle);
  ASSERT_TRUE(anEdgeMaker.IsDone());
  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();

  const double aRangeFirst = aCircle->Period();
  const double aRangeLast  = 2.0 * aCircle->Period();
  BRep_Builder().Range(anEdge, aRangeFirst, aRangeLast, true);

  occ::handle<Geom_Plane>   aPlane = new Geom_Plane(gp::XOY());
  occ::handle<Geom2d_Curve> aPCurve;
  double                    aFirst = 0.0;
  double                    aLast  = 0.0;
  EXPECT_NO_THROW(aPCurve =
                    BRep_Tool::CurveOnPlane(anEdge, aPlane, TopLoc_Location(), aFirst, aLast));

  EXPECT_FALSE(aPCurve.IsNull());
  EXPECT_DOUBLE_EQ(aFirst, aRangeFirst);
  EXPECT_DOUBLE_EQ(aLast, aRangeLast);
}

TEST(BRep_Tool_Test, CurveOnPlane_ProjectsPeriodicEdgeBuiltFromEqualParameters)
{
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(gp_Ax2(gp::Origin(), gp::DZ()), 1.0);
  BRepBuilderAPI_MakeEdge  anEdgeMaker(aCircle, 0.0, 0.0);
  ASSERT_TRUE(anEdgeMaker.IsDone());
  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();

  double aEdgeFirst = 0.0;
  double aEdgeLast  = 0.0;
  BRep_Tool::Range(anEdge, aEdgeFirst, aEdgeLast);
  EXPECT_NEAR(aEdgeLast - aEdgeFirst, aCircle->Period(), Precision::PConfusion());

  occ::handle<Geom_Plane>   aPlane = new Geom_Plane(gp::XOY());
  occ::handle<Geom2d_Curve> aPCurve;
  double                    aFirst = 0.0;
  double                    aLast  = 0.0;
  EXPECT_NO_THROW(aPCurve =
                    BRep_Tool::CurveOnPlane(anEdge, aPlane, TopLoc_Location(), aFirst, aLast));

  EXPECT_FALSE(aPCurve.IsNull());
  EXPECT_DOUBLE_EQ(aFirst, aEdgeFirst);
  EXPECT_DOUBLE_EQ(aLast, aEdgeLast);
}

TEST(BRep_Tool_Test, CurveOnPlane_RejectsEqualPeriodicRange)
{
  occ::handle<Geom_Circle> aCircle = new Geom_Circle(gp_Ax2(gp::Origin(), gp::DZ()), 1.0);
  BRepBuilderAPI_MakeEdge  anEdgeMaker(aCircle);
  ASSERT_TRUE(anEdgeMaker.IsDone());
  const TopoDS_Edge& anEdge = anEdgeMaker.Edge();
  BRep_Builder().Range(anEdge, 0.0, 0.0, true);

  occ::handle<Geom_Plane>   aPlane = new Geom_Plane(gp::XOY());
  occ::handle<Geom2d_Curve> aPCurve;
  double                    aFirst = 1.0;
  double                    aLast  = 1.0;
  EXPECT_NO_THROW(aPCurve =
                    BRep_Tool::CurveOnPlane(anEdge, aPlane, TopLoc_Location(), aFirst, aLast));

  EXPECT_TRUE(aPCurve.IsNull());
  EXPECT_DOUBLE_EQ(aFirst, 0.0);
  EXPECT_DOUBLE_EQ(aLast, 0.0);
}

TEST(BRep_Tool_Test, ClosedSurfacePolygonsPreserveRelativeLocation)
{
  const occ::handle<Geom_Surface>   aSurface  = new Geom_CylindricalSurface(gp_Ax3(), 3.0);
  const occ::handle<Poly_Polygon2D> aForward  = new Poly_Polygon2D(2);
  const occ::handle<Poly_Polygon2D> aReversed = new Poly_Polygon2D(2);
  aForward->ChangeNodes()(1)                  = gp_Pnt2d(0.0, 0.0);
  aForward->ChangeNodes()(2)                  = gp_Pnt2d(0.0, 5.0);
  aReversed->ChangeNodes()(1)                 = gp_Pnt2d(2.0 * M_PI, 0.0);
  aReversed->ChangeNodes()(2)                 = gp_Pnt2d(2.0 * M_PI, 5.0);
  for (const bool isSurfaceLocated : {false, true})
  {
    for (const bool isEdgeLocated : {false, true})
    {
      SCOPED_TRACE(isSurfaceLocated);
      SCOPED_TRACE(isEdgeLocated);
      gp_Trsf aSurfaceTransform, anEdgeTransform;
      if (isSurfaceLocated)
      {
        aSurfaceTransform.SetTranslation(gp_Vec(10.0, 20.0, 30.0));
      }
      if (isEdgeLocated)
      {
        anEdgeTransform.SetTranslation(gp_Vec(1.0, 2.0, 3.0));
      }
      const TopLoc_Location aSurfaceLocation(aSurfaceTransform);
      BRep_Builder          aBuilder;
      TopoDS_Edge           anEdge;
      aBuilder.MakeEdge(anEdge);
      anEdge.Location(TopLoc_Location(anEdgeTransform));
      aBuilder.UpdateEdge(anEdge, aForward, aReversed, aSurface, aSurfaceLocation);
      EXPECT_EQ(BRep_Tool::PolygonOnSurface(anEdge, aSurface, aSurfaceLocation), aForward);
      anEdge.Orientation(TopAbs_REVERSED);
      EXPECT_EQ(BRep_Tool::PolygonOnSurface(anEdge, aSurface, aSurfaceLocation), aReversed);

      aBuilder.UpdateEdge(anEdge, aReversed, aForward, aSurface, aSurfaceLocation);
      EXPECT_EQ(BRep_Tool::PolygonOnSurface(anEdge, aSurface, aSurfaceLocation), aForward);
      anEdge.Orientation(TopAbs_FORWARD);
      EXPECT_EQ(BRep_Tool::PolygonOnSurface(anEdge, aSurface, aSurfaceLocation), aReversed);
      aBuilder.UpdateEdge(anEdge, nullptr, nullptr, aSurface, aSurfaceLocation);
      EXPECT_TRUE(BRep_Tool::PolygonOnSurface(anEdge, aSurface, aSurfaceLocation).IsNull());
    }
  }
}

TEST(BRep_Tool_Test, UpdateLocatedVertexPreservesWorldPoint)
{
  gp_Trsf aTranslation;
  aTranslation.SetTranslation(gp_Vec(3.0, -4.0, 5.0));
  gp_Trsf aRotation;
  aRotation.SetRotation(gp::OZ(), 0.37);
  const TopLoc_Location aLocation    = TopLoc_Location(aTranslation) * TopLoc_Location(aRotation);
  const TopLoc_Location aLocations[] = {TopLoc_Location(), aLocation, aLocation.Powered(-2)};
  BRep_Builder          aBuilder;
  const gp_Pnt          aWorldPoint(7.0, 11.0, -13.0);
  for (const TopLoc_Location& aLoc : aLocations)
  {
    TopoDS_Vertex aVertex;
    aBuilder.MakeVertex(aVertex, gp_Pnt(), Precision::Confusion());
    aVertex.Location(aLoc);
    aBuilder.UpdateVertex(aVertex, aWorldPoint, 1.e-5);
    EXPECT_LT(BRep_Tool::Pnt(aVertex).Distance(aWorldPoint), 1.e-12);
    EXPECT_TRUE(aVertex.Location().IsEqual(aLoc));
    EXPECT_DOUBLE_EQ(BRep_Tool::Tolerance(aVertex), 1.e-5);
  }
}

TEST(BRep_Tool_Test, ToleranceFloorPreservesBoundaryAndNaNBehavior)
{
  BRep_Builder  aBuilder;
  TopoDS_Vertex aVertex;
  TopoDS_Edge   anEdge;
  TopoDS_Face   aFace;
  aBuilder.MakeVertex(aVertex);
  aBuilder.MakeEdge(anEdge);
  aBuilder.MakeFace(aFace);
  auto*        aTVertex    = static_cast<BRep_TVertex*>(aVertex.TShape().get());
  auto*        aTEdge      = static_cast<BRep_TEdge*>(anEdge.TShape().get());
  auto*        aTFace      = static_cast<BRep_TFace*>(aFace.TShape().get());
  const double aMin        = Precision::Confusion();
  const double anInfinity  = std::numeric_limits<double>::infinity();
  const double aCases[][2] = {{-1.0, aMin},
                              {0.0, aMin},
                              {aMin * 0.5, aMin},
                              {aMin, aMin},
                              {aMin * 2.0, aMin * 2.0},
                              {anInfinity, anInfinity},
                              {-anInfinity, aMin},
                              {std::numeric_limits<double>::quiet_NaN(), aMin}};
  for (const auto& aCase : aCases)
  {
    aTVertex->Tolerance(aCase[0]);
    aTEdge->Tolerance(aCase[0]);
    aTFace->Tolerance(aCase[0]);
    EXPECT_DOUBLE_EQ(BRep_Tool::Tolerance(aVertex), aCase[1]);
    EXPECT_DOUBLE_EQ(BRep_Tool::Tolerance(anEdge), aCase[1]);
    EXPECT_DOUBLE_EQ(BRep_Tool::Tolerance(aFace), aCase[1]);
  }
}
