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

#ifndef _math_Functor_HeaderFile
#define _math_Functor_HeaderFile

//! @file math_Functor.hxx
//! @brief Umbrella header for non-virtual functor classes.
//!
//! This header includes all functor utilities for use with the modern math API.
//! For faster compile times, include individual headers:
//! - math_Functor_Scalar.hxx for scalar function functors
//! - math_Functor_Vector.hxx for vector function functors
//!
//! Functors provide ready-to-use function objects that work with template-based
//! solvers without virtual dispatch overhead. They are more efficient than
//! the legacy virtual base class approach (math_Function, etc.).
//!
//! Example usage:
//! @code
//! #include <math_Functor.hxx>
//! #include <math_Roots.hxx>
//!
//! // Using polynomial functor
//! math::Functor::Polynomial aPoly({-6.0, 11.0, -6.0, 1.0});  // x^3 - 6x^2 + 11x - 6
//! auto aResult = math::Roots::Brent(aPoly, 0.5, 1.5);
//!
//! // Using lambda wrapper
//! auto aLambda = math::Functor::MakeScalar([](double x, double& y) {
//!   y = x * x - 2.0;
//!   return true;
//! });
//! auto aResult2 = math::Roots::Brent(aLambda, 0.0, 2.0);
//! @endcode

#include <math_Functor_Scalar.hxx>
#include <math_Functor_Vector.hxx>

#endif // _math_Functor_HeaderFile
