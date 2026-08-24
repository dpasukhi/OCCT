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

#ifndef _ExtremaPS_Torus_HeaderFile
#define _ExtremaPS_Torus_HeaderFile

#include <ExtremaPS.hxx>
#include <gp_Pnt.hxx>
#include <gp_Torus.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Point-torus extrema computation.
//!
//! Computes the extrema between a 3D point and a toroidal surface.
//!
//! A torus can have up to 4 extrema for a general point position:
//! - 2 on the nearest generating circle (min and max along minor radius)
//! - 2 on the farthest generating circle (min and max along minor radius)
//!
//! Algorithm:
//! 1. Find the generating circle nearest and farthest from the point
//! 2. On each circle, find closest and farthest points
//!
//! Two methods are provided:
//! - `Perform()` - finds interior extrema only
//! - `PerformWithBoundary()` - includes edge and corner extrema for bounded domains
//!
//! @note For a point on the torus axis, there are infinite solutions
//!       at all U values for two V values.
//!
//! @note Both U and V parameters are periodic with period 2*PI.
class ExtremaPS_Torus
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with torus geometry (uses full torus domain).
  //! @param[in] theTorus the torus to compute extrema for
  Standard_EXPORT explicit ExtremaPS_Torus(const gp_Torus& theTorus);

  //! Constructor with torus geometry and parameter domain.
  //! @param[in] theTorus the torus to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPS_Torus(const gp_Torus& theTorus, const ExtremaPS::Domain2D& theDomain);

private:
  //! Check if domain is natural (full torus).
  static bool isNaturalDomain(const ExtremaPS::Domain2D& theDomain);

private:
  //! Initialize cached components.
  void initCache();

public:
  //! Find interior extrema only.
  //! Uses domain specified at construction time.
  //!
  //! A torus can have up to 4 interior extrema for a general point.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance
  //! @param[in] theMode search mode (MinMax, Min, Max)
  //! @return const reference to result with interior extrema only
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Find extrema including boundary edges and corners.
  //! Uses domain specified at construction time.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance
  //! @param[in] theMode search mode
  //! @return const reference to result with interior + boundary extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& PerformWithBoundary(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Returns the torus geometry.
  const gp_Torus& Torus() const { return myTorus; }

  //! Returns true if domain is bounded (not natural full torus).
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the optional bounded parameter domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  gp_Torus                           myTorus;  //!< Torus geometry
  std::optional<ExtremaPS::Domain2D> myDomain; //!< Parameter domain (nullopt for full torus)
  mutable ExtremaPS::Result          myResult; //!< Reusable result storage

  double myCenterX, myCenterY, myCenterZ; //!< Torus center
  double myAxisX, myAxisY, myAxisZ;       //!< Torus axis (Z direction)
  double myXDirX, myXDirY, myXDirZ;       //!< Torus X direction
  double myYDirX, myYDirY, myYDirZ;       //!< Torus Y direction
  double myMajorRadius;                   //!< Major radius
  double myMinorRadius;                   //!< Minor radius
};

#endif // _ExtremaPS_Torus_HeaderFile
