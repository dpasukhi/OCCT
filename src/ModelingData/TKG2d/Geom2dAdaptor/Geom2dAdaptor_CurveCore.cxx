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

#define No_Standard_RangeError
#define No_Standard_OutOfRange

#include <Geom2dAdaptor_CurveCore.hxx>

#include <BSplCLib.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Standard_DomainError.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_NullValue.hxx>
#include <TColStd_Array1OfInteger.hxx>

#include "../Geom2d/Geom2d_OffsetCurveUtils.pxx"

static const double PosTol = Precision::PConfusion() / 2;

//==================================================================================================

Geom2dAdaptor_CurveCore::Geom2dAdaptor_CurveCore()
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
}

//==================================================================================================

Geom2dAdaptor_CurveCore::~Geom2dAdaptor_CurveCore() = default;

//==================================================================================================

Geom2dAdaptor_CurveCore::Geom2dAdaptor_CurveCore(const Handle(Geom2d_Curve)& theCurve)
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
  if (!theCurve.IsNull())
  {
    load(theCurve, theCurve->FirstParameter(), theCurve->LastParameter());
  }
}

//==================================================================================================

Geom2dAdaptor_CurveCore::Geom2dAdaptor_CurveCore(const Handle(Geom2d_Curve)& theCurve,
                                                 double                      theUFirst,
                                                 double                      theULast)
    : myTypeCurve(GeomAbs_OtherCurve),
      myFirst(0.0),
      myLast(0.0)
{
  if (theCurve.IsNull())
  {
    throw Standard_NullObject("Geom2dAdaptor_CurveCore - null curve");
  }
  if (theUFirst > theULast + Precision::Confusion())
  {
    throw Standard_ConstructionError("Geom2dAdaptor_CurveCore - invalid parameter range");
  }
  load(theCurve, theUFirst, theULast);
}

//==================================================================================================

Geom2dAdaptor_CurveCore::Geom2dAdaptor_CurveCore(const Geom2dAdaptor_CurveCore& theOther)
    : myCurve(theOther.myCurve),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myTrsf(theOther.myTrsf),
      myParamModifier(theOther.myParamModifier),
      myPostProcessor(theOther.myPostProcessor)
{
  // Deep copy evaluation data based on variant type
  if (const auto* anOffsetData = std::get_if<OffsetData>(&theOther.myEvalData))
  {
    OffsetData aCopyData;
    if (anOffsetData->BasisCore)
    {
      aCopyData.BasisCore = std::make_unique<Geom2dAdaptor_CurveCore>(*anOffsetData->BasisCore);
    }
    aCopyData.Offset = anOffsetData->Offset;
    myEvalData       = std::move(aCopyData);
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
    aCopyData.Curves = aPiecewiseData->Curves; // Vector copy
    aCopyData.Knots  = aPiecewiseData->Knots;
    myEvalData       = std::move(aCopyData);
  }
}

//==================================================================================================

Geom2dAdaptor_CurveCore::Geom2dAdaptor_CurveCore(Geom2dAdaptor_CurveCore&& theOther) noexcept
    : myCurve(std::move(theOther.myCurve)),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myEvalData(std::move(theOther.myEvalData)),
      myTrsf(std::move(theOther.myTrsf)),
      myParamModifier(std::move(theOther.myParamModifier)),
      myPostProcessor(std::move(theOther.myPostProcessor))
{
}

//==================================================================================================

