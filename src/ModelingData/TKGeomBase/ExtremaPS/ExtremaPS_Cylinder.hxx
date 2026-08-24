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

#ifndef _ExtremaPS_Cylinder_HeaderFile
#define _ExtremaPS_Cylinder_HeaderFile

#include <ExtremaPS.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Point-cylinder extrema computation.
//!
//! Computes the extrema between a 3D point and a cylindrical surface.
//!
//! Algorithm:
//! 1. Project point onto the cylinder axis to get V parameter
//! 2. Project point radially to get U parameter (angle)
//! 3. This gives one minimum (closest point on cylinder)
//! 4. The antipodal point (U + PI) gives one maximum
//!
//! The domain is fixed at construction time for optimal performance with multiple queries.
//!
//! Two methods are provided:
//! - `Perform()` - finds interior extrema only (min and/or max on cylinder)
//! - `PerformWithBoundary()` - includes edge and corner extrema for bounded domains
//!
//! @note For a point on the cylinder axis, there are infinite solutions
//!       at all U values for the same V - returns InfiniteSolutions status.
//!
//! @note The U parameter is periodic with period 2*PI.
class ExtremaPS_Cylinder
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with cylinder geometry (uses natural unbounded domain).
  //! @param[in] theCylinder the cylinder to compute extrema for
  Standard_EXPORT explicit ExtremaPS_Cylinder(const gp_Cylinder& theCylinder);

  //! Constructor with cylinder geometry and parameter domain.
  //! @param[in] theCylinder the cylinder to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPS_Cylinder(const gp_Cylinder&         theCylinder,
                                     const ExtremaPS::Domain2D& theDomain);

private:
  //! Check if domain is natural (full U period and infinite V).
  static bool isNaturalDomain(const ExtremaPS::Domain2D& theDomain);

private:
  //! Initialize cached components.
  void initCache();

public:
  //! Find interior extrema only (min and/or max on cylinder surface).
  //! Uses domain specified at construction time.
  //!
  //! For a cylinder, there are two interior extrema:
  //! - Minimum: closest point (radial projection)
  //! - Maximum: antipodal point (opposite side of cylinder)
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for boundary check
  //! @param[in] theMode search mode (MinMax, Min, Max)
  //! @return const reference to result with interior extrema only
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Find extrema including boundary edges and corners.
  //! Uses domain specified at construction time.
  //!
  //! Adds boundary extrema for truly bounded domains or when the V projection
  //! falls outside the V range.
  //!
  //! @param[in] theP query point
  //! @param[in] theTol tolerance
  //! @param[in] theMode search mode
  //! @return const reference to result with interior + boundary extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& PerformWithBoundary(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPS::SearchMode theMode = ExtremaPS::SearchMode::MinMax) const;

  //! Returns the cylinder geometry.
  const gp_Cylinder& Cylinder() const { return myCylinder; }

  //! Returns true if domain is bounded (not natural domain).
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the optional bounded parameter domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  gp_Cylinder                        myCylinder; //!< Cylinder geometry
  std::optional<ExtremaPS::Domain2D> myDomain;   //!< Parameter domain (nullopt for natural)
  mutable ExtremaPS::Result          myResult;   //!< Reusable result storage

  double myLocX, myLocY, myLocZ;    //!< Cylinder axis location
  double myAxisX, myAxisY, myAxisZ; //!< Cylinder axis direction
  double myXDirX, myXDirY, myXDirZ; //!< Cylinder X direction
  double myYDirX, myYDirY, myYDirZ; //!< Cylinder Y direction
  double myRadius;                  //!< Cylinder radius
};

#endif // _ExtremaPS_Cylinder_HeaderFile
