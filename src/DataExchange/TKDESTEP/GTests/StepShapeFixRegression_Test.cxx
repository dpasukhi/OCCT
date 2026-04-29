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

// Investigative regression coverage for the stack-overflow / edge-multiplication
// bug originally fixed in commit aaa82fc4 and the shape-healing regression that
// followed it. Reads a hard-coded local STEP file; the test passes if import
// terminates without unbounded edge growth.

#include <IFSelect_ReturnStatus.hxx>
#include <NCollection_IndexedMap.hxx>
#include <STEPControl_Reader.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopoDS_Shape.hxx>

#include <gtest/gtest.h>

namespace
{
int CountSubShapes(const TopoDS_Shape& theShape, const TopAbs_ShapeEnum theType)
{
  NCollection_IndexedMap<TopoDS_Shape, TopTools_ShapeMapHasher> aMap;
  TopExp::MapShapes(theShape, theType, aMap);
  return aMap.Extent();
}
} // namespace

TEST(StepShapeFixRegression, Shape_step_ImportTerminates)
{
  STEPControl_Reader          aReader;
  const IFSelect_ReturnStatus aStat = aReader.ReadFile("/Users/dpasukhi/work/OCCT/Shape.step");
  ASSERT_EQ(aStat, IFSelect_RetDone);
  ASSERT_GT(aReader.TransferRoots(), 0);

  const TopoDS_Shape aShape = aReader.OneShape();
  ASSERT_FALSE(aShape.IsNull());

  std::cout << "[after import] Solids=" << CountSubShapes(aShape, TopAbs_SOLID)
            << " Shells=" << CountSubShapes(aShape, TopAbs_SHELL)
            << " Faces=" << CountSubShapes(aShape, TopAbs_FACE)
            << " Wires=" << CountSubShapes(aShape, TopAbs_WIRE) << std::endl;
}
