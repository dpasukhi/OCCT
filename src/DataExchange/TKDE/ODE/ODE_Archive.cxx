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

#include <ODE_Archive.hxx>
#include <filesystem>

IMPLEMENT_STANDARD_RTTIEXT(ODE_Archive, Standard_Transient)

//=================================================================================================

ODE_Archive::ODE_Archive()
: myIsOpen(false)
{
}

//=================================================================================================

ODE_Status ODE_Archive::Create(const TCollection_AsciiString& thePath)
{
  if (myIsOpen)
  {
    Close();
  }

  myPath = thePath;

  // Create directory if it doesn't exist
  if (!CreateDirectory())
  {
    return ODE_Status_WriteError;
  }

  // Create new manifest
  myManifest = new ODE_Manifest();
  myIsOpen = true;

  return ODE_Status_OK;
}

//=================================================================================================

ODE_Status ODE_Archive::Open(const TCollection_AsciiString& thePath)
{
  if (myIsOpen)
  {
    Close();
  }

  myPath = thePath;

  // Check if directory exists
  if (!DirectoryExists())
  {
    return ODE_Status_FileNotFound;
  }

  // Read manifest
  myIsOpen = true;
  const ODE_Status aStatus = ReadManifest();
  if (aStatus != ODE_Status_OK)
  {
    myIsOpen = false;
    return aStatus;
  }

  return ODE_Status_OK;
}

//=================================================================================================

void ODE_Archive::Close()
{
  myIsOpen = false;
  myManifest.Nullify();
  myPath.Clear();
}

//=================================================================================================

bool ODE_Archive::IsOpen() const
{
  return myIsOpen;
}

//=================================================================================================

const TCollection_AsciiString& ODE_Archive::Path() const
{
  return myPath;
}

//=================================================================================================

const Handle(ODE_Manifest)& ODE_Archive::Manifest() const
{
  return myManifest;
}

//=================================================================================================

void ODE_Archive::SetManifest(const Handle(ODE_Manifest)& theManifest)
{
  myManifest = theManifest;
}

//=================================================================================================

ODE_Status ODE_Archive::WriteManifest()
{
  if (!myIsOpen || myManifest.IsNull())
  {
    return ODE_Status_InvalidArchive;
  }

  const TCollection_AsciiString aManifestPath = GetFilePath("manifest.json");
  if (!myManifest->WriteToFile(aManifestPath))
  {
    return ODE_Status_WriteError;
  }

  return ODE_Status_OK;
}

//=================================================================================================

ODE_Status ODE_Archive::ReadManifest()
{
  if (!myIsOpen)
  {
    return ODE_Status_InvalidArchive;
  }

  const TCollection_AsciiString aManifestPath = GetFilePath("manifest.json");

  // Check if manifest exists
  if (!FileExists("manifest.json"))
  {
    return ODE_Status_FileNotFound;
  }

  // Create new manifest and read
  myManifest = new ODE_Manifest();
  if (!myManifest->ReadFromFile(aManifestPath))
  {
    myManifest.Nullify();
    return ODE_Status_ParseError;
  }

  // Check format and version
  if (myManifest->Version() != "1.0")
  {
    return ODE_Status_UnsupportedVersion;
  }

  return ODE_Status_OK;
}

//=================================================================================================

TCollection_AsciiString ODE_Archive::GetFilePath(
  const TCollection_AsciiString& theFileName) const
{
  TCollection_AsciiString aPath = myPath;
#ifdef _WIN32
  if (!aPath.IsEmpty() && aPath.Value(aPath.Length()) != '\\' && aPath.Value(aPath.Length()) != '/')
  {
    aPath += "\\";
  }
#else
  if (!aPath.IsEmpty() && aPath.Value(aPath.Length()) != '/')
  {
    aPath += "/";
  }
#endif
  aPath += theFileName;
  return aPath;
}

//=================================================================================================

bool ODE_Archive::FileExists(const TCollection_AsciiString& theFileName) const
{
  const TCollection_AsciiString aFilePath = GetFilePath(theFileName);
  return std::filesystem::exists(aFilePath.ToCString()) &&
         std::filesystem::is_regular_file(aFilePath.ToCString());
}

//=================================================================================================

bool ODE_Archive::CreateDirectory()
{
  if (DirectoryExists())
  {
    return true;
  }

  std::error_code anError;
  return std::filesystem::create_directories(myPath.ToCString(), anError);
}

//=================================================================================================

bool ODE_Archive::DirectoryExists() const
{
  if (myPath.IsEmpty())
  {
    return false;
  }

  return std::filesystem::exists(myPath.ToCString()) &&
         std::filesystem::is_directory(myPath.ToCString());
}
