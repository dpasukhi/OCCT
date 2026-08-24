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

#include <ExtremaPS_OtherSurface.hxx>

#include <math_Vector.hxx>

//==================================================================================================

ExtremaPS_OtherSurface::ExtremaPS_OtherSurface(const occ::handle<Geom_Surface>& theSurface)
    : mySurface(theSurface)
{
  if (mySurface.IsNull())
  {
    return;
  }
  myAdaptor.Load(mySurface);
  myDomain = ExtremaPS::Domain2D(myAdaptor.FirstUParameter(),
                                 myAdaptor.LastUParameter(),
                                 myAdaptor.FirstVParameter(),
                                 myAdaptor.LastVParameter());
  buildGrid();
}

//==================================================================================================

ExtremaPS_OtherSurface::ExtremaPS_OtherSurface(const occ::handle<Geom_Surface>& theSurface,
                                               const ExtremaPS::Domain2D&       theDomain)
    : mySurface(theSurface),
      myDomain(theDomain)
{
  if (!mySurface.IsNull())
  {
    myAdaptor = GeomAdaptor_Surface(mySurface);
  }
  buildGrid();
}

//==================================================================================================

void ExtremaPS_OtherSurface::buildGrid()
{
  if (mySurface.IsNull())
  {
    return;
  }

  math_Vector aUParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.UMin,
                                                myDomain.UMax,
                                                ExtremaPS::THE_DEFAULT_NB_SAMPLES);
  math_Vector aVParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.VMin,
                                                myDomain.VMax,
                                                ExtremaPS::THE_DEFAULT_NB_SAMPLES);

  GeomGridEval_OtherSurface                      anEval(&myAdaptor);
  const NCollection_Array2<GeomGridEval::SurfD1> aGrid =
    anEval.EvaluateGridD1(aUParams.Array1(), aVParams.Array1());
  myEvaluator.SetGrid(aGrid, aUParams, aVParams);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_OtherSurface::Perform(const gp_Pnt&               theP,
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

const ExtremaPS::Result& ExtremaPS_OtherSurface::PerformWithBoundary(
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
