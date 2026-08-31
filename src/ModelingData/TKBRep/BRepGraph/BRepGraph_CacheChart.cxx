// Copyright (c) 2026 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in
// the distribution for complete details on the license and disclaimer of any
// warranty.

#include <BRepGraph_CacheChart.hxx>

#include <Bnd_Box2d.hxx>
#include <BndLib_Add2dCurve.hxx>
#include <BRepGraph_CopyRemap.hxx>
#include <BRepGraph_RefsIterator.hxx>
#include <BRepGraph_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <mutex>
#include <type_traits>

IMPLEMENT_STANDARD_RTTIEXT(BRepGraph_CacheChart, BRepGraph_Cache)
IMPLEMENT_STANDARD_RTTIEXT(BRepGraph_CacheChart::Result, Standard_Transient)

namespace
{

const Standard_GUID& theGUID()
{
  static const Standard_GUID aGUID("4a237ca4-0b08-4d4c-b9c7-5ae49a777c0b");
  return aGUID;
}

void hashCombine(std::size_t& theHash, const std::size_t theValue) noexcept
{
  // This is deliberately local to the chart contract.  It is not a persisted
  // hash and therefore need only be stable for the lifetime of one process.
  theHash ^= theValue + static_cast<std::size_t>(0x9e3779b9U) + (theHash << 6)
             + (theHash >> 2);
}

std::size_t hashDouble(const double theValue) noexcept
{
  uint64_t aBits = 0;
  static_assert(sizeof(aBits) == sizeof(theValue), "unexpected double size");
  std::memcpy(&aBits, &theValue, sizeof(aBits));
  return std::hash<uint64_t>{}(aBits);
}

template <typename T>
void hashIntegral(std::size_t& theHash, const T theValue) noexcept
{
  using UnsignedT = std::make_unsigned_t<T>;
  hashCombine(theHash, std::hash<UnsignedT>{}(static_cast<UnsignedT>(theValue)));
}

void hashTolerance(std::size_t& theHash,
                   const BRepGraph_CacheChart::Tolerance& theTolerance) noexcept
{
  hashCombine(theHash, hashDouble(theTolerance.Tolerance3d));
  hashCombine(theHash, hashDouble(theTolerance.ToleranceU));
  hashCombine(theHash, hashDouble(theTolerance.ToleranceV));
  hashCombine(theHash, hashDouble(theTolerance.CurveParameterTolerance));
  hashCombine(theHash, hashDouble(theTolerance.SingularityTolerance));
}

bool isFinite(const double theValue) noexcept
{
  return std::isfinite(theValue) && !Precision::IsInfinite(theValue);
}

bool close(const double theLeft, const double theRight, const double theTolerance) noexcept
{
  return std::abs(theLeft - theRight) <= theTolerance;
}

struct PeriodicInfo
{
  bool   UPeriodic = false;
  bool   VPeriodic = false;
  double UPeriod   = 0.0;
  double VPeriod   = 0.0;
};

bool readPeriodicInfo(const occ::handle<Geom_Surface>& theSurface, PeriodicInfo& theInfo)
{
  theInfo = PeriodicInfo();
  if (theSurface.IsNull())
  {
    return false;
  }

  theInfo.UPeriodic = theSurface->IsUPeriodic();
  theInfo.VPeriodic = theSurface->IsVPeriodic();
  if (theInfo.UPeriodic)
  {
    theInfo.UPeriod = theSurface->UPeriod();
    if (!isFinite(theInfo.UPeriod) || theInfo.UPeriod <= 0.0)
    {
      return false;
    }
  }
  if (theInfo.VPeriodic)
  {
    theInfo.VPeriod = theSurface->VPeriod();
    if (!isFinite(theInfo.VPeriod) || theInfo.VPeriod <= 0.0)
    {
      return false;
    }
  }
  return true;
}

bool isPeriodicDelta(const double theDelta,
                     const double thePeriod,
                     const double theTolerance,
                     int32_t&      theIndex) noexcept
{
  if (thePeriod <= 0.0 || !isFinite(theDelta))
  {
    return false;
  }

  const double aRealIndex = theDelta / thePeriod;
  const double aRounded   = std::round(aRealIndex);
  if (std::abs(aRealIndex - aRounded) > theTolerance / std::max(thePeriod, 1.0))
  {
    return false;
  }
  if (aRounded < static_cast<double>(std::numeric_limits<int32_t>::min())
      || aRounded > static_cast<double>(std::numeric_limits<int32_t>::max()))
  {
    return false;
  }
  theIndex = static_cast<int32_t>(aRounded);
  return true;
}

bool pointDelta(const gp_Pnt2d&       theLeft,
                const gp_Pnt2d&       theRight,
                const PeriodicInfo&   thePeriodic,
                const BRepGraph_CacheChart::Tolerance& theTolerance,
                BRepGraph_CacheChart::UVLift&     theLift,
                gp_Pnt2d&             theShift) noexcept
{
  const double aDeltaU = theRight.X() - theLeft.X();
  const double aDeltaV = theRight.Y() - theLeft.Y();
  int32_t      aUIndex = 0;
  int32_t      aVIndex = 0;
  if (thePeriodic.UPeriodic)
  {
    if (!isPeriodicDelta(aDeltaU, thePeriodic.UPeriod, theTolerance.ToleranceU, aUIndex))
    {
      return false;
    }
  }
  else if (!close(aDeltaU, 0.0, theTolerance.ToleranceU))
  {
    return false;
  }

  if (thePeriodic.VPeriodic)
  {
    if (!isPeriodicDelta(aDeltaV, thePeriodic.VPeriod, theTolerance.ToleranceV, aVIndex))
    {
      return false;
    }
  }
  else if (!close(aDeltaV, 0.0, theTolerance.ToleranceV))
  {
    return false;
  }

  if (aUIndex == 0 && aVIndex == 0)
  {
    return false;
  }
  theLift.UPeriodIndex = aUIndex;
  theLift.VPeriodIndex = aVIndex;
  theShift.SetCoord(aDeltaU, aDeltaV);
  return true;
}

struct CurveData
{
  occ::handle<Geom2d_Curve> Curve;
  Bnd_Box2d                 UVBounds;
  double                    First = 0.0;
  double                    Last  = 0.0;
  gp_Pnt2d                  FirstPoint;
  gp_Pnt2d                  LastPoint;
  bool                      IsReversed = false;
};

bool readCurveData(const BRepGraph&               theGraph,
                   const BRepGraph_CoEdgeId       theCoEdge,
                   const bool                     theWireReversed,
                   const BRepGraph_CacheChart::Tolerance& theTolerance,
                   CurveData&                     theData)
{
  theData = CurveData();
  const BRepGraphInc::CoEdgeDef& aCoEdge = theGraph.Topo().CoEdges().Definition(theCoEdge);
  const occ::handle<Geom2d_Curve>& aCurve = BRepGraph_Tool::CoEdge::PCurve(theGraph, theCoEdge);
  if (!aCurve.IsNull())
  {
    const std::pair<double, double> aRange = BRepGraph_Tool::CoEdge::Range(theGraph, theCoEdge);
    theData.Curve = aCurve;
    theData.First = aRange.first;
    theData.Last  = aRange.second;
    if (!isFinite(theData.First) || !isFinite(theData.Last)
        || std::abs(theData.Last - theData.First) <= theTolerance.CurveParameterTolerance)
    {
      return false;
    }
    theData.FirstPoint = aCurve->Value(theData.First);
    theData.LastPoint  = aCurve->Value(theData.Last);
    const Geom2dAdaptor_Curve anAdaptor(aCurve, theData.First, theData.Last);
    BndLib_Add2dCurve::Add(anAdaptor, theTolerance.Tolerance3d, theData.UVBounds);
  }
  else
  {
    const Geom2dAdaptor_Curve anAdaptor =
      BRepGraph_Tool::CoEdge::PCurveAdaptor(theGraph, theCoEdge);
    if (!anAdaptor.IsInitialized())
    {
      return false;
    }
    theData.First      = anAdaptor.FirstParameter();
    theData.Last       = anAdaptor.LastParameter();
    theData.FirstPoint = anAdaptor.Value(theData.First);
    theData.LastPoint  = anAdaptor.Value(theData.Last);
    BndLib_Add2dCurve::Add(anAdaptor, theTolerance.Tolerance3d, theData.UVBounds);
  }

  theData.IsReversed = aCoEdge.Orientation.IsReversed ^ theWireReversed;
  if (theData.IsReversed)
  {
    std::swap(theData.FirstPoint, theData.LastPoint);
  }
  return true;
}

bool sameModuloPeriods(const gp_Pnt2d&     theLeft,
                       const gp_Pnt2d&     theRight,
                       const PeriodicInfo& thePeriodic,
                       const BRepGraph_CacheChart::Tolerance& theTolerance) noexcept
{
  const double aDeltaU = theRight.X() - theLeft.X();
  const double aDeltaV = theRight.Y() - theLeft.Y();
  if (thePeriodic.UPeriodic)
  {
    int32_t anIndex = 0;
    if (!isPeriodicDelta(aDeltaU, thePeriodic.UPeriod, theTolerance.ToleranceU, anIndex))
    {
      return false;
    }
  }
  else if (!close(aDeltaU, 0.0, theTolerance.ToleranceU))
  {
    return false;
  }
  if (thePeriodic.VPeriodic)
  {
    int32_t anIndex = 0;
    if (!isPeriodicDelta(aDeltaV, thePeriodic.VPeriod, theTolerance.ToleranceV, anIndex))
    {
      return false;
    }
  }
  else if (!close(aDeltaV, 0.0, theTolerance.ToleranceV))
  {
    return false;
  }
  return true;
}

bool findTransition(const CurveData&                  theLeft,
                    const CurveData&                  theRight,
                    const PeriodicInfo&                thePeriodic,
                    const BRepGraph_CacheChart::Tolerance&   theTolerance,
                    BRepGraph_CacheChart::UVLift&                 theLift,
                    gp_Pnt2d&                          theShift,
                    bool&                              theReversedMatch) noexcept
{
  BRepGraph_CacheChart::UVLift aLift;
  gp_Pnt2d         aShift;
  if (pointDelta(theLeft.FirstPoint,
                 theRight.FirstPoint,
                 thePeriodic,
                 theTolerance,
                 aLift,
                 aShift)
      && pointDelta(theLeft.LastPoint,
                    theRight.LastPoint,
                    thePeriodic,
                    theTolerance,
                    theLift,
                    theShift)
      && aLift.UPeriodIndex == theLift.UPeriodIndex && aLift.VPeriodIndex == theLift.VPeriodIndex)
  {
    theReversedMatch = false;
    return true;
  }

  if (pointDelta(theLeft.FirstPoint,
                 theRight.LastPoint,
                 thePeriodic,
                 theTolerance,
                 aLift,
                 aShift)
      && pointDelta(theLeft.LastPoint,
                    theRight.FirstPoint,
                    thePeriodic,
                    theTolerance,
                    theLift,
                    theShift)
      && aLift.UPeriodIndex == theLift.UPeriodIndex && aLift.VPeriodIndex == theLift.VPeriodIndex)
  {
    theReversedMatch = true;
    return true;
  }
  return false;
}

double coordinateFirst(const BRepGraph_CacheChart::Result& theResult,
                       const BRepGraph_CacheChart::Policy& thePolicy,
                       const BRepGraph_CacheChart::Direction theDirection)
{
  const bool isU = theDirection == BRepGraph_CacheChart::Direction::U;
  const double aFirst  = isU ? theResult.UFirst : theResult.VFirst;
  const double aLast   = isU ? theResult.ULast : theResult.VLast;
  const double aPeriod = isU ? theResult.UPeriod : theResult.VPeriod;
  if (thePolicy.Selection == BRepGraph_CacheChart::Policy::CutSelection::Explicit)
  {
    return isU ? thePolicy.ExplicitU : thePolicy.ExplicitV;
  }
  if (thePolicy.Selection == BRepGraph_CacheChart::Policy::CutSelection::MaxClearance
      && isFinite(aFirst) && isFinite(aLast))
  {
    return 0.5 * (aFirst + aLast);
  }
  if (thePolicy.Selection == BRepGraph_CacheChart::Policy::CutSelection::MinimizeSplits
      && isFinite(aFirst) && isFinite(aLast))
  {
    return aFirst + 0.5 * std::min(std::abs(aLast - aFirst), aPeriod);
  }
  return aFirst;
}

int countCutCrossings(const BRepGraph_CacheChart::Result& theResult,
                      const BRepGraph_CacheChart::Direction theDirection,
                      const double theCut)
{
  const bool isU = theDirection == BRepGraph_CacheChart::Direction::U;
  const double aPeriod = isU ? theResult.UPeriod : theResult.VPeriod;
  int aCount = 0;
  for (const BRepGraph_CacheChart::Loop& aLoop : theResult.Loops)
  {
    for (const BRepGraph_CacheChart::BoundaryUse& aUse : aLoop.Boundary)
    {
      if (aUse.Kind != BRepGraph_CacheChart::BoundaryKind::RealCoEdge)
      {
        continue;
      }
      const double aFirst = isU ? aUse.UVFirst.X() : aUse.UVFirst.Y();
      const double aLast  = isU ? aUse.UVLast.X() : aUse.UVLast.Y();
      if (std::abs(aLast - aFirst) <= (isU ? 1.0e-12 : 1.0e-12))
      {
        continue;
      }
      const double aMin = std::min(aFirst, aLast);
      const double aMax = std::max(aFirst, aLast);
      double       aProbe = theCut;
      while (aProbe < aMin)
      {
        aProbe += aPeriod;
      }
      while (aProbe > aMax)
      {
        aProbe -= aPeriod;
      }
      if (aProbe > aMin && aProbe < aMax)
      {
        ++aCount;
      }
    }
  }
  return aCount;
}

void appendCut(BRepGraph_CacheChart::Result&       theResult,
               const BRepGraph_CacheChart::Policy& thePolicy,
               const BRepGraph_CacheChart::Direction theDirection)
{
  const bool isU = theDirection == BRepGraph_CacheChart::Direction::U;
  const double aPeriod = isU ? theResult.UPeriod : theResult.VPeriod;
  if (aPeriod <= 0.0)
  {
    return;
  }

  double aParameter = coordinateFirst(theResult, thePolicy, theDirection);
  if (thePolicy.Selection == BRepGraph_CacheChart::Policy::CutSelection::MinimizeSplits)
  {
    const double aCanonical = isU ? theResult.UFirst : theResult.VFirst;
    const double aMiddle    = aCanonical + 0.5 * aPeriod;
    if (countCutCrossings(theResult, theDirection, aMiddle)
        < countCutCrossings(theResult, theDirection, aCanonical))
    {
      aParameter = aMiddle;
    }
  }

  BRepGraph_CacheChart::Cut aCut;
  aCut.Direction = theDirection;
  aCut.Parameter = aParameter;
  aCut.Period    = aPeriod;
  if (isU)
  {
    aCut.OtherFirst = theResult.VFirst;
    aCut.OtherLast  = theResult.VLast;
  }
  else
  {
    aCut.OtherFirst = theResult.UFirst;
    aCut.OtherLast  = theResult.ULast;
  }
  aCut.PairIndex = static_cast<int32_t>(theResult.Cuts.Size());
  theResult.Cuts.Append(aCut);
}

void appendChartBoundary(BRepGraph_CacheChart::Loop&          theLoop,
                         const BRepGraph_CacheChart::BoundaryKind theKind,
                         const uint32_t                 theVirtualCutIndex,
                         const int32_t                  thePairIndex,
                         const gp_Pnt2d&                theFirst,
                         const gp_Pnt2d&                theLast)
{
  BRepGraph_CacheChart::BoundaryUse& aUse = theLoop.Boundary.Appended();
  aUse.Kind            = theKind;
  aUse.VirtualCutIndex = theVirtualCutIndex;
  aUse.PairIndex       = thePairIndex;
  aUse.UVFirst         = theFirst;
  aUse.UVLast          = theLast;
}

bool isSingularUSection(const occ::handle<Geom_Surface>& theSurface,
                        const double                     theU,
                        const double                     theV,
                        const double                     theUPeriod,
                        const double                     theTolerance)
{
  if (theSurface.IsNull() || theUPeriod <= 0.0)
  {
    return false;
  }
  const gp_Pnt aFirst = theSurface->Value(theU, theV);
  const gp_Pnt aOther = theSurface->Value(theU + 0.5 * theUPeriod, theV);
  return aFirst.Distance(aOther) <= theTolerance;
}

bool appendCompleteSurfaceChart(BRepGraph_CacheChart::Result&     theResult,
                                const PeriodicInfo&            thePeriodic,
                                const BRepGraph_CacheChart::Tolerance& theTolerance)
{
  if ((!thePeriodic.UPeriodic && !thePeriodic.VPeriodic)
      || !isFinite(theResult.UFirst) || !isFinite(theResult.ULast)
      || !isFinite(theResult.VFirst) || !isFinite(theResult.VLast))
  {
    return false;
  }

  const gp_Pnt2d aLowerLeft(theResult.UFirst, theResult.VFirst);
  const gp_Pnt2d aUpperLeft(theResult.UFirst, theResult.VLast);
  const gp_Pnt2d aUpperRight(theResult.ULast, theResult.VLast);
  const gp_Pnt2d aLowerRight(theResult.ULast, theResult.VFirst);

  const bool aLowerSingular = thePeriodic.UPeriodic
                              && isSingularUSection(theResult.Surface,
                                                    theResult.UFirst,
                                                    theResult.VFirst,
                                                    theResult.UPeriod,
                                                    theTolerance.Tolerance3d);
  const bool anUpperSingular = thePeriodic.UPeriodic
                               && isSingularUSection(theResult.Surface,
                                                     theResult.UFirst,
                                                     theResult.VLast,
                                                     theResult.UPeriod,
                                                     theTolerance.Tolerance3d);

  BRepGraph_CacheChart::Loop& aLoop = theResult.Loops.Appended();
  const int32_t aUTransition = thePeriodic.UPeriodic
                                 ? static_cast<int32_t>(theResult.Transitions.Size())
                                 : -1;
  if (thePeriodic.UPeriodic)
  {
    BRepGraph_CacheChart::Transition& aTransition = theResult.Transitions.Appended();
    aTransition.Direction    = BRepGraph_CacheChart::Direction::U;
    aTransition.Period       = theResult.UPeriod;
    aTransition.CutParameter = theResult.UFirst;
    aTransition.Shift.SetCoord(theResult.UPeriod, 0.0);
    aTransition.PositiveSide = 0;
    aTransition.NegativeSide = 2;
  }

  const int32_t aVTransition = thePeriodic.VPeriodic
                                 ? static_cast<int32_t>(theResult.Transitions.Size())
                                 : -1;
  if (thePeriodic.VPeriodic)
  {
    BRepGraph_CacheChart::Transition& aTransition = theResult.Transitions.Appended();
    aTransition.Direction    = BRepGraph_CacheChart::Direction::V;
    aTransition.Period       = theResult.VPeriod;
    aTransition.CutParameter = theResult.VFirst;
    aTransition.Shift.SetCoord(0.0, theResult.VPeriod);
    aTransition.PositiveSide = 3;
    aTransition.NegativeSide = 1;
  }

  const BRepGraph_CacheChart::BoundaryKind aLeftKind =
    thePeriodic.UPeriodic ? BRepGraph_CacheChart::BoundaryKind::VirtualCut
                          : BRepGraph_CacheChart::BoundaryKind::ParametricBoundary;
  const BRepGraph_CacheChart::BoundaryKind aRightKind = aLeftKind;
  const BRepGraph_CacheChart::BoundaryKind aLowerKind =
    aLowerSingular ? BRepGraph_CacheChart::BoundaryKind::SingularBoundary
                   : (thePeriodic.VPeriodic ? BRepGraph_CacheChart::BoundaryKind::VirtualCut
                                            : BRepGraph_CacheChart::BoundaryKind::ParametricBoundary);
  const BRepGraph_CacheChart::BoundaryKind anUpperKind =
    anUpperSingular ? BRepGraph_CacheChart::BoundaryKind::SingularBoundary
                    : (thePeriodic.VPeriodic ? BRepGraph_CacheChart::BoundaryKind::VirtualCut
                                             : BRepGraph_CacheChart::BoundaryKind::ParametricBoundary);

  appendChartBoundary(aLoop, aLeftKind, 0, aUTransition, aLowerLeft, aUpperLeft);
  appendChartBoundary(aLoop, anUpperKind, 1, aVTransition, aUpperLeft, aUpperRight);
  appendChartBoundary(aLoop, aRightKind, 0, aUTransition, aUpperRight, aLowerRight);
  appendChartBoundary(aLoop, aLowerKind, 1, aVTransition, aLowerRight, aLowerLeft);
  aLoop.IsClosed = true;
  return true;
}

occ::handle<BRepGraph_CacheChart::Result> failure(const BRepGraph_FaceId       theFace,
                                             const BRepGraph_CacheChart::Status theStatus,
                                             const char*                  theDiagnostic)
{
  occ::handle<BRepGraph_CacheChart::Result> aResult = new BRepGraph_CacheChart::Result;
  aResult->Face       = theFace;
  aResult->Status     = theStatus;
  aResult->Diagnostic = theDiagnostic;
  return aResult;
}

} // namespace

