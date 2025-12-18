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

#ifndef _math_Min_HeaderFile
#define _math_Min_HeaderFile

//! @file math_Min.hxx
//! @brief Umbrella header for minimization algorithms.
//!
//! This header includes all minimization solvers for convenience.
//! For faster compile times, include individual headers:
//! - math_Min_Brent.hxx for 1D Brent and Golden section methods
//!
//! Algorithm selection guide for 1D:
//! - Brent: Fast, combines parabolic interpolation with golden section
//! - Golden: Simple, guaranteed convergence, slightly slower than Brent
//! - BrentWithBracket: Automatically finds bracket before minimizing
//!
//! Example usage:
//! @code
//! #include <math_Min.hxx>
//!
//! // Function to minimize: f(x) = (x - 3)^2 + 1
//! class ParabolaFunc
//! {
//! public:
//!   bool Value(double theX, double& theF)
//!   {
//!     theF = (theX - 3.0) * (theX - 3.0) + 1.0;
//!     return true;
//!   }
//! };
//!
//! ParabolaFunc aFunc;
//! auto aResult = math_Min::Brent(aFunc, 0.0, 10.0);
//! if (aResult)
//! {
//!   std::cout << "Minimum at x = " << aResult.Root << std::endl;   // ~3.0
//!   std::cout << "Minimum value = " << aResult.Value << std::endl; // ~1.0
//! }
//!
//! // Or use automatic bracketing
//! auto aResult2 = math_Min::BrentWithBracket(aFunc, 0.0, 1.0);
//! @endcode

#include <math_Min_Brent.hxx>

#endif // _math_Min_HeaderFile
