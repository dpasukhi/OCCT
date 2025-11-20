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

#ifndef _ODE_Object_HeaderFile
#define _ODE_Object_HeaderFile

#include <ODE_Status.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>

//! Base class for all ODE objects.
//! Provides error state tracking and status reporting.
class ODE_Object : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ODE_Object, Standard_Transient)

public:

  //! Default constructor. Initializes status to ODE_Status_OK.
  Standard_EXPORT ODE_Object();

  //! Returns the current operation status.
  ODE_Status Status() const { return myStatus; }

  //! Returns the error message if status is not ODE_Status_OK.
  const TCollection_AsciiString& ErrorMessage() const { return myErrorMsg; }

  //! Checks if the last operation was successful.
  bool IsOk() const { return myStatus == ODE_Status_OK; }

  //! Resets the error state to ODE_Status_OK.
  Standard_EXPORT void ClearError();

protected:

  //! Sets the error state with given status and message.
  Standard_EXPORT void setError (const ODE_Status theStatus,
                                  const TCollection_AsciiString& theMessage);

protected:

  ODE_Status myStatus;                //!< Current operation status
  TCollection_AsciiString myErrorMsg; //!< Error message for non-success status

};

#endif // _ODE_Object_HeaderFile
