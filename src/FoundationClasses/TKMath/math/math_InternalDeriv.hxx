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

#ifndef _math_InternalDeriv_HeaderFile
#define _math_InternalDeriv_HeaderFile

#include <math_Vector.hxx>
#include <math_Matrix.hxx>
#include <math_InternalCore.hxx>

#include <cmath>

namespace math
{

//! Internal numerical differentiation utilities.
//! Used when analytical derivatives are not available.
namespace Internal
{

//! Central difference derivative approximation for scalar functions.
//! f'(x) ≈ (f(x+h) - f(x-h)) / (2h)
//! Accuracy: O(h²)
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to differentiate
//! @param theX point at which to evaluate derivative
//! @param theDeriv computed derivative value
//! @param theStep step size (default 1e-8)
//! @return true if successful
template <typename Function>
bool CentralDifference(Function& theFunc, double theX, double& theDeriv, double theStep = 1.0e-8)
{
  double aFPlus = 0.0;
  double aFMinus = 0.0;

  if (!theFunc.Value(theX + theStep, aFPlus))
  {
    return false;
  }
  if (!theFunc.Value(theX - theStep, aFMinus))
  {
    return false;
  }

  theDeriv = (aFPlus - aFMinus) / (2.0 * theStep);
  return true;
}

//! Forward difference derivative (one-sided).
//! f'(x) ≈ (f(x+h) - f(x)) / h
//! Accuracy: O(h)
//! Useful when central difference is not possible (e.g., at boundaries).
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to differentiate
//! @param theX point at which to evaluate derivative
//! @param theFx function value at theX (if already computed)
//! @param theDeriv computed derivative value
//! @param theStep step size (default 1e-8)
//! @return true if successful
template <typename Function>
bool ForwardDifference(Function& theFunc,
                       double    theX,
                       double    theFx,
                       double&   theDeriv,
                       double    theStep = 1.0e-8)
{
  double aFPlus = 0.0;

  if (!theFunc.Value(theX + theStep, aFPlus))
  {
    return false;
  }

  theDeriv = (aFPlus - theFx) / theStep;
  return true;
}

//! Numerical gradient using central differences for N-D functions.
//! ∂f/∂xᵢ ≈ (f(x + hᵢeᵢ) - f(x - hᵢeᵢ)) / (2hᵢ)
//!
//! @tparam Function type with Value(const math_Vector&, double&) method
//! @param theFunc function to differentiate
//! @param theX point at which to evaluate gradient (temporarily modified)
//! @param theGrad output gradient vector (same dimension as theX)
//! @param theStep step size (default 1e-8)
//! @return true if successful
template <typename Function>
bool NumericalGradient(Function&    theFunc,
                       math_Vector& theX,
                       math_Vector& theGrad,
                       double       theStep = 1.0e-8)
{
  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  for (int i = aLower; i <= aUpper; ++i)
  {
    const double aXi = theX(i);
    double       aFPlus = 0.0;
    double       aFMinus = 0.0;

    // Forward perturbation
    theX(i) = aXi + theStep;
    if (!theFunc.Value(theX, aFPlus))
    {
      theX(i) = aXi;
      return false;
    }

    // Backward perturbation
    theX(i) = aXi - theStep;
    if (!theFunc.Value(theX, aFMinus))
    {
      theX(i) = aXi;
      return false;
    }

    // Restore original value
    theX(i) = aXi;

    // Central difference
    theGrad(i) = (aFPlus - aFMinus) / (2.0 * theStep);
  }

  return true;
}

//! Numerical gradient with adaptive step size.
//! Uses step proportional to |xᵢ| for better conditioning.
//!
//! @tparam Function type with Value(const math_Vector&, double&) method
//! @param theFunc function to differentiate
//! @param theX point at which to evaluate gradient
//! @param theGrad output gradient vector
//! @param theRelStep relative step size (default 1e-8)
//! @return true if successful
template <typename Function>
bool NumericalGradientAdaptive(Function&    theFunc,
                               math_Vector& theX,
                               math_Vector& theGrad,
                               double       theRelStep = 1.0e-8)
{
  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  for (int i = aLower; i <= aUpper; ++i)
  {
    const double aXi = theX(i);
    // Adaptive step: larger for larger |x|, with minimum floor
    const double aStep = theRelStep * std::max(1.0, std::abs(aXi));

    double aFPlus = 0.0;
    double aFMinus = 0.0;

    theX(i) = aXi + aStep;
    if (!theFunc.Value(theX, aFPlus))
    {
      theX(i) = aXi;
      return false;
    }

    theX(i) = aXi - aStep;
    if (!theFunc.Value(theX, aFMinus))
    {
      theX(i) = aXi;
      return false;
    }

    theX(i) = aXi;
    theGrad(i) = (aFPlus - aFMinus) / (2.0 * aStep);
  }

  return true;
}

//! Numerical Jacobian matrix for vector-valued functions.
//! Jᵢⱼ = ∂Fᵢ/∂xⱼ ≈ (Fᵢ(x + hⱼeⱼ) - Fᵢ(x - hⱼeⱼ)) / (2hⱼ)
//!
//! @tparam Function type with Value(const math_Vector& x, math_Vector& F) method
//! @param theFunc vector-valued function F: Rⁿ → Rᵐ
//! @param theX point at which to evaluate Jacobian (n-dimensional)
//! @param theJac output Jacobian matrix (m × n)
//! @param theStep step size (default 1e-8)
//! @return true if successful
template <typename Function>
bool NumericalJacobian(Function&    theFunc,
                       math_Vector& theX,
                       math_Matrix& theJac,
                       double       theStep = 1.0e-8)
{
  const int aNbRows = theJac.RowNumber();
  const int aNbCols = theJac.ColNumber();

  math_Vector aFPlus(1, aNbRows);
  math_Vector aFMinus(1, aNbRows);

  for (int j = 1; j <= aNbCols; ++j)
  {
    const int    aIdx = theX.Lower() + j - 1;
    const double aXj = theX(aIdx);

    // Forward perturbation
    theX(aIdx) = aXj + theStep;
    if (!theFunc.Value(theX, aFPlus))
    {
      theX(aIdx) = aXj;
      return false;
    }

    // Backward perturbation
    theX(aIdx) = aXj - theStep;
    if (!theFunc.Value(theX, aFMinus))
    {
      theX(aIdx) = aXj;
      return false;
    }

    // Restore
    theX(aIdx) = aXj;

    // Fill column of Jacobian
    for (int i = 1; i <= aNbRows; ++i)
    {
      theJac(i, j) = (aFPlus(i) - aFMinus(i)) / (2.0 * theStep);
    }
  }

  return true;
}

//! Numerical Hessian matrix using finite differences.
//! Hᵢⱼ = ∂²f/∂xᵢ∂xⱼ
//! Uses central differences on gradient.
//!
//! @tparam Function type with Value(const math_Vector&, double&) method
//! @param theFunc scalar function f: Rⁿ → R
//! @param theX point at which to evaluate Hessian (n-dimensional)
//! @param theHess output Hessian matrix (n × n, symmetric)
//! @param theStep step size (default 1e-5, larger than gradient step)
//! @return true if successful
template <typename Function>
bool NumericalHessian(Function&    theFunc,
                      math_Vector& theX,
                      math_Matrix& theHess,
                      double       theStep = 1.0e-5)
{
  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  double aFx = 0.0;
  if (!theFunc.Value(theX, aFx))
  {
    return false;
  }

  // Diagonal elements: ∂²f/∂xᵢ² ≈ (f(x+hᵢeᵢ) - 2f(x) + f(x-hᵢeᵢ)) / h²
  for (int i = aLower; i <= aUpper; ++i)
  {
    const double aXi = theX(i);
    double       aFPlus = 0.0;
    double       aFMinus = 0.0;

    theX(i) = aXi + theStep;
    if (!theFunc.Value(theX, aFPlus))
    {
      theX(i) = aXi;
      return false;
    }

    theX(i) = aXi - theStep;
    if (!theFunc.Value(theX, aFMinus))
    {
      theX(i) = aXi;
      return false;
    }

    theX(i) = aXi;

    const int aMatIdx = i - aLower + 1;
    theHess(aMatIdx, aMatIdx) = (aFPlus - 2.0 * aFx + aFMinus) / (theStep * theStep);
  }

  // Off-diagonal elements: ∂²f/∂xᵢ∂xⱼ
  // ≈ (f(x+hᵢeᵢ+hⱼeⱼ) - f(x+hᵢeᵢ-hⱼeⱼ) - f(x-hᵢeᵢ+hⱼeⱼ) + f(x-hᵢeᵢ-hⱼeⱼ)) / (4h²)
  for (int i = aLower; i <= aUpper; ++i)
  {
    for (int j = i + 1; j <= aUpper; ++j)
    {
      const double aXi = theX(i);
      const double aXj = theX(j);
      double       aFpp = 0.0, aFpm = 0.0, aFmp = 0.0, aFmm = 0.0;

      // f(x + hᵢeᵢ + hⱼeⱼ)
      theX(i) = aXi + theStep;
      theX(j) = aXj + theStep;
      if (!theFunc.Value(theX, aFpp))
      {
        theX(i) = aXi;
        theX(j) = aXj;
        return false;
      }

      // f(x + hᵢeᵢ - hⱼeⱼ)
      theX(j) = aXj - theStep;
      if (!theFunc.Value(theX, aFpm))
      {
        theX(i) = aXi;
        theX(j) = aXj;
        return false;
      }

      // f(x - hᵢeᵢ - hⱼeⱼ)
      theX(i) = aXi - theStep;
      if (!theFunc.Value(theX, aFmm))
      {
        theX(i) = aXi;
        theX(j) = aXj;
        return false;
      }

      // f(x - hᵢeᵢ + hⱼeⱼ)
      theX(j) = aXj + theStep;
      if (!theFunc.Value(theX, aFmp))
      {
        theX(i) = aXi;
        theX(j) = aXj;
        return false;
      }

      // Restore
      theX(i) = aXi;
      theX(j) = aXj;

      const int    aMatI = i - aLower + 1;
      const int    aMatJ = j - aLower + 1;
      const double aHij = (aFpp - aFpm - aFmp + aFmm) / (4.0 * theStep * theStep);

      // Symmetric
      theHess(aMatI, aMatJ) = aHij;
      theHess(aMatJ, aMatI) = aHij;
    }
  }

  return true;
}

//! Second derivative using central difference.
//! f''(x) ≈ (f(x+h) - 2f(x) + f(x-h)) / h²
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to differentiate
//! @param theX point at which to evaluate second derivative
//! @param theFx function value at theX (if already computed)
//! @param theD2f computed second derivative value
//! @param theStep step size (default 1e-5)
//! @return true if successful
template <typename Function>
bool SecondDerivative(Function& theFunc,
                      double    theX,
                      double    theFx,
                      double&   theD2f,
                      double    theStep = 1.0e-5)
{
  double aFPlus = 0.0;
  double aFMinus = 0.0;

  if (!theFunc.Value(theX + theStep, aFPlus))
  {
    return false;
  }
  if (!theFunc.Value(theX - theStep, aFMinus))
  {
    return false;
  }

  theD2f = (aFPlus - 2.0 * theFx + aFMinus) / (theStep * theStep);
  return true;
}

} // namespace Internal
} // namespace math

#endif // _math_InternalDeriv_HeaderFile
