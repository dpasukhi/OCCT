// Copyright (c) 2024 OPEN CASCADE SAS
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

#include <GeomAdaptor_CurveCore.hxx>

// Include complete types for unique_ptr destruction
#include <Geom2dAdaptor_CurveCore.hxx>
#include <GeomAdaptor_SurfaceCore.hxx>

#include <BSplCLib.hxx>
#include <BSplCLib_Cache.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <Standard_DomainError.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>
#include <TColStd_Array1OfInteger.hxx>

#include "../Geom/Geom_OffsetCurveUtils.pxx"

static const double PosTol = Precision::PConfusion() / 2;

//==================================================================================================

GeomAdaptor_CurveCore::GeomAdaptor_CurveCore()
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
}

//==================================================================================================

GeomAdaptor_CurveCore::~GeomAdaptor_CurveCore() = default;

//==================================================================================================

GeomAdaptor_CurveCore::GeomAdaptor_CurveCore(const Handle(Geom_Curve)& theCurve)
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
  Load(theCurve);
}

//==================================================================================================

GeomAdaptor_CurveCore::GeomAdaptor_CurveCore(const Handle(Geom_Curve)& theCurve,
                                             double                    theUFirst,
                                             double                    theULast)
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
  Load(theCurve, theUFirst, theULast);
}

//==================================================================================================

GeomAdaptor_CurveCore::GeomAdaptor_CurveCore(const GeomAdaptor_CurveCore& theOther)
    : myCurve(theOther.myCurve),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myTrsf(theOther.myTrsf)
{
  // Deep copy of modifier data based on variant type
  if (const auto* anOffsetData = std::get_if<OffsetData>(&theOther.myEvalData))
  {
    OffsetData aCopyData;
    if (anOffsetData->BasisCore)
    {
      aCopyData.BasisCore = std::make_unique<GeomAdaptor_CurveCore>(*anOffsetData->BasisCore);
    }
    aCopyData.Offset    = anOffsetData->Offset;
    aCopyData.Direction = anOffsetData->Direction;
    myEvalData          = std::move(aCopyData);
  }
  else if (const auto* aBSplineData = std::get_if<BSplineData>(&theOther.myEvalData))
  {
    BSplineData aCopyData;
    aCopyData.Curve = aBSplineData->Curve;
    // Cache is not copied - will be rebuilt on demand
    myEvalData = std::move(aCopyData);
  }
  else if (std::holds_alternative<BezierData>(theOther.myEvalData))
  {
    myEvalData = BezierData{};
  }
  else if (const auto* aPiecewiseData = std::get_if<PiecewiseData>(&theOther.myEvalData))
  {
    PiecewiseData aCopyData;
    aCopyData.Curves = aPiecewiseData->Curves;  // Deep copy of vector
    aCopyData.Knots  = aPiecewiseData->Knots;
    myEvalData       = std::move(aCopyData);
  }
  // CurveOnSurfaceData and IsoCurveData would need forward declarations
  // to be fully implemented here - for now they remain as monostate
}

//==================================================================================================

GeomAdaptor_CurveCore::GeomAdaptor_CurveCore(GeomAdaptor_CurveCore&& theOther) noexcept
    : myCurve(std::move(theOther.myCurve)),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myEvalData(std::move(theOther.myEvalData)),
      myTrsf(std::move(theOther.myTrsf))
{
  theOther.myTypeCurve = GeomAbs_OtherCurve;
  theOther.myFirst     = 0.0;
  theOther.myLast      = 0.0;
}

//==================================================================================================

