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

#include <ODE_TriangulationFile.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ODE_TriangulationFile, Standard_Transient)

//=================================================================================================

ODE_TriangulationFile::ODE_TriangulationFile()
{
}

//=================================================================================================

ODE_ObjectRef ODE_TriangulationFile::AddTriangulation(const Handle(Poly_Triangulation)& theTriangulation)
{
  if (theTriangulation.IsNull())
  {
    return ODE_ObjectRef();
  }

  myTriangulations.Append(theTriangulation);
  const int anIndex = myTriangulations.Size();

  return ODE_ObjectRef("triangulation", anIndex);
}

//=================================================================================================

Handle(Poly_Triangulation) ODE_TriangulationFile::GetTriangulation(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "triangulation")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myTriangulations.Size())
  {
    return nullptr;
  }

  return myTriangulations.Value(anIndex);
}

//=================================================================================================

int ODE_TriangulationFile::TriangulationCount() const
{
  return myTriangulations.Size();
}

//=================================================================================================

void ODE_TriangulationFile::Clear()
{
  myTriangulations.Clear();
}

//=================================================================================================

ODE_Status ODE_TriangulationFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  // TODO: Implement Cap'n Proto serialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}

//=================================================================================================

ODE_Status ODE_TriangulationFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // TODO: Implement Cap'n Proto deserialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}
