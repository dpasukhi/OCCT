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

#include <CSLib_Class2d.hxx>

#include <gtest/gtest.h>

#include <Standard_Real.hxx>
#include <gp_Pnt2d.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_SequenceOfPnt2d.hxx>
#include <Precision.hxx>

namespace
{
// Test point inside a square polygon
TEST(CSLibClass2dTest, PointInsideSquare)
{
  // Create a unit square: (0,0), (1,0), (1,1), (0,1)
  TColgp_Array1OfPnt2d aSquare(1, 4);
  aSquare(1) = gp_Pnt2d(0.0, 0.0);
  aSquare(2) = gp_Pnt2d(1.0, 0.0);
  aSquare(3) = gp_Pnt2d(1.0, 1.0);
  aSquare(4) = gp_Pnt2d(0.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aSquare, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  // Test point clearly inside
  gp_Pnt2d aPointInside(0.5, 0.5);
  Standard_Integer aResult = aClassifier.SiDans(aPointInside);

  EXPECT_EQ(aResult, 1) << "Point (0.5, 0.5) should be inside square";
}

// Test point outside a square polygon
TEST(CSLibClass2dTest, PointOutsideSquare)
{
  TColgp_Array1OfPnt2d aSquare(1, 4);
  aSquare(1) = gp_Pnt2d(0.0, 0.0);
  aSquare(2) = gp_Pnt2d(1.0, 0.0);
  aSquare(3) = gp_Pnt2d(1.0, 1.0);
  aSquare(4) = gp_Pnt2d(0.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aSquare, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  // Test point clearly outside
  gp_Pnt2d aPointOutside(2.0, 2.0);
  Standard_Integer aResult = aClassifier.SiDans(aPointOutside);

  EXPECT_EQ(aResult, -1) << "Point (2.0, 2.0) should be outside square";
}

// Test point on boundary
TEST(CSLibClass2dTest, PointOnBoundary)
{
  TColgp_Array1OfPnt2d aSquare(1, 4);
  aSquare(1) = gp_Pnt2d(0.0, 0.0);
  aSquare(2) = gp_Pnt2d(1.0, 0.0);
  aSquare(3) = gp_Pnt2d(1.0, 1.0);
  aSquare(4) = gp_Pnt2d(0.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aSquare, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  // Test point on edge
  gp_Pnt2d aPointOnEdge(0.5, 0.0);
  Standard_Integer aResult = aClassifier.SiDans(aPointOnEdge);

  // Point on boundary should return 0
  EXPECT_EQ(aResult, 0) << "Point on boundary should return 0";
}

// Test with vertical edge (division by zero protection)
TEST(CSLibClass2dTest, PolygonWithVerticalEdge)
{
  // Create polygon with a vertical edge
  TColgp_Array1OfPnt2d aPolygon(1, 4);
  aPolygon(1) = gp_Pnt2d(0.0, 0.0);
  aPolygon(2) = gp_Pnt2d(1.0, 0.0);
  aPolygon(3) = gp_Pnt2d(1.0, 1.0);  // Vertical edge from (2) to (3)
  aPolygon(4) = gp_Pnt2d(0.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  // This should not crash despite vertical edges (tests division by zero fix)
  CSLib_Class2d aClassifier(aPolygon, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  gp_Pnt2d aPointInside(0.5, 0.5);
  Standard_Integer aResult = aClassifier.SiDans(aPointInside);

  EXPECT_EQ(aResult, 1) << "Should handle vertical edges correctly";
}

// Test with horizontal edge
TEST(CSLibClass2dTest, PolygonWithHorizontalEdge)
{
  // Rectangle with horizontal edges
  TColgp_Array1OfPnt2d aPolygon(1, 4);
  aPolygon(1) = gp_Pnt2d(0.0, 0.0);
  aPolygon(2) = gp_Pnt2d(2.0, 0.0);  // Horizontal edge
  aPolygon(3) = gp_Pnt2d(2.0, 1.0);
  aPolygon(4) = gp_Pnt2d(0.0, 1.0);  // Horizontal edge

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aPolygon, aTolU, aTolV, 0.0, 0.0, 2.0, 1.0);

  gp_Pnt2d aPointInside(1.0, 0.5);
  Standard_Integer aResult = aClassifier.SiDans(aPointInside);

  EXPECT_EQ(aResult, 1) << "Should handle horizontal edges correctly";
}

// Test with triangle
TEST(CSLibClass2dTest, PointInTriangle)
{
  // Create a triangle
  TColgp_Array1OfPnt2d aTriangle(1, 3);
  aTriangle(1) = gp_Pnt2d(0.0, 0.0);
  aTriangle(2) = gp_Pnt2d(2.0, 0.0);
  aTriangle(3) = gp_Pnt2d(1.0, 2.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aTriangle, aTolU, aTolV, 0.0, 0.0, 2.0, 2.0);

  // Point inside triangle
  gp_Pnt2d aPointInside(1.0, 0.5);
  Standard_Integer aResult = aClassifier.SiDans(aPointInside);

  EXPECT_EQ(aResult, 1) << "Point should be inside triangle";

  // Point outside triangle
  gp_Pnt2d aPointOutside(0.0, 1.5);
  aResult = aClassifier.SiDans(aPointOutside);

  EXPECT_EQ(aResult, -1) << "Point should be outside triangle";
}

// Test using sequence constructor
TEST(CSLibClass2dTest, SequenceConstructor)
{
  // Test with TColgp_SequenceOfPnt2d
  TColgp_SequenceOfPnt2d aSequence;
  aSequence.Append(gp_Pnt2d(0.0, 0.0));
  aSequence.Append(gp_Pnt2d(1.0, 0.0));
  aSequence.Append(gp_Pnt2d(1.0, 1.0));
  aSequence.Append(gp_Pnt2d(0.0, 1.0));

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aSequence, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  gp_Pnt2d aPointInside(0.5, 0.5);
  Standard_Integer aResult = aClassifier.SiDans(aPointInside);

  EXPECT_EQ(aResult, 1) << "Sequence constructor should work correctly";
}

// Test SiDans_OnMode
TEST(CSLibClass2dTest, SiDansOnMode)
{
  TColgp_Array1OfPnt2d aSquare(1, 4);
  aSquare(1) = gp_Pnt2d(0.0, 0.0);
  aSquare(2) = gp_Pnt2d(1.0, 0.0);
  aSquare(3) = gp_Pnt2d(1.0, 1.0);
  aSquare(4) = gp_Pnt2d(0.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aSquare, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  gp_Pnt2d aPointInside(0.5, 0.5);
  Standard_Real aTol = 1.0e-6;
  Standard_Integer aResult = aClassifier.SiDans_OnMode(aPointInside, aTol);

  EXPECT_EQ(aResult, 1) << "OnMode should work for interior points";
}

// Test complex polygon
TEST(CSLibClass2dTest, ComplexPolygon)
{
  // Create a more complex polygon (hexagon)
  TColgp_Array1OfPnt2d aHexagon(1, 6);
  aHexagon(1) = gp_Pnt2d(2.0, 0.0);
  aHexagon(2) = gp_Pnt2d(3.0, 1.0);
  aHexagon(3) = gp_Pnt2d(3.0, 2.0);
  aHexagon(4) = gp_Pnt2d(2.0, 3.0);
  aHexagon(5) = gp_Pnt2d(1.0, 2.0);
  aHexagon(6) = gp_Pnt2d(1.0, 1.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  CSLib_Class2d aClassifier(aHexagon, aTolU, aTolV, 1.0, 0.0, 3.0, 3.0);

  // Center of hexagon
  gp_Pnt2d aCenter(2.0, 1.5);
  Standard_Integer aResult = aClassifier.SiDans(aCenter);

  EXPECT_EQ(aResult, 1) << "Center should be inside hexagon";
}

// Test performance with optimized Transform2d
TEST(CSLibClass2dTest, OptimizedTransform2dPerformance)
{
  // Create a polygon with many vertices to test optimized initialization
  const Standard_Integer aNbPoints = 100;
  TColgp_Array1OfPnt2d aPolygon(1, aNbPoints);

  // Create a circular polygon
  const Standard_Real aRadius = 10.0;
  for (Standard_Integer i = 1; i <= aNbPoints; ++i)
  {
    Standard_Real anAngle = 2.0 * M_PI * (i - 1) / aNbPoints;
    aPolygon(i) = gp_Pnt2d(aRadius * std::cos(anAngle), aRadius * std::sin(anAngle));
  }

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  // This tests the optimized Transform2d implementation
  CSLib_Class2d aClassifier(aPolygon, aTolU, aTolV, -aRadius, -aRadius, aRadius, aRadius);

  // Test center point
  gp_Pnt2d aCenter(0.0, 0.0);
  Standard_Integer aResult = aClassifier.SiDans(aCenter);

  EXPECT_EQ(aResult, 1) << "Center should be inside circular polygon";
}

// Test edge case: degenerate polygon (insufficient points)
TEST(CSLibClass2dTest, DegeneratePolygon)
{
  // Polygon with only 2 points (invalid)
  TColgp_Array1OfPnt2d aDegenerate(1, 2);
  aDegenerate(1) = gp_Pnt2d(0.0, 0.0);
  aDegenerate(2) = gp_Pnt2d(1.0, 0.0);

  Standard_Real aTolU = 1.0e-7;
  Standard_Real aTolV = 1.0e-7;

  // Should handle degenerate polygon gracefully
  CSLib_Class2d aClassifier(aDegenerate, aTolU, aTolV, 0.0, 0.0, 1.0, 1.0);

  gp_Pnt2d aPoint(0.5, 0.0);
  Standard_Integer aResult = aClassifier.SiDans(aPoint);

  // With insufficient points, should return 0 (invalid)
  EXPECT_EQ(aResult, 0) << "Degenerate polygon should return 0";
}

} // anonymous namespace
