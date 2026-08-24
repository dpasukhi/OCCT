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

#include <ExtremaPS_SurfaceOfExtrusion.hxx>

#include <ExtremaPC_Line.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>
#include <gp.hxx>
#include <gp_Ax3.hxx>

#include <algorithm>

//==================================================================================================

ExtremaPS_SurfaceOfExtrusion::ExtremaPS_SurfaceOfExtrusion(
  const occ::handle<Geom_SurfaceOfLinearExtrusion>& theSurface)
    : mySurface(theSurface),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_SurfaceOfExtrusion::ExtremaPS_SurfaceOfExtrusion(
  const occ::handle<Geom_SurfaceOfLinearExtrusion>& theSurface,
  const ExtremaPS::Domain2D&                        theDomain)
    : mySurface(theSurface),
      myDomain(theDomain)
{
  initCache();
}

//==================================================================================================

void ExtremaPS_SurfaceOfExtrusion::initCache()
{
  if (mySurface.IsNull())
  {
    return;
  }
  const occ::handle<Adaptor3d_Curve> aBasisAdaptor = new GeomAdaptor_Curve(mySurface->BasisCurve());
  myAdaptor = GeomAdaptor_SurfaceOfLinearExtrusion(aBasisAdaptor, mySurface->Direction());

  double aUMin = aBasisAdaptor->FirstParameter();
  double aUMax = aBasisAdaptor->LastParameter();
  if (myDomain.has_value())
  {
    aUMin = std::max(aUMin, myDomain->UMin);
    aUMax = std::min(aUMax, myDomain->UMax);
  }
  if (!MathUtils::IsFinite(aUMin) || !MathUtils::IsFinite(aUMax)
      || aUMax - aUMin <= Precision::PConfusion())
  {
    return;
  }

  myProjectedCurve = ProjLib_ProjectOnPlane(gp_Ax3(gp::Origin(), mySurface->Direction()));
  myProjectedCurve.Load(aBasisAdaptor, Precision::Approximation(), true);
  myCurveExtrema.emplace(myProjectedCurve, aUMin, aUMax);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_SurfaceOfExtrusion::Perform(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsFinitePoint(theP) || !ExtremaPC::IsValidTolerance(theTol)
      || (myDomain.has_value() && !myDomain->IsValid()))
  {
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  if (mySurface.IsNull())
  {
    myResult.Status = ExtremaPS::Status::NotDone;
    return myResult;
  }

  if (theMode == ExtremaPS::SearchMode::Max)
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
    return myResult;
  }

  if (!myCurveExtrema.has_value())
  {
    const occ::handle<Adaptor3d_Curve> aBasisAdaptor = myAdaptor.BasisCurve();
    if (myDomain.has_value() || aBasisAdaptor.IsNull() || aBasisAdaptor->GetType() != GeomAbs_Line)
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
      return myResult;
    }

    const gp_Lin& aLine        = aBasisAdaptor->Line();
    const gp_XYZ& aLineDir     = aLine.Direction().XYZ();
    const gp_XYZ& aDirection   = myAdaptor.Direction().XYZ();
    const double  aCoupling    = aLineDir.Dot(aDirection);
    const double  aDeterminant = 1.0 - aCoupling * aCoupling;
    if (aDeterminant <= Precision::Angular() * Precision::Angular())
    {
      ExtremaPC_Line           aLineExtrema(aLine);
      const ExtremaPC::Result& aLineResult =
        aLineExtrema.Perform(theP, theTol, ExtremaPC::SearchMode::Min);
      if (!aLineResult.IsDone() || aLineResult.Extrema.IsEmpty())
      {
        myResult.Status = ExtremaPS::Status::NumericalError;
        return myResult;
      }
      myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = aLineResult.MinSquareDistance();
      return myResult;
    }

    const gp_XYZ aDelta                = theP.XYZ() - aLine.Location().XYZ();
    const double aLineProjection       = aDelta.Dot(aLineDir);
    const double anExtrusionProjection = aDelta.Dot(aDirection);
    const double aU = (aLineProjection - aCoupling * anExtrusionProjection) / aDeterminant;
    const double aV = (anExtrusionProjection - aCoupling * aLineProjection) / aDeterminant;
    const gp_Pnt aSurfacePoint = myAdaptor.EvalD0(aU, aV);
    myResult.Extrema.Append(
      ExtremaPS::ExtremumResult{aU, aV, aSurfacePoint, theP.SquareDistance(aSurfacePoint), true});
    myResult.Status = ExtremaPS::Status::OK;
    return myResult;
  }

  const gp_XYZ             aDirection       = myAdaptor.Direction().XYZ();
  const double             aPointProjection = theP.XYZ().Dot(aDirection);
  const gp_Pnt             aProjectedPoint(theP.XYZ() - aPointProjection * aDirection);
  const ExtremaPC::Result& aCurveResult =
    myCurveExtrema->Perform(aProjectedPoint, theTol, ExtremaPC::SearchMode::Min);
  if (aCurveResult.IsInfinite())
  {
    myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
    myResult.InfiniteSquareDistance = aCurveResult.InfiniteSquareDistance;
    return myResult;
  }
  if (!aCurveResult.IsDone())
  {
    myResult.Status = aCurveResult.Status == ExtremaPC::Status::InvalidInput
                        ? ExtremaPS::Status::InvalidInput
                        : ExtremaPS::Status::NumericalError;
    return myResult;
  }

  const double                       aSquareTolerance = theTol * theTol;
  const occ::handle<Adaptor3d_Curve> aBasisAdaptor    = myAdaptor.BasisCurve();
  for (size_t anIndex = 0; anIndex < aCurveResult.Extrema.Size(); ++anIndex)
  {
    const ExtremaPC::ExtremumResult& aCurveExtremum = aCurveResult.Extrema.Value(anIndex);
    if (!aCurveExtremum.IsMinimum)
    {
      continue;
    }
    const gp_Pnt aBasisPoint = aBasisAdaptor->EvalD0(aCurveExtremum.Parameter);
    double       aV          = (theP.XYZ() - aBasisPoint.XYZ()).Dot(aDirection);
    if (myDomain.has_value() && !myDomain->V().Contains(aV, theTol))
    {
      continue;
    }
    if (myDomain.has_value())
    {
      aV = myDomain->V().Clamp(aV);
    }
    const gp_Pnt aSurfacePoint = myAdaptor.EvalD0(aCurveExtremum.Parameter, aV);
    bool         isDuplicate   = false;
    for (size_t aResultIndex = 0; aResultIndex < myResult.Extrema.Size(); ++aResultIndex)
    {
      if (myResult.Extrema.Value(aResultIndex).Point.SquareDistance(aSurfacePoint)
          <= aSquareTolerance)
      {
        isDuplicate = true;
        break;
      }
    }
    if (isDuplicate)
    {
      continue;
    }

    ExtremaPS::ExtremumResult anExtremum;
    anExtremum.U              = aCurveExtremum.Parameter;
    anExtremum.V              = aV;
    anExtremum.Point          = aSurfacePoint;
    anExtremum.SquareDistance = theP.SquareDistance(aSurfacePoint);
    anExtremum.IsMinimum      = true;
    myResult.Extrema.Append(anExtremum);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_SurfaceOfExtrusion::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result& anInteriorResult = Perform(theP, theTol, theMode);
  if (!anInteriorResult.IsDone() || !myDomain.has_value())
  {
    return anInteriorResult;
  }

  ExtremaPS::AddBoundaryExtrema(myResult, theP, *myDomain, myAdaptor, theTol, theMode);

  if (myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
  }
  else
  {
    myResult.Status = ExtremaPS::Status::OK;
  }

  return myResult;
}
