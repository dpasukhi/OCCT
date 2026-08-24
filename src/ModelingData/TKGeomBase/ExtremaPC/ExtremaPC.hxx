// Copyright (c) 2025 OPEN CASCADE SAS
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

#ifndef _ExtremaPC_HeaderFile
#define _ExtremaPC_HeaderFile

#include <gp_Pnt.hxx>
#include <MathUtils_Domain.hxx>
#include <NCollection_LinearVector.hxx>
#include <Precision.hxx>
#include <Standard_Macro.hxx>

#include <cmath>
#include <cstddef>

//! @file ExtremaPC.hxx
//! @brief Common types and utilities for Point-Curve extrema computation.
//!
//! The ExtremaPC package provides modern C++ implementation of point-curve
//! extrema computation using std::variant for curve type dispatch and
//! analytical and derivative-aware numerical algorithms.
//!
//! An evaluator is bound to one curve and parameter domain at construction.
//! Geometry-dependent sampling data is therefore built once and reused for
//! subsequent query points. `Perform()` reports stationary extrema in the
//! parameter-domain interior, while evaluator-specific `PerformWithEndpoints()`
//! methods additionally classify finite domain endpoints.
//!
//! Returned results belong to the evaluator and are replaced by its next
//! computation. `Min` and `Max` select local extrema of the squared distance;
//! callers can use `Result::MinIndex()` or `Result::MaxIndex()` to select the
//! global closest or farthest entry from the returned set.

namespace ExtremaPC
{

//! Returns whether a geometric tolerance is finite and strictly positive.
//! @param[in] theTolerance tolerance to validate
//! @return true when the tolerance can be used for extrema computation
Standard_EXPORT bool IsValidTolerance(const double theTolerance);

//! Returns whether all point coordinates are finite OCCT values.
//! @param[in] thePoint point to validate
//! @return true when every coordinate is finite
Standard_EXPORT bool IsFinitePoint(const gp_Pnt& thePoint);

//! Ratio of parameter range for neighbor point sampling.
//! Used to evaluate if an endpoint is a local extremum.
constexpr double THE_NEIGHBOR_STEP_RATIO = 0.01;

//! Minimum number of samples for Bezier curves.
constexpr size_t THE_BEZIER_MIN_SAMPLES = 24;

//! Multiplier for degree to compute Bezier samples: samples = max(min, multiplier * (degree + 1)).
constexpr size_t THE_BEZIER_DEGREE_MULTIPLIER = 3;

//! Default number of samples for general (other) curves.
constexpr size_t THE_OTHER_CURVE_NB_SAMPLES = 64;

//! Fallback number of samples for BSpline curves when curve is null.
constexpr size_t THE_BSPLINE_FALLBACK_SAMPLES = 32;

//! 1D parameter domain for curves (alias for MathUtils::Domain1D).
using Domain1D = MathUtils::Domain1D;

//! Returns whether a finite domain spans one or more complete periods.
//! @param[in] theDomain parameter domain to examine
//! @param[in] thePeriod positive period of the curve
//! @param[in] theTolerance admissible difference from an integral number of periods
//! @return true when both domain endpoints represent the same periodic seam
Standard_EXPORT bool IsClosedPeriodicDomain(const Domain1D& theDomain,
                                            const double    thePeriod,
                                            const double    theTolerance);

//! Returns whether a parameter can be evaluated as a finite endpoint.
//! @param[in] theParameter parameter to validate
//! @return true for a finite parameter that is not an OCCT infinity sentinel
Standard_EXPORT bool IsFiniteParameter(const double theParameter);

//! Status of extrema computation.
enum class Status
{
  OK,                //!< Computation succeeded, finite number of extrema found
  NotDone,           //!< Computation not performed
  InfiniteSolutions, //!< Infinite solutions exist (e.g., point at circle center)
  NoSolution,        //!< No extrema found in the given parameter range
  InvalidInput,      //!< Invalid tolerance or parameter domain
  NumericalError     //!< Numerical issues during computation
};

//! Search mode for extrema computation.
//! Controls which extrema to find, enabling performance optimizations.
enum class SearchMode
{
  MinMax, //!< Find all local extrema (both minima and maxima) - default
  Min,    //!< Find local minima only
  Max     //!< Find local maxima only
};

//! Result of a single extremum computation.
struct ExtremumResult
{
  double Parameter = 0.0;        //!< Parameter value on curve
  gp_Pnt Point;                  //!< Point on curve at parameter
  double SquareDistance = 0.0;   //!< Square of the distance from query point to curve point
  bool   IsMinimum      = true;  //!< True if this is a local minimum
  bool   IsMaximum      = false; //!< True if this is a local maximum
};

//! Result of extrema computation containing all found extrema.
//! Non-copyable to enforce use of const reference from Perform().
struct Result
{
  //! Sentinel returned by index queries when no extremum exists.
  static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

  ExtremaPC::Status Status = ExtremaPC::Status::NotDone; //!< Computation status
  NCollection_LinearVector<ExtremumResult> Extrema{8};   //!< Collection of found extrema

  //! For infinite solutions, stores the constant squared distance.
  //! Only meaningful when Status == Status::InfiniteSolutions.
  double InfiniteSquareDistance = 0.0;

  //! Default constructor.
  Result() = default;

  //! Copy constructor is deleted.
  Result(const Result&) = delete;

  //! Copy assignment is deleted.
  Result& operator=(const Result&) = delete;

  //! Move constructor.
  Result(Result&&) = default;

