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

#include <ODE_TopologyFile.hxx>
#include <ODE_SurfaceFile.hxx>
#include <ODE_Curve3dFile.hxx>
#include <ODE_Curve2dFile.hxx>
#include <ODE_TriangulationFile.hxx>
#include <ODE_PolygonFile.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ODE_TopologyFile, Standard_Transient)

//=================================================================================================

ODE_TopologyFile::ODE_TopologyFile()
{
}

//=================================================================================================

void ODE_TopologyFile::SetSurfaceFile(const Handle(ODE_SurfaceFile)& theFile)
{
  mySurfaceFile = theFile;
}

//=================================================================================================

void ODE_TopologyFile::SetCurve3dFile(const Handle(ODE_Curve3dFile)& theFile)
{
  myCurve3dFile = theFile;
}

//=================================================================================================

void ODE_TopologyFile::SetCurve2dFile(const Handle(ODE_Curve2dFile)& theFile)
{
  myCurve2dFile = theFile;
}

//=================================================================================================

void ODE_TopologyFile::SetTriangulationFile(const Handle(ODE_TriangulationFile)& theFile)
{
  myTriangulationFile = theFile;
}

//=================================================================================================

void ODE_TopologyFile::SetPolygonFile(const Handle(ODE_PolygonFile)& theFile)
{
  myPolygonFile = theFile;
}

//=================================================================================================

ODE_ObjectRef ODE_TopologyFile::AddShape(const TopoDS_Shape& theShape)
{
  if (theShape.IsNull())
  {
    return ODE_ObjectRef();
  }

  // TODO: Implement recursive shape traversal
  // For now, just store the shape directly
  myShapes.Append(theShape);
  const int anIndex = myShapes.Size();

  return ODE_ObjectRef("topology", anIndex);
}

//=================================================================================================

TopoDS_Shape ODE_TopologyFile::GetShape(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "topology")
  {
    return TopoDS_Shape();
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myShapes.Size())
  {
    return TopoDS_Shape();
  }

  return myShapes.Value(anIndex);
}

//=================================================================================================

int ODE_TopologyFile::ShapeCount() const
{
  return myShapes.Size();
}

//=================================================================================================

void ODE_TopologyFile::Clear()
{
  myShapes.Clear();
}

//=================================================================================================

ODE_Status ODE_TopologyFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  // TODO: Implement Cap'n Proto serialization
  // This requires:
  // 1. Traverse shape hierarchy recursively
  // 2. For each sub-shape, serialize type, orientation, location
  // 3. For edges: add 3D curve, PCurves, tolerance
  // 4. For faces: add surface, wire boundaries, tolerance
  // 5. For vertices: add point, tolerance
  // 6. Write triangulation and polygon references if present

  (void)thePath;
  return ODE_Status_NotImplemented;
}

//=================================================================================================

ODE_Status ODE_TopologyFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // TODO: Implement Cap'n Proto deserialization
  // This requires:
  // 1. Read shape hierarchy from file
  // 2. Reconstruct TopoDS shapes from serialized data
  // 3. Resolve geometry references (surfaces, curves, PCurves)
  // 4. Apply orientations and locations
  // 5. Build compound structure with correct hierarchy

  (void)thePath;
  return ODE_Status_NotImplemented;
}
