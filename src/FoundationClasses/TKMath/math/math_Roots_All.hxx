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

#ifndef _math_Roots_All_HeaderFile
#define _math_Roots_All_HeaderFile

#include <math_Types.hxx>
#include <math_Config.hxx>
#include <math_Roots_Multiple.hxx>

#include <cmath>
#include <vector>

namespace math
{

namespace Roots
{

//! Represents an interval where the function is null (within tolerance).
struct NullInterval
{
  double A     = 0.0; //!< Interval start
  double B     = 0.0; //!< Interval end
  int    State = 0;   //!< State number (for parametric curves)
};

//! Result for all roots finder including null intervals.
struct AllRootsResult
{
  Status                     Status = Status::NotConverged;
  std::vector<double>        Roots;         //!< Isolated root locations
  std::vector<int>           RootStates;    //!< State numbers for roots
  std::vector<NullInterval>  NullIntervals; //!< Intervals where function is null

  bool IsDone() const { return Status == Status::OK; }
  explicit operator bool() const { return IsDone(); }

  int NbRoots() const { return static_cast<int>(Roots.size()); }
  int NbIntervals() const { return static_cast<int>(NullIntervals.size()); }
};

//! Find all roots of a function using sampling and refinement.
//!
//! Uses a sample of the function to find:
//! 1. Null intervals: where |F(x)| <= EpsNul for consecutive sample points
//! 2. Isolated roots: single points where F(x) = 0
//!
//! The algorithm:
//! 1. Evaluates F at sample points
//! 2. Identifies null intervals where |F| <= EpsNul for 2+ consecutive points
//! 3. Refines interval boundaries using root finding
//! 4. Finds isolated roots between null intervals using MultipleRoots
//!
//! @tparam Func function type with:
//!   - bool Value(double, double&)
//!   - bool Values(double, double&, double&) for derivative
//! @param theFunc function with derivative to analyze
//! @param theSamples sample points array
//! @param theEpsX tolerance for root x-value
//! @param theEpsF tolerance for function value at root
//! @param theEpsNul tolerance for null interval detection
//! @return AllRootsResult with roots and null intervals
template <typename Func>
AllRootsResult FindAllRoots(Func&                      theFunc,
                            const std::vector<double>& theSamples,
                            double                     theEpsX   = 1.0e-10,
                            double                     theEpsF   = 1.0e-10,
                            double                     theEpsNul = 1.0e-10)
{
  AllRootsResult aResult;

  const int aNbp = static_cast<int>(theSamples.size());
  if (aNbp < 2)
  {
    aResult.Status = Status::InvalidInput;
    return aResult;
  }

  // Evaluate function at first sample point
  double aVal, aPrevVal;
  if (!theFunc.Value(theSamples[0], aPrevVal))
  {
    aResult.Status = Status::NotConverged;
    return aResult;
  }

  bool aPrevNul = std::abs(aPrevVal) <= theEpsNul;
  if (!aPrevNul)
  {
    // Save non-null value for later use
  }

  bool   aInInterval = false;
  bool   aNulStart   = false;
  bool   aNulEnd     = false;
  double aDebNul = 0.0, aFinNul = 0.0;
  double aValSav = aPrevVal;

  std::vector<double> aIntervalStarts, aIntervalEnds;

  // Scan through samples to find null intervals
  for (int i = 1; i < aNbp; ++i)
  {
    if (!theFunc.Value(theSamples[i], aVal))
    {
      aResult.Status = Status::NotConverged;
      return aResult;
    }

    bool aCurNul = std::abs(aVal) <= theEpsNul;
    if (!aCurNul)
    {
      aValSav = aVal;
    }

    if (aInInterval && !aCurNul)
    {
      // End of null interval
      aInInterval = false;
      aIntervalStarts.push_back(aDebNul);

      // Refine end of null interval
      double aCst = (aVal > 0.0) ? theEpsNul : -theEpsNul;

      // Use root finding to locate precise boundary
      MultipleResult aRes = FindMultipleRoots(theFunc, theSamples[i - 1], theSamples[i], 10,
                                              theEpsX, theEpsF, aCst);
      if (aRes.IsDone() && aRes.NbRoots() > 0)
      {
        aFinNul = aRes.Roots[0];
      }
      else
      {
        aFinNul = theSamples[i - 1];
      }

      // Try opposite sign
      aCst     = -aCst;
      auto aRes2 = FindMultipleRoots(theFunc, theSamples[i - 1], theSamples[i], 10,
                                     theEpsX, theEpsF, aCst);
      if (aRes2.IsDone() && aRes2.NbRoots > 0)
      {
        if (aRes2.Roots[0] < aFinNul)
        {
          aFinNul = aRes2.Roots[0];
        }
      }

      aIntervalEnds.push_back(aFinNul);
    }
    else if (!aInInterval && aPrevNul && aCurNul)
    {
      // Start of null interval
      aInInterval = true;
      if (i == 1)
      {
        aDebNul   = theSamples[0];
        aNulStart = true;
      }
      else
      {
        // Refine start of null interval
        double aCst = (aValSav > 0.0) ? theEpsNul : -theEpsNul;

        MultipleResult aRes = FindMultipleRoots(theFunc, theSamples[i - 2], theSamples[i - 1], 10,
                                                theEpsX, theEpsF, aCst);
        if (aRes.IsDone() && aRes.NbRoots() > 0)
        {
          aDebNul = aRes.Roots[aRes.NbRoots() - 1];
        }
        else
        {
          aDebNul = theSamples[i - 1];
        }

        // Try opposite sign
        aCst     = -aCst;
        auto aRes2 = FindMultipleRoots(theFunc, theSamples[i - 2], theSamples[i - 1], 10,
                                       theEpsX, theEpsF, aCst);
        if (aRes2.IsDone() && aRes2.NbRoots() > 0)
        {
          if (aRes2.Roots[aRes2.NbRoots() - 1] > aDebNul)
          {
            aDebNul = aRes2.Roots[aRes2.NbRoots() - 1];
          }
        }
      }
    }

    aPrevNul = aCurNul;
  }

  // Handle interval ending at last sample
  if (aInInterval)
  {
    aIntervalStarts.push_back(aDebNul);
    aFinNul = theSamples[aNbp - 1];
    aIntervalEnds.push_back(aFinNul);
    aNulEnd = true;
  }

  // Store null intervals
  for (size_t k = 0; k < aIntervalStarts.size(); ++k)
  {
    NullInterval anInt;
    anInt.A = aIntervalStarts[k];
    anInt.B = aIntervalEnds[k];
    aResult.NullIntervals.push_back(anInt);
  }

  // Find isolated roots between null intervals
  if (aIntervalStarts.empty())
  {
    // No null intervals - find all roots in entire range
    MultipleResult aRes = FindMultipleRoots(theFunc, theSamples[0], theSamples[aNbp - 1],
                                            aNbp, theEpsX, theEpsF);
    if (aRes.IsDone())
    {
      for (size_t j = 0; j < aRes.NbRoots(); ++j)
      {
        aResult.Roots.push_back(aRes.Roots[j]);
        aResult.RootStates.push_back(0);
      }
    }
  }
  else
  {
    // Find roots before first null interval
    if (!aNulStart)
    {
      double aStart = theSamples[0];
      double aEnd   = aIntervalStarts[0];
      int    aNbrpt = std::max(3, static_cast<int>(
        std::abs((aEnd - aStart) / (theSamples[aNbp - 1] - theSamples[0])) * aNbp));

      MultipleResult aRes = FindMultipleRoots(theFunc, aStart, aEnd, aNbrpt, theEpsX, theEpsF);
      if (aRes.IsDone())
      {
        for (size_t j = 0; j < aRes.NbRoots(); ++j)
        {
          aResult.Roots.push_back(aRes.Roots[j]);
          aResult.RootStates.push_back(0);
        }
      }
    }

    // Find roots between null intervals
    for (size_t k = 1; k < aIntervalStarts.size(); ++k)
    {
      double aStart = aIntervalEnds[k - 1];
      double aEnd   = aIntervalStarts[k];
      int    aNbrpt = std::max(3, static_cast<int>(
        std::abs((aEnd - aStart) / (theSamples[aNbp - 1] - theSamples[0])) * aNbp));

      MultipleResult aRes = FindMultipleRoots(theFunc, aStart, aEnd, aNbrpt, theEpsX, theEpsF);
      if (aRes.IsDone())
      {
        for (size_t j = 0; j < aRes.NbRoots(); ++j)
        {
          aResult.Roots.push_back(aRes.Roots[j]);
          aResult.RootStates.push_back(0);
        }
      }
    }

    // Find roots after last null interval
    if (!aNulEnd)
    {
      double aStart = aIntervalEnds.back();
      double aEnd   = theSamples[aNbp - 1];
      int    aNbrpt = std::max(3, static_cast<int>(
        std::abs((aEnd - aStart) / (theSamples[aNbp - 1] - theSamples[0])) * aNbp));

      MultipleResult aRes = FindMultipleRoots(theFunc, aStart, aEnd, aNbrpt, theEpsX, theEpsF);
      if (aRes.IsDone())
      {
        for (size_t j = 0; j < aRes.NbRoots(); ++j)
        {
          aResult.Roots.push_back(aRes.Roots[j]);
          aResult.RootStates.push_back(0);
        }
      }
    }
  }

  aResult.Status = Status::OK;
  return aResult;
}

//! Find all roots using uniform sampling.
//!
//! @tparam Func function type with Value and Values methods
//! @param theFunc function with derivative
//! @param theA interval start
//! @param theB interval end
//! @param theNbSamples number of sample points
//! @param theEpsX tolerance for root x-value
//! @param theEpsF tolerance for function value
//! @param theEpsNul tolerance for null interval detection
//! @return AllRootsResult with roots and null intervals
template <typename Func>
AllRootsResult FindAllRoots(Func&  theFunc,
                            double theA,
                            double theB,
                            int    theNbSamples,
                            double theEpsX   = 1.0e-10,
                            double theEpsF   = 1.0e-10,
                            double theEpsNul = 1.0e-10)
{
  std::vector<double> aSamples(theNbSamples);
  const double        aStep = (theB - theA) / (theNbSamples - 1);
  for (int i = 0; i < theNbSamples; ++i)
  {
    aSamples[i] = theA + i * aStep;
  }
  // Ensure last point is exactly theB
  aSamples[theNbSamples - 1] = theB;

  return FindAllRoots(theFunc, aSamples, theEpsX, theEpsF, theEpsNul);
}

} // namespace Roots
} // namespace math

#endif // _math_Roots_All_HeaderFile
