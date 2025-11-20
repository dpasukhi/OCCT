// Created on: 2025-01-20
// Created by: ODE Format Implementation
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

#include <ODE_ObjectRef.hxx>

//=================================================================================================

ODE_ObjectRef::ODE_ObjectRef()
: myIndex (-1)
{
}

//=================================================================================================

ODE_ObjectRef::ODE_ObjectRef (const TCollection_AsciiString& theFileType,
                                int theIndex)
: myFileType (theFileType),
  myIndex (theIndex)
{
}

//=================================================================================================

ODE_ObjectRef::ODE_ObjectRef (const TCollection_AsciiString& theFileType,
                                int theIndex,
                                int theSubIndex)
: myFileType (theFileType),
  myIndex (theIndex),
  mySubIndex (theSubIndex)
{
}

//=================================================================================================

TCollection_AsciiString ODE_ObjectRef::ToString() const
{
  if (!IsValid())
  {
    return TCollection_AsciiString();
  }

  TCollection_AsciiString aResult = myFileType;
  aResult += "#";
  aResult += TCollection_AsciiString (myIndex);

  if (mySubIndex.has_value())
  {
    aResult += ".";
    aResult += TCollection_AsciiString (mySubIndex.value());
  }

  return aResult;
}

//=================================================================================================

bool ODE_ObjectRef::FromString (const TCollection_AsciiString& theString)
{
  // Reset to invalid state
  myFileType.Clear();
  myIndex = -1;
  mySubIndex = std::nullopt;

  if (theString.IsEmpty())
  {
    return false;
  }

  // Find '#' separator
  Standard_Integer aHashPos = theString.Location ("#", 1, theString.Length());
  if (aHashPos == 0)
  {
    return false;
  }

  // Extract file type
  myFileType = theString.SubString (1, aHashPos - 1);
  if (myFileType.IsEmpty())
  {
    return false;
  }

  // Extract remaining part (index or index.subIndex)
  TCollection_AsciiString aRemainder = theString.SubString (aHashPos + 1, theString.Length());

  // Check for '.' separator (indicates sub-index presence)
  Standard_Integer aDotPos = aRemainder.Location (".", 1, aRemainder.Length());

  if (aDotPos == 0)
  {
    // No sub-index, just parse index
    if (!aRemainder.IsIntegerValue())
    {
      myFileType.Clear();
      return false;
    }
    myIndex = aRemainder.IntegerValue();
  }
  else
  {
    // Parse both index and sub-index
    TCollection_AsciiString anIndexStr = aRemainder.SubString (1, aDotPos - 1);
    TCollection_AsciiString aSubIndexStr = aRemainder.SubString (aDotPos + 1, aRemainder.Length());

    if (!anIndexStr.IsIntegerValue() || !aSubIndexStr.IsIntegerValue())
    {
      myFileType.Clear();
      return false;
    }

    myIndex = anIndexStr.IntegerValue();
    mySubIndex = aSubIndexStr.IntegerValue();
  }

  return true;
}
