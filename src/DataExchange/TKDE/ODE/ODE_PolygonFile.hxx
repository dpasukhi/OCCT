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

#ifndef _ODE_PolygonFile_HeaderFile
#define _ODE_PolygonFile_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>
#include <Poly_Polygon3D.hxx>
#include <Poly_Polygon2D.hxx>
#include <ODE_Status.hxx>
#include <ODE_ObjectRef.hxx>

//! Manages serialization and deserialization of Poly_Polygon3D and Poly_Polygon2D objects
//! to Cap'n Proto format. No deduplication for polygons (each is unique).
class ODE_PolygonFile : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_PolygonFile, Standard_Transient)

public:
  //! Constructor
  Standard_EXPORT ODE_PolygonFile();

  //! Adds a 3D polygon to the file.
  //! @param thePolygon Polygon to add
  //! @return Object reference to the polygon
  Standard_EXPORT ODE_ObjectRef AddPolygon3D(const Handle(Poly_Polygon3D)& thePolygon);

  //! Gets a 3D polygon by its object reference.
  //! @param theRef Object reference
  //! @return Polygon handle (may be null if reference is invalid)
  Standard_EXPORT Handle(Poly_Polygon3D) GetPolygon3D(const ODE_ObjectRef& theRef) const;

  //! Adds a 2D polygon to the file.
  //! @param thePolygon Polygon to add
  //! @return Object reference to the polygon
  Standard_EXPORT ODE_ObjectRef AddPolygon2D(const Handle(Poly_Polygon2D)& thePolygon);

  //! Gets a 2D polygon by its object reference.
  //! @param theRef Object reference
  //! @return Polygon handle (may be null if reference is invalid)
  Standard_EXPORT Handle(Poly_Polygon2D) GetPolygon2D(const ODE_ObjectRef& theRef) const;

  //! Gets total number of 3D polygons
  Standard_EXPORT int Polygon3DCount() const;

  //! Gets total number of 2D polygons
  Standard_EXPORT int Polygon2DCount() const;

  //! Clears all polygons and resets the file
  Standard_EXPORT void Clear();

  //! Writes polygons to a Cap'n Proto binary file
  //! @param thePath Path to the output file
  //! @return Status code
  Standard_EXPORT ODE_Status WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads polygons from a Cap'n Proto binary file
  //! @param thePath Path to the input file
  //! @return Status code
  Standard_EXPORT ODE_Status ReadFromFile(const TCollection_AsciiString& thePath);

private:
  //! Sequence of 3D polygons
  NCollection_Sequence<Handle(Poly_Polygon3D)> myPolygons3D;

  //! Sequence of 2D polygons
  NCollection_Sequence<Handle(Poly_Polygon2D)> myPolygons2D;
};

#endif // _ODE_PolygonFile_HeaderFile
