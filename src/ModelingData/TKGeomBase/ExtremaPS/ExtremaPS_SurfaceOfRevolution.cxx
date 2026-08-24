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

#include <ExtremaPS_SurfaceOfRevolution.hxx>

#include <ElCLib.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>

#include <algorithm>
#include <cmath>

namespace
{
bool adjustPeriodicParameter(double&                                   theU,
                             const std::optional<ExtremaPS::Domain2D>& theDomain,
                             const double                              theTolerance)
{
  if (!theDomain.has_value())
  {
    theU = ElCLib::InPeriod(theU, 0.0, MathUtils::THE_2PI);
    return true;
  }

  const ExtremaPS::Domain2D& aDomain = *theDomain;
  if (!aDomain.IsUFullPeriod(MathUtils::THE_2PI, theTolerance)
      && !ExtremaPS::IsInPeriodicRange(theU, aDomain.U(), theTolerance))
  {
    return false;
  }
  theU = ElCLib::InPeriod(theU, aDomain.UMin, aDomain.UMin + MathUtils::THE_2PI);
  theU = aDomain.U().Clamp(theU);
  return true;
}
} // namespace

//==================================================================================================

ExtremaPS_SurfaceOfRevolution::ExtremaPS_SurfaceOfRevolution(
  const occ::handle<Geom_SurfaceOfRevolution>& theSurface)
    : mySurface(theSurface),
      myDomain(std::nullopt)
{
  initCache();
}

//==================================================================================================

ExtremaPS_SurfaceOfRevolution::ExtremaPS_SurfaceOfRevolution(
  const occ::handle<Geom_SurfaceOfRevolution>& theSurface,
  const ExtremaPS::Domain2D&                   theDomain)
    : mySurface(theSurface),
      myDomain(theDomain)
{
  initCache();
}

//==================================================================================================

void ExtremaPS_SurfaceOfRevolution::initCache()
{
  if (mySurface.IsNull())
  {
    return;
  }

  const occ::handle<Adaptor3d_Curve> aBasisAdaptor = new GeomAdaptor_Curve(mySurface->BasisCurve());
  myAdaptor = GeomAdaptor_SurfaceOfRevolution(aBasisAdaptor, mySurface->Axis());

  double aVMin = aBasisAdaptor->FirstParameter();
  double aVMax = aBasisAdaptor->LastParameter();
  if (myDomain.has_value())
  {
    aVMin = std::max(aVMin, myDomain->VMin);
    aVMax = std::min(aVMax, myDomain->VMax);
  }
  if (aVMax - aVMin <= Precision::PConfusion())
  {
    return;
  }

  myCurveExtrema.emplace(*aBasisAdaptor, aVMin, aVMax);
}

//==================================================================================================

