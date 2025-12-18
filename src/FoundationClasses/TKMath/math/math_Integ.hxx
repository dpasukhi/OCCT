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

#ifndef _math_Integ_HeaderFile
#define _math_Integ_HeaderFile

//! @file math_Integ.hxx
//! @brief Umbrella header for numerical integration algorithms.
//!
//! This header includes all integration methods for convenience.
//! For faster compile times, include individual headers:
//! - math_Integ_Gauss.hxx for Gauss-Legendre quadrature
//!
//! Algorithm selection guide:
//! - Gauss: Fixed-order quadrature, fast, best for smooth functions
//! - GaussAdaptive: Automatic error control, subdivides as needed
//! - GaussComposite: Simple subdivision, good for periodic or oscillatory functions
//!
//! Example usage:
//! @code
//! #include <math_Integ.hxx>
//!
//! // Integrate sin(x) from 0 to pi (should be 2.0)
//! class SinFunc
//! {
//! public:
//!   bool Value(double theX, double& theF)
//!   {
//!     theF = std::sin(theX);
//!     return true;
//!   }
//! };
//!
//! SinFunc aFunc;
//! auto aResult = math_Integ::Gauss(aFunc, 0.0, 3.14159265358979, 15);
//! if (aResult)
//! {
//!   std::cout << "Integral = " << aResult.Value << std::endl;  // ~2.0
//! }
//!
//! // Adaptive integration with error control
//! math_IntegConfig aConfig;
//! aConfig.Tolerance = 1.0e-12;
//! auto aResult2 = math_Integ::GaussAdaptive(aFunc, 0.0, 3.14159265358979, aConfig);
//! if (aResult2)
//! {
//!   std::cout << "Integral = " << aResult2.Value << std::endl;
//!   std::cout << "Error estimate = " << aResult2.AbsoluteError << std::endl;
//! }
//! @endcode

#include <math_Integ_Gauss.hxx>

#endif // _math_Integ_HeaderFile
