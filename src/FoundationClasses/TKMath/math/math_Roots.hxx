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

#ifndef _math_Roots_HeaderFile
#define _math_Roots_HeaderFile

//! @file math_Roots.hxx
//! @brief Umbrella header for root finding algorithms.
//!
//! This header includes all root finding solvers for convenience.
//! For faster compile times, include individual headers:
//! - math_Roots_Newton.hxx for Newton-Raphson and Secant methods
//! - math_Roots_Brent.hxx for Brent's method
//! - math_Roots_Bisection.hxx for Bisection and hybrid methods
//!
//! Algorithm selection guide:
//! - Newton: Fast quadratic convergence, requires derivative, may diverge
//! - NewtonBounded: Newton with fallback to bisection, more robust
//! - Secant: Superlinear convergence, no derivative needed, may diverge
//! - Brent: Guaranteed convergence, no derivative, requires valid bracket
//! - Bisection: Guaranteed convergence, simple but slow, requires bracket
//! - BisectionNewton: Best of both worlds, requires derivative and bracket
//!
//! Example usage:
//! @code
//! #include <math_Roots.hxx>
//!
//! // Function with derivative: f(x) = x^2 - 2, f'(x) = 2x
//! class SqrtTwoFunc
//! {
//! public:
//!   bool Values(double theX, double& theF, double& theDf)
//!   {
//!     theF = theX * theX - 2.0;
//!     theDf = 2.0 * theX;
//!     return true;
//!   }
//! };
//!
//! SqrtTwoFunc aFunc;
//! auto aResult = math_Roots::Newton(aFunc, 1.0);
//! if (aResult)
//! {
//!   std::cout << "sqrt(2) = " << aResult.Root << std::endl;
//! }
//!
//! // Function without derivative
//! class ValueOnlyFunc
//! {
//! public:
//!   bool Value(double theX, double& theF)
//!   {
//!     theF = std::cos(theX) - theX;
//!     return true;
//!   }
//! };
//!
//! ValueOnlyFunc aFunc2;
//! auto aResult2 = math_Roots::Brent(aFunc2, 0.0, 1.0);
//! @endcode

#include <math_Roots_Newton.hxx>
#include <math_Roots_Brent.hxx>
#include <math_Roots_Bisection.hxx>
#include <math_Roots_Secant.hxx>
#include <math_Roots_Multiple.hxx>
#include <math_Roots_Trig.hxx>
#include <math_Roots_All.hxx>

#endif // _math_Roots_HeaderFile
