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

#ifndef _ExtremaPS_SurfaceOfRevolution_HeaderFile
#define _ExtremaPS_SurfaceOfRevolution_HeaderFile

#include <ExtremaPC_Curve.hxx>
#include <ExtremaPS.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <GeomAdaptor_SurfaceOfRevolution.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Computes extrema between a point and a surface of revolution.
//!
//! The surface is parameterized by rotation angle U and basis-curve parameter
//! V. Rotational stationarity places an extremum in one of the two meridian
//! half-planes defined by the query point and the revolution axis. The angular
//! parameters of these half-planes are computed analytically. ExtremaPC then
//! solves the two corresponding point-curve problems on the original basis
//! adaptor, preserving its exact curve type and V parameterization.
//!
//! `Perform()` reports stationary extrema in the interior of the configured
//! parameter domain. For a bounded angular range, a stationary angle outside
//! that range is excluded rather than clamped. `PerformWithBoundary()` also
//! evaluates the four trimming curves and their corners, providing constrained
//! extrema on the excluded angular and meridian boundaries.
//!
//! If the query point lies on the revolution axis within the requested
//! tolerance, distance is independent of U. A valid meridian extremum therefore
//! represents infinitely many surface extrema and is reported with
//! `ExtremaPS::Status::InfiniteSolutions` and the corresponding squared distance.
//!
//! The surface and domain are fixed at construction. Geometry-dependent
//! adaptors are prepared once and reused for subsequent query points; create a
//! new evaluator after modifying the surface or its basis curve. Returned
//! results are owned by this object and are replaced by its next computation.
class ExtremaPS_SurfaceOfRevolution
{
public:
  DEFINE_STANDARD_ALLOC

  //! Creates an evaluator on the natural surface domain.
  //! U covers one full revolution and V follows the natural basis-curve domain.
  //! @param[in] theSurface surface of revolution; a null handle is reported as
  //!                       `ExtremaPS::Status::NotDone` by computation methods
  Standard_EXPORT explicit ExtremaPS_SurfaceOfRevolution(
    const occ::handle<Geom_SurfaceOfRevolution>& theSurface);

  //! Creates an evaluator on a fixed rectangular parameter domain.
  //! U bounds the rotation angle and V is intersected with the natural domain
  //! of the basis curve.
  //! @param[in] theSurface surface of revolution; a null handle is reported as
  //!                       `ExtremaPS::Status::NotDone` by computation methods
  //! @param[in] theDomain parameter domain used by every computation
  Standard_EXPORT ExtremaPS_SurfaceOfRevolution(
    const occ::handle<Geom_SurfaceOfRevolution>& theSurface,
    const ExtremaPS::Domain2D&                   theDomain);

  //! Copy constructor is deleted (contains non-copyable ExtremaPC_Curve).
  ExtremaPS_SurfaceOfRevolution(const ExtremaPS_SurfaceOfRevolution&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPS_SurfaceOfRevolution& operator=(const ExtremaPS_SurfaceOfRevolution&) = delete;

  //! Move construction is disabled because the curve evaluator keeps internal references.
  ExtremaPS_SurfaceOfRevolution(ExtremaPS_SurfaceOfRevolution&&) = delete;

  //! Move assignment is disabled because the curve evaluator keeps internal references.
  ExtremaPS_SurfaceOfRevolution& operator=(ExtremaPS_SurfaceOfRevolution&&) = delete;

  //! Computes stationary extrema in the interior of the configured domain.
  //! The two possible stationary rotation angles are obtained analytically;
  //! ExtremaPC classifies extrema along the basis-curve parameter. Solutions
  //! on finite domain edges are intentionally excluded.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol positive tolerance used for axis detection, numerical
  //!                   classification, and duplicate removal
  //! @param[in] theMode requested extrema type
  //! @return evaluator-owned result, valid until the next computation on this object
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Computes extrema in the interior and on all edges of a bounded domain.
  //! Boundary extrema include constrained extrema of each isoparametric edge
  //! and its corners. Without an explicitly supplied domain this method is
  //! equivalent to `Perform()`.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol positive tolerance used for axis detection, numerical
  //!                   classification, and duplicate removal
  //! @param[in] theMode requested extrema type
  //! @return evaluator-owned result, valid until the next computation on this object
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& PerformWithBoundary(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Returns the surface fixed at construction.
  const occ::handle<Geom_SurfaceOfRevolution>& Surface() const { return mySurface; }

  //! Returns true when an explicit rectangular domain was supplied.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the explicit domain, or `std::nullopt` for the natural domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  //! Builds the specialized surface adaptor and basis-curve evaluator.
  void initCache();

  //! Converts extrema on one meridian half-plane into surface extrema.
  //! @param[in] theCurveResult extrema found on the basis curve
  //! @param[in] thePoint original three-dimensional query point
  //! @param[in] theU stationary rotation angle
  //! @param[in] theAngularSign sign selecting the query-facing or opposite half-plane
  //! @param[in] theTolerance tolerance used for classification and duplicate removal
  //! @param[in] theMode requested extrema type
  void appendMeridianExtrema(const ExtremaPC::Result&    theCurveResult,
                             const gp_Pnt&               thePoint,
                             const double                theU,
                             const double                theAngularSign,
                             const double                theTolerance,
                             const ExtremaPS::SearchMode theMode) const;

  occ::handle<Geom_SurfaceOfRevolution> mySurface;      //!< Surface geometry
  GeomAdaptor_SurfaceOfRevolution       myAdaptor;      //!< Surface adaptor
  std::optional<ExtremaPS::Domain2D>    myDomain;       //!< Parameter domain
  mutable ExtremaPS::Result             myResult;       //!< Reusable result storage
  std::optional<ExtremaPC_Curve>        myCurveExtrema; //!< Basis-curve extrema evaluator
};

#endif // _ExtremaPS_SurfaceOfRevolution_HeaderFile