GeomAdaptor_CurveCore& GeomAdaptor_CurveCore::operator=(const GeomAdaptor_CurveCore& theOther)
{
  if (this != &theOther)
  {
    GeomAdaptor_CurveCore aCopy(theOther);
    *this = std::move(aCopy);
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_CurveCore& GeomAdaptor_CurveCore::operator=(GeomAdaptor_CurveCore&& theOther) noexcept
{
  if (this != &theOther)
  {
    myCurve              = std::move(theOther.myCurve);
    myTypeCurve          = theOther.myTypeCurve;
    myFirst              = theOther.myFirst;
    myLast               = theOther.myLast;
    myEvalData           = std::move(theOther.myEvalData);
    myTrsf               = std::move(theOther.myTrsf);
    theOther.myTypeCurve = GeomAbs_OtherCurve;
    theOther.myFirst     = 0.0;
    theOther.myLast      = 0.0;
  }
  return *this;
}

//==================================================================================================

void GeomAdaptor_CurveCore::Reset()
{
  myTypeCurve = GeomAbs_OtherCurve;
  myCurve.Nullify();
  myEvalData = std::monostate{};
  myFirst = myLast = 0.0;
  myTrsf.reset();
}

//==================================================================================================

const gp_Trsf& GeomAdaptor_CurveCore::Transformation() const
{
  if (!myTrsf.has_value())
  {
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::Transformation - no transformation set");
  }
  return *myTrsf;
}

//==================================================================================================

void GeomAdaptor_CurveCore::load(const Handle(Geom_Curve)& theCurve,
                                 double                    theUFirst,
                                 double                    theULast)
{
  myFirst = theUFirst;
  myLast  = theULast;
  myTrsf.reset();  // Clear transformation on new curve load

  if (myCurve != theCurve)
  {
    myCurve    = theCurve;
    myEvalData = std::monostate{};

    const Handle(Standard_Type)& aType = theCurve->DynamicType();
    if (aType == STANDARD_TYPE(Geom_TrimmedCurve))
    {
      Load(Handle(Geom_TrimmedCurve)::DownCast(theCurve)->BasisCurve(), theUFirst, theULast);
    }
    else if (aType == STANDARD_TYPE(Geom_Circle))
    {
      myTypeCurve = GeomAbs_Circle;
    }
    else if (aType == STANDARD_TYPE(Geom_Line))
    {
      myTypeCurve = GeomAbs_Line;
    }
    else if (aType == STANDARD_TYPE(Geom_Ellipse))
    {
      myTypeCurve = GeomAbs_Ellipse;
    }
    else if (aType == STANDARD_TYPE(Geom_Parabola))
    {
      myTypeCurve = GeomAbs_Parabola;
    }
    else if (aType == STANDARD_TYPE(Geom_Hyperbola))
    {
      myTypeCurve = GeomAbs_Hyperbola;
    }
    else if (aType == STANDARD_TYPE(Geom_BezierCurve))
    {
      myTypeCurve = GeomAbs_BezierCurve;
      myEvalData  = BezierData{};
    }
    else if (aType == STANDARD_TYPE(Geom_BSplineCurve))
    {
      myTypeCurve = GeomAbs_BSplineCurve;
      BSplineData aBSplineData;
      aBSplineData.Curve = Handle(Geom_BSplineCurve)::DownCast(theCurve);
      myEvalData         = std::move(aBSplineData);
    }
    else if (aType == STANDARD_TYPE(Geom_OffsetCurve))
    {
      myTypeCurve                            = GeomAbs_OffsetCurve;
      Handle(Geom_OffsetCurve) anOffsetCurve = Handle(Geom_OffsetCurve)::DownCast(theCurve);
      OffsetData               anOffsetData;
      anOffsetData.BasisCore =
        std::make_unique<GeomAdaptor_CurveCore>(anOffsetCurve->BasisCurve());
      anOffsetData.Offset    = anOffsetCurve->Offset();
      anOffsetData.Direction = anOffsetCurve->Direction();
      myEvalData             = std::move(anOffsetData);
    }
    else
    {
      myTypeCurve = GeomAbs_OtherCurve;
    }
  }
  else
  {
    // Same curve, but need to invalidate cache if bounds changed
    if (auto* aBSplineData = std::get_if<BSplineData>(&myEvalData))
    {
      aBSplineData->Cache.Nullify();
    }
    else if (auto* aBezierData = std::get_if<BezierData>(&myEvalData))
    {
      aBezierData->Cache.Nullify();
    }
  }
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_CurveCore::localContinuity(double theU1, double theU2) const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_BSplineCurve, " ");
  const auto&                    aBSpl  = std::get<BSplineData>(myEvalData).Curve;
  int                            aNb    = aBSpl->NbKnots();
  int                            anIdx1 = 0;
  int                            anIdx2 = 0;
  double                         aNewFirst, aNewLast;
  const TColStd_Array1OfReal&    aTK = aBSpl->Knots();
  const TColStd_Array1OfInteger& aTM = aBSpl->Multiplicities();
  BSplCLib::LocateParameter(aBSpl->Degree(),
                            aTK,
                            aTM,
                            theU1,
                            aBSpl->IsPeriodic(),
                            1,
                            aNb,
                            anIdx1,
                            aNewFirst);
  BSplCLib::LocateParameter(aBSpl->Degree(),
                            aTK,
                            aTM,
                            theU2,
                            aBSpl->IsPeriodic(),
                            1,
                            aNb,
                            anIdx2,
                            aNewLast);
  if (std::abs(aNewFirst - aTK(anIdx1 + 1)) < Precision::PConfusion())
  {
    if (anIdx1 < aNb)
      anIdx1++;
  }
  if (std::abs(aNewLast - aTK(anIdx2)) < Precision::PConfusion())
    anIdx2--;
  int aMultMax;
  if ((aBSpl->IsPeriodic()) && (anIdx1 == aNb))
    anIdx1 = 1;

  if ((anIdx2 - anIdx1 <= 0) && (!aBSpl->IsPeriodic()))
  {
    aMultMax = 100;  // CN between 2 consecutive knots
  }
  else
  {
    aMultMax = aTM(anIdx1 + 1);
    for (int i = anIdx1 + 1; i <= anIdx2; i++)
    {
      if (aTM(i) > aMultMax)
        aMultMax = aTM(i);
    }
    aMultMax = aBSpl->Degree() - aMultMax;
  }
  if (aMultMax <= 0)
  {
    return GeomAbs_C0;
  }
  else if (aMultMax == 1)
  {
    return GeomAbs_C1;
  }
  else if (aMultMax == 2)
  {
    return GeomAbs_C2;
  }
  else if (aMultMax == 3)
  {
    return GeomAbs_C3;
  }
  else
  {
    return GeomAbs_CN;
  }
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_CurveCore::Continuity() const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
    return localContinuity(myFirst, myLast);

  if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    const GeomAbs_Shape S = Handle(Geom_OffsetCurve)::DownCast(myCurve)->GetBasisCurveContinuity();
    switch (S)
    {
      case GeomAbs_CN:
        return GeomAbs_CN;
      case GeomAbs_C3:
        return GeomAbs_C2;
      case GeomAbs_C2:
        return GeomAbs_C1;
      case GeomAbs_C1:
        return GeomAbs_C0;
      case GeomAbs_G1:
        return GeomAbs_G1;
      case GeomAbs_G2:
        return GeomAbs_G2;
      default:
        throw Standard_NoSuchObject("GeomAdaptor_CurveCore::Continuity");
    }
  }
  else if (myTypeCurve == GeomAbs_OtherCurve)
  {
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::Continuity");
  }

  return GeomAbs_CN;
}