  //! Move assignment.
  Result& operator=(Result&&) = default;

  //! Returns true if a finite search completed, including a search with no matching extrema.
  bool IsDone() const { return Status == Status::OK || Status == Status::NoSolution; }

  //! Returns true if there are infinite solutions.
  bool IsInfinite() const { return Status == Status::InfiniteSolutions; }

  //! Returns number of extrema found (0 if infinite or failed).
  size_t NbExt() const { return Extrema.Size(); }

  //! Access extremum by 0-based index.
  const ExtremumResult& operator[](const size_t theIndex) const { return Extrema.Value(theIndex); }

  //! Returns the squared distance of the closest extremum.
  //! Returns infinity if no extrema found.
  Standard_EXPORT double MinSquareDistance() const;

  //! Returns the index of the closest extremum (0-based).
  //! Returns INVALID_INDEX if no extrema are available.
  Standard_EXPORT size_t MinIndex() const;

  //! Returns the squared distance of the farthest extremum.
  //! Returns 0 if no extrema found.
  Standard_EXPORT double MaxSquareDistance() const;

  //! Returns the index of the farthest extremum (0-based).
  //! Returns INVALID_INDEX if no extrema are available.
  Standard_EXPORT size_t MaxIndex() const;

  //! Clear the result for reuse.
  //! Preserves allocated memory in Extrema vector.
  Standard_EXPORT void Clear();
};

//! Adds finite non-duplicate endpoints that satisfy the requested extremum classification.
//! Each endpoint is compared with a nearby point inside the domain to determine
//! whether it is a constrained minimum or maximum. Closed periodic seams are
//! omitted because they are not curve boundaries.
//! @tparam CurveEvaluator elementary geometry or curve adaptor type
//! @param[in, out] theResult result to extend
//! @param[in] theP query point
//! @param[in] theDomain parameter domain
//! @param[in] theEval curve evaluator
//! @param[in] theMode search mode
//! @param[in] theIsClosed whether the domain represents a closed curve
template <typename CurveEvaluator>
inline void AddEndpointExtrema(Result&               theResult,
                               const gp_Pnt&         theP,
                               const Domain1D&       theDomain,
                               const CurveEvaluator& theEval,
                               const SearchMode      theMode,
                               const bool            theIsClosed)
{
  if (!theDomain.IsValid())
  {
    return;
  }

  const double theUMin      = theDomain.Min;
  const double theUMax      = theDomain.Max;
  const bool   hasFiniteMin = IsFiniteParameter(theUMin);
  const bool   hasFiniteMax = IsFiniteParameter(theUMax);
  if (!hasFiniteMin && !hasFiniteMax)
  {
    return;
  }

  // Helper to check if parameter or point already exists in result
  auto isDuplicate = [&](const double theU) -> bool {
    for (size_t anIndex = 0; anIndex < theResult.Extrema.Size(); ++anIndex)
    {
      // Check parameter proximity
      if (std::abs(theResult.Extrema.Value(anIndex).Parameter - theU) < Precision::PConfusion())
      {
        return true;
      }
    }
    return false;
  };

  if (theUMin == theUMax)
  {
    if (hasFiniteMin && !isDuplicate(theUMin))
    {
      const gp_Pnt   aPoint = theEval.EvalD0(theUMin);
      ExtremumResult anExt;
      anExt.Parameter      = theUMin;
      anExt.Point          = aPoint;
      anExt.SquareDistance = theP.SquareDistance(aPoint);
      anExt.IsMinimum      = theMode != SearchMode::Max;
      anExt.IsMaximum      = theMode != SearchMode::Min;
      theResult.Extrema.Append(anExt);
    }
    return;
  }

  if (theIsClosed)
  {
    return;
  }

  const bool hasFiniteSpan = hasFiniteMin && hasFiniteMax;
  auto       addEndpoint   = [&](const double theParameter, const bool theIsLower) {
    if (isDuplicate(theParameter))
    {
      return;
    }

    const double aStep =
      hasFiniteSpan
        ? std::min((theUMax - theUMin) * THE_NEIGHBOR_STEP_RATIO, (theUMax - theUMin) * 0.5)
        : std::max(THE_NEIGHBOR_STEP_RATIO, std::abs(theParameter) * THE_NEIGHBOR_STEP_RATIO);
    const gp_Pnt aPoint            = theEval.EvalD0(theParameter);
    const gp_Pnt aNeighbor         = theEval.EvalD0(theParameter + (theIsLower ? aStep : -aStep));
    const double aSquareDistance   = theP.SquareDistance(aPoint);
    const double aNeighborDistance = theP.SquareDistance(aNeighbor);
    const bool   isMinimum         = aSquareDistance <= aNeighborDistance;
    const bool   isMaximum         = aSquareDistance >= aNeighborDistance;

    if ((theMode == SearchMode::Min || theMode == SearchMode::MinMax) && isMinimum)
    {
      theResult.Extrema.Append(ExtremumResult{theParameter, aPoint, aSquareDistance, true, false});
    }
    else if ((theMode == SearchMode::Max || theMode == SearchMode::MinMax) && isMaximum)
    {
      theResult.Extrema.Append(ExtremumResult{theParameter, aPoint, aSquareDistance, false, true});
    }
  };

  if (hasFiniteMin)
  {
    addEndpoint(theUMin, true);
  }
  if (hasFiniteMax)
  {
    addEndpoint(theUMax, false);
  }
}

} // namespace ExtremaPC

#endif // _ExtremaPC_HeaderFile
