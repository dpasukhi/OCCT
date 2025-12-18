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

#ifndef _math_Poly_Cubic_HeaderFile
#define _math_Poly_Cubic_HeaderFile

#include <math_Types.hxx>
#include <math_InternalCore.hxx>
#include <math_InternalPoly.hxx>
#include <math_Poly_Quadratic.hxx>

#include <cmath>

namespace math
{

namespace Poly
{

//! Solve cubic equation: a*x^3 + b*x^2 + c*x + d = 0
//! Uses Cardano's method with Vieta substitution and trigonometric solution.
//!
//! Algorithm:
//! 1. Handle lower degree cases (a = 0 -> quadratic)
//! 2. Transform to depressed cubic: t^3 + pt + q = 0 via x = t - b/(3a)
//! 3. Compute discriminant Delta = q^2/4 + p^3/27
//! 4. For Delta > 0: one real root (Cardano's formula)
//! 5. For Delta < 0: three real roots (trigonometric method)
//! 6. For Delta = 0: multiple roots
//! 7. Apply Newton-Raphson refinement
//!
//! @param theA coefficient of x^3
//! @param theB coefficient of x^2
//! @param theC coefficient of x
//! @param theD constant term
//! @return result containing 1, 2, or 3 real roots (sorted in ascending order)
inline PolyResult Cubic(double theA, double theB, double theC, double theD)
{
  PolyResult aResult;

  // Reduce to quadratic if leading coefficient is zero
  if (Internal::IsZero(theA))
  {
    return Quadratic(theB, theC, theD);
  }

  // Scale coefficients for numerical stability
  const double aScale = std::max({std::abs(theA), std::abs(theB), std::abs(theC), std::abs(theD)});
  if (aScale < Internal::THE_ZERO_TOL)
  {
    aResult.Status = Status::InfiniteSolutions;
    return aResult;
  }

  // Normalize to monic cubic: x^3 + px^2 + qx + r = 0
  const double aP = theB / theA;
  const double aQ = theC / theA;
  const double aR = theD / theA;

  // Substitute x = t - p/3 to get depressed cubic: t^3 + at + b = 0
  const double aP3 = aP / 3.0;
  const double aP3_sq = aP3 * aP3;
  const double a = aQ - 3.0 * aP3_sq;
  const double b = aR - aP3 * aQ + 2.0 * aP3_sq * aP3;

  // Discriminant: Delta = (b/2)^2 + (a/3)^3
  const double aHalfB = b / 2.0;
  const double aThirdA = a / 3.0;
  const double aDisc = aHalfB * aHalfB + aThirdA * aThirdA * aThirdA;

  // Tolerance for discriminant
  const double aDiscTol = Internal::THE_ZERO_TOL
                        * std::max(aHalfB * aHalfB, std::abs(aThirdA * aThirdA * aThirdA));

  // Store original coefficients for refinement
  const double aCoeffs[4] = {theD, theC, theB, theA};

  if (aDisc > aDiscTol)
  {
    // One real root, two complex conjugate roots
    // Cardano's formula: t = cbrt(-b/2 + sqrt(disc)) + cbrt(-b/2 - sqrt(disc))
    const double aSqrtDisc = std::sqrt(aDisc);
    const double aU = Internal::CubeRoot(-aHalfB + aSqrtDisc);
    const double aV = Internal::CubeRoot(-aHalfB - aSqrtDisc);

    aResult.Status = Status::OK;
    aResult.NbRoots = 1;
    aResult.Roots[0] = aU + aV - aP3;

    // Refine root
    aResult.Roots[0] = Internal::RefinePolyRoot(aCoeffs, 3, aResult.Roots[0]);
  }
  else if (aDisc < -aDiscTol)
  {
    // Three distinct real roots - use trigonometric method (casus irreducibilis)
    // t = 2*sqrt(-a/3) * cos(theta/3 + 2*k*pi/3), k = 0, 1, 2
    // where cos(theta) = -b / (2 * sqrt((-a/3)^3))

    const double aR_val = std::sqrt(-aThirdA * aThirdA * aThirdA);
    const double aCosArg = Internal::Clamp(-aHalfB / aR_val, -1.0, 1.0);
    const double aTheta = std::acos(aCosArg);
    const double aTwoSqrtNegA3 = 2.0 * std::sqrt(-aThirdA);

    aResult.Status = Status::OK;
    aResult.NbRoots = 3;
    aResult.Roots[0] = aTwoSqrtNegA3 * std::cos(aTheta / 3.0) - aP3;
    aResult.Roots[1] = aTwoSqrtNegA3 * std::cos((aTheta + 2.0 * Internal::THE_PI) / 3.0) - aP3;
    aResult.Roots[2] = aTwoSqrtNegA3 * std::cos((aTheta + 4.0 * Internal::THE_PI) / 3.0) - aP3;

    // Refine all roots
    for (int i = 0; i < 3; ++i)
    {
      aResult.Roots[i] = Internal::RefinePolyRoot(aCoeffs, 3, aResult.Roots[i]);
    }

    // Sort roots
    Internal::SortRoots(aResult.Roots.data(), 3);
  }
  else
  {
    // Discriminant is zero - multiple roots
    const double aU = Internal::CubeRoot(-aHalfB);

    aResult.Status = Status::OK;

    if (Internal::IsZero(aU))
    {
      // Triple root at x = -p/3
      aResult.NbRoots = 1;
      aResult.Roots[0] = -aP3;
    }
    else
    {
      // One single root and one double root
      aResult.NbRoots = 2;
      aResult.Roots[0] = 2.0 * aU - aP3; // Single root
      aResult.Roots[1] = -aU - aP3;      // Double root

      // Refine roots
      aResult.Roots[0] = Internal::RefinePolyRoot(aCoeffs, 3, aResult.Roots[0]);
      aResult.Roots[1] = Internal::RefinePolyRoot(aCoeffs, 3, aResult.Roots[1]);

      // Sort roots
      if (aResult.Roots[0] > aResult.Roots[1])
      {
        std::swap(aResult.Roots[0], aResult.Roots[1]);
      }
    }
  }

  return aResult;
}

} // namespace Poly
} // namespace math

#endif // _math_Poly_Cubic_HeaderFile
