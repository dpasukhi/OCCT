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

#ifndef _math_Roots_Newton_HeaderFile
#define _math_Roots_Newton_HeaderFile

#include <math_Types.hxx>
#include <math_Config.hxx>
#include <math_InternalCore.hxx>
#include <math_InternalConvergence.hxx>

#include <cmath>

//! Root finding algorithms for scalar functions.
namespace math
{

namespace Roots
{

//! Newton-Raphson root finding algorithm.
//! Finds x such that f(x) = 0 using Newton's method with derivative.
//!
//! Algorithm:
//! x_{n+1} = x_n - f(x_n) / f'(x_n)
//!
//! Requires a function providing both value and derivative.
//! Converges quadratically near the root for simple roots.
//!
//! @tparam Function type with Values(double theX, double& theF, double& theDf) method
//!         returning bool (true if evaluation succeeded)
//! @param theFunc function object providing value and derivative
//! @param theGuess initial guess for the root
//! @param theConfig solver configuration (tolerances, max iterations)
//! @return result containing root location and convergence status
template <typename Function>
ScalarResult Newton(Function& theFunc, double theGuess, const Config& theConfig = Config())
{
  ScalarResult aResult;
  double            aX = theGuess;
  double            aFx = 0.0;
  double            aDfx = 0.0;

  for (int anIter = 0; anIter < theConfig.MaxIterations; ++anIter)
  {
    const double anXOld = aX;

    // Evaluate function and derivative
    if (!theFunc.Values(aX, aFx, aDfx))
    {
      aResult.Status = Status::NumericalError;
      aResult.Root = aX;
      aResult.NbIterations = anIter;
      return aResult;
    }

    // Check for zero derivative (stationary point)
    if (Internal::IsZero(aDfx))
    {
      // Try to continue with a small perturbation if not converged
      if (!Internal::IsFConverged(aFx, theConfig.FTolerance))
      {
        aResult.Status = Status::NumericalError;
        aResult.Root = aX;
        aResult.Value = aFx;
        aResult.Derivative = aDfx;
        aResult.NbIterations = anIter;
        return aResult;
      }
      // Zero derivative at a root is fine (multiple root)
    }
    else
    {
      // Newton step
      aX -= aFx / aDfx;
    }

    aResult.NbIterations = anIter + 1;

    // Check convergence
    if (Internal::IsConverged(anXOld, aX, aFx, theConfig))
    {
      aResult.Status = Status::OK;
      aResult.Root = aX;
      aResult.Value = aFx;
      aResult.Derivative = aDfx;
      return aResult;
    }
  }

  // Maximum iterations reached
  aResult.Status = Status::MaxIterations;
  aResult.Root = aX;
  aResult.Value = aFx;
  aResult.Derivative = aDfx;
  return aResult;
}

//! Newton-Raphson with bounds checking.
//! Falls back to bisection step if Newton step goes outside bounds.
//! More robust than pure Newton for ill-conditioned problems.
//!
//! @tparam Function type with Values(double theX, double& theF, double& theDf) method
//! @param theFunc function object providing value and derivative
//! @param theGuess initial guess for the root
//! @param theLower lower bound of search interval
//! @param theUpper upper bound of search interval
//! @param theConfig solver configuration
//! @return result containing root location and convergence status
template <typename Function>
ScalarResult NewtonBounded(Function&          theFunc,
                                double             theGuess,
                                double             theLower,
                                double             theUpper,
                                const Config& theConfig = Config())
{
  ScalarResult aResult;

  // Clamp initial guess to bounds
  double aX = Internal::Clamp(theGuess, theLower, theUpper);
  double aXLo = theLower;
  double aXHi = theUpper;

  double aFx = 0.0;
  double aDfx = 0.0;
  double aFLo = 0.0;
  double aFHi = 0.0;
  double aDummy = 0.0;

  // Initialize bounds with function values
  if (!theFunc.Values(aXLo, aFLo, aDummy))
  {
    aResult.Status = Status::NumericalError;
    return aResult;
  }
  if (!theFunc.Values(aXHi, aFHi, aDummy))
  {
    aResult.Status = Status::NumericalError;
    return aResult;
  }

  // Check if bounds bracket a root
  const bool aBracketed = (aFLo * aFHi < 0.0);

  for (int anIter = 0; anIter < theConfig.MaxIterations; ++anIter)
  {
    const double anXOld = aX;

    // Evaluate function and derivative at current point
    if (!theFunc.Values(aX, aFx, aDfx))
    {
      aResult.Status = Status::NumericalError;
      aResult.Root = aX;
      aResult.NbIterations = anIter;
      return aResult;
    }

    aResult.NbIterations = anIter + 1;

    // Check convergence
    if (Internal::IsFConverged(aFx, theConfig.FTolerance))
    {
      aResult.Status = Status::OK;
      aResult.Root = aX;
      aResult.Value = aFx;
      aResult.Derivative = aDfx;
      return aResult;
    }

    // Compute Newton step
    double aXNew = aX;
    if (!Internal::IsZero(aDfx))
    {
      aXNew = aX - aFx / aDfx;
    }

    // Check if Newton step is within bounds
    if (aXNew < aXLo || aXNew > aXHi)
    {
      // Fall back to bisection if bracketed
      if (aBracketed)
      {
        aXNew = 0.5 * (aXLo + aXHi);
      }
      else
      {
        // Just clamp to bounds
        aXNew = Internal::Clamp(aXNew, aXLo, aXHi);
      }
    }

    aX = aXNew;

    // Update bracket if root is bracketed
    if (aBracketed)
    {
      if (aFx * aFLo < 0.0)
      {
        aXHi = anXOld;
        aFHi = aFx;
      }
      else
      {
        aXLo = anXOld;
        aFLo = aFx;
      }
    }

    // Check X convergence
    if (Internal::IsXConverged(anXOld, aX, theConfig.XTolerance))
    {
      // Re-evaluate at final position
      theFunc.Values(aX, aFx, aDfx);
      aResult.Status = Status::OK;
      aResult.Root = aX;
      aResult.Value = aFx;
      aResult.Derivative = aDfx;
      return aResult;
    }
  }

  // Maximum iterations reached
  aResult.Status = Status::MaxIterations;
  aResult.Root = aX;
  aResult.Value = aFx;
  aResult.Derivative = aDfx;
  return aResult;
}

} // namespace Roots
} // namespace math

#endif // _math_Roots_Newton_HeaderFile