//==================================================================================================

int GeomAdaptor_CurveCore::NbIntervals(GeomAbs_Shape theS) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    const auto& aBSpl = std::get<BSplineData>(myEvalData).Curve;
    if ((!aBSpl->IsPeriodic() && theS <= Continuity()) || theS == GeomAbs_C0)
    {
      return 1;
    }

    int aDegree = aBSpl->Degree();
    int aCont;

    switch (theS)
    {
      case GeomAbs_C1:
        aCont = 1;
        break;
      case GeomAbs_C2:
        aCont = 2;
        break;
      case GeomAbs_C3:
        aCont = 3;
        break;
      case GeomAbs_CN:
        aCont = aDegree;
        break;
      default:
        throw Standard_DomainError("GeomAdaptor_CurveCore::NbIntervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    return BSplCLib::Intervals(aBSpl->Knots(),
                               aBSpl->Multiplicities(),
                               aDegree,
                               aBSpl->IsPeriodic(),
                               aCont,
                               myFirst,
                               myLast,
                               anEps,
                               nullptr);
  }
  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    int           aNbIntervals = 1;
    GeomAbs_Shape aBaseS       = GeomAbs_C0;
    switch (theS)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("GeomAdaptor_CurveCore::NbIntervals");
        break;
      case GeomAbs_C0:
        aBaseS = GeomAbs_C1;
        break;
      case GeomAbs_C1:
        aBaseS = GeomAbs_C2;
        break;
      case GeomAbs_C2:
        aBaseS = GeomAbs_C3;
        break;
      default:
        aBaseS = GeomAbs_CN;
    }
    GeomAdaptor_CurveCore aBaseCurve(
      Handle(Geom_OffsetCurve)::DownCast(myCurve)->BasisCurve(),
      myFirst,
      myLast);
    int iNbBasisInt = aBaseCurve.NbIntervals(aBaseS);
    if (iNbBasisInt > 1)
    {
      TColStd_Array1OfReal rdfInter(1, 1 + iNbBasisInt);
      aBaseCurve.Intervals(rdfInter, aBaseS);
      for (int iInt = 1; iInt <= iNbBasisInt; iInt++)
        if (rdfInter(iInt) > myFirst && rdfInter(iInt) < myLast)
          aNbIntervals++;
    }
    return aNbIntervals;
  }
  else
  {
    return 1;
  }
}

