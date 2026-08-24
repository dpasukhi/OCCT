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

#ifndef _ExtremaPC_BezierCurve_HeaderFile
#define _ExtremaPC_BezierCurve_HeaderFile

#include <ExtremaPC.hxx>
#include <ExtremaPC2d_BezierCurve.hxx>
#include <ExtremaPC_GridEvaluator.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>

#include <optional>

//! @brief Point-BezierCurve extrema computation using numerical root isolation.
//!
//! Computes roots of the normalized point-curve stationarity function with a
//! derivative-aware multiple-root solver.
//!
//! The parameter partition is built at construction and reused for point queries.
//!
//! The algorithm:
//! 1. Build a parameter partition based on curve degree
//! 2. Isolate all roots of the stationarity function, including tangential roots
//! 3. Classify roots from the stationarity sign on both sides
//!
//! This approach is simpler and more stable than BVH-based methods,
//! with comparable accuracy for typical Bezier curves.
//!
//! The geometry and domain are fixed for the evaluator lifetime.
class ExtremaPC_BezierCurve
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with Bezier curve (uses full curve domain).
  //! @param[in] theCurve Bezier curve handle
  Standard_EXPORT explicit ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve);

  //! Constructor with Bezier curve and parameter domain.
  //! @param[in] theCurve Bezier curve handle
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPC_BezierCurve(const occ::handle<Geom_BezierCurve>& theCurve,
                                        const ExtremaPC::Domain1D&           theDomain);

  //! Copy constructor is deleted.
  ExtremaPC_BezierCurve(const ExtremaPC_BezierCurve&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC_BezierCurve& operator=(const ExtremaPC_BezierCurve&) = delete;

  //! Move constructor.
  ExtremaPC_BezierCurve(ExtremaPC_BezierCurve&&) = default;

  //! Move assignment operator.
  ExtremaPC_BezierCurve& operator=(ExtremaPC_BezierCurve&&) = default;

  //! Returns true if domain is bounded (subset of curve domain).
  bool IsBounded() const { return true; } // Bezier curves are always bounded

  //! Returns the domain.
  const ExtremaPC::Domain1D& Domain() const { return myDomain; }

  //! Compute extrema between point P and the curve.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for root finding
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Compute extrema between point P and the curve including endpoints.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for root finding
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& PerformWithEndpoints(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Returns the Bezier curve.
  const occ::handle<Geom_BezierCurve>& Curve() const { return myCurve; }

private:
  //! Build parameter partition for the curve.
  void buildParams();

  //! Builds the exact planar evaluator when the curve has a supported planar representation.
  void initPlanar();

  //! Performs through the exact 2D representation when the current curve is planar.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for root finding
  //! @param[in] theMode search mode
  //! @param[in] theIncludeEndpoints whether endpoints should be included
  //! @return true when the 2D result was accepted
  bool performPlanar(const gp_Pnt&               theP,
                     const double                theTol,
                     const ExtremaPC::SearchMode theMode,
                     const bool                  theIncludeEndpoints) const;

  occ::handle<Geom_BezierCurve>          myCurve;       //!< Bezier curve
  GeomAdaptor_Curve                      myAdaptor;     //!< Curve adaptor
  ExtremaPC::Domain1D                    myDomain;      //!< Parameter domain (fixed)
  gp_Pln                                 myPlanarPlane; //!< Plane of the exact 2D representation
  std::optional<ExtremaPC2d_BezierCurve> myPlanarEvaluator; //!< Cached planar evaluator

  // Numerical evaluator with cached parameter partition and result
  mutable ExtremaPC_GridEvaluator myEvaluator;
};

#endif // _ExtremaPC_BezierCurve_HeaderFile
