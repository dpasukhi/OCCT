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

#include <ExtremaPS.hxx>

#include <Adaptor3d_IsoCurve.hxx>
#include <Adaptor3d_Surface.hxx>
#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <ExtremaPC_Circle.hxx>
#include <ExtremaPC_Curve.hxx>
#include <ExtremaPC_Line.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Pln.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <MathUtils_Core.hxx>
#include <Precision.hxx>

#include <cmath>
#include <limits>

namespace
{
bool canAddBoundary(const ExtremaPS::Domain2D& theDomain,
                    const gp_Pnt&              thePoint,
                    const double               theTolerance)
{
  return theDomain.IsValid() && ExtremaPC::IsFinitePoint(thePoint)
         && ExtremaPC::IsValidTolerance(theTolerance);
}

void appendExtrema(ExtremaPS::Result&       theResult,
                   const ExtremaPC::Result& theCurveResult,
                   const double             theFixedParameter,
                   const bool               theIsUFixed,
                   const double             theTolerance)
{
  const double aSquareTolerance = theTolerance * theTolerance;
  for (size_t anIndex = 0; anIndex < theCurveResult.Extrema.Size(); ++anIndex)
  {
    const ExtremaPC::ExtremumResult& aCurveExtremum = theCurveResult.Extrema.Value(anIndex);
    const double aU = theIsUFixed ? theFixedParameter : aCurveExtremum.Parameter;
    const double aV = theIsUFixed ? aCurveExtremum.Parameter : theFixedParameter;

    bool isDuplicate = false;
    for (size_t aResultIndex = 0; aResultIndex < theResult.Extrema.Size(); ++aResultIndex)
    {
      const ExtremaPS::ExtremumResult& anExisting = theResult.Extrema.Value(aResultIndex);
      if ((std::abs(anExisting.U - aU) < Precision::PConfusion()
           && std::abs(anExisting.V - aV) < Precision::PConfusion())
          || anExisting.Point.SquareDistance(aCurveExtremum.Point) < aSquareTolerance)
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
    anExtremum.U              = aU;
    anExtremum.V              = aV;
    anExtremum.Point          = aCurveExtremum.Point;
    anExtremum.SquareDistance = aCurveExtremum.SquareDistance;
    anExtremum.IsMinimum      = aCurveExtremum.IsMinimum;
    theResult.Extrema.Append(anExtremum);
  }
}

void addLine(ExtremaPS::Result&          theResult,
             const gp_Pnt&               thePoint,
             const gp_Lin&               theLine,
             const MathUtils::Domain1D&  theDomain,
             const double                theFixedParameter,
             const bool                  theIsUFixed,
             const double                theTolerance,
             const ExtremaPS::SearchMode theMode)
{
  ExtremaPC_Line anEvaluator(theLine, theDomain);
  appendExtrema(theResult,
                anEvaluator.PerformWithEndpoints(thePoint, theTolerance, theMode),
                theFixedParameter,
                theIsUFixed,
                theTolerance);
}

void addCircle(ExtremaPS::Result&          theResult,
               const gp_Pnt&               thePoint,
               const gp_Circ&              theCircle,
               const MathUtils::Domain1D&  theDomain,
               const double                theFixedParameter,
               const bool                  theIsUFixed,
               const double                theTolerance,
               const ExtremaPS::SearchMode theMode)
{
  ExtremaPC_Circle anEvaluator(theCircle, theDomain);
  appendExtrema(theResult,
                anEvaluator.PerformWithEndpoints(thePoint, theTolerance, theMode),
                theFixedParameter,
                theIsUFixed,
                theTolerance);
}

void addIso(ExtremaPS::Result&                    theResult,
            const gp_Pnt&                         thePoint,
            const occ::handle<Adaptor3d_Surface>& theSurface,
            const GeomAbs_IsoType                 theIsoType,
            const MathUtils::Domain1D&            theDomain,
            const double                          theFixedParameter,
            const double                          theTolerance,
            const ExtremaPS::SearchMode           theMode)
{
  Adaptor3d_IsoCurve anIso(theSurface, theIsoType, theFixedParameter, theDomain.Min, theDomain.Max);
  ExtremaPC_Curve    anEvaluator(anIso, theDomain.Min, theDomain.Max);
  appendExtrema(theResult,
                anEvaluator.PerformWithEndpoints(thePoint, theTolerance, theMode),
                theFixedParameter,
                theIsoType == GeomAbs_IsoU,
                theTolerance);
}
} // namespace

bool ExtremaPS::IsInPeriodicRange(const double               theParam,
                                  const MathUtils::Domain1D& theDomain,
                                  const double               theTol)
{
  if (!MathUtils::IsFinite(theParam) || !theDomain.IsValid()
      || !ExtremaPC::IsValidTolerance(theTol))
  {
    return false;
  }
  const double aNormalizedParam =
    ElCLib::InPeriod(theParam, theDomain.Min, theDomain.Min + MathUtils::THE_2PI);
  return theDomain.Contains(aNormalizedParam, theTol);
}

//==================================================================================================

double ExtremaPS::Result::MinSquareDistance() const
{
  if (Extrema.IsEmpty())
  {
    return std::numeric_limits<double>::infinity();
  }
  return Extrema.Value(MinIndex()).SquareDistance;
}

size_t ExtremaPS::Result::MinIndex() const
{
  if (Extrema.IsEmpty())
  {
    return INVALID_INDEX;
  }
  size_t aMinIndex = 0;
  for (size_t anIndex = 1; anIndex < Extrema.Size(); ++anIndex)
  {
    if (Extrema.Value(anIndex).SquareDistance < Extrema.Value(aMinIndex).SquareDistance)
    {
      aMinIndex = anIndex;
    }
  }
  return aMinIndex;
}

double ExtremaPS::Result::MaxSquareDistance() const
{
  return Extrema.IsEmpty() ? 0.0 : Extrema.Value(MaxIndex()).SquareDistance;
}

size_t ExtremaPS::Result::MaxIndex() const
{
  if (Extrema.IsEmpty())
  {
    return INVALID_INDEX;
  }
  size_t aMaxIndex = 0;
  for (size_t anIndex = 1; anIndex < Extrema.Size(); ++anIndex)
  {
    if (Extrema.Value(anIndex).SquareDistance > Extrema.Value(aMaxIndex).SquareDistance)
    {
      aMaxIndex = anIndex;
    }
  }
  return aMaxIndex;
}

void ExtremaPS::Result::Clear()
{
  Status = Status::NotDone;
  Extrema.Clear();
  InfiniteSquareDistance = 0.0;
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&          theResult,
                                   const gp_Pnt&    thePoint,
                                   const Domain2D&  theDomain,
                                   const gp_Pln&    thePlane,
                                   const double     theTolerance,
                                   const SearchMode theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const gp_Ax3& aPosition = thePlane.Position();
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addLine(theResult,
            thePoint,
            ElSLib::PlaneUIso(aPosition, theDomain.UMin),
            theDomain.V(),
            theDomain.UMin,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addLine(theResult,
            thePoint,
            ElSLib::PlaneUIso(aPosition, theDomain.UMax),
            theDomain.V(),
            theDomain.UMax,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addLine(theResult,
            thePoint,
            ElSLib::PlaneVIso(aPosition, theDomain.VMin),
            theDomain.U(),
            theDomain.VMin,
            false,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addLine(theResult,
            thePoint,
            ElSLib::PlaneVIso(aPosition, theDomain.VMax),
            theDomain.U(),
            theDomain.VMax,
            false,
            theTolerance,
            theMode);
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&            theResult,
                                   const gp_Pnt&      thePoint,
                                   const Domain2D&    theDomain,
                                   const gp_Cylinder& theCylinder,
                                   const double       theTolerance,
                                   const SearchMode   theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const gp_Ax3& aPosition = theCylinder.Position();
  const double  aRadius   = theCylinder.Radius();
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addLine(theResult,
            thePoint,
            ElSLib::CylinderUIso(aPosition, aRadius, theDomain.UMin),
            theDomain.V(),
            theDomain.UMin,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addLine(theResult,
            thePoint,
            ElSLib::CylinderUIso(aPosition, aRadius, theDomain.UMax),
            theDomain.V(),
            theDomain.UMax,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addCircle(theResult,
              thePoint,
              ElSLib::CylinderVIso(aPosition, aRadius, theDomain.VMin),
              theDomain.U(),
              theDomain.VMin,
              false,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addCircle(theResult,
              thePoint,
              ElSLib::CylinderVIso(aPosition, aRadius, theDomain.VMax),
              theDomain.U(),
              theDomain.VMax,
              false,
              theTolerance,
              theMode);
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&          theResult,
                                   const gp_Pnt&    thePoint,
                                   const Domain2D&  theDomain,
                                   const gp_Cone&   theCone,
                                   const double     theTolerance,
                                   const SearchMode theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const gp_Ax3& aPosition = theCone.Position();
  const double  aRadius   = theCone.RefRadius();
  const double  anAngle   = theCone.SemiAngle();
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addLine(theResult,
            thePoint,
            ElSLib::ConeUIso(aPosition, aRadius, anAngle, theDomain.UMin),
            theDomain.V(),
            theDomain.UMin,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addLine(theResult,
            thePoint,
            ElSLib::ConeUIso(aPosition, aRadius, anAngle, theDomain.UMax),
            theDomain.V(),
            theDomain.UMax,
            true,
            theTolerance,
            theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addCircle(theResult,
              thePoint,
              ElSLib::ConeVIso(aPosition, aRadius, anAngle, theDomain.VMin),
              theDomain.U(),
              theDomain.VMin,
              false,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addCircle(theResult,
              thePoint,
              ElSLib::ConeVIso(aPosition, aRadius, anAngle, theDomain.VMax),
              theDomain.U(),
              theDomain.VMax,
              false,
              theTolerance,
              theMode);
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&          theResult,
                                   const gp_Pnt&    thePoint,
                                   const Domain2D&  theDomain,
                                   const gp_Sphere& theSphere,
                                   const double     theTolerance,
                                   const SearchMode theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const gp_Ax3& aPosition = theSphere.Position();
  const double  aRadius   = theSphere.Radius();
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addCircle(theResult,
              thePoint,
              ElSLib::SphereUIso(aPosition, aRadius, theDomain.UMin),
              theDomain.V(),
              theDomain.UMin,
              true,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addCircle(theResult,
              thePoint,
              ElSLib::SphereUIso(aPosition, aRadius, theDomain.UMax),
              theDomain.V(),
              theDomain.UMax,
              true,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addCircle(theResult,
              thePoint,
              ElSLib::SphereVIso(aPosition, aRadius, theDomain.VMin),
              theDomain.U(),
              theDomain.VMin,
              false,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addCircle(theResult,
              thePoint,
              ElSLib::SphereVIso(aPosition, aRadius, theDomain.VMax),
              theDomain.U(),
              theDomain.VMax,
              false,
              theTolerance,
              theMode);
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&          theResult,
                                   const gp_Pnt&    thePoint,
                                   const Domain2D&  theDomain,
                                   const gp_Torus&  theTorus,
                                   const double     theTolerance,
                                   const SearchMode theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const gp_Ax3& aPosition    = theTorus.Position();
  const double  aMajorRadius = theTorus.MajorRadius();
  const double  aMinorRadius = theTorus.MinorRadius();
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addCircle(theResult,
              thePoint,
              ElSLib::TorusUIso(aPosition, aMajorRadius, aMinorRadius, theDomain.UMin),
              theDomain.V(),
              theDomain.UMin,
              true,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addCircle(theResult,
              thePoint,
              ElSLib::TorusUIso(aPosition, aMajorRadius, aMinorRadius, theDomain.UMax),
              theDomain.V(),
              theDomain.UMax,
              true,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addCircle(theResult,
              thePoint,
              ElSLib::TorusVIso(aPosition, aMajorRadius, aMinorRadius, theDomain.VMin),
              theDomain.U(),
              theDomain.VMin,
              false,
              theTolerance,
              theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addCircle(theResult,
              thePoint,
              ElSLib::TorusVIso(aPosition, aMajorRadius, aMinorRadius, theDomain.VMax),
              theDomain.U(),
              theDomain.VMax,
              false,
              theTolerance,
              theMode);
}

//==================================================================================================

void ExtremaPS::AddBoundaryExtrema(Result&                  theResult,
                                   const gp_Pnt&            thePoint,
                                   const Domain2D&          theDomain,
                                   const Adaptor3d_Surface& theSurface,
                                   const double             theTolerance,
                                   const SearchMode         theMode)
{
  if (!canAddBoundary(theDomain, thePoint, theTolerance))
  {
    return;
  }
  const occ::handle<Adaptor3d_Surface> aSurface = theSurface.ShallowCopy();
  if (aSurface.IsNull())
  {
    return;
  }
  if (ExtremaPC::IsFiniteParameter(theDomain.VMin))
    addIso(theResult,
           thePoint,
           aSurface,
           GeomAbs_IsoV,
           theDomain.U(),
           theDomain.VMin,
           theTolerance,
           theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.VMax))
    addIso(theResult,
           thePoint,
           aSurface,
           GeomAbs_IsoV,
           theDomain.U(),
           theDomain.VMax,
           theTolerance,
           theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMin))
    addIso(theResult,
           thePoint,
           aSurface,
           GeomAbs_IsoU,
           theDomain.V(),
           theDomain.UMin,
           theTolerance,
           theMode);
  if (ExtremaPC::IsFiniteParameter(theDomain.UMax))
    addIso(theResult,
           thePoint,
           aSurface,
           GeomAbs_IsoU,
           theDomain.V(),
           theDomain.UMax,
           theTolerance,
           theMode);
}