//=================================================================================================

std::size_t BRepGraph_CacheChart::Policy::Hash() const noexcept
{
  std::size_t aHash = 0;
  hashIntegral(aHash, static_cast<uint8_t>(Selection));
  hashIntegral(aHash, static_cast<uint8_t>(PreferredDirection));
  hashCombine(aHash, hashDouble(ExplicitU));
  hashCombine(aHash, hashDouble(ExplicitV));
  hashTolerance(aHash, Tolerance);
  hashIntegral(aHash, static_cast<uint8_t>(Caching));
  return aHash;
}

//=================================================================================================

std::size_t BRepGraph_CacheChart::Policy::ExplicitClassHash() const noexcept
{
  BRepGraph_CacheChart::Policy aClass = *this;
  aClass.ExplicitU             = 0.0;
  aClass.ExplicitV             = 0.0;
  return aClass.Hash();
}

//=================================================================================================

std::size_t BRepGraph_CacheChart::Requirements::Hash() const noexcept
{
  std::size_t aHash = 0;
  hashIntegral(aHash, static_cast<uint8_t>(RequireFiniteUVDomain));
  hashIntegral(aHash, static_cast<uint8_t>(RequireClosedUVLoops));
  hashIntegral(aHash, static_cast<uint8_t>(RequireMaterializableTopology));
  return aHash;
}

