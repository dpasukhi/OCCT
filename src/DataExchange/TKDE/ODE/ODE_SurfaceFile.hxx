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

#ifndef _ODE_SurfaceFile_HeaderFile
#define _ODE_SurfaceFile_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>
#include <NCollection_DataMap.hxx>
#include <Geom_Surface.hxx>
#include <ODE_Status.hxx>
#include <ODE_ObjectRef.hxx>
#include <unordered_map>

// Forward declaration of Cap'n Proto types
namespace capnp {
  class MallocMessageBuilder;
  class FlatArrayMessageReader;
}

//! Manages serialization and deserialization of Geom_Surface objects
//! to Cap'n Proto format with deduplication support.
class ODE_SurfaceFile : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_SurfaceFile, Standard_Transient)

public:
  //! Constructor
  Standard_EXPORT ODE_SurfaceFile();

  //! Adds a surface to the file with deduplication.
  //! Returns an object reference that can be used to retrieve the surface later.
  //! If the surface is geometrically identical to an existing surface,
  //! returns a reference to the existing surface.
  //! @param theSurface Surface to add
  //! @return Object reference to the surface
  Standard_EXPORT ODE_ObjectRef AddSurface(const Handle(Geom_Surface)& theSurface);

  //! Gets a surface by its object reference.
  //! @param theRef Object reference
  //! @return Surface handle (may be null if reference is invalid)
  Standard_EXPORT Handle(Geom_Surface) GetSurface(const ODE_ObjectRef& theRef) const;

  //! Gets total number of unique surfaces
  Standard_EXPORT int SurfaceCount() const;

  //! Gets total number of surface instances (including duplicates)
  Standard_EXPORT int InstanceCount() const;

  //! Clears all surfaces and resets the file
  Standard_EXPORT void Clear();

  //! Writes surfaces to a Cap'n Proto binary file
  //! @param thePath Path to the output file
  //! @return Status code
  Standard_EXPORT ODE_Status WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads surfaces from a Cap'n Proto binary file
  //! @param thePath Path to the input file
  //! @return Status code
  Standard_EXPORT ODE_Status ReadFromFile(const TCollection_AsciiString& thePath);

private:
  //! Internal storage for surface and its handle instances
  struct SurfaceEntry
  {
    Handle(Geom_Surface) MySurface;    //!< The actual surface object
    int SubIndexCount;                  //!< Number of handle instances (for statistics)
  };

  //! Sequence of unique surfaces
  NCollection_Sequence<SurfaceEntry> mySurfaces;

  //! Deduplication map: surface hash -> index in mySurfaces
  std::unordered_map<Standard_Size, int> myHashToIndex;

  //! Instance counter for statistics
  int myInstanceCount;
};

#endif // _ODE_SurfaceFile_HeaderFile
