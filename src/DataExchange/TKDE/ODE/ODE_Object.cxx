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

#include <ODE_Object.hxx>

IMPLEMENT_STANDARD_RTTIEXT(ODE_Object, Standard_Transient)

//=================================================================================================

ODE_Object::ODE_Object()
: myStatus (ODE_Status_OK)
{
}

//=================================================================================================

void ODE_Object::ClearError()
{
  myStatus = ODE_Status_OK;
  myErrorMsg.Clear();
}

//=================================================================================================

void ODE_Object::setError (const ODE_Status theStatus,
                            const TCollection_AsciiString& theMessage)
{
  myStatus = theStatus;
  myErrorMsg = theMessage;
}
