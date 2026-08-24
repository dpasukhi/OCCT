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

#ifndef _ExtremaPS_SurfaceOfExtrusion_HeaderFile
#define _ExtremaPS_SurfaceOfExtrusion_HeaderFile

#include <ExtremaPC_Curve.hxx>
#include <ExtremaPS.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <GeomAdaptor_SurfaceOfLinearExtrusion.hxx>
#include <gp_Pnt.hxx>
#include <ProjLib_ProjectOnPlane.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Computes extrema between a point and a surface of linear extrusion.
//!
//! The surface is parameterized as S(U,V) = C(U) + V * D, where C is the basis
//! curve and D is the unit extrusion direction. For each U, the stationary V
//! is obtained by projecting the query point onto the ruling through C(U).
//! The remaining stationarity equation is exactly the point-curve extrema
//! problem for C projected onto a plane normal to D. The projection preserves
//! the basis-curve parameter, so returned U values are parameters of C and
//! returned V values are signed distances along D.
//!
//! `Perform()` returns stationary extrema in the interior of the configured
//! parameter domain. Since the squared distance is strictly convex along a
//! non-degenerate ruling, such extrema can only be minima. Consequently,
//! interior `Max` queries return `ExtremaPS::Status::NoSolution`.
//! `PerformWithBoundary()` additionally examines the four boundary curves of
//! a bounded domain and can therefore return constrained maxima and minima.
//!
//! The surface and domain are fixed at construction. Geometry-dependent
//! adaptors are prepared once and reused for subsequent query points; create a
//! new evaluator after modifying the surface or its basis curve. Returned
//! results are owned by this object and are replaced by its next computation.
class ExtremaPS_SurfaceOfExtrusion
{
public:
  DEFINE_STANDARD_ALLOC

  //! Creates an evaluator on the natural surface domain.
  //! U follows the natural domain of the basis curve and V is unbounded.
  //! @param[in] theSurface surface of linear extrusion; a null handle is reported
  //!                       as `ExtremaPS::Status::NotDone` by computation methods
  Standard_EXPORT explicit ExtremaPS_SurfaceOfExtrusion(
    const occ::handle<Geom_SurfaceOfLinearExtrusion>& theSurface);

  //! Creates an evaluator on a fixed rectangular parameter domain.
  //! The U range is intersected with the natural domain of the basis curve;
  //! the V range bounds the signed extrusion distance.
  //! @param[in] theSurface surface of linear extrusion; a null handle is reported
  //!                       as `ExtremaPS::Status::NotDone` by computation methods
  //! @param[in] theDomain parameter domain used by every computation
  Standard_EXPORT ExtremaPS_SurfaceOfExtrusion(
    const occ::handle<Geom_SurfaceOfLinearExtrusion>& theSurface,
    const ExtremaPS::Domain2D&                        theDomain);

  //! Copy constructor is deleted.
  ExtremaPS_SurfaceOfExtrusion(const ExtremaPS_SurfaceOfExtrusion&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPS_SurfaceOfExtrusion& operator=(const ExtremaPS_SurfaceOfExtrusion&) = delete;

  //! Move construction is disabled.
  ExtremaPS_SurfaceOfExtrusion(ExtremaPS_SurfaceOfExtrusion&&) = delete;

  //! Move assignment is disabled.
  ExtremaPS_SurfaceOfExtrusion& operator=(ExtremaPS_SurfaceOfExtrusion&&) = delete;

  //! Computes stationary extrema in the interior of the configured domain.
  //! The extrusion parameter is eliminated analytically and ExtremaPC solves
  //! the reduced projected-curve problem. Solutions on finite domain edges are
  //! intentionally excluded; use `PerformWithBoundary()` when they are needed.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol positive tolerance used for numerical classification and
  //!                   duplicate removal
  //! @param[in] theMode requested extrema type; `Max` has no interior solution
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
  //! @param[in] theTol positive tolerance used for numerical classification and
  //!                   duplicate removal
  //! @param[in] theMode requested extrema type
  //! @return evaluator-owned result, valid until the next computation on this object
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& PerformWithBoundary(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Returns the surface fixed at construction.
  const occ::handle<Geom_SurfaceOfLinearExtrusion>& Surface() const { return mySurface; }

  //! Returns true when an explicit rectangular domain was supplied.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the explicit domain, or `std::nullopt` for the natural domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  //! Builds the basis and projected-curve adaptors for repeated point queries.
  void initCache();

  occ::handle<Geom_SurfaceOfLinearExtrusion> mySurface;        //!< Surface geometry
  GeomAdaptor_SurfaceOfLinearExtrusion       myAdaptor;        //!< Surface adaptor
  std::optional<ExtremaPS::Domain2D>         myDomain;         //!< Parameter domain
  mutable ExtremaPS::Result                  myResult;         //!< Reusable result storage
  ProjLib_ProjectOnPlane                     myProjectedCurve; //!< Orthogonal curve projection
  std::optional<ExtremaPC_Curve>             myCurveExtrema;   //!< Projected-curve evaluator
};

#endif // _ExtremaPS_SurfaceOfExtrusion_HeaderFile
