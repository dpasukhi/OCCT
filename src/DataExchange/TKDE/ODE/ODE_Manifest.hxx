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

#ifndef _ODE_Manifest_HeaderFile
#define _ODE_Manifest_HeaderFile

#include <Standard.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>
#include <NCollection_Sequence.hxx>

//! Represents the manifest.json file in an ODE archive.
//! The manifest contains metadata about the archive format, version,
//! and a registry of all Cap'n Proto data files.
class ODE_Manifest : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_Manifest, Standard_Transient)

public:
  //! Entry for a single data file in the manifest
  struct FileEntry
  {
    TCollection_AsciiString UUID;         //!< Unique identifier for the file
    TCollection_AsciiString Name;         //!< Filename (e.g., "topology.capnp")
    TCollection_AsciiString Type;         //!< File type ("topology", "surfaces", "curves3d", etc.)
    TCollection_AsciiString Encoding;     //!< Encoding format ("capnp")
    int ObjectCount;                      //!< Number of objects in the file
    TCollection_AsciiString SHA256;       //!< SHA-256 checksum (optional, empty if not computed)
  };

public:
  //! Constructor - creates empty manifest with default version
  Standard_EXPORT ODE_Manifest();

  //! Sets the format version (default: "1.0")
  Standard_EXPORT void SetVersion(const TCollection_AsciiString& theVersion);

  //! Gets the format version
  Standard_EXPORT const TCollection_AsciiString& Version() const;

  //! Sets the generator string (e.g., "OCCT 7.9.0")
  Standard_EXPORT void SetGenerator(const TCollection_AsciiString& theGenerator);

  //! Gets the generator string
  Standard_EXPORT const TCollection_AsciiString& Generator() const;

  //! Sets the creation timestamp (ISO 8601 format)
  Standard_EXPORT void SetCreated(const TCollection_AsciiString& theTimestamp);

  //! Gets the creation timestamp
  Standard_EXPORT const TCollection_AsciiString& Created() const;

  //! Adds a file entry to the manifest
  Standard_EXPORT void AddFile(const FileEntry& theEntry);

  //! Gets the number of registered files
  Standard_EXPORT int FileCount() const;

  //! Gets file entry by index (1-based)
  Standard_EXPORT const FileEntry& File(int theIndex) const;

  //! Clears all file entries
  Standard_EXPORT void ClearFiles();

  //! Writes manifest to JSON file
  //! @param thePath Path to manifest.json file to write
  //! @return true if successful, false otherwise
  Standard_EXPORT bool WriteToFile(const TCollection_AsciiString& thePath) const;

  //! Reads manifest from JSON file
  //! @param thePath Path to manifest.json file to read
  //! @return true if successful, false otherwise
  Standard_EXPORT bool ReadFromFile(const TCollection_AsciiString& thePath);

private:
  TCollection_AsciiString myVersion;     //!< Format version
  TCollection_AsciiString myGenerator;   //!< Generator software identifier
  TCollection_AsciiString myCreated;     //!< Creation timestamp
  NCollection_Sequence<FileEntry> myFiles;  //!< List of data files
};

#endif // _ODE_Manifest_HeaderFile