Geom2dAdaptor_CurveCore& Geom2dAdaptor_CurveCore::operator=(
  const Geom2dAdaptor_CurveCore& theOther)
{
  if (this != &theOther)
  {
    myCurve         = theOther.myCurve;
    myTypeCurve     = theOther.myTypeCurve;
    myFirst         = theOther.myFirst;
    myLast          = theOther.myLast;
    myTrsf          = theOther.myTrsf;
    myParamModifier = theOther.myParamModifier;
    myPostProcessor = theOther.myPostProcessor;

    // Deep copy evaluation data
    if (const auto* anOffsetData = std::get_if<OffsetData>(&theOther.myEvalData))
    {
      OffsetData aCopyData;
      if (anOffsetData->BasisCore)
      {
        aCopyData.BasisCore = std::make_unique<Geom2dAdaptor_CurveCore>(*anOffsetData->BasisCore);
      }
      aCopyData.Offset = anOffsetData->Offset;
      myEvalData       = std::move(aCopyData);
    }
    else if (const auto* aBSplineData = std::get_if<BSplineData>(&theOther.myEvalData))
    {
      BSplineData aCopyData;
      aCopyData.Curve = aBSplineData->Curve;
      myEvalData      = std::move(aCopyData);
    }
    else if (std::holds_alternative<BezierData>(theOther.myEvalData))
    {
      myEvalData = BezierData{};
    }
    else if (const auto* aPiecewiseData = std::get_if<PiecewiseData>(&theOther.myEvalData))
    {
      PiecewiseData aCopyData;
      aCopyData.Curves = aPiecewiseData->Curves;
      aCopyData.Knots  = aPiecewiseData->Knots;
      myEvalData       = std::move(aCopyData);
    }
    else
    {
      myEvalData = std::monostate{};
    }
  }
  return *this;
}

//==================================================================================================

