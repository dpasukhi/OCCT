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

#ifndef _ODE_TopologyFile_HeaderFile
#define _ODE_TopologyFile_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>
#include <NCollection_DataMap.hxx>
#include <TopoDS_Shape.hxx>
#include <ODE_Status.hxx>
#include <ODE_ObjectRef.hxx>

// Forward declarations for geometry file classes
class ODE_SurfaceFile;
class ODE_Curve3dFile;
class ODE_Curve2dFile;
class ODE_TriangulationFile;
class ODE_PolygonFile;

//! Manages serialization and deserialization of TopoDS_Shape objects
//! to Cap'n Proto format with cross-references to geometry files.
class ODE_TopologyFile : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_TopologyFile, Standard_Transient)

public:
  //! Constructor
  Standard_EXPORT ODE_TopologyFile();

  //! Sets the surface file for resolving surface references
  Standard_EXPORT void SetSurfaceFile(const Handle(ODE_SurfaceFile)& theFile);

  //! Sets the 3D curve file for resolving curve references
  Standard_EXPORT void SetCurve3dFile(const Handle(ODE_Curve3dFile)& theFile);

  //! Sets the 2D curve file for resolving curve references
  Standard_EXPORT void SetCurve2dFile(const Handle(ODE_Curve2dFile)& theFile);

  //! Sets the triangulation file for resolving triangulation references
  Standard_EXPORT void SetTriangulationFile(const Handle(ODE_TriangulationFile)& theFile);

  //! Sets the polygon file for resolving polygon references
  Standard_EXPORT void SetPolygonFile(const Handle(ODE_PolygonFile)& theFile);

  //! Adds a shape to the file.
  //! This will recursively traverse the shape hierarchy and add all sub-shapes.
  //! @param theShape Shape to add
  //! @return Object reference to the root shape
  Standard_EXPORT ODE_ObjectRef AddShape(const TopoDS_Shape& theShape);

  //! Gets a shape by its object reference.
  //! This will reconstruct the shape from its serialized form.
  //! @param theRef Object reference
  //! @return Shape (may be null if reference is invalid)
  Standard_EXPORT TopoDS_Shape GetShape(const ODE_ObjectRef& theRef) const;

  //! Gets total number of shapes
  Standard_EXPORT int ShapeCount() const;

  //! Clears all shapes and resets the file
  Standard_EXPORT void Clear();

  //! Writes shapes to a Cap'n Proto binary file
  //! @param thePath Path to the output file
  //! @return Status code
  Standard_EXPORT ODE_Status WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads shapes from a Cap'n Proto binary file
  //! @param thePath Path to the input file
  //! @return Status code
  Standard_EXPORT ODE_Status ReadFromFile(const TCollection_AsciiString& thePath);

private:
  //! Sequence of root shapes
  NCollection_Sequence<TopoDS_Shape> myShapes;

  //! Reference to surface file
  Handle(ODE_SurfaceFile) mySurfaceFile;

  //! Reference to 3D curve file
  Handle(ODE_Curve3dFile) myCurve3dFile;

  //! Reference to 2D curve file
  Handle(ODE_Curve2dFile) myCurve2dFile;

  //! Reference to triangulation file
  Handle(ODE_TriangulationFile) myTriangulationFile;

  //! Reference to polygon file
  Handle(ODE_PolygonFile) myPolygonFile;
};

#endif // _ODE_TopologyFile_HeaderFile
