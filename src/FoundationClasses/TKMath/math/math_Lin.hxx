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

#ifndef _math_Lin_HeaderFile
#define _math_Lin_HeaderFile

//! @file math_Lin.hxx
//! @brief Umbrella header for modern linear algebra solvers.
//!
//! This header includes all linear algebra algorithms in the math::Lin namespace.
//!
//! Available algorithms:
//! - Gauss: Gaussian elimination with LU decomposition (partial pivoting)
//!
//! Example usage:
//! @code
//!   math_Matrix A(1, 3, 1, 3);
//!   math_Vector B(1, 3);
//!   // ... fill A and B ...
//!
//!   // Solve AX = B
//!   auto result = math::Lin::Solve(A, B);
//!   if (result.IsDone())
//!   {
//!     const math_Vector& X = *result.Solution;
//!   }
//!
//!   // Compute determinant
//!   auto detResult = math::Lin::Determinant(A);
//!   if (detResult.IsDone())
//!   {
//!     double det = *detResult.Determinant;
//!   }
//!
//!   // Compute inverse
//!   auto invResult = math::Lin::Invert(A);
//!   if (invResult.IsDone())
//!   {
//!     const math_Matrix& invA = *invResult.Inverse;
//!   }
//! @endcode

#include <math_Lin_Gauss.hxx>
#include <math_Lin_SVD.hxx>
#include <math_Lin_Householder.hxx>
#include <math_Lin_Jacobi.hxx>
#include <math_Lin_LeastSquares.hxx>
#include <math_Lin_Crout.hxx>
#include <math_Lin_EigenSearch.hxx>

#endif // _math_Lin_HeaderFile
