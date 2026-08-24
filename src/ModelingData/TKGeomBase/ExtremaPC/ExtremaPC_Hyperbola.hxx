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

#ifndef _ExtremaPC_Hyperbola_HeaderFile
#define _ExtremaPC_Hyperbola_HeaderFile

#include <ExtremaPC.hxx>
#include <gp_Hypr.hxx>
#include <gp_Pnt.hxx>
#include <MathPoly_Types.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! @brief Point-Hyperbola extrema computation.
//!
//! Computes the extrema between a 3D point and a hyperbola.
//! Uses quartic polynomial solving via MathPoly::Quartic with substitution.
//!
//! The algorithm:
//! 1. Projects point P onto the hyperbola plane -> Pp
//! 2. For hyperbola C(u) = (R*cosh(u), r*sinh(u)) with major radius R and minor radius r,
//!    substitutes v = e^u to convert the transcendental equation to a polynomial:
//!    ((R^2 + r^2)/4) * v^4 - ((X*R + Y*r)/2) * v^3 + ((X*R - Y*r)/2) * v - ((R^2 + r^2)/4) = 0
//! 3. Filters positive roots (v > 0) and converts back via u = ln(v)
//!
//! @note A hyperbola can have up to 4 extrema.
//!
//! The domain is fixed at construction time for optimal performance.
//! For infinite hyperbola, construct without domain or with nullopt.
class ExtremaPC_Hyperbola
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with hyperbola geometry (infinite).
  //! @param[in] theHyperbola the hyperbola to compute extrema for
  Standard_EXPORT explicit ExtremaPC_Hyperbola(const gp_Hypr& theHyperbola);

  //! Constructor with hyperbola geometry and parameter domain.
  //! @param[in] theHyperbola the hyperbola to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPC_Hyperbola(const gp_Hypr&             theHyperbola,
                                      const ExtremaPC::Domain1D& theDomain);

  //! Copy constructor is deleted.
  ExtremaPC_Hyperbola(const ExtremaPC_Hyperbola&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC_Hyperbola& operator=(const ExtremaPC_Hyperbola&) = delete;

  //! Move constructor.
  ExtremaPC_Hyperbola(ExtremaPC_Hyperbola&&) = default;

  //! Move assignment operator.
  ExtremaPC_Hyperbola& operator=(ExtremaPC_Hyperbola&&) = default;

  //! Evaluates a point using the cached hyperbola representation.
  Standard_EXPORT gp_Pnt EvalD0(const double theU) const;

  //! Returns true if domain is bounded.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the domain (only valid if IsBounded() is true).
  const ExtremaPC::Domain1D& Domain() const { return *myDomain; }

  //! Compute extrema between point P and the hyperbola.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for duplicate detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Compute extrema between point P and the hyperbola arc including endpoints.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for duplicate detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& PerformWithEndpoints(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Returns the hyperbola geometry.
  const gp_Hypr& Hyperbola() const { return myHyperbola; }

private:
  //! Cache geometry components for fast computation.
  void cacheGeometry();

  //! Solve the quartic equation and return polynomial result using cached geometry.
  MathPoly::PolyResult solveQuartic(const gp_Pnt& theP) const;

  //! Core algorithm - finds extrema with optional bounds checking.
  //! Stores results in myResult.
  //! @param[in] theP query point
  //! @param[in] theDomain optional parameter domain (nullopt for unbounded)
  //! @param[in] theTol tolerance for duplicate detection
  //! @param[in] theMode search mode
  void performCore(const gp_Pnt&                             theP,
                   const std::optional<ExtremaPC::Domain1D>& theDomain,
                   const double                              theTol,
                   const ExtremaPC::SearchMode               theMode) const;

  gp_Hypr                            myHyperbola; //!< Hyperbola geometry
  std::optional<ExtremaPC::Domain1D> myDomain;    //!< Parameter domain (nullopt for infinite)
  mutable ExtremaPC::Result          myResult;    //!< Reusable result storage

  // Cached geometry components for fast computation
  double myCenterX, myCenterY, myCenterZ; //!< Center location
  double myXDirX, myXDirY, myXDirZ;       //!< X-axis direction
  double myYDirX, myYDirY, myYDirZ;       //!< Y-axis direction
  double myAxisX, myAxisY, myAxisZ;       //!< Main axis direction
  double myMajorR;                        //!< Major radius
  double myMinorR;                        //!< Minor radius
  double myR2PlusR2Over4;                 //!< Precomputed (R^2 + r^2)/4
};

#endif // _ExtremaPC_Hyperbola_HeaderFile
