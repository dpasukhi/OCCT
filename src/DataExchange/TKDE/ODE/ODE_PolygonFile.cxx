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

#include <ODE_PolygonFile.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ODE_PolygonFile, Standard_Transient)

//=================================================================================================

ODE_PolygonFile::ODE_PolygonFile()
{
}

//=================================================================================================

ODE_ObjectRef ODE_PolygonFile::AddPolygon3D(const Handle(Poly_Polygon3D)& thePolygon)
{
  if (thePolygon.IsNull())
  {
    return ODE_ObjectRef();
  }

  myPolygons3D.Append(thePolygon);
  const int anIndex = myPolygons3D.Size();

  return ODE_ObjectRef("polygons3d", anIndex);
}

//=================================================================================================

Handle(Poly_Polygon3D) ODE_PolygonFile::GetPolygon3D(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "polygons3d")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myPolygons3D.Size())
  {
    return nullptr;
  }

  return myPolygons3D.Value(anIndex);
}

//=================================================================================================

ODE_ObjectRef ODE_PolygonFile::AddPolygon2D(const Handle(Poly_Polygon2D)& thePolygon)
{
  if (thePolygon.IsNull())
  {
    return ODE_ObjectRef();
  }

  myPolygons2D.Append(thePolygon);
  const int anIndex = myPolygons2D.Size();

  return ODE_ObjectRef("polygons2d", anIndex);
}

//=================================================================================================

Handle(Poly_Polygon2D) ODE_PolygonFile::GetPolygon2D(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "polygons2d")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myPolygons2D.Size())
  {
    return nullptr;
  }

  return myPolygons2D.Value(anIndex);
}

//=================================================================================================

int ODE_PolygonFile::Polygon3DCount() const
{
  return myPolygons3D.Size();
}

//=================================================================================================

int ODE_PolygonFile::Polygon2DCount() const
{
  return myPolygons2D.Size();
}

//=================================================================================================

void ODE_PolygonFile::Clear()
{
  myPolygons3D.Clear();
  myPolygons2D.Clear();
}

//=================================================================================================

ODE_Status ODE_PolygonFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  // TODO: Implement Cap'n Proto serialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}

//=================================================================================================

ODE_Status ODE_PolygonFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // TODO: Implement Cap'n Proto deserialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}