//=================================================================================================

occ::handle<BRepGraph_CacheChart::Result> BRepGraph_CacheChart::Build(
  const BRepGraph& theGraph,
  const BRepGraph_FaceId theFace)
{
  return Build(theGraph, theFace, BRepGraph_CacheChart::Policy(), BRepGraph_CacheChart::Requirements());
}

//=================================================================================================

occ::handle<BRepGraph_CacheChart::Result> BRepGraph_CacheChart::Build(
  const BRepGraph&                   theGraph,
  const BRepGraph_FaceId             theFace,
  const BRepGraph_CacheChart::Policy&       thePolicy,
  const BRepGraph_CacheChart::Requirements& theRequirements)
{
  if (!theFace.IsValid(theGraph.Topo().Faces().Nb()) || theFace.IsRemoved(theGraph))
  {
    return failure(theFace, BRepGraph_CacheChart::Status::MissingBoundary, "Invalid or removed face");
  }

  occ::handle<BRepGraph_CacheChart::Result> aResult = new BRepGraph_CacheChart::Result;
  aResult->Face    = theFace;
  aResult->Surface = theGraph.Topo().Faces().Surface(theFace);
  if (aResult->Surface.IsNull())
  {
    aResult->Status     = BRepGraph_CacheChart::Status::MissingSurface;
    aResult->Diagnostic = "Face has no supporting surface";
    return aResult;
  }

  const bool hasPersistentWires = BRepGraph_Tool::Face::NbWires(theGraph, theFace) != 0;

  PeriodicInfo aPeriodic;
  if (!readPeriodicInfo(aResult->Surface, aPeriodic))
  {
    aResult->Status     = BRepGraph_CacheChart::Status::MissingGeometry;
    aResult->Diagnostic = "Surface periodicity has an invalid period";
    return aResult;
  }
  aResult->UPeriodic = aPeriodic.UPeriodic;
  aResult->VPeriodic = aPeriodic.VPeriodic;
  aResult->UPeriod   = aPeriodic.UPeriod;
  aResult->VPeriod   = aPeriodic.VPeriod;
  aResult->Surface->Bounds(aResult->UFirst,
                           aResult->ULast,
                           aResult->VFirst,
                           aResult->VLast);

  const bool hasFiniteSurfaceBounds = isFinite(aResult->UFirst) && isFinite(aResult->ULast)
                                      && isFinite(aResult->VFirst) && isFinite(aResult->VLast);

  if (theRequirements.RequireFiniteUVDomain && !hasFiniteSurfaceBounds
      && !hasPersistentWires)
  {
    aResult->Status     = BRepGraph_CacheChart::Status::UnboundedDomain;
    aResult->Diagnostic = "Supporting surface does not provide finite chart bounds";
    return aResult;
  }

  if (!hasPersistentWires)
  {
    if (!appendCompleteSurfaceChart(*aResult, aPeriodic, thePolicy.Tolerance))
    {
      aResult->Status     = BRepGraph_CacheChart::Status::MissingBoundary;
      aResult->Diagnostic = "Face without wires is not a supported complete finite surface";
      return aResult;
    }
    if (aPeriodic.UPeriodic)
    {
      appendCut(*aResult, thePolicy, BRepGraph_CacheChart::Direction::U);
    }
    if (aPeriodic.VPeriodic)
    {
      appendCut(*aResult, thePolicy, BRepGraph_CacheChart::Direction::V);
    }
    aResult->Status = BRepGraph_CacheChart::Status::Ready;
    return aResult;
  }

  Bnd_Box2d aBoundaryBounds;
  for (BRepGraph_RefsWireOfFace aWireIt(theGraph, theFace); aWireIt.More(); aWireIt.Next())
  {
    const BRepGraphInc::WireRef& aWireRef = theGraph.Refs().Wires().Entry(aWireIt.CurrentId());
    const BRepGraph_WireId       aWireId  = aWireRef.ChildWireId;
    BRepGraph_CacheChart::Loop&  aLoop    = aResult->Loops.Appended();
    aLoop.SourceWire                         = aWireId;
    const bool aWireReversed                 = aWireRef.Orientation.IsReversed;

    for (BRepGraph_CoEdgesOfWire aCoEdgeIt(theGraph, aWireId); aCoEdgeIt.More(); aCoEdgeIt.Next())
    {
      const BRepGraph_CoEdgeId aCoEdge = aCoEdgeIt.CurrentId();
      CurveData                aCurve;
      if (!readCurveData(theGraph, aCoEdge, aWireReversed, thePolicy.Tolerance, aCurve))
      {
        aResult->Status     = BRepGraph_CacheChart::Status::MissingGeometry;
        aResult->Diagnostic = "Boundary coedge has no valid pcurve or range";
        return aResult;
      }
      BRepGraph_CacheChart::BoundaryUse& aUse = aLoop.Boundary.Appended();
      aUse.SourceCoEdge               = aCoEdge;
      aUse.Orientation.IsReversed     = aCurve.IsReversed;
      aUse.ParamFirst                 = aCurve.First;
      aUse.ParamLast                  = aCurve.Last;
      aUse.UVFirst                    = aCurve.FirstPoint;
      aUse.UVLast                     = aCurve.LastPoint;
      aBoundaryBounds.Add(aCurve.UVBounds);
    }

    if (aLoop.Boundary.IsEmpty())
    {
      aResult->Loops.EraseLast();
      continue;
    }

    for (size_t anIndex = 0; anIndex < aLoop.Boundary.Size(); ++anIndex)
    {
      BRepGraph_CacheChart::BoundaryUse& aLeft = aLoop.Boundary.ChangeValue(anIndex);
      if (aLeft.Kind != BRepGraph_CacheChart::BoundaryKind::RealCoEdge)
      {
        continue;
      }
      const BRepGraph_EdgeId aLeftEdge =
        theGraph.Topo().CoEdges().Definition(aLeft.SourceCoEdge).ChildEdgeId;
      if (!aLeftEdge.IsValid())
      {
        continue;
      }
      for (size_t anOtherIndex = anIndex + 1; anOtherIndex < aLoop.Boundary.Size(); ++anOtherIndex)
      {
        BRepGraph_CacheChart::BoundaryUse& aRight = aLoop.Boundary.ChangeValue(anOtherIndex);
        if (aRight.Kind != BRepGraph_CacheChart::BoundaryKind::RealCoEdge)
        {
          continue;
        }
        const BRepGraph_EdgeId aRightEdge =
          theGraph.Topo().CoEdges().Definition(aRight.SourceCoEdge).ChildEdgeId;
        if (aLeftEdge != aRightEdge)
        {
          continue;
        }

        CurveData aLeftCurve;
        CurveData aRightCurve;
        if (!readCurveData(theGraph,
                           aLeft.SourceCoEdge,
                           aWireReversed,
                           thePolicy.Tolerance,
                           aLeftCurve)
            || !readCurveData(theGraph,
                              aRight.SourceCoEdge,
                              aWireReversed,
                              thePolicy.Tolerance,
                              aRightCurve))
        {
          continue;
        }

        BRepGraph_CacheChart::UVLift aLift;
        gp_Pnt2d         aShift;
        bool             isReversedMatch = false;
        if (!findTransition(aLeftCurve,
                            aRightCurve,
                            aPeriodic,
                            thePolicy.Tolerance,
                            aLift,
                            aShift,
                            isReversedMatch))
        {
          continue;
        }

        const int32_t aTransitionIndex = static_cast<int32_t>(aResult->Transitions.Size());
        BRepGraph_CacheChart::Transition& aTransition = aResult->Transitions.Appended();
        aTransition.Direction = std::abs(aLift.UPeriodIndex) != 0
                                  ? BRepGraph_CacheChart::Direction::U
                                  : BRepGraph_CacheChart::Direction::V;
        aTransition.Period = aTransition.Direction == BRepGraph_CacheChart::Direction::U
                               ? aPeriodic.UPeriod
                               : aPeriodic.VPeriod;
        aTransition.Shift = aShift;
        aTransition.PositiveSide = static_cast<int32_t>(anIndex);
        aTransition.NegativeSide = static_cast<int32_t>(anOtherIndex);
        aTransition.CutParameter = aTransition.Direction == BRepGraph_CacheChart::Direction::U
                                     ? std::min(aLeft.UVFirst.X(), aRight.UVFirst.X())
                                     : std::min(aLeft.UVFirst.Y(), aRight.UVFirst.Y());

        aLeft.Kind       = BRepGraph_CacheChart::BoundaryKind::VirtualCut;
        aRight.Kind      = BRepGraph_CacheChart::BoundaryKind::VirtualCut;
        aLeft.PairIndex  = aTransitionIndex;
        aRight.PairIndex = aTransitionIndex;
        aLeft.Lift       = aLift;
        aRight.Lift      = aLift;
        (void)isReversedMatch;
        break;
      }
    }

    aLoop.IsClosed = sameModuloPeriods(aLoop.Boundary.Last().UVLast,
                                       aLoop.Boundary.First().UVFirst,
                                       aPeriodic,
                                       thePolicy.Tolerance);
    if (theRequirements.RequireClosedUVLoops && !aLoop.IsClosed)
    {
      aResult->Status     = BRepGraph_CacheChart::Status::InvalidBoundary;
      aResult->Diagnostic = "Boundary coedges do not form a closed lifted UV loop";
      return aResult;
    }
  }

  if (!hasFiniteSurfaceBounds)
  {
    if (aBoundaryBounds.IsVoid())
    {
      aResult->Status     = BRepGraph_CacheChart::Status::MissingBoundary;
      aResult->Diagnostic = "Persistent boundaries have no finite UV extent";
      return aResult;
    }

    double aUMin = 0.0;
    double aVMin = 0.0;
    double aUMax = 0.0;
    double aVMax = 0.0;
    aBoundaryBounds.Get(aUMin, aVMin, aUMax, aVMax);
    if (!isFinite(aResult->UFirst) || !isFinite(aResult->ULast))
    {
      aResult->UFirst = aUMin;
      aResult->ULast  = aUMax;
    }
    if (!isFinite(aResult->VFirst) || !isFinite(aResult->VLast))
    {
      aResult->VFirst = aVMin;
      aResult->VLast  = aVMax;
    }
  }

  if (theRequirements.RequireFiniteUVDomain
      && (!isFinite(aResult->UFirst) || !isFinite(aResult->ULast)
          || !isFinite(aResult->VFirst) || !isFinite(aResult->VLast)))
  {
    aResult->Status     = BRepGraph_CacheChart::Status::UnboundedDomain;
    aResult->Diagnostic = "Persistent boundaries do not provide finite chart bounds";
    return aResult;
  }

  if (aPeriodic.UPeriodic)
  {
    appendCut(*aResult, thePolicy, BRepGraph_CacheChart::Direction::U);
  }
  if (aPeriodic.VPeriodic)
  {
    appendCut(*aResult, thePolicy, BRepGraph_CacheChart::Direction::V);
  }

  if (!aPeriodic.UPeriodic && !aPeriodic.VPeriodic)
  {
    aResult->Status = BRepGraph_CacheChart::Status::NotPeriodic;
  }
  else
  {
    aResult->Status = BRepGraph_CacheChart::Status::Ready;
  }
  return aResult;
}

