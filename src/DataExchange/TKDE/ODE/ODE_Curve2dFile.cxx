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

#include <ODE_Curve2dFile.hxx>
#include <ODEHash_Curve2dHasher.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ODE_Curve2dFile, Standard_Transient)

//=================================================================================================

ODE_Curve2dFile::ODE_Curve2dFile()
: myInstanceCount(0)
{
}

//=================================================================================================

ODE_ObjectRef ODE_Curve2dFile::AddCurve(const Handle(Geom2d_Curve)& theCurve)
{
  if (theCurve.IsNull())
  {
    return ODE_ObjectRef();
  }

  // Compute hash using polymorphic hasher
  const ODEHash_Curve2dHasher aHasher;
  const Standard_Size aHash = aHasher(theCurve);

  // Check if curve already exists
  const auto it = myHashToIndex.find(aHash);
  if (it != myHashToIndex.end())
  {
    // Found duplicate - check for exact equality
    const int anIndex = it->second;
    CurveEntry& anEntry = myCurves.ChangeValue(anIndex);

    if (aHasher(anEntry.MyCurve, theCurve))
    {
      // Exact match - increment sub-index counter
      anEntry.SubIndexCount++;
      myInstanceCount++;

      // Return reference with sub-index
      return ODE_ObjectRef("curves2d", anIndex, anEntry.SubIndexCount - 1);
    }
  }

  // No duplicate found - add new curve
  CurveEntry aNewEntry;
  aNewEntry.MyCurve = theCurve;
  aNewEntry.SubIndexCount = 1;

  myCurves.Append(aNewEntry);
  const int aNewIndex = myCurves.Size();
  myHashToIndex[aHash] = aNewIndex;
  myInstanceCount++;

  // Return reference without sub-index (first instance)
  return ODE_ObjectRef("curves2d", aNewIndex);
}

//=================================================================================================

Handle(Geom2d_Curve) ODE_Curve2dFile::GetCurve(const ODE_ObjectRef& theRef) const
{
  if (theRef.FileType() != "curves2d")
  {
    return nullptr;
  }

  const int anIndex = theRef.Index();
  if (anIndex < 1 || anIndex > myCurves.Size())
  {
    return nullptr;
  }

  const CurveEntry& anEntry = myCurves.Value(anIndex);

  // In OCCT, Handle sharing is automatic
  return anEntry.MyCurve;
}

//=================================================================================================

int ODE_Curve2dFile::CurveCount() const
{
  return myCurves.Size();
}

//=================================================================================================

int ODE_Curve2dFile::InstanceCount() const
{
  return myInstanceCount;
}

//=================================================================================================

void ODE_Curve2dFile::Clear()
{
  myCurves.Clear();
  myHashToIndex.clear();
  myInstanceCount = 0;
}

//=================================================================================================

ODE_Status ODE_Curve2dFile::WriteToFile(const TCollection_AsciiString& thePath) const
{
  // TODO: Implement Cap'n Proto serialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}

//=================================================================================================

ODE_Status ODE_Curve2dFile::ReadFromFile(const TCollection_AsciiString& thePath)
{
  // TODO: Implement Cap'n Proto deserialization
  (void)thePath;
  return ODE_Status_NotImplemented;
}
