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

#ifndef _ODE_ObjectRef_HeaderFile
#define _ODE_ObjectRef_HeaderFile

#include <Standard_Type.hxx>
#include <TCollection_AsciiString.hxx>
#include <optional>

//! Represents a reference to an object in another ODE file.
//! Format: fileType#index or fileType#index.subIndex
//!
//! - fileType: Type of file containing the object ("surfaces", "curves3d", etc.)
//! - index: Object index within that file (0-based)
//! - subIndex: Optional sharing group ID (std::nullopt = no sub-index, indicating deep copy)
//!
//! When subIndex has a value, it indicates handle sharing.
//! When subIndex is std::nullopt, it indicates the object should be deep-copied.
class ODE_ObjectRef
{
public:

  //! Default constructor. Creates an invalid reference.
  Standard_EXPORT ODE_ObjectRef();

  //! Constructor with file type and index (no sub-index = deep copy).
  Standard_EXPORT ODE_ObjectRef (const TCollection_AsciiString& theFileType,
                                  int theIndex);

  //! Constructor with file type, index, and sub-index (handle sharing).
  Standard_EXPORT ODE_ObjectRef (const TCollection_AsciiString& theFileType,
                                  int theIndex,
                                  int theSubIndex);

  //! Returns the file type identifier.
  const TCollection_AsciiString& FileType() const { return myFileType; }

  //! Returns the object index.
  int Index() const { return myIndex; }

  //! Returns the sub-index (std::nullopt if not present).
  const std::optional<int>& SubIndex() const { return mySubIndex; }

  //! Checks if this reference has a sub-index (indicates handle sharing).
  bool HasSubIndex() const { return mySubIndex.has_value(); }

  //! Checks if this reference is valid (non-empty file type and valid index).
  bool IsValid() const { return !myFileType.IsEmpty() && myIndex >= 0; }

  //! Converts this reference to a string representation.
  //! Format: "fileType#index" or "fileType#index.subIndex"
  Standard_EXPORT TCollection_AsciiString ToString() const;

  //! Parses a reference string and sets this object's values.
  //! Returns true if parsing succeeded.
  Standard_EXPORT bool FromString (const TCollection_AsciiString& theString);

  //! Equality comparison operator.
  bool operator== (const ODE_ObjectRef& theOther) const
  {
    return myFileType.IsEqual (theOther.myFileType) &&
           myIndex == theOther.myIndex &&
           mySubIndex == theOther.mySubIndex;
  }

  //! Inequality comparison operator.
  bool operator!= (const ODE_ObjectRef& theOther) const
  {
    return !(*this == theOther);
  }

private:

  TCollection_AsciiString myFileType; //!< Type of file ("surfaces", "curves3d", etc.)
  int myIndex;                        //!< Object index in file
  std::optional<int> mySubIndex;      //!< Sub-index for handle sharing (nullopt = none)

};

#endif // _ODE_ObjectRef_HeaderFile