void ExtremaPS_SurfaceOfRevolution::appendMeridianExtrema(const ExtremaPC::Result& theCurveResult,
                                                          const gp_Pnt&            thePoint,
                                                          const double             theU,
                                                          const double             theAngularSign,
                                                          const double             theTolerance,
                                                          const ExtremaPS::SearchMode theMode) const
{
  const gp_Ax1  anAxis           = myAdaptor.AxeOfRevolution();
  const gp_XYZ& anAxisDirection  = anAxis.Direction().XYZ();
  const gp_XYZ& anAxisLocation   = anAxis.Location().XYZ();
  const gp_XYZ& aMeridian        = myAdaptor.Axis().XDirection().XYZ();
  const double  aSquareTolerance = theTolerance * theTolerance;

  for (size_t anIndex = 0; anIndex < theCurveResult.Extrema.Size(); ++anIndex)
  {
    const ExtremaPC::ExtremumResult& aCurveExtremum = theCurveResult.Extrema.Value(anIndex);
    const gp_XYZ                     aDelta         = aCurveExtremum.Point.XYZ() - anAxisLocation;
    const double                     anAxial        = aDelta.Dot(anAxisDirection);
    const double aSignedRadius      = (aDelta - anAxial * anAxisDirection).Dot(aMeridian);
    const double anAngularCurvature = theAngularSign * aSignedRadius;
    const bool   isMinimum          = aCurveExtremum.IsMinimum && anAngularCurvature > theTolerance;
    const bool   isMaximum = aCurveExtremum.IsMaximum && anAngularCurvature < -theTolerance;
    if ((!isMinimum && !isMaximum) || (theMode == ExtremaPS::SearchMode::Min && !isMinimum)
        || (theMode == ExtremaPS::SearchMode::Max && !isMaximum))
    {
      continue;
    }

    const gp_Pnt aSurfacePoint = myAdaptor.EvalD0(theU, aCurveExtremum.Parameter);
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
    anExtremum.U              = theU;
    anExtremum.V              = aCurveExtremum.Parameter;
    anExtremum.Point          = aSurfacePoint;
    anExtremum.SquareDistance = thePoint.SquareDistance(aSurfacePoint);
    anExtremum.IsMinimum      = isMinimum;
    myResult.Extrema.Append(anExtremum);
  }
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_SurfaceOfRevolution::Perform(
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
  if (mySurface.IsNull() || !myCurveExtrema.has_value())
  {
    myResult.Status = ExtremaPS::Status::NotDone;
    return myResult;
  }

  const gp_Ax1  anAxis          = myAdaptor.AxeOfRevolution();
  const gp_XYZ& anAxisDirection = anAxis.Direction().XYZ();
  const gp_XYZ  aDelta          = theP.XYZ() - anAxis.Location().XYZ();
  const double  anAxial         = aDelta.Dot(anAxisDirection);
  const gp_XYZ  aQueryRadial    = aDelta - anAxial * anAxisDirection;
  const double  aQueryRadius    = std::sqrt(aQueryRadial.SquareModulus());

  if (aQueryRadius <= theTol)
  {
    const gp_Pnt             anAxisPoint(anAxis.Location().XYZ() + anAxial * anAxisDirection);
    const ExtremaPC::Result& aCurveResult = myCurveExtrema->Perform(anAxisPoint, theTol, theMode);
    if (aCurveResult.IsInfinite())
    {
      myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = aCurveResult.InfiniteSquareDistance;
    }
    else if (aCurveResult.IsDone() && !aCurveResult.Extrema.IsEmpty())
    {
      myResult.Status                 = ExtremaPS::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = theMode == ExtremaPS::SearchMode::Max
                                          ? aCurveResult.MaxSquareDistance()
                                          : aCurveResult.MinSquareDistance();
    }
    else if (aCurveResult.IsDone())
    {
      myResult.Status = ExtremaPS::Status::NoSolution;
    }
    else
    {
      myResult.Status = aCurveResult.Status == ExtremaPC::Status::InvalidInput
                          ? ExtremaPS::Status::InvalidInput
                          : ExtremaPS::Status::NumericalError;
    }
    return myResult;
  }

  ExtremaPC::SearchMode aCurveMode = ExtremaPC::SearchMode::MinMax;
  if (theMode == ExtremaPS::SearchMode::Min)
  {
    aCurveMode = ExtremaPC::SearchMode::Min;
  }
  else if (theMode == ExtremaPS::SearchMode::Max)
  {
    aCurveMode = ExtremaPC::SearchMode::Max;
  }

  const gp_XYZ& aMeridian = myAdaptor.Axis().XDirection().XYZ();
  double        aU =
    std::atan2(anAxisDirection.Dot(aMeridian.Crossed(aQueryRadial)), aMeridian.Dot(aQueryRadial));
  const gp_Pnt aMeridianPoint(anAxis.Location().XYZ() + anAxial * anAxisDirection
                              + aQueryRadius * aMeridian);
  if (adjustPeriodicParameter(aU, myDomain, theTol))
  {
    const ExtremaPC::Result& aCurveResult =
      myCurveExtrema->Perform(aMeridianPoint, theTol, aCurveMode);
    if (!aCurveResult.IsDone())
    {
      myResult.Status = ExtremaPS::Status::NumericalError;
      return myResult;
    }
    appendMeridianExtrema(aCurveResult, theP, aU, 1.0, theTol, theMode);
  }

  double aUOpposite = aU + MathUtils::THE_PI;
  if (adjustPeriodicParameter(aUOpposite, myDomain, theTol))
  {
    const gp_Pnt             anOppositePoint(anAxis.Location().XYZ() + anAxial * anAxisDirection
                                             - aQueryRadius * aMeridian);
    const ExtremaPC::Result& anOppositeResult =
      myCurveExtrema->Perform(anOppositePoint, theTol, aCurveMode);
    if (!anOppositeResult.IsDone())
    {
      myResult.Clear();
      myResult.Status = ExtremaPS::Status::NumericalError;
      return myResult;
    }
    appendMeridianExtrema(anOppositeResult, theP, aUOpposite, -1.0, theTol, theMode);
  }

  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_SurfaceOfRevolution::PerformWithBoundary(
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
  myResult.Status =
    myResult.Extrema.IsEmpty() ? ExtremaPS::Status::NoSolution : ExtremaPS::Status::OK;
  return myResult;
}