Geom2dAdaptor_CurveCore& Geom2dAdaptor_CurveCore::operator=(
  Geom2dAdaptor_CurveCore&& theOther) noexcept
{
  if (this != &theOther)
  {
    myCurve         = std::move(theOther.myCurve);
    myTypeCurve     = theOther.myTypeCurve;
    myFirst         = theOther.myFirst;
    myLast          = theOther.myLast;
    myEvalData      = std::move(theOther.myEvalData);
    myTrsf          = std::move(theOther.myTrsf);
    myParamModifier = std::move(theOther.myParamModifier);
    myPostProcessor = std::move(theOther.myPostProcessor);
  }
  return *this;
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::Reset()
{
  myCurve.Nullify();
  myTypeCurve     = GeomAbs_OtherCurve;
  myFirst = myLast = 0.0;
  myEvalData      = std::monostate{};
  myTrsf.reset();
  myParamModifier = std::monostate{};
  myPostProcessor = std::monostate{};
}

//==================================================================================================

const gp_Trsf2d& Geom2dAdaptor_CurveCore::Transformation() const
{
  if (!myTrsf.has_value())
  {
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::Transformation - no transformation set");
  }
  return *myTrsf;
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::load(const Handle(Geom2d_Curve)& theCurve,
                                   double                      theUFirst,
                                   double                      theULast)
{
  myFirst = theUFirst;
  myLast  = theULast;

  if (myCurve != theCurve)
  {
    myCurve    = theCurve;
    myEvalData = std::monostate{};

    Handle(Standard_Type) aType = theCurve->DynamicType();
    if (aType == STANDARD_TYPE(Geom2d_TrimmedCurve))
    {
      load(Handle(Geom2d_TrimmedCurve)::DownCast(theCurve)->BasisCurve(), theUFirst, theULast);
    }
    else if (aType == STANDARD_TYPE(Geom2d_Circle))
    {
      myTypeCurve = GeomAbs_Circle;
    }
    else if (aType == STANDARD_TYPE(Geom2d_Line))
    {
      myTypeCurve = GeomAbs_Line;
    }
    else if (aType == STANDARD_TYPE(Geom2d_Ellipse))
    {
      myTypeCurve = GeomAbs_Ellipse;
    }
    else if (aType == STANDARD_TYPE(Geom2d_Parabola))
    {
      myTypeCurve = GeomAbs_Parabola;
    }
    else if (aType == STANDARD_TYPE(Geom2d_Hyperbola))
    {
      myTypeCurve = GeomAbs_Hyperbola;
    }
    else if (aType == STANDARD_TYPE(Geom2d_BezierCurve))
    {
      myTypeCurve = GeomAbs_BezierCurve;
      myEvalData  = BezierData{};
    }
    else if (aType == STANDARD_TYPE(Geom2d_BSplineCurve))
    {
      myTypeCurve = GeomAbs_BSplineCurve;
      BSplineData aBSplineData;
      aBSplineData.Curve = Handle(Geom2d_BSplineCurve)::DownCast(myCurve);
      myEvalData         = std::move(aBSplineData);
    }
    else if (aType == STANDARD_TYPE(Geom2d_OffsetCurve))
    {
      myTypeCurve = GeomAbs_OffsetCurve;
      Handle(Geom2d_OffsetCurve) anOffsetCurve = Handle(Geom2d_OffsetCurve)::DownCast(myCurve);
      Handle(Geom2d_Curve) aBaseCurve = anOffsetCurve->BasisCurve();

      OffsetData anOffsetData;
      anOffsetData.BasisCore = std::make_unique<Geom2dAdaptor_CurveCore>(aBaseCurve, theUFirst, theULast);
      anOffsetData.Offset    = anOffsetCurve->Offset();
      myEvalData             = std::move(anOffsetData);
    }
    else
    {
      myTypeCurve = GeomAbs_OtherCurve;
    }
  }
  else
  {
    // Same curve but potentially different parameters - invalidate cache
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

void Geom2dAdaptor_CurveCore::rebuildCache(double theParameter) const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
  {
    auto&                      aBezierData = std::get<BezierData>(myEvalData);
    Handle(Geom2d_BezierCurve) aBezier     = Handle(Geom2d_BezierCurve)::DownCast(myCurve);
    int                        aDeg        = aBezier->Degree();
    TColStd_Array1OfReal       aFlatKnots(BSplCLib::FlatBezierKnots(aDeg), 1, 2 * (aDeg + 1));
    if (aBezierData.Cache.IsNull())
    {
      aBezierData.Cache = new BSplCLib_Cache(aDeg,
                                             aBezier->IsPeriodic(),
                                             aFlatKnots,
                                             aBezier->Poles(),
                                             aBezier->Weights());
    }
    aBezierData.Cache->BuildCache(theParameter, aFlatKnots, aBezier->Poles(), aBezier->Weights());
  }
  else if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    auto&       aBSplineData = std::get<BSplineData>(myEvalData);
    const auto& aBSpline     = aBSplineData.Curve;
    if (aBSplineData.Cache.IsNull())
    {
      aBSplineData.Cache = new BSplCLib_Cache(aBSpline->Degree(),
                                              aBSpline->IsPeriodic(),
                                              aBSpline->KnotSequence(),
                                              aBSpline->Poles(),
                                              aBSpline->Weights());
    }
    aBSplineData.Cache->BuildCache(theParameter,
                                   aBSpline->KnotSequence(),
                                   aBSpline->Poles(),
                                   aBSpline->Weights());
  }
}

//==================================================================================================

bool Geom2dAdaptor_CurveCore::isBoundary(double theU, int& theSpanStart, int& theSpanFinish) const
{
  const auto* aBSplineData = std::get_if<BSplineData>(&myEvalData);
  if (aBSplineData != nullptr && !aBSplineData->Curve.IsNull()
      && (theU == myFirst || theU == myLast))
  {
    const auto& aBSpline = aBSplineData->Curve;
    if (theU == myFirst)
    {
      aBSpline->LocateU(myFirst, PosTol, theSpanStart, theSpanFinish);
      if (theSpanStart < 1)
        theSpanStart = 1;
      if (theSpanStart >= theSpanFinish)
        theSpanFinish = theSpanStart + 1;
    }
    else if (theU == myLast)
    {
      aBSpline->LocateU(myLast, PosTol, theSpanStart, theSpanFinish);
      if (theSpanFinish > aBSpline->NbKnots())
        theSpanFinish = aBSpline->NbKnots();
      if (theSpanStart >= theSpanFinish)
        theSpanStart = theSpanFinish - 1;
    }
    return true;
  }
  return false;
}

//==================================================================================================

GeomAbs_Shape Geom2dAdaptor_CurveCore::localContinuity(double theU1, double theU2) const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_BSplineCurve, " ");
  const auto&             aBSpline = std::get<BSplineData>(myEvalData).Curve;
  int                     aNb      = aBSpline->NbKnots();
  int                     anIndex1 = 0;
  int                     anIndex2 = 0;
  double                  aNewFirst, aNewLast;
  TColStd_Array1OfReal    aTK(1, aNb);
  TColStd_Array1OfInteger aTM(1, aNb);
  aBSpline->Knots(aTK);
  aBSpline->Multiplicities(aTM);
  BSplCLib::LocateParameter(aBSpline->Degree(),
                            aTK,
                            aTM,
                            theU1,
                            aBSpline->IsPeriodic(),
                            1,
                            aNb,
                            anIndex1,
                            aNewFirst);
  BSplCLib::LocateParameter(aBSpline->Degree(),
                            aTK,
                            aTM,
                            theU2,
                            aBSpline->IsPeriodic(),
                            1,
                            aNb,
                            anIndex2,
                            aNewLast);
  if (std::abs(aNewFirst - aTK(anIndex1 + 1)) < Precision::PConfusion())
  {
    if (anIndex1 < aNb)
      anIndex1++;
  }
  if (std::abs(aNewLast - aTK(anIndex2)) < Precision::PConfusion())
    anIndex2--;
  int aMultMax;
  // Handle periodic curves
  if (aBSpline->IsPeriodic() && anIndex1 == aNb)
    anIndex1 = 1;

  if ((anIndex2 - anIndex1 <= 0) && (!aBSpline->IsPeriodic()))
  {
    aMultMax = 100; // CN between 2 consecutive knots
  }
  else
  {
    aMultMax = aTM(anIndex1 + 1);
    for (int i = anIndex1 + 1; i <= anIndex2; i++)
    {
      if (aTM(i) > aMultMax)
        aMultMax = aTM(i);
    }
    aMultMax = aBSpline->Degree() - aMultMax;
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

GeomAbs_Shape Geom2dAdaptor_CurveCore::Continuity() const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    return localContinuity(myFirst, myLast);
  }
  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    GeomAbs_Shape S = Handle(Geom2d_OffsetCurve)::DownCast(myCurve)->GetBasisCurveContinuity();
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
        throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::Continuity");
    }
  }
  else if (myTypeCurve == GeomAbs_OtherCurve)
  {
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::Continuity");
  }
  else
  {
    return GeomAbs_CN;
  }
}

