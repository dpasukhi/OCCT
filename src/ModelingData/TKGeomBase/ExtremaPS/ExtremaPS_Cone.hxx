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

#ifndef _ExtremaPS_Cone_HeaderFile
#define _ExtremaPS_Cone_HeaderFile

#include <ExtremaPS.hxx>
#include <gp_Cone.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! Point-cone extrema computation.
//!
//! Computes the extrema between a 3D point and a conical surface.
//!
//! Algorithm:
//! 1. Project point onto cone axis to find axial position
//! 2. For each axial position, compute the circle radius at that V
//! 3. Find U parameter (angle) by projecting onto the circle
//! 4. Minimum is at closest point, maximum at antipodal
//!
//! Two methods are provided:
//! - `Perform()` - finds interior extrema only
//! - `PerformWithBoundary()` - includes edge and corner extrema for bounded domains
//!
//! @note For a point on the cone axis, there are infinite solutions
//!       at all U values for the corresponding V.
//!
//! @note The cone has a singularity at its apex (V = -RefRadius/tan(SemiAngle)).
class ExtremaPS_Cone
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with cone geometry (uses natural unbounded domain).
  //! @param[in] theCone the cone to compute extrema for
  Standard_EXPORT explicit ExtremaPS_Cone(const gp_Cone& theCone);

  //! Constructor with cone geometry and parameter domain.
  //! @param[in] theCone the cone to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPS_Cone(const gp_Cone& theCone, const ExtremaPS::Domain2D& theDomain);

private:
  //! Check if domain is natural (full U period and infinite V).
  static bool isNaturalDomain(const ExtremaPS::Domain2D& theDomain);

private:
  //! Initialize cached components.
  void initCache();

public:
  //! Find interior extrema only.
  //! Uses domain specified at construction time.
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

  //! Returns the cone geometry.
  const gp_Cone& Cone() const { return myCone; }

  //! Returns true if domain is bounded (not natural domain).
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the optional bounded parameter domain.
  const std::optional<ExtremaPS::Domain2D>& Domain() const { return myDomain; }

private:
  gp_Cone                            myCone;   //!< Cone geometry
  std::optional<ExtremaPS::Domain2D> myDomain; //!< Parameter domain (nullopt for natural)
  mutable ExtremaPS::Result          myResult; //!< Reusable result storage

  double myLocX, myLocY, myLocZ;    //!< Cone axis location
  double myAxisX, myAxisY, myAxisZ; //!< Cone axis direction
  double myXDirX, myXDirY, myXDirZ; //!< Cone X direction
  double myYDirX, myYDirY, myYDirZ; //!< Cone Y direction
  double myRefRadius;               //!< Reference radius
  double mySemiAngle;               //!< Semi-angle
  double myTanAngle;                //!< tan(semi-angle)
  double myCosAngle;                //!< cos(semi-angle)
  double mySinAngle;                //!< sin(semi-angle)
  double myApexV;                   //!< Apex V parameter
};

#endif // _ExtremaPS_Cone_HeaderFile