//=================================================================================================

BRepGraph_CacheChart::BRepGraph_CacheChart() = default;

//=================================================================================================

const Standard_GUID& BRepGraph_CacheChart::GetID()
{
  return theGUID();
}

//=================================================================================================

const Standard_GUID& BRepGraph_CacheChart::ID() const
{
  return GetID();
}

//=================================================================================================

const TCollection_AsciiString& BRepGraph_CacheChart::Name() const
{
  static const TCollection_AsciiString aName("BRepGraph.CacheChart");
  return aName;
}

//=================================================================================================

void BRepGraph_CacheChart::Clear() noexcept
{
  std::unique_lock<std::shared_mutex> aLock(myMutex);
  myEntries.Clear(true);
  myUseClock = 0;
}

//=================================================================================================

void BRepGraph_CacheChart::CopyFreshTo(const BRepGraph_CopyRemap&) const
{
  // Chart results contain source graph IDs, selected cuts, and source handles.
  // Rebuilding in the target graph is both cheaper than remapping all derived
  // occurrences and safer than retaining a chart tied to the source graph.
}

//=================================================================================================

uint32_t BRepGraph_CacheChart::NbEntries() const
{
  std::shared_lock<std::shared_mutex> aLock(myMutex);
  return static_cast<uint32_t>(myEntries.Size());
}