//==================================================================================================

int Geom2dAdaptor_CurveCore::NbIntervals(GeomAbs_Shape theS) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    const auto& aBSpline = std::get<BSplineData>(myEvalData).Curve;
    if ((!aBSpline->IsPeriodic() && theS <= Continuity()) || theS == GeomAbs_C0)
    {
      return 1;
    }

    int aDegree = aBSpline->Degree();
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
        throw Standard_DomainError("Geom2dAdaptor_CurveCore::NbIntervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    return BSplCLib::Intervals(aBSpline->Knots(),
                               aBSpline->Multiplicities(),
                               aDegree,
                               aBSpline->IsPeriodic(),
                               aCont,
                               myFirst,
                               myLast,
                               anEps,
                               nullptr);
  }
  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    GeomAbs_Shape aBaseS = GeomAbs_C0;
    switch (theS)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("Geom2dAdaptor_CurveCore::NbIntervals");
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
    const auto& anOffsetData = std::get<OffsetData>(myEvalData);
    if (anOffsetData.BasisCore)
    {
      return anOffsetData.BasisCore->NbIntervals(aBaseS);
    }
    return 1;
  }
  else
  {
    return 1;
  }
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::Intervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    const auto& aBSpline = std::get<BSplineData>(myEvalData).Curve;
    if ((!aBSpline->IsPeriodic() && theS <= Continuity()) || theS == GeomAbs_C0)
    {
      theT(theT.Lower())     = myFirst;
      theT(theT.Lower() + 1) = myLast;
      return;
    }

    int aDegree = aBSpline->Degree();
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
        throw Standard_DomainError("Geom2dAdaptor_CurveCore::Intervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    BSplCLib::Intervals(aBSpline->Knots(),
                        aBSpline->Multiplicities(),
                        aDegree,
                        aBSpline->IsPeriodic(),
                        aCont,
                        myFirst,
                        myLast,
                        anEps,
                        &theT);
  }
  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    GeomAbs_Shape aBaseS = GeomAbs_C0;
    switch (theS)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("Geom2dAdaptor_CurveCore::Intervals");
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
    const auto& anOffsetData = std::get<OffsetData>(myEvalData);
    if (anOffsetData.BasisCore)
    {
      int aNbIntervals = anOffsetData.BasisCore->NbIntervals(aBaseS);
      anOffsetData.BasisCore->Intervals(theT, aBaseS);
      theT(theT.Lower())               = myFirst;
      theT(theT.Lower() + aNbIntervals) = myLast;
    }
    else
    {
      theT(theT.Lower())     = myFirst;
      theT(theT.Lower() + 1) = myLast;
    }
  }
  else
  {
    theT(theT.Lower())     = myFirst;
    theT(theT.Lower() + 1) = myLast;
  }
}

