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

#ifndef _ExtremaPC_Ellipse_HeaderFile
#define _ExtremaPC_Ellipse_HeaderFile

#include <ExtremaPC.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! @brief Point-Ellipse extrema computation.
//!
//! Computes the extrema between a 3D point and an ellipse.
//! Uses trigonometric equation solving via MathRoot::Trigonometric.
//!
//! The algorithm:
//! 1. Projects point P onto the ellipse plane -> Pp
//! 2. Solves: (B^2 - A^2)*cos(u)*sin(u) - B*Y*cos(u) + A*X*sin(u) = 0
//!    where A = major radius, B = minor radius, and (X,Y) are coordinates
//!    of Pp in the ellipse local coordinate system.
//!
//! @note Degenerate case: When P projects to the ellipse center and A = B
//!       (i.e., the ellipse is a circle), returns Status::InfiniteSolutions.
//!
//! @note An ellipse can have up to 4 extrema.
//!
//! The domain is fixed at construction time for optimal performance.
//! For full ellipse, construct without domain or with nullopt.
class ExtremaPC_Ellipse
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with ellipse geometry (full ellipse).
  //! @param[in] theEllipse the ellipse to compute extrema for
  Standard_EXPORT explicit ExtremaPC_Ellipse(const gp_Elips& theEllipse);

  //! Constructor with ellipse geometry and parameter domain.
  //! @param[in] theEllipse the ellipse to compute extrema for
  //! @param[in] theDomain parameter domain in radians (fixed for all queries)
  Standard_EXPORT ExtremaPC_Ellipse(const gp_Elips&            theEllipse,
                                    const ExtremaPC::Domain1D& theDomain);

  //! Copy constructor is deleted.
  ExtremaPC_Ellipse(const ExtremaPC_Ellipse&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC_Ellipse& operator=(const ExtremaPC_Ellipse&) = delete;

  //! Move constructor.
  ExtremaPC_Ellipse(ExtremaPC_Ellipse&&) = default;

  //! Move assignment operator.
  ExtremaPC_Ellipse& operator=(ExtremaPC_Ellipse&&) = default;

  //! Evaluates a point on the ellipse.
  Standard_EXPORT gp_Pnt EvalD0(const double theU) const;

  //! Returns true if domain is bounded (partial arc).
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the domain (only valid if IsBounded() is true).
  const ExtremaPC::Domain1D& Domain() const { return *myDomain; }

  //! Compute extrema between point P and the ellipse.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for degenerate case detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing extrema or InfiniteSolutions status
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Compute extrema between point P and the ellipse arc including endpoints.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for degenerate case detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema or InfiniteSolutions
  //! status
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& PerformWithEndpoints(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Returns the ellipse geometry.
  const gp_Elips& Ellipse() const { return myEllipse; }

private:
  //! Core algorithm - finds extrema with bounds checking.
  //! Stores results in myResult.
  void performCore(const gp_Pnt&               theP,
                   const ExtremaPC::Domain1D&  theDomain,
                   const double                theTol,
                   const ExtremaPC::SearchMode theMode) const;

  gp_Elips                           myEllipse; //!< Ellipse geometry
  std::optional<ExtremaPC::Domain1D> myDomain;  //!< Parameter domain (nullopt for full ellipse)
  mutable ExtremaPC::Result          myResult;  //!< Reusable result storage
};

#endif // _ExtremaPC_Ellipse_HeaderFile
