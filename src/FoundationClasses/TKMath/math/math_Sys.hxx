// Copyright (c) 2024 OPEN CASCADE SAS
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

#ifndef _math_Sys_HeaderFile
#define _math_Sys_HeaderFile

//! @file math_Sys.hxx
//! @brief Umbrella header for nonlinear systems of equations solvers.
//!
//! This header includes all nonlinear system solving algorithms in the math::Sys namespace.
//!
//! Available algorithms:
//! - Newton: Newton-Raphson method for systems with Jacobian
//!
//! Example usage:
//! @code
//!   // Define a function set with derivatives
//!   class MySystem : public math_FunctionSetWithDerivatives
//!   {
//!     // Implement NbVariables(), NbEquations(), Value(), Derivatives(), Values()
//!   };
//!
//!   MySystem aFunc;
//!   math_Vector aStart(1, 2);  // Initial guess
//!   aStart(1) = 1.0;
//!   aStart(2) = 1.0;
//!
//!   math_Vector aTolX(1, 2, 1.0e-10);  // Tolerance for X
//!   double aTolF = 1.0e-10;            // Tolerance for function values
//!
//!   auto result = math::Sys::Newton(aFunc, aStart, aTolX, aTolF);
//!   if (result.IsDone())
//!   {
//!     const math_Vector& X = *result.Solution;
//!   }
//! @endcode

#include <math_Sys_Newton.hxx>

#endif // _math_Sys_HeaderFile