//==================================================================================================

gp_Pnt2d Geom2dAdaptor_CurveCore::Value(double theU) const
{
  gp_Pnt2d aP;
  D0(theU, aP);
  return aP;
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::D0(double theU, gp_Pnt2d& theP) const
{
  // Apply parameter modifier (pre-evaluation)
  const double aU = applyParamModifier(theU);

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aBezierData = std::get<BezierData>(myEvalData);
      if (aBezierData.Cache.IsNull() || !aBezierData.Cache->IsCacheValid(aU))
        rebuildCache(aU);
      aBezierData.Cache->D0(aU, theP);
      break;
    }

    case GeomAbs_BSplineCurve: {
      auto& aBSplineData = std::get<BSplineData>(myEvalData);
      int   aStart = 0, aFinish = 0;
      if (isBoundary(aU, aStart, aFinish))
      {
        aBSplineData.Curve->LocalD0(aU, aStart, aFinish, theP);
      }
      else
      {
        if (aBSplineData.Cache.IsNull() || !aBSplineData.Cache->IsCacheValid(aU))
          rebuildCache(aU);
        aBSplineData.Cache->D0(aU, theP);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom2d_OffsetCurveUtils::EvaluateD0(aU,
                                               anOffsetData.BasisCore.get(),
                                               anOffsetData.Offset,
                                               theP))
      {
        throw Standard_NullValue("Geom2dAdaptor_CurveCore::D0: Unable to calculate offset point");
      }
      break;
    }

    default:
      myCurve->D0(aU, theP);
  }
  applyTransform(theP);
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::D1(double theU, gp_Pnt2d& theP, gp_Vec2d& theV) const
{
  // Apply parameter modifier (pre-evaluation)
  const double aU = applyParamModifier(theU);

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aBezierData = std::get<BezierData>(myEvalData);
      if (aBezierData.Cache.IsNull() || !aBezierData.Cache->IsCacheValid(aU))
        rebuildCache(aU);
      aBezierData.Cache->D1(aU, theP, theV);
      break;
    }

    case GeomAbs_BSplineCurve: {
      auto& aBSplineData = std::get<BSplineData>(myEvalData);
      int   aStart = 0, aFinish = 0;
      if (isBoundary(aU, aStart, aFinish))
      {
        aBSplineData.Curve->LocalD1(aU, aStart, aFinish, theP, theV);
      }
      else
      {
        if (aBSplineData.Cache.IsNull() || !aBSplineData.Cache->IsCacheValid(aU))
          rebuildCache(aU);
        aBSplineData.Cache->D1(aU, theP, theV);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom2d_OffsetCurveUtils::EvaluateD1(aU,
                                               anOffsetData.BasisCore.get(),
                                               anOffsetData.Offset,
                                               theP,
                                               theV))
      {
        throw Standard_NullValue("Geom2dAdaptor_CurveCore::D1: Unable to calculate offset D1");
      }
      break;
    }

    default:
      myCurve->D1(aU, theP, theV);
  }
  applyTransform(theP);
  applyTransform(theV);
  applyPostProcessor(theV, 1);
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::D2(double theU, gp_Pnt2d& theP, gp_Vec2d& theV1, gp_Vec2d& theV2) const
{
  // Apply parameter modifier (pre-evaluation)
  const double aU = applyParamModifier(theU);

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aBezierData = std::get<BezierData>(myEvalData);
      if (aBezierData.Cache.IsNull() || !aBezierData.Cache->IsCacheValid(aU))
        rebuildCache(aU);
      aBezierData.Cache->D2(aU, theP, theV1, theV2);
      break;
    }

    case GeomAbs_BSplineCurve: {
      auto& aBSplineData = std::get<BSplineData>(myEvalData);
      int   aStart = 0, aFinish = 0;
      if (isBoundary(aU, aStart, aFinish))
      {
        aBSplineData.Curve->LocalD2(aU, aStart, aFinish, theP, theV1, theV2);
      }
      else
      {
        if (aBSplineData.Cache.IsNull() || !aBSplineData.Cache->IsCacheValid(aU))
          rebuildCache(aU);
        aBSplineData.Cache->D2(aU, theP, theV1, theV2);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom2d_OffsetCurveUtils::EvaluateD2(aU,
                                               anOffsetData.BasisCore.get(),
                                               anOffsetData.Offset,
                                               theP,
                                               theV1,
                                               theV2))
      {
        throw Standard_NullValue("Geom2dAdaptor_CurveCore::D2: Unable to calculate offset D2");
      }
      break;
    }

    default:
      myCurve->D2(aU, theP, theV1, theV2);
  }
  applyTransform(theP);
  applyTransform(theV1);
  applyTransform(theV2);
  applyPostProcessor(theV1, 1);
  applyPostProcessor(theV2, 2);
}