//=================================================================================================

const BRepGraph_CacheChart::Entry* BRepGraph_CacheChart::findFreshLocked(
  const BRepGraph_FaceId theFace,
  const std::size_t       thePolicyHash,
  const std::size_t       theRequirementsHash) const
{
  for (const Entry& anEntry : myEntries)
  {
    if (anEntry.Face != theFace || anEntry.PolicyHash != thePolicyHash
        || anEntry.RequirementsHash != theRequirementsHash || anEntry.Chart.IsNull()
        || !anEntry.IsFreshSubtree(*this, BRepGraph_NodeId(theFace)))
    {
      continue;
    }
    return &anEntry;
  }
  return nullptr;
}

//=================================================================================================

void BRepGraph_CacheChart::eraseExplicitLocked(const BRepGraph_FaceId theFace,
                                               const std::size_t       thePolicyClassHash,
                                               const std::size_t       theRequirementsHash)
{
  for (size_t anIndex = 0; anIndex < myEntries.Size();)
  {
    const Entry& anEntry = myEntries.Value(anIndex);
    if (!anEntry.IsExplicit || anEntry.Face != theFace
        || anEntry.PolicyClassHash != thePolicyClassHash
        || anEntry.RequirementsHash != theRequirementsHash)
    {
      ++anIndex;
      continue;
    }
    myEntries.Erase(anIndex);
  }
}

