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

#ifndef _math_Poly_HeaderFile
#define _math_Poly_HeaderFile

//! @file math_Poly.hxx
//! @brief Umbrella header for polynomial root finding algorithms.
//!
//! This header includes all polynomial solvers for convenience.
//! For faster compile times, include individual headers:
//! - math_Poly_Quadratic.hxx for quadratic equations
//! - math_Poly_Cubic.hxx for cubic equations
//! - math_Poly_Quartic.hxx for quartic equations
//!
//! Example usage:
//! @code
//! #include <math_Poly.hxx>
//!
//! // Solve x^2 - 5x + 6 = 0
//! auto aResult = math_Poly::Quadratic(1.0, -5.0, 6.0);
//! if (aResult)
//! {
//!   for (int i = 0; i < aResult.NbRoots; ++i)
//!   {
//!     std::cout << "Root " << i << ": " << aResult[i] << std::endl;
//!   }
//! }
//!
//! // Solve x^3 - 6x^2 + 11x - 6 = 0 (roots: 1, 2, 3)
//! auto aCubicResult = math_Poly::Cubic(1.0, -6.0, 11.0, -6.0);
//! @endcode

#include <math_Poly_Quadratic.hxx>
#include <math_Poly_Cubic.hxx>
#include <math_Poly_Quartic.hxx>

#endif // _math_Poly_HeaderFile