//==================================================================================================

void Geom2dAdaptor_CurveCore::D3(double    theU,
                                 gp_Pnt2d& theP,
                                 gp_Vec2d& theV1,
                                 gp_Vec2d& theV2,
                                 gp_Vec2d& theV3) const
{
  // Apply parameter modifier (pre-evaluation)
  const double aU = applyParamModifier(theU);

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve: {
      auto& aBezierData = std::get<BezierData>(myEvalData);
      if (aBezierData.Cache.IsNull() || !aBezierData.Cache->IsCacheValid(aU))
        rebuildCache(aU);
      aBezierData.Cache->D3(aU, theP, theV1, theV2, theV3);
      break;
    }

    case GeomAbs_BSplineCurve: {
      auto& aBSplineData = std::get<BSplineData>(myEvalData);
      int   aStart = 0, aFinish = 0;
      if (isBoundary(aU, aStart, aFinish))
      {
        aBSplineData.Curve->LocalD3(aU, aStart, aFinish, theP, theV1, theV2, theV3);
      }
      else
      {
        if (aBSplineData.Cache.IsNull() || !aBSplineData.Cache->IsCacheValid(aU))
          rebuildCache(aU);
        aBSplineData.Cache->D3(aU, theP, theV1, theV2, theV3);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom2d_OffsetCurveUtils::EvaluateD3(aU,
                                               anOffsetData.BasisCore.get(),
                                               anOffsetData.Offset,
                                               theP,
                                               theV1,
                                               theV2,
                                               theV3))
      {
        throw Standard_NullValue("Geom2dAdaptor_CurveCore::D3: Unable to calculate offset D3");
      }
      break;
    }

    default:
      myCurve->D3(aU, theP, theV1, theV2, theV3);
  }
  applyTransform(theP);
  applyTransform(theV1);
  applyTransform(theV2);
  applyTransform(theV3);
  applyPostProcessor(theV1, 1);
  applyPostProcessor(theV2, 2);
  applyPostProcessor(theV3, 3);
}

