// Created on: 2025-01-20
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

#ifndef _Draw_CommandInterface_HeaderFile
#define _Draw_CommandInterface_HeaderFile

#include <Standard.hxx>

//! Unified interface for command interpreters (TCL or Python)
//! This header provides compatibility macros for dual compilation support

#ifdef USE_PYTHON_INTERPRETER
  #include <Draw_PythonInterpretor.hxx>
  #define DRAW_INTERPRETOR Draw_PythonInterpretor
#else
  #include <Draw_Interpretor.hxx>
  #define DRAW_INTERPRETOR Draw_Interpretor
#endif

//! Unified command function signature for both TCL and Python
typedef Standard_Integer (*Draw_CommandFunction)(DRAW_INTERPRETOR& theDI,
                                               Standard_Integer  theArgNb,
                                               const char**      theArgVec);

//! Unified command registration macro
//! Works with both TCL and Python interpreters
#define DRAW_ADD_COMMAND(interp, name, help, file, func, group) \
  (interp).Add(name, help, file, func, group)

//! Simplified command registration macro without filename
#define DRAW_ADD_SIMPLE_COMMAND(interp, name, help, func, group) \
  (interp).Add(name, help, func, group)

//! Command registration with default group
#define DRAW_ADD_DEFAULT_COMMAND(interp, name, help, func) \
  (interp).Add(name, help, func, "User Commands")

#endif // _Draw_CommandInterface_HeaderFile