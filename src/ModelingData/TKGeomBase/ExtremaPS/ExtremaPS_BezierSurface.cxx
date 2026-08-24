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

#include <ExtremaPS_BezierSurface.hxx>

#include <utility>

//==================================================================================================

ExtremaPS_BezierSurface::ExtremaPS_BezierSurface(const occ::handle<Geom_BezierSurface>& theSurface)
    : mySurface(theSurface),
      myDomain{0.0, 1.0, 0.0, 1.0} // Bezier surfaces always have domain [0,1]x[0,1]
{
  if (mySurface.IsNull())
  {
    return;
  }
  myAdaptor.Load(mySurface);

  buildGrid();
}

//==================================================================================================

ExtremaPS_BezierSurface::ExtremaPS_BezierSurface(const occ::handle<Geom_BezierSurface>& theSurface,
                                                 const ExtremaPS::Domain2D&             theDomain)
    : mySurface(theSurface),
      myDomain(theDomain)
{
  if (mySurface.IsNull())
  {
    return;
  }
  myAdaptor.Load(mySurface);

  buildGrid();
}

//==================================================================================================

void ExtremaPS_BezierSurface::buildGrid()
{
  if (mySurface.IsNull())
  {
    return;
  }

  const size_t aNbUSamples = std::clamp(ExtremaPS::THE_BEZIER_DEGREE_MULTIPLIER
                                          * static_cast<size_t>(mySurface->UDegree() + 1),
                                        ExtremaPS::THE_BEZIER_MIN_SAMPLES,
                                        ExtremaPS::THE_BEZIER_MAX_SAMPLES);
  const size_t aNbVSamples = std::clamp(ExtremaPS::THE_BEZIER_DEGREE_MULTIPLIER
                                          * static_cast<size_t>(mySurface->VDegree() + 1),
                                        ExtremaPS::THE_BEZIER_MIN_SAMPLES,
                                        ExtremaPS::THE_BEZIER_MAX_SAMPLES);

  NCollection_Array1<double> aUParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.UMin, myDomain.UMax, aNbUSamples);
  NCollection_Array1<double> aVParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.VMin, myDomain.VMax, aNbVSamples);

  const GeomGridEval_BezierSurface anEval(mySurface);
  myEvaluator.SetGrid(anEval.EvaluateGridD1Coords(aUParams, aVParams),
                      std::move(aUParams),
                      std::move(aVParams));
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_BezierSurface::Perform(const gp_Pnt&               theP,
                                                          const double                theTol,
                                                          const ExtremaPS::SearchMode theMode) const
{
  if (mySurface.IsNull())
  {
    myEvaluator.ChangeResult().Clear();
    myEvaluator.ChangeResult().Status = ExtremaPS::Status::NotDone;
    return myEvaluator.ChangeResult();
  }
  return myEvaluator.Perform(myAdaptor, theP, myDomain, theTol, theMode);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_BezierSurface::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone())
  {
    return anInteriorResult;
  }
  ExtremaPS::Result& aResult = myEvaluator.ChangeResult();
  ExtremaPS::AddBoundaryExtrema(aResult, theP, myDomain, myAdaptor, theTol, theMode);

  if (!aResult.Extrema.IsEmpty())
  {
    aResult.Status = ExtremaPS::Status::OK;
  }

  return aResult;
}
