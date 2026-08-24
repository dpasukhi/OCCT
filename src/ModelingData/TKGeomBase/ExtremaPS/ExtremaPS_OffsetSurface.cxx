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

#include <ExtremaPS_OffsetSurface.hxx>


//==================================================================================================

ExtremaPS_OffsetSurface::ExtremaPS_OffsetSurface(const occ::handle<Geom_OffsetSurface>& theSurface)
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

ExtremaPS_OffsetSurface::ExtremaPS_OffsetSurface(const occ::handle<Geom_OffsetSurface>& theSurface,
                                                 const ExtremaPS::Domain2D&             theDomain)
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

void ExtremaPS_OffsetSurface::buildGrid()
{
  if (mySurface.IsNull())
  {
    return;
  }

  NCollection_Array1<double> aUParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.UMin,
                                                myDomain.UMax,
                                                ExtremaPS::THE_DEFAULT_NB_SAMPLES);
  NCollection_Array1<double> aVParams =
    ExtremaPS_GridEvaluator::BuildUniformParams(myDomain.VMin,
                                                myDomain.VMax,
                                                ExtremaPS::THE_DEFAULT_NB_SAMPLES);

  GeomGridEval_OffsetSurface                     anEval(mySurface);
  const NCollection_Array2<GeomGridEval::SurfD1> aGrid =
    anEval.EvaluateGridD1(aUParams, aVParams);
  myEvaluator.SetGrid(aGrid, aUParams, aVParams);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_OffsetSurface::Perform(const gp_Pnt&               theP,
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

const ExtremaPS::Result& ExtremaPS_OffsetSurface::PerformWithBoundary(
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
