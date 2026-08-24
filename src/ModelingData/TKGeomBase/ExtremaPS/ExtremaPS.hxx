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

#ifndef _ExtremaPS_HeaderFile
#define _ExtremaPS_HeaderFile

#include <ExtremaPC.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <MathUtils_Domain.hxx>
#include <NCollection_LinearVector.hxx>
#include <Standard_Macro.hxx>

class Adaptor3d_Surface;
class gp_Cone;
class gp_Cylinder;
class gp_Pln;
class gp_Sphere;
class gp_Torus;

//! @file ExtremaPS.hxx
//! @brief Common types and boundary utilities for point-surface extrema computation.
//!
//! ExtremaPS combines analytical evaluators for elementary surfaces with
//! derivative-based numerical evaluators for free-form surfaces. Each evaluator
//! is bound to one surface and parameter domain at construction, allowing its
//! geometry-dependent grid or auxiliary evaluator to be reused for multiple
//! query points.
//!
//! `Perform()` reports stationary extrema in the interior of the parameter
//! domain. `PerformWithBoundary()` additionally evaluates the four isoparametric
//! boundary curves and their corners. This distinction prevents an unconstrained
//! query from silently returning a point clamped to a trimming boundary.
//!
//! Returned results are evaluator-owned reusable storage and remain valid only
//! until the next computation on that evaluator. Search modes select local
//! minima or maxima; `Result::MinIndex()` and `Result::MaxIndex()` select the
//! global closest or farthest entry from the returned set.

namespace ExtremaPS
{

//! Default number of samples used by numerical surface evaluators.
constexpr size_t THE_DEFAULT_NB_SAMPLES = 32;

//! Sampling bounds used for Bezier surface grids.
constexpr size_t THE_BEZIER_MIN_SAMPLES       = 16;
constexpr size_t THE_BEZIER_MAX_SAMPLES       = 128;
constexpr size_t THE_BEZIER_DEGREE_MULTIPLIER = 6;

//! Tests whether a periodic parameter has a representative inside a domain.
//! The input value is not modified.
//! @param[in] theParam parameter to test
//! @param[in] theDomain admissible parameter interval
//! @param[in] theTol tolerance applied to the interval bounds
//! @return true when an equivalent periodic parameter lies in the domain
Standard_EXPORT bool IsInPeriodicRange(const double               theParam,
                                       const MathUtils::Domain1D& theDomain,
                                       const double               theTol);

//! Rectangular U/V parameter domain shared by all surface evaluators.
using MathUtils::Domain2D;

//! Status of extrema computation.
enum class Status
{
  OK,                //!< Computation succeeded, finite number of extrema found
  NotDone,           //!< Computation not performed
  InfiniteSolutions, //!< Infinite solutions exist (e.g., point on plane)
  NoSolution,        //!< No extrema found in the given parameter range
  InvalidInput,      //!< Invalid point, tolerance, or parameter domain
  NumericalError     //!< Numerical issues during computation
};

//! Selects local minima, local maxima, or both during an extrema search.
using SearchMode = ExtremaPC::SearchMode;

//! Result of a single extremum computation.
struct ExtremumResult
{
  double U = 0.0;               //!< U parameter value on surface
  double V = 0.0;               //!< V parameter value on surface
  gp_Pnt Point;                 //!< Point on surface at (U, V)
  double SquareDistance = 0.0;  //!< Square of the distance from query point to surface point
  bool   IsMinimum      = true; //!< True if this is a local minimum, false if maximum

  //! Returns UV parameters as gp_Pnt2d.
  gp_Pnt2d UV() const { return gp_Pnt2d(U, V); }
};

//! Result of extrema computation containing all found extrema.
//! Non-copyable to enforce use of const reference from Perform().
struct Result
{
  //! Sentinel returned by index queries when no extremum exists.
  static constexpr size_t INVALID_INDEX = ExtremaPC::Result::INVALID_INDEX;

  Status Status = Status::NotDone; //!< Computation status

  //! Collection of found extrema.
  //! Uses small increment (8) since analytical surfaces typically have 1-4 extrema.
  NCollection_LinearVector<ExtremumResult> Extrema{8};

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

//! Appends extrema located on the four boundary curves of a trimmed plane.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] thePlane plane defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&          theResult,
                                        const gp_Pnt&    thePoint,
                                        const Domain2D&  theDomain,
                                        const gp_Pln&    thePlane,
                                        const double     theTolerance,
                                        const SearchMode theMode);

//! Appends extrema located on the four boundary curves of a trimmed cylinder.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] theCylinder cylinder defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&            theResult,
                                        const gp_Pnt&      thePoint,
                                        const Domain2D&    theDomain,
                                        const gp_Cylinder& theCylinder,
                                        const double       theTolerance,
                                        const SearchMode   theMode);

//! Appends extrema located on the four boundary curves of a trimmed cone.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] theCone cone defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&          theResult,
                                        const gp_Pnt&    thePoint,
                                        const Domain2D&  theDomain,
                                        const gp_Cone&   theCone,
                                        const double     theTolerance,
                                        const SearchMode theMode);

//! Appends extrema located on the four boundary curves of a trimmed sphere.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] theSphere sphere defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&          theResult,
                                        const gp_Pnt&    thePoint,
                                        const Domain2D&  theDomain,
                                        const gp_Sphere& theSphere,
                                        const double     theTolerance,
                                        const SearchMode theMode);

//! Appends extrema located on the four boundary curves of a trimmed torus.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] theTorus torus defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&          theResult,
                                        const gp_Pnt&    thePoint,
                                        const Domain2D&  theDomain,
                                        const gp_Torus&  theTorus,
                                        const double     theTolerance,
                                        const SearchMode theMode);

//! Appends extrema located on the four boundary curves of a trimmed adapted surface.
//! @param[in, out] theResult result receiving unique boundary extrema
//! @param[in] thePoint query point
//! @param[in] theDomain trimmed surface parameter domain
//! @param[in] theSurface surface adaptor defining the boundary curves
//! @param[in] theTolerance tolerance used for extrema computation and duplicate removal
//! @param[in] theMode requested extrema type
Standard_EXPORT void AddBoundaryExtrema(Result&                  theResult,
                                        const gp_Pnt&            thePoint,
                                        const Domain2D&          theDomain,
                                        const Adaptor3d_Surface& theSurface,
                                        const double             theTolerance,
                                        const SearchMode         theMode);

} // namespace ExtremaPS

#endif // _ExtremaPS_HeaderFile