//=================================================================================================

void BRepGraph_CacheChart::eraseIdentityLocked(const BRepGraph_FaceId theFace,
                                               const std::size_t       thePolicyHash,
                                               const std::size_t       theRequirementsHash)
{
  for (size_t anIndex = 0; anIndex < myEntries.Size();)
  {
    const Entry& anEntry = myEntries.Value(anIndex);
    if (anEntry.Face != theFace || anEntry.PolicyHash != thePolicyHash
        || anEntry.RequirementsHash != theRequirementsHash)
    {
      ++anIndex;
      continue;
    }
    myEntries.Erase(anIndex);
  }
}

//=================================================================================================

occ::handle<BRepGraph_CacheChart::Result> BRepGraph_CacheChart::Get(
  const BRepGraph_FaceId theFace)
{
  return Get(theFace, BRepGraph_CacheChart::Policy(), BRepGraph_CacheChart::Requirements());
}

//=================================================================================================

occ::handle<BRepGraph_CacheChart::Result> BRepGraph_CacheChart::Get(
  const BRepGraph_FaceId              theFace,
  const BRepGraph_CacheChart::Policy& thePolicy)
{
  return Get(theFace, thePolicy, BRepGraph_CacheChart::Requirements());
}

//=================================================================================================

occ::handle<BRepGraph_CacheChart::Result> BRepGraph_CacheChart::Get(
  const BRepGraph_FaceId                    theFace,
  const BRepGraph_CacheChart::Policy&       thePolicy,
  const BRepGraph_CacheChart::Requirements& theRequirements)
{
  if (!IsAttached())
  {
    occ::handle<BRepGraph_CacheChart::Result> aResult = new BRepGraph_CacheChart::Result;
    aResult->Face       = theFace;
    aResult->Status     = BRepGraph_CacheChart::Status::Unsupported;
    aResult->Diagnostic = "Chart cache is detached from a graph";
    return aResult;
  }

  const std::size_t aPolicyHash       = thePolicy.Hash();
  const std::size_t aPolicyClassHash  = thePolicy.ExplicitClassHash();
  const std::size_t aRequirementsHash = theRequirements.Hash();
  const bool        isExplicit =
    thePolicy.Selection == BRepGraph_CacheChart::Policy::CutSelection::Explicit;

  if (thePolicy.Caching != BRepGraph_CacheChart::Policy::CacheMode::Provisional)
  {
    std::shared_lock<std::shared_mutex> aReadLock(myMutex);
    if (const Entry* anEntry = findFreshLocked(theFace, aPolicyHash, aRequirementsHash))
    {
      return anEntry->Chart;
    }
  }

  // Build outside the cache lock so unrelated chart requests do not serialize
  // on surface evaluation or pcurve traversal.
  occ::handle<BRepGraph_CacheChart::Result> aResult =
    BRepGraph_CacheChart::Build(Graph(), theFace, thePolicy, theRequirements);
  if (thePolicy.Caching == BRepGraph_CacheChart::Policy::CacheMode::Provisional)
  {
    return aResult;
  }

  std::unique_lock<std::shared_mutex> aWriteLock(myMutex);
  if (const Entry* anEntry = findFreshLocked(theFace, aPolicyHash, aRequirementsHash))
  {
    return anEntry->Chart;
  }

  if (isExplicit)
  {
    // A movable interactive cut has one stable cache slot per face,
    // requirements bundle, and policy class.
    eraseExplicitLocked(theFace, aPolicyClassHash, aRequirementsHash);
  }
  else
  {
    // Replace the stale entry for this exact recipe rather than accumulating
    // an archive after repeated graph edits.
    eraseIdentityLocked(theFace, aPolicyHash, aRequirementsHash);
  }

  Entry& anEntry          = myEntries.Appended();
  anEntry.Face             = theFace;
  anEntry.PolicyHash       = aPolicyHash;
  anEntry.PolicyClassHash  = aPolicyClassHash;
  anEntry.RequirementsHash = aRequirementsHash;
  anEntry.Chart            = aResult;
  anEntry.LastUse          = ++myUseClock;
  anEntry.IsExplicit       = isExplicit;
  if (!anEntry.BindSubtreeGen(*this, BRepGraph_NodeId(theFace)))
  {
    myEntries.EraseLast();
    return aResult;
  }
  return aResult;
}
