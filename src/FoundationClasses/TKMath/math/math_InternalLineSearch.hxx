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

#ifndef _math_InternalLineSearch_HeaderFile
#define _math_InternalLineSearch_HeaderFile

#include <math_Vector.hxx>
#include <math_InternalCore.hxx>

#include <cmath>

namespace math
{

//! Internal line search algorithms for N-dimensional optimization.
//! These are used by gradient-based methods like BFGS, FRPR, etc.
namespace Internal
{

//! Result of line search operation.
struct LineSearchResult
{
  bool   IsValid = false;  //!< True if line search succeeded
  double Alpha   = 0.0;    //!< Step size found
  double FNew    = 0.0;    //!< Function value at new point
  int    NbEvals = 0;      //!< Number of function evaluations
};

//! Backtracking line search with Armijo condition.
//! Finds α such that f(x + α*d) ≤ f(x) + c₁*α*∇f·d
//! (sufficient decrease condition)
//!
//! Algorithm:
//! 1. Start with α = αᵢₙᵢₜ
//! 2. If Armijo condition satisfied, return α
//! 3. Otherwise α = ρ * α and repeat
//!
//! @tparam Function type with Value(const math_Vector&, double&) method
//! @param theFunc objective function
//! @param theX current point
//! @param theDir search direction (should be descent direction)
//! @param theGrad gradient at current point
//! @param theFx function value at current point
//! @param theAlphaInit initial step size (default 1.0)
//! @param theC1 Armijo parameter (default 1e-4)
//! @param theRho backtracking factor (default 0.5)
//! @param theMaxIter maximum iterations (default 50)
//! @return line search result with step size
template <typename Function>
LineSearchResult ArmijoBacktrack(Function&          theFunc,
                                 const math_Vector& theX,
                                 const math_Vector& theDir,
                                 const math_Vector& theGrad,
                                 double             theFx,
                                 double             theAlphaInit = 1.0,
                                 double             theC1        = 1.0e-4,
                                 double             theRho       = 0.5,
                                 int                theMaxIter   = 50)
{
  LineSearchResult aResult;
  aResult.Alpha = theAlphaInit;
  aResult.NbEvals = 0;

  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  // Compute directional derivative: ∇f · d
  double aDirDeriv = 0.0;
  for (int i = aLower; i <= aUpper; ++i)
  {
    aDirDeriv += theGrad(i) * theDir(i);
  }

  // Ensure descent direction
  if (aDirDeriv >= 0.0)
  {
    // Not a descent direction, return failure
    aResult.IsValid = false;
    return aResult;
  }

  // Temporary vector for new point
  math_Vector aXNew(aLower, aUpper);

  for (int k = 0; k < theMaxIter; ++k)
  {
    // Compute new point: x + α*d
    for (int i = aLower; i <= aUpper; ++i)
    {
      aXNew(i) = theX(i) + aResult.Alpha * theDir(i);
    }

    // Evaluate function at new point
    double aFNew = 0.0;
    if (!theFunc.Value(aXNew, aFNew))
    {
      // Function evaluation failed, reduce step
      aResult.Alpha *= theRho;
      ++aResult.NbEvals;
      continue;
    }
    ++aResult.NbEvals;

    // Check Armijo condition: f(x + αd) ≤ f(x) + c₁αφ'(0)
    if (aFNew <= theFx + theC1 * aResult.Alpha * aDirDeriv)
    {
      aResult.IsValid = true;
      aResult.FNew = aFNew;
      return aResult;
    }

    // Reduce step size
    aResult.Alpha *= theRho;

    // Check for too small step
    if (aResult.Alpha < THE_EPSILON)
    {
      break;
    }
  }

  // Failed to satisfy Armijo condition
  aResult.IsValid = false;
  return aResult;
}

//! Strong Wolfe line search.
//! Finds α satisfying both:
//! 1. Armijo: f(x + αd) ≤ f(x) + c₁α∇f·d
//! 2. Curvature: |∇f(x + αd)·d| ≤ c₂|∇f·d|
//!
//! Uses bracketing and zoom procedure for guaranteed convergence.
//!
//! @tparam Function type with:
//!   - Value(const math_Vector&, double&) for function value
//!   - Gradient(const math_Vector&, math_Vector&) for gradient
//! @param theFunc objective function with gradient
//! @param theX current point
//! @param theDir search direction
//! @param theGrad gradient at current point
//! @param theFx function value at current point
//! @param theAlphaInit initial step size
//! @param theC1 Armijo parameter (default 1e-4)
//! @param theC2 curvature parameter (default 0.9)
//! @param theMaxIter maximum iterations
//! @return line search result
template <typename Function>
LineSearchResult WolfeSearch(Function&          theFunc,
                             const math_Vector& theX,
                             const math_Vector& theDir,
                             const math_Vector& theGrad,
                             double             theFx,
                             double             theAlphaInit = 1.0,
                             double             theC1        = 1.0e-4,
                             double             theC2        = 0.9,
                             int                theMaxIter   = 20)
{
  LineSearchResult aResult;
  aResult.NbEvals = 0;

  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  // Compute initial directional derivative
  double aPhi0Prime = 0.0;
  for (int i = aLower; i <= aUpper; ++i)
  {
    aPhi0Prime += theGrad(i) * theDir(i);
  }

  if (aPhi0Prime >= 0.0)
  {
    aResult.IsValid = false;
    return aResult;
  }

  math_Vector aXNew(aLower, aUpper);
  math_Vector aGradNew(aLower, aUpper);

  double aAlphaLo = 0.0;
  double aAlphaHi = theAlphaInit * 2.0;
  double aAlpha = theAlphaInit;

  double aPhiLo = theFx;
  
  // Phase 1: Bracket finding
  for (int k = 0; k < theMaxIter; ++k)
  {
    // Evaluate at current alpha
    for (int i = aLower; i <= aUpper; ++i)
    {
      aXNew(i) = theX(i) + aAlpha * theDir(i);
    }

    double aPhi = 0.0;
    if (!theFunc.Value(aXNew, aPhi))
    {
      // Shrink interval
      aAlphaHi = aAlpha;
      aAlpha = 0.5 * (aAlphaLo + aAlphaHi);
      ++aResult.NbEvals;
      continue;
    }
    ++aResult.NbEvals;

    // Check Armijo
    if (aPhi > theFx + theC1 * aAlpha * aPhi0Prime || (k > 0 && aPhi >= aPhiLo))
    {
      // Found bracket [αₗₒ, α]
      aAlphaHi = aAlpha;
      break;
    }

    // Evaluate gradient at new point
    if (!theFunc.Gradient(aXNew, aGradNew))
    {
      aResult.IsValid = false;
      return aResult;
    }

    double aPhiPrime = 0.0;
    for (int i = aLower; i <= aUpper; ++i)
    {
      aPhiPrime += aGradNew(i) * theDir(i);
    }

    // Check curvature condition
    if (std::abs(aPhiPrime) <= -theC2 * aPhi0Prime)
    {
      aResult.IsValid = true;
      aResult.Alpha = aAlpha;
      aResult.FNew = aPhi;
      return aResult;
    }

    if (aPhiPrime >= 0.0)
    {
      // Found bracket [α, αₗₒ]
      aAlphaHi = aAlphaLo;
      aAlphaLo = aAlpha;
      aPhiLo = aPhi;
      break;
    }

    // Increase alpha
    aAlphaLo = aAlpha;
    aPhiLo = aPhi;
    aAlpha = 0.5 * (aAlpha + aAlphaHi);
  }

  // Phase 2: Zoom
  for (int k = 0; k < theMaxIter; ++k)
  {
    // Bisection
    aAlpha = 0.5 * (aAlphaLo + aAlphaHi);

    for (int i = aLower; i <= aUpper; ++i)
    {
      aXNew(i) = theX(i) + aAlpha * theDir(i);
    }

    double aPhi = 0.0;
    if (!theFunc.Value(aXNew, aPhi))
    {
      aAlphaHi = aAlpha;
      ++aResult.NbEvals;
      continue;
    }
    ++aResult.NbEvals;

    if (aPhi > theFx + theC1 * aAlpha * aPhi0Prime || aPhi >= aPhiLo)
    {
      aAlphaHi = aAlpha;
    }
    else
    {
      if (!theFunc.Gradient(aXNew, aGradNew))
      {
        break;
      }

      double aPhiPrime = 0.0;
      for (int i = aLower; i <= aUpper; ++i)
      {
        aPhiPrime += aGradNew(i) * theDir(i);
      }

      if (std::abs(aPhiPrime) <= -theC2 * aPhi0Prime)
      {
        aResult.IsValid = true;
        aResult.Alpha = aAlpha;
        aResult.FNew = aPhi;
        return aResult;
      }

      if (aPhiPrime * (aAlphaHi - aAlphaLo) >= 0.0)
      {
        aAlphaHi = aAlphaLo;
      }

      aAlphaLo = aAlpha;
      aPhiLo = aPhi;
    }

    // Check for convergence
    if (std::abs(aAlphaHi - aAlphaLo) < THE_EPSILON)
    {
      break;
    }
  }

  // Return best found
  aResult.IsValid = true;
  aResult.Alpha = aAlpha;
  aResult.FNew = aPhiLo;
  return aResult;
}

//! Exact line search using Brent's method.
//! Minimizes f(x + α*d) over α ∈ [-αₘₐₓ, αₘₐₓ].
//! First brackets the minimum by exploring both directions.
//! More expensive than inexact line search but can be more robust.
//!
//! @tparam Function type with Value(const math_Vector&, double&) method
//! @param theFunc objective function
//! @param theX current point
//! @param theDir search direction
//! @param theAlphaMax maximum step size (searches in both directions)
//! @param theTolerance convergence tolerance
//! @param theMaxIter maximum iterations
//! @return line search result
template <typename Function>
LineSearchResult ExactLineSearch(Function&          theFunc,
                                 const math_Vector& theX,
                                 const math_Vector& theDir,
                                 double             theAlphaMax = 10.0,
                                 double             theTolerance = 1.0e-6,
                                 int                theMaxIter   = 100)
{
  LineSearchResult aResult;
  aResult.NbEvals = 0;

  const int aLower = theX.Lower();
  const int aUpper = theX.Upper();

  math_Vector aXNew(aLower, aUpper);

  // Lambda to evaluate φ(α) = f(x + αd)
  auto aEvalPhi = [&](double theAlpha, double& thePhi) -> bool
  {
    for (int i = aLower; i <= aUpper; ++i)
    {
      aXNew(i) = theX(i) + theAlpha * theDir(i);
    }
    ++aResult.NbEvals;
    return theFunc.Value(aXNew, thePhi);
  };

  // Brent's method for 1D minimization
  // Search in [-theAlphaMax, theAlphaMax] to handle both directions
  double aA = -theAlphaMax;
  double aB = theAlphaMax;
  double aX = 0.0; // Start at the current point
  double aW = aX;
  double aV = aX;

  double aFx = 0.0;
  if (!aEvalPhi(aX, aFx))
  {
    aResult.IsValid = false;
    return aResult;
  }
  double aFw = aFx;
  double aFv = aFx;

  double aD = 0.0;
  double aE = 0.0;

  for (int anIter = 0; anIter < theMaxIter; ++anIter)
  {
    const double aXm = 0.5 * (aA + aB);
    const double aTol1 = theTolerance * std::abs(aX) + THE_ZERO_TOL / 10.0;
    const double aTol2 = 2.0 * aTol1;

    // Check convergence
    if (std::abs(aX - aXm) <= (aTol2 - 0.5 * (aB - aA)))
    {
      aResult.IsValid = true;
      aResult.Alpha = aX;
      aResult.FNew = aFx;
      return aResult;
    }

    double aU = 0.0;
    bool   aUseParabolic = false;

    // Try parabolic interpolation
    if (std::abs(aE) > aTol1)
    {
      const double aR = (aX - aW) * (aFx - aFv);
      double       aQ = (aX - aV) * (aFx - aFw);
      double       aP = (aX - aV) * aQ - (aX - aW) * aR;
      aQ = 2.0 * (aQ - aR);

      if (aQ > 0.0)
      {
        aP = -aP;
      }
      else
      {
        aQ = -aQ;
      }

      const double aETmp = aE;
      aE = aD;

      if (std::abs(aP) < std::abs(0.5 * aQ * aETmp) && aP > aQ * (aA - aX)
          && aP < aQ * (aB - aX))
      {
        aD = aP / aQ;
        aU = aX + aD;
        if ((aU - aA) < aTol2 || (aB - aU) < aTol2)
        {
          aD = SignTransfer(aTol1, aXm - aX);
        }
        aUseParabolic = true;
      }
    }

    if (!aUseParabolic)
    {
      aE = (aX < aXm) ? (aB - aX) : (aA - aX);
      aD = THE_GOLDEN_SECTION * aE;
    }

    if (std::abs(aD) >= aTol1)
    {
      aU = aX + aD;
    }
    else
    {
      aU = aX + SignTransfer(aTol1, aD);
    }

    double aFu = 0.0;
    if (!aEvalPhi(aU, aFu))
    {
      aResult.IsValid = false;
      aResult.Alpha = aX;
      aResult.FNew = aFx;
      return aResult;
    }

    // Update bracket
    if (aFu <= aFx)
    {
      if (aU < aX)
      {
        aB = aX;
      }
      else
      {
        aA = aX;
      }

      aV = aW;
      aW = aX;
      aX = aU;
      aFv = aFw;
      aFw = aFx;
      aFx = aFu;
    }
    else
    {
      if (aU < aX)
      {
        aA = aU;
      }
      else
      {
        aB = aU;
      }

      if (aFu <= aFw || aW == aX)
      {
        aV = aW;
        aW = aU;
        aFv = aFw;
        aFw = aFu;
      }
      else if (aFu <= aFv || aV == aX || aV == aW)
      {
        aV = aU;
        aFv = aFu;
      }
    }
  }

  aResult.IsValid = true;
  aResult.Alpha = aX;
  aResult.FNew = aFx;
  return aResult;
}

//! Quadratic interpolation step for line search.
//! Given φ(0), φ'(0), and φ(α₁), finds minimum of quadratic fit.
//!
//! @param thePhi0 function value at 0
//! @param thePhi0Prime directional derivative at 0
//! @param theAlpha1 current step size
//! @param thePhi1 function value at alpha1
//! @return interpolated step size
inline double QuadraticInterpolation(double thePhi0,
                                     double thePhi0Prime,
                                     double theAlpha1,
                                     double thePhi1)
{
  // Quadratic: φ(α) = φ(0) + φ'(0)α + cα²
  // where c = (φ(α₁) - φ(0) - φ'(0)α₁) / α₁²
  // Minimum at α* = -φ'(0) / (2c)
  const double aNum = thePhi0Prime * theAlpha1 * theAlpha1;
  const double aDenom = 2.0 * (thePhi1 - thePhi0 - thePhi0Prime * theAlpha1);

  if (std::abs(aDenom) < THE_ZERO_TOL)
  {
    return 0.5 * theAlpha1;
  }

  double aAlphaNew = -aNum / aDenom;

  // Safeguard: ensure we stay within reasonable bounds
  if (aAlphaNew < 0.1 * theAlpha1)
  {
    aAlphaNew = 0.1 * theAlpha1;
  }
  else if (aAlphaNew > 0.9 * theAlpha1)
  {
    aAlphaNew = 0.5 * theAlpha1;
  }

  return aAlphaNew;
}

} // namespace Internal
} // namespace math

#endif // _math_InternalLineSearch_HeaderFile
