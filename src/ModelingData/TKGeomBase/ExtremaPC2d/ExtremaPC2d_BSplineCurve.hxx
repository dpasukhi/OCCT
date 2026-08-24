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

#ifndef _ExtremaPC2d_BSplineCurve_HeaderFile
#define _ExtremaPC2d_BSplineCurve_HeaderFile

#include <ExtremaPC2d.hxx>
#include <ExtremaPC2d_GridEvaluator.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <gp_Pnt2d.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>
#include <math_Vector.hxx>

//! @brief Point-BSplineCurve extrema computation using numerical root isolation.
//!
//! Computes roots of the normalized point-curve stationarity function with a
//! derivative-aware multiple-root solver.
//!
//! The knot-aware parameter partition is cached while geometry is evaluated for each query.
//!
//! The algorithm:
//! 1. Build a knot-aware parameter partition
//! 2. Select per-span root samples from the degree of the stationarity numerator
//! 3. Isolate all roots of the stationarity function, including tangential roots
//! 4. Classify roots from the stationarity sign on both sides
//!
//! This approach is simpler and more stable than BVH-based methods,
//! with comparable accuracy for typical BSpline curves.
//!
//! The geometry, domain, and parameter partition are fixed for the evaluator lifetime.
class ExtremaPC2d_BSplineCurve
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with BSpline curve (uses full curve domain).
  //! @param[in] theCurve BSpline curve handle
  Standard_EXPORT explicit ExtremaPC2d_BSplineCurve(
    const occ::handle<Geom2d_BSplineCurve>& theCurve);

  //! Constructor with BSpline curve and parameter domain.
  //! @param[in] theCurve BSpline curve handle
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPC2d_BSplineCurve(const occ::handle<Geom2d_BSplineCurve>& theCurve,
                                           const ExtremaPC2d::Domain1D&            theDomain);

  //! Copy constructor is deleted.
  ExtremaPC2d_BSplineCurve(const ExtremaPC2d_BSplineCurve&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC2d_BSplineCurve& operator=(const ExtremaPC2d_BSplineCurve&) = delete;

  //! Move constructor.
  ExtremaPC2d_BSplineCurve(ExtremaPC2d_BSplineCurve&&) = default;

  //! Move assignment operator.
  ExtremaPC2d_BSplineCurve& operator=(ExtremaPC2d_BSplineCurve&&) = default;

  //! Evaluates point on curve at parameter.
  //! @param theU parameter
  //! @return point on curve
  Standard_EXPORT gp_Pnt2d Value(double theU) const;

  //! Returns true if domain is bounded (subset of curve domain).
  bool IsBounded() const { return true; } // BSpline curves are always bounded

  //! Returns the domain.
  const ExtremaPC2d::Domain1D& Domain() const { return myDomain; }

  //! Compute extrema between point P and the curve.
  //! Uses domain specified at construction time.
  //! @param theP query point
  //! @param theTol tolerance for root finding
  //! @param theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC2d::Result& Perform(
    const gp_Pnt2d&         theP,
    double                  theTol,
    ExtremaPC2d::SearchMode theMode = ExtremaPC2d::SearchMode::MinMax) const;

  //! Compute extrema between point P and the curve including endpoints.
  //! Uses domain specified at construction time.
  //! @param theP query point
  //! @param theTol tolerance for root finding
  //! @param theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC2d::Result& PerformWithEndpoints(
    const gp_Pnt2d&         theP,
    double                  theTol,
    ExtremaPC2d::SearchMode theMode = ExtremaPC2d::SearchMode::MinMax) const;

  //! Returns the BSpline curve.
  const occ::handle<Geom2d_BSplineCurve>& Curve() const { return myCurve; }

private:
  //! Builds the parameter partition from BSpline knot spans.
  //! @return knot-span boundaries clipped to the evaluator domain
  math_Vector buildKnotAwareParams() const;

  //! Builds the knot partition and configures degree-aware root sampling.
  void buildParams();

  occ::handle<Geom2d_BSplineCurve> myCurve;   //!< BSpline curve
  Geom2dAdaptor_Curve              myAdaptor; //!< Curve adaptor
  ExtremaPC2d::Domain1D            myDomain;  //!< Parameter domain (fixed)

  // Numerical evaluator with cached parameter partition and result
  mutable ExtremaPC2d_GridEvaluator myEvaluator;
};

#endif // _ExtremaPC2d_BSplineCurve_HeaderFile