//==================================================================================================

void GeomAdaptor_CurveCore::Intervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    const auto& aBSpl = std::get<BSplineData>(myEvalData).Curve;
    if ((!aBSpl->IsPeriodic() && theS <= Continuity()) || theS == GeomAbs_C0)
    {
      theT(theT.Lower())     = myFirst;
      theT(theT.Lower() + 1) = myLast;
      return;
    }

    int aDegree = aBSpl->Degree();
    int aCont;

    switch (theS)
    {
      case GeomAbs_C1:
        aCont = 1;
        break;
      case GeomAbs_C2:
        aCont = 2;
        break;
      case GeomAbs_C3:
        aCont = 3;
        break;
      case GeomAbs_CN:
        aCont = aDegree;
        break;
      default:
        throw Standard_DomainError("GeomAdaptor_CurveCore::Intervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    BSplCLib::Intervals(aBSpl->Knots(),
                        aBSpl->Multiplicities(),
                        aDegree,
                        aBSpl->IsPeriodic(),
                        aCont,
                        myFirst,
                        myLast,
                        anEps,
                        &theT);
  }
  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    int           aNbIntervals = 1;
    GeomAbs_Shape aBaseS       = GeomAbs_C0;
    switch (theS)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("GeomAdaptor_CurveCore::Intervals");
        break;
      case GeomAbs_C0:
        aBaseS = GeomAbs_C1;
        break;
      case GeomAbs_C1:
        aBaseS = GeomAbs_C2;
        break;
      case GeomAbs_C2:
        aBaseS = GeomAbs_C3;
        break;
      default:
        aBaseS = GeomAbs_CN;
    }
    GeomAdaptor_CurveCore aBaseCurve(
      Handle(Geom_OffsetCurve)::DownCast(myCurve)->BasisCurve(),
      myFirst,
      myLast);
    int iNbBasisInt = aBaseCurve.NbIntervals(aBaseS);
    if (iNbBasisInt > 1)
    {
      TColStd_Array1OfReal rdfInter(1, 1 + iNbBasisInt);
      aBaseCurve.Intervals(rdfInter, aBaseS);
      for (int iInt = 1; iInt <= iNbBasisInt; iInt++)
        if (rdfInter(iInt) > myFirst && rdfInter(iInt) < myLast)
          theT(++aNbIntervals) = rdfInter(iInt);
    }
    theT(theT.Lower())               = myFirst;
    theT(theT.Lower() + aNbIntervals) = myLast;
  }
  else
  {
    theT(theT.Lower())     = myFirst;
    theT(theT.Lower() + 1) = myLast;
  }
}

//==================================================================================================

bool GeomAdaptor_CurveCore::IsClosed() const
{
  if (!Precision::IsPositiveInfinite(myLast) && !Precision::IsNegativeInfinite(myFirst))
  {
    const gp_Pnt aPd = Value(myFirst);
    const gp_Pnt aPf = Value(myLast);
    return (aPd.Distance(aPf) <= Precision::Confusion());
  }
  return false;
}

//==================================================================================================

bool GeomAdaptor_CurveCore::IsPeriodic() const
{
  return myCurve->IsPeriodic();
}

//==================================================================================================

double GeomAdaptor_CurveCore::Period() const
{
  return myCurve->LastParameter() - myCurve->FirstParameter();
}

//==================================================================================================

