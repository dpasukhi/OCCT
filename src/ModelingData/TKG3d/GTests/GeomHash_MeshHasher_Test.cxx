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

#include <GeomHash_Polygon2DHasher.hxx>
#include <GeomHash_Polygon3DHasher.hxx>
#include <GeomHash_PolygonOnTriHasher.hxx>
#include <GeomHash_TriangulationHasher.hxx>
#include <NCollection_Array1.hxx>
#include <Poly_Polygon2D.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

namespace
{
occ::handle<Poly_Polygon2D> makePolygon2D(const double theDeflection)
{
  occ::handle<Poly_Polygon2D> aPoly = new Poly_Polygon2D(2);
  aPoly->ChangeNodes().SetValue(1, gp_Pnt2d(0.0, 0.0));
  aPoly->ChangeNodes().SetValue(2, gp_Pnt2d(1.0, 0.0));
  aPoly->Deflection(theDeflection);
  return aPoly;
}

occ::handle<Poly_Polygon3D> makePolygon3D(const double theDeflection)
{
  occ::handle<Poly_Polygon3D> aPoly = new Poly_Polygon3D(2, false);
  aPoly->ChangeNodes().SetValue(1, gp_Pnt(0.0, 0.0, 0.0));
  aPoly->ChangeNodes().SetValue(2, gp_Pnt(1.0, 0.0, 0.0));
  aPoly->Deflection(theDeflection);
  return aPoly;
}

occ::handle<Poly_PolygonOnTriangulation> makePolygonOnTriangulation(const double theDeflection)
{
  occ::handle<Poly_PolygonOnTriangulation> aPoly =
    new Poly_PolygonOnTriangulation(2, false);
  aPoly->SetNode(1, 1);
  aPoly->SetNode(2, 2);
  aPoly->Deflection(theDeflection);
  return aPoly;
}

occ::handle<Poly_Triangulation> makeTriangulation(const double theDeflection)
{
  occ::handle<Poly_Triangulation> aTri = new Poly_Triangulation(3, 1, false);
  aTri->SetNode(1, gp_Pnt(0.0, 0.0, 0.0));
  aTri->SetNode(2, gp_Pnt(1.0, 0.0, 0.0));
  aTri->SetNode(3, gp_Pnt(0.0, 1.0, 0.0));
  aTri->SetTriangle(1, Poly_Triangle(1, 2, 3));
  aTri->Deflection(theDeflection);
  return aTri;
}
} // namespace

TEST(GeomHash_MeshHasherTest, Polygon2D_CloseDeflectionKeepsEqualHash)
{
  const GeomHash_Polygon2DHasher aHasher(0.1, 0.01);
  const occ::handle<Poly_Polygon2D> aPoly1 = makePolygon2D(0.004);
  const occ::handle<Poly_Polygon2D> aPoly2 = makePolygon2D(0.015);

  ASSERT_TRUE(aHasher(aPoly1, aPoly2));
  EXPECT_EQ(aHasher(aPoly1), aHasher(aPoly2));
}

TEST(GeomHash_MeshHasherTest, Polygon3D_CloseDeflectionKeepsEqualHash)
{
  const GeomHash_Polygon3DHasher aHasher(0.1, 0.01);
  const occ::handle<Poly_Polygon3D> aPoly1 = makePolygon3D(0.004);
  const occ::handle<Poly_Polygon3D> aPoly2 = makePolygon3D(0.015);

  ASSERT_TRUE(aHasher(aPoly1, aPoly2));
  EXPECT_EQ(aHasher(aPoly1), aHasher(aPoly2));
}

TEST(GeomHash_MeshHasherTest, PolygonOnTriangulation_CloseDeflectionKeepsEqualHash)
{
  const GeomHash_PolygonOnTriHasher aHasher(0.1, 0.01);
  const PolygonOnTriHashKey         aKey1{makePolygonOnTriangulation(0.004), 7};
  const PolygonOnTriHashKey         aKey2{makePolygonOnTriangulation(0.015), 7};

  ASSERT_TRUE(aHasher(aKey1, aKey2));
  EXPECT_EQ(aHasher(aKey1), aHasher(aKey2));
}

TEST(GeomHash_MeshHasherTest, Triangulation_CloseDeflectionKeepsEqualHash)
{
  const GeomHash_TriangulationHasher aHasher(0.1, 0.01);
  const occ::handle<Poly_Triangulation> aTri1 = makeTriangulation(0.004);
  const occ::handle<Poly_Triangulation> aTri2 = makeTriangulation(0.015);

  ASSERT_TRUE(aHasher(aTri1, aTri2));
  EXPECT_EQ(aHasher(aTri1), aHasher(aTri2));
}
