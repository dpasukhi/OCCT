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

#ifndef _ExtremaPC_Parabola_HeaderFile
#define _ExtremaPC_Parabola_HeaderFile

#include <ExtremaPC.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <MathPoly_Types.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! @brief Point-Parabola extrema computation.
//!
//! Computes the extrema between a 3D point and a parabola.
//! Uses cubic polynomial solving via MathPoly::Cubic.
//!
//! The algorithm:
//! 1. Projects point P onto the parabola plane -> Pp
//! 2. For parabola C(u) = ((u^2)/(4F), u) with focal length F,
//!    solves: (1/(4F)) * u^3 + (2F - X) * u - 2F*Y = 0
//!    where (X, Y) are coordinates of Pp in parabola local frame.
//!
//! @note A parabola can have up to 3 extrema.
//!
//! The domain is fixed at construction time for optimal performance.
//! For infinite parabola, construct without domain or with nullopt.
class ExtremaPC_Parabola
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with parabola geometry (infinite).
  //! @param[in] theParabola the parabola to compute extrema for
  Standard_EXPORT explicit ExtremaPC_Parabola(const gp_Parab& theParabola);

  //! Constructor with parabola geometry and parameter domain.
  //! @param[in] theParabola the parabola to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPC_Parabola(const gp_Parab&            theParabola,
                                     const ExtremaPC::Domain1D& theDomain);

  //! Copy constructor is deleted.
  ExtremaPC_Parabola(const ExtremaPC_Parabola&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC_Parabola& operator=(const ExtremaPC_Parabola&) = delete;

  //! Move constructor.
  ExtremaPC_Parabola(ExtremaPC_Parabola&&) = default;

  //! Move assignment operator.
  ExtremaPC_Parabola& operator=(ExtremaPC_Parabola&&) = default;

  //! Evaluates a point using the cached parabola representation.
  Standard_EXPORT gp_Pnt EvalD0(const double theU) const;

  //! Returns true if domain is bounded.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the domain (only valid if IsBounded() is true).
  const ExtremaPC::Domain1D& Domain() const { return *myDomain; }

  //! Compute extrema between point P and the parabola.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for duplicate detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Compute extrema between point P and the parabola arc including endpoints.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for duplicate detection
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& PerformWithEndpoints(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Returns the parabola geometry.
  const gp_Parab& Parabola() const { return myParabola; }

private:
  //! Cache geometry components for fast computation.
  void cacheGeometry();

  //! Solve the cubic equation and return polynomial result using cached geometry.
  MathPoly::PolyResult solveCubic(const gp_Pnt& theP) const;

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

  gp_Parab                           myParabola; //!< Parabola geometry
  std::optional<ExtremaPC::Domain1D> myDomain;   //!< Parameter domain (nullopt for infinite)
  mutable ExtremaPC::Result          myResult;   //!< Reusable result storage

  // Cached geometry components for fast computation
  double myVertexX, myVertexY, myVertexZ; //!< Vertex location
  double myXDirX, myXDirY, myXDirZ;       //!< X-axis direction
  double myYDirX, myYDirY, myYDirZ;       //!< Y-axis direction
  double myAxisX, myAxisY, myAxisZ;       //!< Main axis direction
  double myFocal;                         //!< Focal length
  double my1Over4F;                       //!< Precomputed 1/(4*F)
  double my2F;                            //!< Precomputed 2*F
};

#endif // _ExtremaPC_Parabola_HeaderFile