void GeomAdaptor_CurveCore::rebuildCache(double theParameter) const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
  {
    auto&                    aCache  = std::get<BezierData>(myEvalData).Cache;
    Handle(Geom_BezierCurve) aBezier = Handle(Geom_BezierCurve)::DownCast(myCurve);
    int                      aDeg    = aBezier->Degree();
    TColStd_Array1OfReal     aFlatKnots(BSplCLib::FlatBezierKnots(aDeg), 1, 2 * (aDeg + 1));
    if (aCache.IsNull())
      aCache = new BSplCLib_Cache(aDeg,
                                  aBezier->IsPeriodic(),
                                  aFlatKnots,
                                  aBezier->Poles(),
                                  aBezier->Weights());
    aCache->BuildCache(theParameter, aFlatKnots, aBezier->Poles(), aBezier->Weights());
  }
  else if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    auto&       aBSplData = std::get<BSplineData>(myEvalData);
    const auto& aBSpl     = aBSplData.Curve;
    auto&       aCache    = aBSplData.Cache;
    if (aCache.IsNull())
      aCache = new BSplCLib_Cache(aBSpl->Degree(),
                                  aBSpl->IsPeriodic(),
                                  aBSpl->KnotSequence(),
                                  aBSpl->Poles(),
                                  aBSpl->Weights());
    aCache->BuildCache(theParameter, aBSpl->KnotSequence(), aBSpl->Poles(), aBSpl->Weights());
  }
}

//==================================================================================================

bool GeomAdaptor_CurveCore::isBoundary(double theU, int& theSpanStart, int& theSpanFinish) const
{
  const auto* aBSplData = std::get_if<BSplineData>(&myEvalData);
  if (aBSplData != nullptr && (theU == myFirst || theU == myLast))
  {
    const auto& aBSpl = aBSplData->Curve;
    if (theU == myFirst)
    {
      aBSpl->LocateU(myFirst, PosTol, theSpanStart, theSpanFinish);
      if (theSpanStart < 1)
        theSpanStart = 1;
      if (theSpanStart >= theSpanFinish)
        theSpanFinish = theSpanStart + 1;
    }
    else if (theU == myLast)
    {
      aBSpl->LocateU(myLast, PosTol, theSpanStart, theSpanFinish);
      if (theSpanFinish > aBSpl->NbKnots())
        theSpanFinish = aBSpl->NbKnots();
      if (theSpanStart >= theSpanFinish)
        theSpanStart = theSpanFinish - 1;
    }
    return true;
  }
  return false;
}

//==================================================================================================

gp_Pnt GeomAdaptor_CurveCore::Value(double theU) const
{
  gp_Pnt aValue;
  D0(theU, aValue);
  return aValue;
}

//==================================================================================================

void GeomAdaptor_CurveCore::D0(double theU, gp_Pnt& theP) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(theU))
        rebuildCache(theU);
      aCache->D0(theU, theP);
      break;
    }

    case GeomAbs_BSplineCurve: {
      int   aStart = 0, aFinish = 0;
      auto& aBSplData = std::get<BSplineData>(myEvalData);
      if (isBoundary(theU, aStart, aFinish))
      {
        aBSplData.Curve->LocalD0(theU, aStart, aFinish, theP);
      }
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(theU))
          rebuildCache(theU);
        aBSplData.Cache->D0(theU, theP);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom_OffsetCurveUtils::EvaluateD0(theU,
                                             anOffsetData.BasisCore.get(),
                                             anOffsetData.Direction,
                                             anOffsetData.Offset,
                                             theP))
      {
        throw Standard_NullValue("GeomAdaptor_CurveCore::D0: Unable to calculate offset point");
      }
      break;
    }

    default:
      myCurve->D0(theU, theP);
  }

  // Apply transformation if set
  applyTransform(theP);
}

//==================================================================================================

void GeomAdaptor_CurveCore::D1(double theU, gp_Pnt& theP, gp_Vec& theV) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(theU))
        rebuildCache(theU);
      aCache->D1(theU, theP, theV);
      break;
    }

    case GeomAbs_BSplineCurve: {
      int   aStart = 0, aFinish = 0;
      auto& aBSplData = std::get<BSplineData>(myEvalData);
      if (isBoundary(theU, aStart, aFinish))
      {
        aBSplData.Curve->LocalD1(theU, aStart, aFinish, theP, theV);
      }
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(theU))
          rebuildCache(theU);
        aBSplData.Cache->D1(theU, theP, theV);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom_OffsetCurveUtils::EvaluateD1(theU,
                                             anOffsetData.BasisCore.get(),
                                             anOffsetData.Direction,
                                             anOffsetData.Offset,
                                             theP,
                                             theV))
      {
        throw Standard_NullValue("GeomAdaptor_CurveCore::D1: Unable to calculate offset D1");
      }
      break;
    }

    default:
      myCurve->D1(theU, theP, theV);
  }

  // Apply transformation if set
  applyTransform(theP);
  applyTransform(theV);
}

//==================================================================================================

