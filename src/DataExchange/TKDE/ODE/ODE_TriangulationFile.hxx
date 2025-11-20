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

#ifndef _ODE_TriangulationFile_HeaderFile
#define _ODE_TriangulationFile_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>
#include <Poly_Triangulation.hxx>
#include <ODE_Status.hxx>
#include <ODE_ObjectRef.hxx>

//! Manages serialization and deserialization of Poly_Triangulation objects
//! to Cap'n Proto format. No deduplication for triangulations (each is unique).
class ODE_TriangulationFile : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_TriangulationFile, Standard_Transient)

public:
  //! Constructor
  Standard_EXPORT ODE_TriangulationFile();

  //! Adds a triangulation to the file.
  //! @param theTriangulation Triangulation to add
  //! @return Object reference to the triangulation
  Standard_EXPORT ODE_ObjectRef AddTriangulation(const Handle(Poly_Triangulation)& theTriangulation);

  //! Gets a triangulation by its object reference.
  //! @param theRef Object reference
  //! @return Triangulation handle (may be null if reference is invalid)
  Standard_EXPORT Handle(Poly_Triangulation) GetTriangulation(const ODE_ObjectRef& theRef) const;

  //! Gets total number of triangulations
  Standard_EXPORT int TriangulationCount() const;

  //! Clears all triangulations and resets the file
  Standard_EXPORT void Clear();

  //! Writes triangulations to a Cap'n Proto binary file
  //! @param thePath Path to the output file
  //! @return Status code
  Standard_EXPORT ODE_Status WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads triangulations from a Cap'n Proto binary file
  //! @param thePath Path to the input file
  //! @return Status code
  Standard_EXPORT ODE_Status ReadFromFile(const TCollection_AsciiString& thePath);

private:
  //! Sequence of triangulations
  NCollection_Sequence<Handle(Poly_Triangulation)> myTriangulations;
};

#endif // _ODE_TriangulationFile_HeaderFile
