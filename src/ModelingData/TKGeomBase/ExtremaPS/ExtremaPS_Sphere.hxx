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

#ifndef _ExtremaPS_Sphere_HeaderFile
#define _ExtremaPS_Sphere_HeaderFile

#include <ExtremaPS.hxx>
#include <gp_Pnt.hxx>
#include <gp_Sphere.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Point-sphere extrema computation.
//!
//! Computes the extrema between a 3D point and a spherical surface.
//!
//! Algorithm:
//! 1. Compute direction from sphere center to point
//! 2. Closest point is at center + R * direction (minimum)
//! 3. Farthest point is at center - R * direction (maximum)
//!
//! The domain is fixed at construction time for optimal performance with multiple queries.
//!
//! Two methods are provided:
//! - `Perform()` - finds interior extrema only (min and max on sphere)
//! - `PerformWithBoundary()` - includes edge and corner extrema for bounded domains
//!
//! @note For a point at the sphere center, there are infinite solutions
//!       at all (U, V) - returns InfiniteSolutions status.
//!
//! @note At the poles (V = +/- PI/2), the U parameter is undefined.
//!       The implementation uses U = 0 for poles.
class ExtremaPS_Sphere
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with sphere geometry (uses full sphere domain).
  //! @param[in] theSphere the sphere to compute extrema for
  Standard_EXPORT explicit ExtremaPS_Sphere(const gp_Sphere& theSphere);

  //! Constructor with sphere geometry and parameter domain.
  //! @param[in] theSphere the sphere to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPS_Sphere(const gp_Sphere&           theSphere,
                                   const ExtremaPS::Domain2D& theDomain);

private:
  //! Check if domain is natural (full sphere).
  static bool isNaturalDomain(const ExtremaPS::Domain2D& theDomain);

private:
  //! Initialize cached components.
  void initCache();

public:
  //! Find interior extrema only (min and max on sphere surface).
  //! Uses domain specified at construction time.
  //!
  //! For a sphere, there are two interior extrema:
  //! - Minimum: closest point (toward query point)
  //! - Maximum: antipodal point (away from query point)
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
  //! Adds boundary extrema for truly bounded domains.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance
  //! @param[in] theMode search mode
  //! @return const reference to result with interior + boundary extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& PerformWithBoundary(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Returns the sphere geometry.
  const gp_Sphere& Sphere() const { return mySphere; }

  //! Returns true if domain is bounded (not natural domain).
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the optional bounded parameter domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  gp_Sphere                          mySphere; //!< Sphere geometry
  std::optional<ExtremaPS::Domain2D> myDomain; //!< Parameter domain (nullopt for natural)
  mutable ExtremaPS::Result          myResult; //!< Reusable result storage

  double myCenterX, myCenterY, myCenterZ; //!< Sphere center
  double myAxisX, myAxisY, myAxisZ;       //!< Sphere axis (Z direction)
  double myXDirX, myXDirY, myXDirZ;       //!< Sphere X direction
  double myYDirX, myYDirY, myYDirZ;       //!< Sphere Y direction
  double myRadius;                        //!< Sphere radius
};

#endif // _ExtremaPS_Sphere_HeaderFile