//==================================================================================================

gp_Vec2d Geom2dAdaptor_CurveCore::DN(double theU, int theN) const
{
  // Apply parameter modifier (pre-evaluation)
  const double aU = applyParamModifier(theU);

  gp_Vec2d aResult;
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
      aResult = myCurve->DN(aU, theN);
      break;

    case GeomAbs_BSplineCurve: {
      int aStart = 0, aFinish = 0;
      if (isBoundary(aU, aStart, aFinish))
      {
        aResult = std::get<BSplineData>(myEvalData).Curve->LocalDN(aU, aStart, aFinish, theN);
      }
      else
      {
        aResult = myCurve->DN(aU, theN);
      }
      break;
    }

    case GeomAbs_OffsetCurve: {
      Standard_RangeError_Raise_if(theN < 1, "Geom2dAdaptor_CurveCore::DN(): N < 1");

      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (!Geom2d_OffsetCurveUtils::EvaluateDN(aU,
                                               anOffsetData.BasisCore.get(),
                                               anOffsetData.Offset,
                                               theN,
                                               aResult))
      {
        if (theN > 3)
        {
          throw Standard_NotImplemented(
            "Geom2dAdaptor_CurveCore::DN: Derivative order > 3 not supported");
        }
        throw Standard_NullValue("Geom2dAdaptor_CurveCore::DN: Unable to calculate offset DN");
      }
      break;
    }

    default:
      aResult = myCurve->DN(aU, theN);
  }
  applyTransform(aResult);
  applyPostProcessor(aResult, theN);
  return aResult;
}

//==================================================================================================

double Geom2dAdaptor_CurveCore::Resolution(double theR2d) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_Line:
      return theR2d;
    case GeomAbs_Circle: {
      double aR = Handle(Geom2d_Circle)::DownCast(myCurve)->Circ2d().Radius();
      if (aR > theR2d / 2.)
        return 2 * std::asin(theR2d / (2 * aR));
      else
        return 2 * M_PI;
    }
    case GeomAbs_Ellipse: {
      return theR2d / Handle(Geom2d_Ellipse)::DownCast(myCurve)->MajorRadius();
    }
    case GeomAbs_BezierCurve: {
      double aRes;
      Handle(Geom2d_BezierCurve)::DownCast(myCurve)->Resolution(theR2d, aRes);
      return aRes;
    }
    case GeomAbs_BSplineCurve: {
      double aRes;
      Handle(Geom2d_BSplineCurve)::DownCast(myCurve)->Resolution(theR2d, aRes);
      return aRes;
    }
    default:
      return Precision::Parametric(theR2d);
  }
}

//==================================================================================================

gp_Lin2d Geom2dAdaptor_CurveCore::Line() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Line,
                                 "Geom2dAdaptor_CurveCore::Line() - curve is not a Line");
  gp_Lin2d aResult = Handle(Geom2d_Line)::DownCast(myCurve)->Lin2d();
  if (myTrsf.has_value())
  {
    aResult.Transform(*myTrsf);
  }
  return aResult;
}

//==================================================================================================

gp_Circ2d Geom2dAdaptor_CurveCore::Circle() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Circle,
                                 "Geom2dAdaptor_CurveCore::Circle() - curve is not a Circle");
  gp_Circ2d aResult = Handle(Geom2d_Circle)::DownCast(myCurve)->Circ2d();
  if (myTrsf.has_value())
  {
    aResult.Transform(*myTrsf);
  }
  return aResult;
}

//==================================================================================================

gp_Elips2d Geom2dAdaptor_CurveCore::Ellipse() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Ellipse,
                                 "Geom2dAdaptor_CurveCore::Ellipse() - curve is not an Ellipse");
  gp_Elips2d aResult = Handle(Geom2d_Ellipse)::DownCast(myCurve)->Elips2d();
  if (myTrsf.has_value())
  {
    aResult.Transform(*myTrsf);
  }
  return aResult;
}