void GeomAdaptor_CurveCore::D2(double theU, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(theU))
        rebuildCache(theU);
      aCache->D2(theU, theP, theV1, theV2);
      break;
    }

    case GeomAbs_BSplineCurve: {
      int   aStart = 0, aFinish = 0;
      auto& aBSplData = std::get<BSplineData>(myEvalData);
      if (isBoundary(theU, aStart, aFinish))
      {
        aBSplData.Curve->LocalD2(theU, aStart, aFinish, theP, theV1, theV2);
      }
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(theU))
          rebuildCache(theU);
        aBSplData.Cache->D2(theU, theP, theV1, theV2);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom_OffsetCurveUtils::EvaluateD2(theU,
                                             anOffsetData.BasisCore.get(),
                                             anOffsetData.Direction,
                                             anOffsetData.Offset,
                                             theP,
                                             theV1,
                                             theV2))
      {
        throw Standard_NullValue("GeomAdaptor_CurveCore::D2: Unable to calculate offset D2");
      }
      break;
    }

    default:
      myCurve->D2(theU, theP, theV1, theV2);
  }

  // Apply transformation if set
  applyTransform(theP);
  applyTransform(theV1);
  applyTransform(theV2);
}

//==================================================================================================

void GeomAdaptor_CurveCore::D3(double   theU,
                               gp_Pnt&  theP,
                               gp_Vec&  theV1,
                               gp_Vec&  theV2,
                               gp_Vec&  theV3) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(theU))
        rebuildCache(theU);
      aCache->D3(theU, theP, theV1, theV2, theV3);
      break;
    }

    case GeomAbs_BSplineCurve: {
      int   aStart = 0, aFinish = 0;
      auto& aBSplData = std::get<BSplineData>(myEvalData);
      if (isBoundary(theU, aStart, aFinish))
      {
        aBSplData.Curve->LocalD3(theU, aStart, aFinish, theP, theV1, theV2, theV3);
      }
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(theU))
          rebuildCache(theU);
        aBSplData.Cache->D3(theU, theP, theV1, theV2, theV3);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom_OffsetCurveUtils::EvaluateD3(theU,
                                             anOffsetData.BasisCore.get(),
                                             anOffsetData.Direction,
                                             anOffsetData.Offset,
                                             theP,
                                             theV1,
                                             theV2,
                                             theV3))
      {
        throw Standard_NullValue("GeomAdaptor_CurveCore::D3: Unable to calculate offset D3");
      }
      break;
    }

    default:
      myCurve->D3(theU, theP, theV1, theV2, theV3);
  }

  // Apply transformation if set
  applyTransform(theP);
  applyTransform(theV1);
  applyTransform(theV2);
  applyTransform(theV3);
}

//==================================================================================================

gp_Vec GeomAdaptor_CurveCore::DN(double theU, int theN) const
{
  gp_Vec aDN;

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
      aDN = myCurve->DN(theU, theN);
      break;

    case GeomAbs_BSplineCurve: {
      int aStart = 0, aFinish = 0;
      if (isBoundary(theU, aStart, aFinish))
      {
        aDN = std::get<BSplineData>(myEvalData).Curve->LocalDN(theU, aStart, aFinish, theN);
      }
      else
      {
        aDN = myCurve->DN(theU, theN);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom_OffsetCurveUtils::EvaluateDN(theU,
                                             anOffsetData.BasisCore.get(),
                                             anOffsetData.Direction,
                                             anOffsetData.Offset,
                                             theN,
                                             aDN))
      {
        throw Standard_NullValue("GeomAdaptor_CurveCore::DN: Unable to calculate offset DN");
      }
      break;
    }

    default:
      aDN = myCurve->DN(theU, theN);
  }

  // Apply transformation if set
  applyTransform(aDN);
  return aDN;
}

//==================================================================================================

double GeomAdaptor_CurveCore::Resolution(double theR3d) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_Line:
      return theR3d;
    case GeomAbs_Circle: {
      double R = Handle(Geom_Circle)::DownCast(myCurve)->Circ().Radius();
      if (R > theR3d / 2.)
        return 2 * std::asin(theR3d / (2 * R));
      else
        return 2 * M_PI;
    }
    case GeomAbs_Ellipse: {
      return theR3d / Handle(Geom_Ellipse)::DownCast(myCurve)->MajorRadius();
    }
    case GeomAbs_BezierCurve: {
      double res;
      Handle(Geom_BezierCurve)::DownCast(myCurve)->Resolution(theR3d, res);
      return res;
    }
    case GeomAbs_BSplineCurve: {
      double res;
      std::get<BSplineData>(myEvalData).Curve->Resolution(theR3d, res);
      return res;
    }
    default:
      return Precision::Parametric(theR3d);
  }
}

