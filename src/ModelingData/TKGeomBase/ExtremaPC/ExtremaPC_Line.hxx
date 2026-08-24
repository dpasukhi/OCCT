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

#ifndef _ExtremaPC_Line_HeaderFile
#define _ExtremaPC_Line_HeaderFile

#include <ExtremaPC.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <Standard_DefineAlloc.hxx>

#include <optional>

//! @brief Point-Line extrema computation.
//!
//! Computes the extremum (closest point) between a 3D point and a line.
//! Uses direct analytical projection via dot product.
//!
//! For a line defined by origin O and direction D, the closest point
//! to P is at parameter u = (P - O) . D, which gives the minimum distance.
//!
//! The domain is fixed at construction time for optimal performance.
//! For unbounded line, construct without domain or with nullopt.
//!
//! @note Lines always have exactly one extremum (minimum) if within bounds.
class ExtremaPC_Line
{
public:
  DEFINE_STANDARD_ALLOC

  //! Constructor with line geometry (unbounded).
  //! @param[in] theLine the line to compute extrema for
  Standard_EXPORT explicit ExtremaPC_Line(const gp_Lin& theLine);

  //! Constructor with line geometry and parameter domain.
  //! @param[in] theLine the line to compute extrema for
  //! @param[in] theDomain parameter domain (fixed for all queries)
  Standard_EXPORT ExtremaPC_Line(const gp_Lin& theLine, const ExtremaPC::Domain1D& theDomain);

  //! Copy constructor is deleted.
  ExtremaPC_Line(const ExtremaPC_Line&) = delete;

  //! Copy assignment operator is deleted.
  ExtremaPC_Line& operator=(const ExtremaPC_Line&) = delete;

  //! Move constructor.
  ExtremaPC_Line(ExtremaPC_Line&&) = default;

  //! Move assignment operator.
  ExtremaPC_Line& operator=(ExtremaPC_Line&&) = default;

  //! Evaluates a point on the line.
  Standard_EXPORT gp_Pnt EvalD0(const double theU) const;

  //! Returns true if domain is bounded.
  bool IsBounded() const { return myDomain.has_value(); }

  //! Returns the domain (only valid if IsBounded() is true).
  const ExtremaPC::Domain1D& Domain() const { return *myDomain; }

  //! Compute extrema between point P and the line.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for parameter comparison
  //! @param[in] theMode search mode (Max returns no interior extremum)
  //! @return const reference to result containing the extremum
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Compute extrema between point P and the line segment including endpoints.
  //! Uses domain specified at construction time.
  //! @param[in] theP query point
  //! @param[in] theTol tolerance for parameter comparison
  //! @param[in] theMode search mode (MinMax, Min, or Max)
  //! @return const reference to result containing interior + endpoint extrema
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& PerformWithEndpoints(
    const gp_Pnt&               theP,
    const double                theTol,
    const ExtremaPC::SearchMode theMode = ExtremaPC::SearchMode::MinMax) const;

  //! Returns the line geometry.
  const gp_Lin& Line() const { return myLine; }

private:
  gp_Lin                             myLine;   //!< Line geometry
  std::optional<ExtremaPC::Domain1D> myDomain; //!< Parameter domain (nullopt for unbounded)
  mutable ExtremaPC::Result          myResult; //!< Reusable result storage
};

#endif // _ExtremaPC_Line_HeaderFile