//==================================================================================================

gp_Hypr2d Geom2dAdaptor_CurveCore::Hyperbola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Hyperbola,
                                 "Geom2dAdaptor_CurveCore::Hyperbola() - curve is not a Hyperbola");
  gp_Hypr2d aResult = Handle(Geom2d_Hyperbola)::DownCast(myCurve)->Hypr2d();
  if (myTrsf.has_value())
  {
    aResult.Transform(*myTrsf);
  }
  return aResult;
}

//==================================================================================================

gp_Parab2d Geom2dAdaptor_CurveCore::Parabola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Parabola,
                                 "Geom2dAdaptor_CurveCore::Parabola() - curve is not a Parabola");
  gp_Parab2d aResult = Handle(Geom2d_Parabola)::DownCast(myCurve)->Parab2d();
  if (myTrsf.has_value())
  {
    aResult.Transform(*myTrsf);
  }
  return aResult;
}

//==================================================================================================

int Geom2dAdaptor_CurveCore::Degree() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom2d_BezierCurve)::DownCast(myCurve)->Degree();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return std::get<BSplineData>(myEvalData).Curve->Degree();
  else
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::Degree");
}

//==================================================================================================

bool Geom2dAdaptor_CurveCore::IsRational() const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BSplineCurve:
      return std::get<BSplineData>(myEvalData).Curve->IsRational();
    case GeomAbs_BezierCurve:
      return Handle(Geom2d_BezierCurve)::DownCast(myCurve)->IsRational();
    default:
      return false;
  }
}

//==================================================================================================

int Geom2dAdaptor_CurveCore::NbPoles() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom2d_BezierCurve)::DownCast(myCurve)->NbPoles();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return std::get<BSplineData>(myEvalData).Curve->NbPoles();
  else
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::NbPoles");
}

//==================================================================================================

int Geom2dAdaptor_CurveCore::NbKnots() const
{
  if (myTypeCurve != GeomAbs_BSplineCurve)
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::NbKnots");
  return std::get<BSplineData>(myEvalData).Curve->NbKnots();
}

//==================================================================================================

Handle(Geom2d_BezierCurve) Geom2dAdaptor_CurveCore::Bezier() const
{
  return Handle(Geom2d_BezierCurve)::DownCast(myCurve);
}

//==================================================================================================

Handle(Geom2d_BSplineCurve) Geom2dAdaptor_CurveCore::BSpline() const
{
  if (const auto* aBSplineData = std::get_if<BSplineData>(&myEvalData))
  {
    return aBSplineData->Curve;
  }
  return Handle(Geom2d_BSplineCurve)();
}

//==================================================================================================

Handle(Geom2d_OffsetCurve) Geom2dAdaptor_CurveCore::OffsetCurve() const
{
  if (myTypeCurve != GeomAbs_OffsetCurve)
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::OffsetCurve");
  return Handle(Geom2d_OffsetCurve)::DownCast(myCurve);
}

//==================================================================================================

bool Geom2dAdaptor_CurveCore::IsClosed() const
{
  if (!Precision::IsPositiveInfinite(myLast) && !Precision::IsNegativeInfinite(myFirst))
  {
    gp_Pnt2d aPd = Value(myFirst);
    gp_Pnt2d aPf = Value(myLast);
    return (aPd.Distance(aPf) <= Precision::Confusion());
  }
  return false;
}

//==================================================================================================

bool Geom2dAdaptor_CurveCore::IsPeriodic() const
{
  return myCurve.IsNull() ? false : myCurve->IsPeriodic();
}

//==================================================================================================

double Geom2dAdaptor_CurveCore::Period() const
{
  if (myCurve.IsNull() || !myCurve->IsPeriodic())
  {
    throw Standard_NoSuchObject("Geom2dAdaptor_CurveCore::Period - curve is not periodic");
  }
  return myCurve->LastParameter() - myCurve->FirstParameter();
}
