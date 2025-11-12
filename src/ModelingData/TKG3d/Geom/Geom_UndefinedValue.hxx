// Created on: 1993-03-09
// Created by: JCV
// Copyright (c) 1993-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

#ifndef _Geom_UndefinedValue_HeaderFile
#define _Geom_UndefinedValue_HeaderFile

#include <Standard_Type.hxx>
#include <Standard_DefineException.hxx>
#include <Standard_SStream.hxx>
#include <Standard_DomainError.hxx>

//! @deprecated This exception class is deprecated and should not be used in new code.
//! Use boolean return values or output parameters to signal errors instead.
class Standard_DEPRECATED("Use boolean return values or output parameters instead of throwing exceptions")
  Geom_UndefinedValue;
DEFINE_STANDARD_HANDLE(Geom_UndefinedValue, Standard_DomainError)

#if !defined No_Exception && !defined No_Geom_UndefinedValue
  //! @deprecated This macro is deprecated. Use boolean return values instead.
  #define Geom_UndefinedValue_Raise_if(CONDITION, MESSAGE)                                         \
    if (CONDITION)                                                                                 \
      throw Geom_UndefinedValue(MESSAGE);
#else
  #define Geom_UndefinedValue_Raise_if(CONDITION, MESSAGE)
#endif

DEFINE_STANDARD_EXCEPTION(Geom_UndefinedValue, Standard_DomainError)

#endif // _Geom_UndefinedValue_HeaderFile