//==================================================================================================

gp_Lin GeomAdaptor_CurveCore::Line() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Line,
                                 "GeomAdaptor_CurveCore::Line() - curve is not a Line");
  gp_Lin aLine = Handle(Geom_Line)::DownCast(myCurve)->Lin();
  if (myTrsf.has_value())
  {
    aLine.Transform(*myTrsf);
  }
  return aLine;
}

//==================================================================================================

gp_Circ GeomAdaptor_CurveCore::Circle() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Circle,
                                 "GeomAdaptor_CurveCore::Circle() - curve is not a Circle");
  gp_Circ aCirc = Handle(Geom_Circle)::DownCast(myCurve)->Circ();
  if (myTrsf.has_value())
  {
    aCirc.Transform(*myTrsf);
  }
  return aCirc;
}

//==================================================================================================

gp_Elips GeomAdaptor_CurveCore::Ellipse() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Ellipse,
                                 "GeomAdaptor_CurveCore::Ellipse() - curve is not an Ellipse");
  gp_Elips anElips = Handle(Geom_Ellipse)::DownCast(myCurve)->Elips();
  if (myTrsf.has_value())
  {
    anElips.Transform(*myTrsf);
  }
  return anElips;
}

//==================================================================================================

gp_Hypr GeomAdaptor_CurveCore::Hyperbola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Hyperbola,
                                 "GeomAdaptor_CurveCore::Hyperbola() - curve is not a Hyperbola");
  gp_Hypr aHypr = Handle(Geom_Hyperbola)::DownCast(myCurve)->Hypr();
  if (myTrsf.has_value())
  {
    aHypr.Transform(*myTrsf);
  }
  return aHypr;
}

//==================================================================================================

gp_Parab GeomAdaptor_CurveCore::Parabola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Parabola,
                                 "GeomAdaptor_CurveCore::Parabola() - curve is not a Parabola");
  gp_Parab aParab = Handle(Geom_Parabola)::DownCast(myCurve)->Parab();
  if (myTrsf.has_value())
  {
    aParab.Transform(*myTrsf);
  }
  return aParab;
}

//==================================================================================================

int GeomAdaptor_CurveCore::Degree() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom_BezierCurve)::DownCast(myCurve)->Degree();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return std::get<BSplineData>(myEvalData).Curve->Degree();
  else
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::Degree");
}

//==================================================================================================

bool GeomAdaptor_CurveCore::IsRational() const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BSplineCurve:
      return std::get<BSplineData>(myEvalData).Curve->IsRational();
    case GeomAbs_BezierCurve:
      return Handle(Geom_BezierCurve)::DownCast(myCurve)->IsRational();
    default:
      return false;
  }
}

//==================================================================================================

int GeomAdaptor_CurveCore::NbPoles() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom_BezierCurve)::DownCast(myCurve)->NbPoles();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return std::get<BSplineData>(myEvalData).Curve->NbPoles();
  else
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::NbPoles");
}

//==================================================================================================

int GeomAdaptor_CurveCore::NbKnots() const
{
  if (myTypeCurve != GeomAbs_BSplineCurve)
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::NbKnots");
  return std::get<BSplineData>(myEvalData).Curve->NbKnots();
}

//==================================================================================================

Handle(Geom_BezierCurve) GeomAdaptor_CurveCore::Bezier() const
{
  if (myTypeCurve != GeomAbs_BezierCurve)
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::Bezier");
  return Handle(Geom_BezierCurve)::DownCast(myCurve);
}

//==================================================================================================

Handle(Geom_BSplineCurve) GeomAdaptor_CurveCore::BSpline() const
{
  if (myTypeCurve != GeomAbs_BSplineCurve)
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::BSpline");
  return std::get<BSplineData>(myEvalData).Curve;
}

//==================================================================================================

Handle(Geom_OffsetCurve) GeomAdaptor_CurveCore::OffsetCurve() const
{
  if (myTypeCurve != GeomAbs_OffsetCurve)
    throw Standard_NoSuchObject("GeomAdaptor_CurveCore::OffsetCurve");
  return Handle(Geom_OffsetCurve)::DownCast(myCurve);
}
