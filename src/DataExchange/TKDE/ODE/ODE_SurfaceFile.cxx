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

#include <ODE_SurfaceFile.hxx>
#include <ODEHash_SurfaceHasher.hxx>
#include <fstream>

// Cap'n Proto includes will be added when implementing full serialization
// #include <capnp/message.h>
// #include <capnp/serialize.h>
// #include "surfaces.capnp.h"

IMPLEMENT_STANDARD_RTTIEXT(ODE_SurfaceFile, Standard_Transient)

//=================================================================================================


ODE_SurfaceFile::ODE_SurfaceFile()
: myInstanceCount(0)
{
}

//=================================================================================================


ODE_ObjectRef ODE_SurfaceFile::AddSurface(const Handle(Geom_Surface)& theSurface)
{
  if (theSurface.IsNull())
  {
    return ODE_ObjectRef();
  }

  // Compute hash using polymorphic hasher
  const ODEHash_SurfaceHasher aHasher;
  const Standard_Size aHash = aHasher(theSurface);

  // Check if surface already exists
  const auto it = myHashToIndex.find(aHash);
  if (it != myHashToIndex.end())
  {
    // Found duplicate - check for exact equality
    const int anIndex = it->second;
    SurfaceEntry& anEntry = mySurfaces.ChangeValue(anIndex);

    if (aHasher(anEntry.MySurface, theSurface))
    {
      // Exact match - increment sub-index counter
      anEntry.SubIndexCount++;
      myInstanceCount++;

      // Return reference with sub-index
      return ODE_ObjectRef("surfaces", anIndex, anEntry.SubIndexCount - 1);
    }
  }

  // No duplicate found - add new surface
  SurfaceEntry aNewEntry;
  aNewEntry.MySurface = theSurface;
  aNewEntry.SubIndexCount = 1;

  mySurfaces.Append(aNewEntry);
  const int aNewIndex = mySurfaces.Size();
  myHashToIndex[aHash] = aNewIndex;
  myInstanceCount++;

  // Return reference without sub-index (first instance)
  return ODE_ObjectRef("surfaces", aNewIndex);
}

//=================================================================================================

Handle(Geom_Surface) ODE_SurfaceFile::GetSurface(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "surfaces")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > mySurfaces.Size())
  {
    return nullptr;
  }

  const SurfaceEntry& anEntry = mySurfaces.Value(anIndex);

  // In OCCT, Handle sharing is automatic
  // Whether or not there's a sub-index, we return the same handle
  return anEntry.MySurface;
}

//=================================================================================================

int ODE_SurfaceFile::SurfaceCount() const
{
  return mySurfaces.Size();
}

//=================================================================================================

int ODE_SurfaceFile::InstanceCount() const
{
  return myInstanceCount;
}

//=================================================================================================

void ODE_SurfaceFile::Clear()
{
  mySurfaces.Clear();
  myHashToIndex.clear();
  myInstanceCount = 0;
}

//=================================================================================================

ODE_Status ODE_SurfaceFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  // TODO: Implement Cap'n Proto serialization
  // This requires:
  // 1. Create capnp::MallocMessageBuilder
  // 2. Build SurfaceFile message
  // 3. For each surface, determine type and populate appropriate union field
  // 4. Write message to file using capnp::writeMessageToFd

  (void)thePath;  // Suppress unused warning
  return ODE_Status_NotImplemented;
}

//=================================================================================================

ODE_Status ODE_SurfaceFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // TODO: Implement Cap'n Proto deserialization
  // This requires:
  // 1. Read file into capnp::FlatArrayMessageReader
  // 2. Get SurfaceFile root
  // 3. Iterate surfaces and determine type from union
  // 4. Create appropriate Geom_Surface subclass for each
  // 5. Populate mySurfaces sequence

  (void)thePath;  // Suppress unused warning
  return ODE_Status_NotImplemented;
}
