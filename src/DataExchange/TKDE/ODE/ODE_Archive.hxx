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

#ifndef _ODE_Archive_HeaderFile
#define _ODE_Archive_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <ODE_Manifest.hxx>
#include <ODE_Status.hxx>

//! Represents an ODE archive directory containing manifest and data files.
//! The archive provides methods for creating, opening, and managing the
//! directory structure and accessing data files.
class ODE_Archive : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_Archive, Standard_Transient)

public:
  //! Constructor - creates uninitialized archive
  Standard_EXPORT ODE_Archive();

  //! Creates a new archive directory at the specified path.
  //! @param thePath Directory path for the archive
  //! @return Status code indicating success or failure
  Standard_EXPORT ODE_Status Create(const TCollection_AsciiString& thePath);

  //! Opens an existing archive directory.
  //! @param thePath Directory path of the archive
  //! @return Status code indicating success or failure
  Standard_EXPORT ODE_Status Open(const TCollection_AsciiString& thePath);

  //! Closes the archive
  Standard_EXPORT void Close();

  //! Returns true if the archive is open
  Standard_EXPORT bool IsOpen() const;

  //! Gets the archive directory path
  Standard_EXPORT const TCollection_AsciiString& Path() const;

  //! Gets the manifest object (may be null if not loaded)
  Standard_EXPORT const Handle(ODE_Manifest)& Manifest() const;

  //! Sets the manifest object
  Standard_EXPORT void SetManifest(const Handle(ODE_Manifest)& theManifest);

  //! Writes the manifest to the archive directory
  //! @return Status code indicating success or failure
  Standard_EXPORT ODE_Status WriteManifest();

  //! Reads the manifest from the archive directory
  //! @return Status code indicating success or failure
  Standard_EXPORT ODE_Status ReadManifest();

  //! Gets the full path to a file in the archive
  //! @param theFileName Name of the file (e.g., "topology.capnp")
  //! @return Full path to the file
  Standard_EXPORT TCollection_AsciiString GetFilePath(
    const TCollection_AsciiString& theFileName) const;

  //! Checks if a file exists in the archive
  //! @param theFileName Name of the file
  //! @return true if file exists
  Standard_EXPORT bool FileExists(const TCollection_AsciiString& theFileName) const;

  //! Creates the archive directory if it doesn't exist
  //! @return true if directory was created or already exists
  Standard_EXPORT bool CreateDirectory();

  //! Checks if the archive directory exists
  //! @return true if directory exists
  Standard_EXPORT bool DirectoryExists() const;

private:
  TCollection_AsciiString myPath;        //!< Archive directory path
  Handle(ODE_Manifest) myManifest;       //!< Manifest object
  bool myIsOpen;                         //!< Open state flag
};

#endif // _ODE_Archive_HeaderFile
