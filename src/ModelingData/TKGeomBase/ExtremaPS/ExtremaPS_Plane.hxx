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

#ifndef _ExtremaPS_Plane_HeaderFile
#define _ExtremaPS_Plane_HeaderFile

#include <ExtremaPS.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Point-plane extrema computation.
//!
//! Computes the extrema between a 3D point and a plane.
//! For a plane, there is exactly one extremum - the orthogonal projection.
//!
//! The projection point is computed as:
//!   P_proj = P - ((P - P0) . N) * N
//! where P0 is any point on the plane and N is the unit normal.
//!
//! The domain is fixed at construction time for optimal performance with multiple queries.
//!
//! Two methods are provided:
//! - `Perform()` - finds interior extrema only (the orthogonal projection)
//! - `PerformWithBoundary()` - includes edge and corner extrema for bounded domains
//!
//! @note If the point lies exactly on the plane, the distance is zero
//!       but this is still a valid single extremum (not infinite solutions).
class ExtremaPS_Plane
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with plane geometry (unbounded domain).
  //! @param[in] thePlane the plane to compute extrema for
  Standard_EXPORT explicit ExtremaPS_Plane(const gp_Pln& thePlane);

  //! Constructor with plane geometry and parameter domain.
  //! @param[in] thePlane the plane to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPS_Plane(const gp_Pln& thePlane, const ExtremaPS::Domain2D& theDomain);

private:
  //! Initialize cached components.
  void initCache();

public:
  //! Find interior extrema only (orthogonal projection).
  //! Uses domain specified at construction time.
  //!
  //! For a plane, there is exactly one interior extremum - the projection point.
  //! This is the minimum distance point. Maximum distances only occur at boundaries.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for boundary check
  //! @param[in] theMode search mode (MinMax, Min, Max)
  //! @return const reference to result with interior extrema only
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Find extrema including the boundary.
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

  //! Returns the plane geometry.
  const gp_Pln& Plane() const { return myPlane; }

  //! Returns true if domain is bounded.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the optional bounded parameter domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  gp_Pln                             myPlane;  //!< Plane geometry
  std::optional<ExtremaPS::Domain2D> myDomain; //!< Parameter domain (nullopt for unbounded)
  mutable ExtremaPS::Result          myResult; //!< Reusable result storage

  double myLocX, myLocY, myLocZ;    //!< Plane location
  double myNormX, myNormY, myNormZ; //!< Plane normal
  double myXDirX, myXDirY, myXDirZ; //!< Plane X direction
  double myYDirX, myYDirY, myYDirZ; //!< Plane Y direction
};

#endif // _ExtremaPS_Plane_HeaderFile
