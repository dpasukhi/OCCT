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

#ifndef _math_Integ_Kronrod_HeaderFile
#define _math_Integ_Kronrod_HeaderFile

#include <math_Types.hxx>
#include <math_Config.hxx>
#include <math_ComputeKronrodPointsAndWeights.hxx>
#include <math_InternalCore.hxx>

#include <cmath>
#include <vector>

namespace math
{

namespace Integ
{

//! Configuration for Gauss-Kronrod integration.
struct KronrodConfig : IntegConfig
{
  int    NbGaussPoints = 7;   //!< Number of Gauss points (n), Kronrod will use 2n+1 points
  bool   Adaptive      = true; //!< Whether to use adaptive subdivision

  //! Default constructor.
  KronrodConfig() = default;

  //! Constructor with tolerance.
  explicit KronrodConfig(double theTolerance, int theMaxIter = 100)
      : IntegConfig(theTolerance, theMaxIter)
  {
  }
};

//! Apply Gauss-Kronrod rule to a single interval.
//!
//! The Gauss-Kronrod rule uses n Gauss points embedded in 2n+1 Kronrod points.
//! The difference between the Gauss and Kronrod estimates provides
//! an error estimate without additional function evaluations.
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to integrate
//! @param theLower lower integration bound
//! @param theUpper upper integration bound
//! @param theNbGauss number of Gauss points (determines rule order)
//! @return integration result with error estimate
template <typename Function>
IntegResult KronrodRule(Function& theFunc,
                        double    theLower,
                        double    theUpper,
                        int       theNbGauss = 7)
{
  IntegResult aResult;

  // Number of points
  const int aNbKronrod = 2 * theNbGauss + 1;

  // Get Gauss-Kronrod points and weights
  math_Vector aGaussP(1, theNbGauss);
  math_Vector aGaussW(1, theNbGauss);
  math_Vector aKronrodP(1, aNbKronrod);
  math_Vector aKronrodW(1, aNbKronrod);

  if (!math_ComputeKronrodPointsAndWeights::Compute(theNbGauss, 1.0e-15, aGaussP, aGaussW,
                                                     aKronrodP, aKronrodW))
  {
    aResult.Status = Status::NumericalError;
    return aResult;
  }

  // Transform interval [theLower, theUpper] to [-1, 1]
  const double aHalfLen = 0.5 * (theUpper - theLower);
  const double aMid     = 0.5 * (theUpper + theLower);

  // Evaluate at Kronrod points and accumulate
  double aKronrodSum = 0.0;
  double aGaussSum   = 0.0;
  int    aGaussIdx   = 1;

  for (int i = 1; i <= aNbKronrod; ++i)
  {
    const double aX = aMid + aHalfLen * aKronrodP(i);
    double       aF = 0.0;

    if (!theFunc.Value(aX, aF))
    {
      aResult.Status = Status::NumericalError;
      return aResult;
    }

    aKronrodSum += aKronrodW(i) * aF;

    // Check if this point is also a Gauss point
    // Gauss points are at odd Kronrod indices (1, 3, 5, ..., 2n+1)
    // More precisely, they are embedded at positions 2, 4, 6, ..., 2n in 0-based Kronrod
    if (i % 2 == 0 && aGaussIdx <= theNbGauss)
    {
      aGaussSum += aGaussW(aGaussIdx) * aF;
      ++aGaussIdx;
    }
  }

  // Scale by interval length
  double aKronrodValue = aHalfLen * aKronrodSum;
  double aGaussValue   = aHalfLen * aGaussSum;

  // Error estimate
  double aAbsError = std::abs(aKronrodValue - aGaussValue);

  aResult.Status        = Status::OK;
  aResult.Value         = aKronrodValue;
  aResult.AbsoluteError = aAbsError;
  aResult.RelativeError = aAbsError / std::max(std::abs(aKronrodValue), 1.0e-15);
  aResult.NbPoints      = static_cast<size_t>(aNbKronrod);
  aResult.NbIterations  = 1;
  return aResult;
}

//! Gauss-Kronrod adaptive integration.
//!
//! Uses adaptive bisection to achieve the requested tolerance.
//! At each subdivision, the interval with the largest error estimate
//! is bisected, and both halves are reintegrated.
//!
//! This method is very efficient for smooth functions and functions
//! with integrable singularities at the endpoints.
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to integrate
//! @param theLower lower integration bound
//! @param theUpper upper integration bound
//! @param theConfig integration configuration
//! @return integration result with error estimate
template <typename Function>
IntegResult Kronrod(Function&            theFunc,
                    double               theLower,
                    double               theUpper,
                    const KronrodConfig& theConfig = KronrodConfig())
{
  IntegResult aResult;

  if (!theConfig.Adaptive)
  {
    // Single application of Kronrod rule
    return KronrodRule(theFunc, theLower, theUpper, theConfig.NbGaussPoints);
  }

  // Adaptive integration using a heap of intervals
  struct Interval
  {
    double Lower;
    double Upper;
    double Value;
    double Error;
  };

  // Initialize with the whole interval
  IntegResult anInitResult = KronrodRule(theFunc, theLower, theUpper, theConfig.NbGaussPoints);
  if (!anInitResult.IsDone())
  {
    return anInitResult;
  }

  std::vector<Interval> aHeap;
  aHeap.push_back({theLower, theUpper, *anInitResult.Value, *anInitResult.AbsoluteError});

  double aTotalValue = *anInitResult.Value;
  double aTotalError = *anInitResult.AbsoluteError;
  size_t aTotalPoints = anInitResult.NbPoints;
  int    aIterations = 1;

  // Adaptive refinement
  while (aIterations < theConfig.MaxIterations)
  {
    // Check convergence
    if (aTotalError < theConfig.Tolerance * std::abs(aTotalValue))
    {
      break;
    }

    // Find interval with largest error
    size_t aMaxIdx   = 0;
    double aMaxError = 0.0;
    for (size_t i = 0; i < aHeap.size(); ++i)
    {
      if (aHeap[i].Error > aMaxError)
      {
        aMaxError = aHeap[i].Error;
        aMaxIdx   = i;
      }
    }

    if (aMaxError < Internal::THE_ZERO_TOL)
    {
      break; // No more refinement needed
    }

    // Bisect the interval with largest error
    const Interval& aWorst = aHeap[aMaxIdx];
    const double    aMid   = 0.5 * (aWorst.Lower + aWorst.Upper);

    IntegResult aLeftResult  = KronrodRule(theFunc, aWorst.Lower, aMid, theConfig.NbGaussPoints);
    IntegResult aRightResult = KronrodRule(theFunc, aMid, aWorst.Upper, theConfig.NbGaussPoints);

    if (!aLeftResult.IsDone() || !aRightResult.IsDone())
    {
      aResult.Status = Status::NumericalError;
      aResult.Value  = aTotalValue;
      aResult.AbsoluteError = aTotalError;
      aResult.NbPoints = aTotalPoints;
      aResult.NbIterations = static_cast<size_t>(aIterations);
      return aResult;
    }

    // Update totals
    aTotalValue -= aWorst.Value;
    aTotalError -= aWorst.Error;
    aTotalValue += *aLeftResult.Value + *aRightResult.Value;
    aTotalError += *aLeftResult.AbsoluteError + *aRightResult.AbsoluteError;
    aTotalPoints += aLeftResult.NbPoints + aRightResult.NbPoints;
    ++aIterations;

    // Replace the worst interval with the two new intervals
    aHeap[aMaxIdx] = {aWorst.Lower, aMid, *aLeftResult.Value, *aLeftResult.AbsoluteError};
    aHeap.push_back({aMid, aWorst.Upper, *aRightResult.Value, *aRightResult.AbsoluteError});
  }

  aResult.Status        = Status::OK;
  aResult.Value         = aTotalValue;
  aResult.AbsoluteError = aTotalError;
  aResult.RelativeError = aTotalError / std::max(std::abs(aTotalValue), 1.0e-15);
  aResult.NbPoints      = aTotalPoints;
  aResult.NbIterations  = static_cast<size_t>(aIterations);
  return aResult;
}

//! Gauss-Kronrod integration with automatic order selection.
//!
//! Starts with a low-order rule and increases the order until
//! the tolerance is met or the maximum order is reached.
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to integrate
//! @param theLower lower integration bound
//! @param theUpper upper integration bound
//! @param theTolerance relative tolerance
//! @param theMaxOrder maximum Gauss order to try
//! @return integration result
template <typename Function>
IntegResult KronrodAuto(Function& theFunc,
                        double    theLower,
                        double    theUpper,
                        double    theTolerance = 1.0e-10,
                        int       theMaxOrder  = 30)
{
  IntegResult aBestResult;
  aBestResult.Status = Status::NotConverged;

  // Try increasing orders
  for (int aOrder = 7; aOrder <= theMaxOrder; aOrder += 4)
  {
    IntegResult aResult = KronrodRule(theFunc, theLower, theUpper, aOrder);
    if (!aResult.IsDone())
    {
      continue;
    }

    aBestResult = aResult;

    // Check if tolerance is met
    if (aResult.RelativeError && *aResult.RelativeError < theTolerance)
    {
      return aResult;
    }
  }

  // If fixed order didn't work, try adaptive
  KronrodConfig aConfig;
  aConfig.Tolerance      = theTolerance;
  aConfig.NbGaussPoints  = 7;
  aConfig.Adaptive       = true;
  aConfig.MaxIterations  = 50;

  return Kronrod(theFunc, theLower, theUpper, aConfig);
}

//! Gauss-Kronrod integration over infinite interval (-∞, +∞).
//!
//! Uses the substitution x = t / (1 - t²) to map (-∞, +∞) to (-1, 1).
//! The function must decay sufficiently fast at infinity.
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to integrate
//! @param theConfig integration configuration
//! @return integration result
template <typename Function>
IntegResult KronrodInfinite(Function& theFunc, const KronrodConfig& theConfig = KronrodConfig())
{
  // Wrapper that applies the transformation
  class TransformedFunc
  {
  public:
    TransformedFunc(Function& theF)
        : myFunc(theF)
    {
    }

    bool Value(double theT, double& theF)
    {
      // Transformation: x = t / (1 - t²), dx = (1 + t²) / (1 - t²)² dt
      if (std::abs(theT) >= 1.0)
      {
        theF = 0.0;
        return true;
      }

      const double aT2    = theT * theT;
      const double aX     = theT / (1.0 - aT2);
      const double aJacob = (1.0 + aT2) / Internal::Sqr(1.0 - aT2);

      double aFx = 0.0;
      if (!myFunc.Value(aX, aFx))
      {
        return false;
      }

      theF = aFx * aJacob;
      return true;
    }

  private:
    Function& myFunc;
  };

  TransformedFunc aTransformed(theFunc);
  return Kronrod(aTransformed, -1.0, 1.0, theConfig);
}

//! Gauss-Kronrod integration over semi-infinite interval [a, +∞).
//!
//! Uses the substitution x = a + t / (1 - t) to map [a, +∞) to [0, 1).
//!
//! @tparam Function type with Value(double theX, double& theF) method
//! @param theFunc function to integrate
//! @param theLower lower bound a
//! @param theConfig integration configuration
//! @return integration result
template <typename Function>
IntegResult KronrodSemiInfinite(Function&            theFunc,
                                double               theLower,
                                const KronrodConfig& theConfig = KronrodConfig())
{
  class TransformedFunc
  {
  public:
    TransformedFunc(Function& theF, double theA)
        : myFunc(theF),
          myA(theA)
    {
    }

    bool Value(double theT, double& theF)
    {
      if (theT >= 1.0)
      {
        theF = 0.0;
        return true;
      }

      const double aX     = myA + theT / (1.0 - theT);
      const double aJacob = 1.0 / Internal::Sqr(1.0 - theT);

      double aFx = 0.0;
      if (!myFunc.Value(aX, aFx))
      {
        return false;
      }

      theF = aFx * aJacob;
      return true;
    }

  private:
    Function& myFunc;
    double    myA;
  };

  TransformedFunc aTransformed(theFunc, theLower);
  return Kronrod(aTransformed, 0.0, 1.0, theConfig);
}

} // namespace Integ
} // namespace math

#endif // _math_Integ_Kronrod_HeaderFile
