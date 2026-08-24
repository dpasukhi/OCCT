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

#include <ExtremaPC_Line.hxx>

#include <ExtremaPC_Planar.hxx>
#include <ExtremaPC2d_Line.hxx>
#include <gp_Vec.hxx>
#include <ProjLib.hxx>

#include <algorithm>

ExtremaPC_Line::ExtremaPC_Line(const gp_Lin& theLine)
    : myLine(theLine),
      myDomain(std::nullopt)
{
}

//==================================================================================================

ExtremaPC_Line::ExtremaPC_Line(const gp_Lin& theLine, const ExtremaPC::Domain1D& theDomain)
    : myLine(theLine),
      myDomain(theDomain)
{
}

//==================================================================================================

gp_Pnt ExtremaPC_Line::EvalD0(const double theU) const
{
  return myLine.Location().Translated(theU * gp_Vec(myLine.Direction()));
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Line::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return myResult;
  }

  const gp_Pln aPlane = ExtremaPC::LinePlane(myLine);
  if (ExtremaPC::PerformPlanar<ExtremaPC2d_Line>(ProjLib::Project(aPlane, myLine),
                                                 myDomain,
                                                 ProjLib::Project(aPlane, theP),
                                                 theP,
                                                 aPlane,
                                                 *this,
                                                 theTol,
                                                 theMode,
                                                 false,
                                                 myResult))
  {
    return myResult;
  }

  if (theMode == ExtremaPC::SearchMode::Max)
  {
    myResult.Status = ExtremaPC::Status::NoSolution;
    return myResult;
  }

  // Compute projection parameter: u = (P - O) . Direction
  const gp_Dir& aDir    = myLine.Direction();
  const gp_Pnt& aOrigin = myLine.Location();
  gp_Vec        aVec(aOrigin, theP);
  double        aU = aVec.Dot(gp_Vec(aDir));

  // Check bounds if domain is specified
  if (myDomain.has_value())
  {
    if (aU < myDomain->Min - theTol || aU > myDomain->Max + theTol)
    {
      // Projection is outside bounds - no interior extremum
      myResult.Status = ExtremaPC::Status::NoSolution;
      return myResult;
    }
    // Clamp to bounds
    aU = std::clamp(aU, myDomain->Min, myDomain->Max);
  }

  // Compute point on line at projected parameter
  gp_Pnt aPtOnLine = aOrigin.Translated(aU * gp_Vec(aDir));

  // Store result
  ExtremaPC::ExtremumResult anExt;
  anExt.Parameter      = aU;
  anExt.Point          = aPtOnLine;
  anExt.SquareDistance = theP.SquareDistance(aPtOnLine);
  anExt.IsMinimum      = true;
  anExt.IsMaximum      = false;

  myResult.Extrema.Append(anExt);
  myResult.Status = ExtremaPC::Status::OK;
  return myResult;
}

//==================================================================================================

[[nodiscard]] const ExtremaPC::Result& ExtremaPC_Line::PerformWithEndpoints(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPC::SearchMode theMode) const
{
  if (!ExtremaPC::IsValidTolerance(theTol) || !ExtremaPC::IsFinitePoint(theP)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Clear();
    myResult.Status = ExtremaPC::Status::InvalidInput;
    return myResult;
  }

  if (myDomain.has_value() && myDomain->Min == myDomain->Max)
  {
    myResult.Clear();
    ExtremaPC::AddEndpointExtrema(myResult, theP, *myDomain, *this, theMode, false);
    myResult.Status =
      myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution : ExtremaPC::Status::OK;
    return myResult;
  }

  const ExtremaPC::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }
  ExtremaPC::AddEndpointExtrema(myResult, theP, *myDomain, *this, theMode, false);
  if (!myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPC::Status::OK;
  }

  return myResult;
}
