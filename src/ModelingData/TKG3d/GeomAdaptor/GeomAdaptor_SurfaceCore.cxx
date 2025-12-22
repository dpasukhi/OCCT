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

#include <GeomAdaptor_SurfaceCore.hxx>

// Include complete types for unique_ptr destruction
#include <GeomAdaptor_CurveCore.hxx>

#include "../Geom/Geom_ExtrusionUtils.pxx"
#include "../Geom/Geom_OffsetSurfaceUtils.pxx"
#include "../Geom/Geom_RevolutionUtils.pxx"

#include <BSplCLib.hxx>
#include <BSplSLib_Cache.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Precision.hxx>
#include <Standard_DomainError.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_NumericError.hxx>
#include <TColStd_Array1OfInteger.hxx>

static const double PosTol = Precision::PConfusion() / 2;

namespace
{

//==================================================================================================

GeomAbs_Shape LocalContinuity(int                      theDegree,
                              int                      theNb,
                              TColStd_Array1OfReal&    theTK,
                              TColStd_Array1OfInteger& theTM,
                              double                   thePFirst,
                              double                   thePLast,
                              bool                     theIsPeriodic)
{
  Standard_DomainError_Raise_if((theTK.Length() != theNb || theTM.Length() != theNb), " ");
  int    anIndex1 = 0;
  int    anIndex2 = 0;
  double aNewFirst, aNewLast;
  BSplCLib::LocateParameter(theDegree, theTK, theTM, thePFirst, theIsPeriodic, 1, theNb, anIndex1, aNewFirst);
  BSplCLib::LocateParameter(theDegree, theTK, theTM, thePLast, theIsPeriodic, 1, theNb, anIndex2, aNewLast);
  constexpr double anEpsKnot = Precision::PConfusion();
  if (std::abs(aNewFirst - theTK(anIndex1 + 1)) < anEpsKnot)
    anIndex1++;
  if (std::abs(aNewLast - theTK(anIndex2)) < anEpsKnot)
    anIndex2--;
  if ((theIsPeriodic) && (anIndex1 == theNb))
    anIndex1 = 1;

  if (anIndex2 != anIndex1)
  {
    int aMultmax = theTM(anIndex1 + 1);
    for (int i = anIndex1 + 1; i <= anIndex2; i++)
    {
      if (theTM(i) > aMultmax)
        aMultmax = theTM(i);
    }
    aMultmax = theDegree - aMultmax;
    if (aMultmax <= 0)
      return GeomAbs_C0;
    switch (aMultmax)
    {
      case 1:
        return GeomAbs_C1;
      case 2:
        return GeomAbs_C2;
      case 3:
        return GeomAbs_C3;
    }
  }
  return GeomAbs_CN;
}

//! Offset surface D0 evaluation with retry mechanism for singular points.
//! Uses equivalent surface core for faster evaluation when available.
inline void offsetD0(const double                                theU,
                     const double                                theV,
                     const GeomAdaptor_SurfaceCore::OffsetData&  theData,
                     gp_Pnt&                                     theValue)
{
  if (theData.EquivalentCore)
  {
    theData.EquivalentCore->D0(theU, theV, theValue);
    return;
  }
  if (!Geom_OffsetSurfaceUtils::EvaluateD0(theU,
                                           theV,
                                           theData.BasisCore.get(),
                                           theData.Offset,
                                           theData.OffsetSurface.get(),
                                           theValue))
  {
    throw Standard_NumericError("GeomAdaptor_SurfaceCore: Unable to calculate offset D0");
  }
}

//! Offset surface D1 evaluation with retry mechanism for singular points.
//! Uses equivalent surface core for faster evaluation when available.
inline void offsetD1(const double                                theU,
                     const double                                theV,
                     const GeomAdaptor_SurfaceCore::OffsetData&  theData,
                     gp_Pnt&                                     theValue,
                     gp_Vec&                                     theD1U,
                     gp_Vec&                                     theD1V)
{
  if (theData.EquivalentCore)
  {
    theData.EquivalentCore->D1(theU, theV, theValue, theD1U, theD1V);
    return;
  }
  if (!Geom_OffsetSurfaceUtils::EvaluateD1(theU,
                                           theV,
                                           theData.BasisCore.get(),
                                           theData.Offset,
                                           theData.OffsetSurface.get(),
                                           theValue,
                                           theD1U,
                                           theD1V))
  {
    throw Standard_NumericError("GeomAdaptor_SurfaceCore: Unable to calculate offset D1");
  }
}

//! Offset surface D2 evaluation with retry mechanism for singular points.
//! Uses equivalent surface core for faster evaluation when available.
inline void offsetD2(const double                                theU,
                     const double                                theV,
                     const GeomAdaptor_SurfaceCore::OffsetData&  theData,
                     gp_Pnt&                                     theValue,
                     gp_Vec&                                     theD1U,
                     gp_Vec&                                     theD1V,
                     gp_Vec&                                     theD2U,
                     gp_Vec&                                     theD2V,
                     gp_Vec&                                     theD2UV)
{
  if (theData.EquivalentCore)
  {
    theData.EquivalentCore->D2(theU, theV, theValue, theD1U, theD1V, theD2U, theD2V, theD2UV);
    return;
  }
  if (!Geom_OffsetSurfaceUtils::EvaluateD2(theU,
                                           theV,
                                           theData.BasisCore.get(),
                                           theData.Offset,
                                           theData.OffsetSurface.get(),
                                           theValue,
                                           theD1U,
                                           theD1V,
                                           theD2U,
                                           theD2V,
                                           theD2UV))
  {
    throw Standard_NumericError("GeomAdaptor_SurfaceCore: Unable to calculate offset D2");
  }
}

//! Offset surface D3 evaluation with retry mechanism for singular points.
//! Uses equivalent surface core for faster evaluation when available.
inline void offsetD3(const double                                theU,
                     const double                                theV,
                     const GeomAdaptor_SurfaceCore::OffsetData&  theData,
                     gp_Pnt&                                     theValue,
                     gp_Vec&                                     theD1U,
                     gp_Vec&                                     theD1V,
                     gp_Vec&                                     theD2U,
                     gp_Vec&                                     theD2V,
                     gp_Vec&                                     theD2UV,
                     gp_Vec&                                     theD3U,
                     gp_Vec&                                     theD3V,
                     gp_Vec&                                     theD3UUV,
                     gp_Vec&                                     theD3UVV)
{
  if (theData.EquivalentCore)
  {
    theData.EquivalentCore->D3(theU,
                               theV,
                               theValue,
                               theD1U,
                               theD1V,
                               theD2U,
                               theD2V,
                               theD2UV,
                               theD3U,
                               theD3V,
                               theD3UUV,
                               theD3UVV);
    return;
  }
  if (!Geom_OffsetSurfaceUtils::EvaluateD3(theU,
                                           theV,
                                           theData.BasisCore.get(),
                                           theData.Offset,
                                           theData.OffsetSurface.get(),
                                           theValue,
                                           theD1U,
                                           theD1V,
                                           theD2U,
                                           theD2V,
                                           theD2UV,
                                           theD3U,
                                           theD3V,
                                           theD3UUV,
                                           theD3UVV))
  {
    throw Standard_NumericError("GeomAdaptor_SurfaceCore: Unable to calculate offset D3");
  }
}

//! Offset surface DN evaluation.
//! Uses equivalent surface core for faster evaluation when available.
inline gp_Vec offsetDN(const double                                theU,
                       const double                                theV,
                       const GeomAdaptor_SurfaceCore::OffsetData&  theData,
                       int                                         theNu,
                       int                                         theNv)
{
  if (theData.EquivalentCore)
  {
    return theData.EquivalentCore->DN(theU, theV, theNu, theNv);
  }
  gp_Vec aResult;
  if (!Geom_OffsetSurfaceUtils::EvaluateDN(theU,
                                           theV,
                                           theNu,
                                           theNv,
                                           theData.BasisCore.get(),
                                           theData.Offset,
                                           theData.OffsetSurface.get(),
                                           aResult))
  {
    throw Standard_NumericError("GeomAdaptor_SurfaceCore: Unable to calculate offset DN");
  }
  return aResult;
}

//! Processes the finding span for BSpline local evaluation.
//! @param[in] theSide side indicator (-1, 0, or 1)
//! @param[in] theIdeb input start span
//! @param[in] theIfin input end span
//! @param[out] theOutIdeb output start span
//! @param[out] theOutIfin output end span
//! @param[in] theFKIndx first knot index
//! @param[in] theLKIndx last knot index
inline void span(int  theSide,
                 int  theIdeb,
                 int  theIfin,
                 int& theOutIdeb,
                 int& theOutIfin,
                 int  theFKIndx,
                 int  theLKIndx)
{
  if (theIdeb != theIfin) // not a knot
  {
    if (theIdeb < theFKIndx)
    {
      theOutIdeb = theFKIndx;
      theOutIfin = theFKIndx + 1;
    }
    else if (theIfin > theLKIndx)
    {
      theOutIdeb = theLKIndx - 1;
      theOutIfin = theLKIndx;
    }
    else if (theIdeb >= (theLKIndx - 1))
    {
      theOutIdeb = theLKIndx - 1;
      theOutIfin = theLKIndx;
    }
    else if (theIfin <= theFKIndx + 1)
    {
      theOutIdeb = theFKIndx;
      theOutIfin = theFKIndx + 1;
    }
    else if (theIdeb > theIfin)
    {
      theOutIdeb = theIfin - 1;
      theOutIfin = theIfin;
    }
    else
    {
      theOutIdeb = theIdeb;
      theOutIfin = theIfin;
    }
  }
  else
  {
    if (theIdeb <= theFKIndx)
    {
      theOutIdeb = theFKIndx;
      theOutIfin = theFKIndx + 1;
    } // first knot
    else if (theIfin >= theLKIndx)
    {
      theOutIdeb = theLKIndx - 1;
      theOutIfin = theLKIndx;
    } // last knot
    else
    {
      if (theSide == -1)
      {
        theOutIdeb = theIdeb - 1;
        theOutIfin = theIfin;
      }
      else
      {
        theOutIdeb = theIdeb;
        theOutIfin = theIfin + 1;
      }
    }
  }
}

//! Locates U,V parameters for BSpline local evaluation.
//! @param[in] theBSpl BSpline surface
//! @param[in] theU U parameter
//! @param[in] theV V parameter
//! @param[out] theIOutDeb output U start span
//! @param[out] theIOutFin output U end span
//! @param[out] theIOutVDeb output V start span
//! @param[out] theIOutVFin output V end span
//! @param[in] theUSide U side indicator (-1, 0, or 1)
//! @param[in] theVSide V side indicator (-1, 0, or 1)
//! @return true if local evaluation is needed
inline bool ifUVBound(const Handle(Geom_BSplineSurface)& theBSpl,
                      double                             theU,
                      double                             theV,
                      int&                               theIOutDeb,
                      int&                               theIOutFin,
                      int&                               theIOutVDeb,
                      int&                               theIOutVFin,
                      int                                theUSide,
                      int                                theVSide)
{
  int Ideb, Ifin;
  int anUFKIndx = theBSpl->FirstUKnotIndex(), anULKIndx = theBSpl->LastUKnotIndex(),
      aVFKIndx = theBSpl->FirstVKnotIndex(), aVLKIndx = theBSpl->LastVKnotIndex();
  theBSpl->LocateU(theU, PosTol, Ideb, Ifin, false);
  bool Local = (Ideb == Ifin);
  span(theUSide, Ideb, Ifin, Ideb, Ifin, anUFKIndx, anULKIndx);
  int IVdeb, IVfin;
  theBSpl->LocateV(theV, PosTol, IVdeb, IVfin, false);
  if (IVdeb == IVfin)
    Local = true;
  span(theVSide, IVdeb, IVfin, IVdeb, IVfin, aVFKIndx, aVLKIndx);

  theIOutDeb  = Ideb;
  theIOutFin  = Ifin;
  theIOutVDeb = IVdeb;
  theIOutVFin = IVfin;

  return Local;
}

} // end of anonymous namespace

//==================================================================================================

GeomAdaptor_SurfaceCore::GeomAdaptor_SurfaceCore()
    : mySurfaceType(GeomAbs_OtherSurface),
      myUFirst(0.0),
      myULast(0.0),
      myVFirst(0.0),
      myVLast(0.0),
      myTolU(0.0),
      myTolV(0.0)
{
}

//==================================================================================================

GeomAdaptor_SurfaceCore::~GeomAdaptor_SurfaceCore() = default;

//==================================================================================================

GeomAdaptor_SurfaceCore::GeomAdaptor_SurfaceCore(const Handle(Geom_Surface)& theSurface)
    : mySurfaceType(GeomAbs_OtherSurface),
      myUFirst(0.0),
      myULast(0.0),
      myVFirst(0.0),
      myVLast(0.0),
      myTolU(0.0),
      myTolV(0.0)
{
  if (!theSurface.IsNull())
  {
    double aU1, aU2, aV1, aV2;
    theSurface->Bounds(aU1, aU2, aV1, aV2);
    load(theSurface, aU1, aU2, aV1, aV2, 0.0, 0.0);
  }
}

//==================================================================================================

GeomAdaptor_SurfaceCore::GeomAdaptor_SurfaceCore(const Handle(Geom_Surface)& theSurface,
                                                 double                      theUFirst,
                                                 double                      theULast,
                                                 double                      theVFirst,
                                                 double                      theVLast,
                                                 double                      theTolU,
                                                 double                      theTolV)
    : mySurfaceType(GeomAbs_OtherSurface),
      myUFirst(0.0),
      myULast(0.0),
      myVFirst(0.0),
      myVLast(0.0),
      myTolU(0.0),
      myTolV(0.0)
{
  load(theSurface, theUFirst, theULast, theVFirst, theVLast, theTolU, theTolV);
}

//==================================================================================================

GeomAdaptor_SurfaceCore::GeomAdaptor_SurfaceCore(const GeomAdaptor_SurfaceCore& theOther)
    : mySurface(theOther.mySurface),
      mySurfaceType(theOther.mySurfaceType),
      myUFirst(theOther.myUFirst),
      myULast(theOther.myULast),
      myVFirst(theOther.myVFirst),
      myVLast(theOther.myVLast),
      myTolU(theOther.myTolU),
      myTolV(theOther.myTolV),
      myParamModifier(theOther.myParamModifier),
      myTrsf(theOther.myTrsf),
      myPostProcessor(theOther.myPostProcessor)
{
  // Deep copy evaluation data based on variant type
  if (const auto* aBSplineData = std::get_if<BSplineData>(&theOther.myEvalData))
  {
    BSplineData aCopyData;
    aCopyData.Surface = aBSplineData->Surface;
    // Cache is not copied - will be rebuilt on demand
    myEvalData = std::move(aCopyData);
  }
  else if (std::holds_alternative<BezierData>(theOther.myEvalData))
  {
    myEvalData = BezierData{};
  }
  else if (const auto* anOffsetData = std::get_if<OffsetData>(&theOther.myEvalData))
  {
    OffsetData aCopyData;
    if (anOffsetData->BasisCore)
    {
      aCopyData.BasisCore = std::make_unique<GeomAdaptor_SurfaceCore>(*anOffsetData->BasisCore);
    }
    if (anOffsetData->EquivalentCore)
    {
      aCopyData.EquivalentCore = std::make_unique<GeomAdaptor_SurfaceCore>(*anOffsetData->EquivalentCore);
    }
    aCopyData.OffsetSurface = anOffsetData->OffsetSurface;
    aCopyData.Offset        = anOffsetData->Offset;
    myEvalData              = std::move(aCopyData);
  }
  else if (const auto* anExtData = std::get_if<ExtrusionData>(&theOther.myEvalData))
  {
    ExtrusionData aCopyData;
    if (anExtData->BasisCurve)
    {
      aCopyData.BasisCurve = std::make_unique<GeomAdaptor_CurveCore>(*anExtData->BasisCurve);
    }
    aCopyData.Direction = anExtData->Direction;
    myEvalData          = std::move(aCopyData);
  }
  else if (const auto* aRevData = std::get_if<RevolutionData>(&theOther.myEvalData))
  {
    RevolutionData aCopyData;
    if (aRevData->BasisCurve)
    {
      aCopyData.BasisCurve = std::make_unique<GeomAdaptor_CurveCore>(*aRevData->BasisCurve);
    }
    aCopyData.Axis = aRevData->Axis;
    myEvalData     = std::move(aCopyData);
  }
}

//==================================================================================================

GeomAdaptor_SurfaceCore::GeomAdaptor_SurfaceCore(GeomAdaptor_SurfaceCore&& theOther) noexcept
    : mySurface(std::move(theOther.mySurface)),
      mySurfaceType(theOther.mySurfaceType),
      myUFirst(theOther.myUFirst),
      myULast(theOther.myULast),
      myVFirst(theOther.myVFirst),
      myVLast(theOther.myVLast),
      myTolU(theOther.myTolU),
      myTolV(theOther.myTolV),
      myParamModifier(std::move(theOther.myParamModifier)),
      myEvalData(std::move(theOther.myEvalData)),
      myTrsf(std::move(theOther.myTrsf)),
      myPostProcessor(std::move(theOther.myPostProcessor))
{
  theOther.mySurfaceType = GeomAbs_OtherSurface;
  theOther.myUFirst = theOther.myULast = theOther.myVFirst = theOther.myVLast = 0.0;
  theOther.myTolU = theOther.myTolV = 0.0;
}

//==================================================================================================

GeomAdaptor_SurfaceCore& GeomAdaptor_SurfaceCore::operator=(const GeomAdaptor_SurfaceCore& theOther)
{
  if (this != &theOther)
  {
    GeomAdaptor_SurfaceCore aCopy(theOther);
    *this = std::move(aCopy);
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_SurfaceCore& GeomAdaptor_SurfaceCore::operator=(
  GeomAdaptor_SurfaceCore&& theOther) noexcept
{
  if (this != &theOther)
  {
    mySurface              = std::move(theOther.mySurface);
    mySurfaceType          = theOther.mySurfaceType;
    myUFirst               = theOther.myUFirst;
    myULast                = theOther.myULast;
    myVFirst               = theOther.myVFirst;
    myVLast                = theOther.myVLast;
    myTolU                 = theOther.myTolU;
    myTolV                 = theOther.myTolV;
    myParamModifier        = std::move(theOther.myParamModifier);
    myEvalData             = std::move(theOther.myEvalData);
    myTrsf                 = std::move(theOther.myTrsf);
    myPostProcessor        = std::move(theOther.myPostProcessor);
    theOther.mySurfaceType = GeomAbs_OtherSurface;
    theOther.myUFirst = theOther.myULast = theOther.myVFirst = theOther.myVLast = 0.0;
    theOther.myTolU = theOther.myTolV = 0.0;
  }
  return *this;
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::Reset()
{
  mySurface.Nullify();
  mySurfaceType   = GeomAbs_OtherSurface;
  myUFirst = myULast = myVFirst = myVLast = 0.0;
  myTolU = myTolV = 0.0;
  myParamModifier = std::monostate{};
  myEvalData      = std::monostate{};
  myTrsf.reset();
  myPostProcessor = std::monostate{};
}

//==================================================================================================

const gp_Trsf& GeomAdaptor_SurfaceCore::Transformation() const
{
  if (!myTrsf.has_value())
  {
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Transformation - no transformation set");
  }
  return *myTrsf;
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::load(const Handle(Geom_Surface)& theSurface,
                                   double                      theUFirst,
                                   double                      theULast,
                                   double                      theVFirst,
                                   double                      theVLast,
                                   double                      theTolU,
                                   double                      theTolV)
{
  myTolU   = theTolU;
  myTolV   = theTolV;
  myUFirst = theUFirst;
  myULast  = theULast;
  myVFirst = theVFirst;
  myVLast  = theVLast;
  myTrsf.reset();

  if (mySurface != theSurface)
  {
    mySurface  = theSurface;
    myEvalData = std::monostate{};

    const Handle(Standard_Type)& aType = theSurface->DynamicType();
    if (aType == STANDARD_TYPE(Geom_RectangularTrimmedSurface))
    {
      load(Handle(Geom_RectangularTrimmedSurface)::DownCast(theSurface)->BasisSurface(),
           theUFirst,
           theULast,
           theVFirst,
           theVLast,
           theTolU,
           theTolV);
    }
    else if (aType == STANDARD_TYPE(Geom_Plane))
    {
      mySurfaceType = GeomAbs_Plane;
    }
    else if (aType == STANDARD_TYPE(Geom_CylindricalSurface))
    {
      mySurfaceType = GeomAbs_Cylinder;
    }
    else if (aType == STANDARD_TYPE(Geom_ConicalSurface))
    {
      mySurfaceType = GeomAbs_Cone;
    }
    else if (aType == STANDARD_TYPE(Geom_SphericalSurface))
    {
      mySurfaceType = GeomAbs_Sphere;
    }
    else if (aType == STANDARD_TYPE(Geom_ToroidalSurface))
    {
      mySurfaceType = GeomAbs_Torus;
    }
    else if (aType == STANDARD_TYPE(Geom_SurfaceOfRevolution))
    {
      mySurfaceType = GeomAbs_SurfaceOfRevolution;
      Handle(Geom_SurfaceOfRevolution) aRevSurf =
        Handle(Geom_SurfaceOfRevolution)::DownCast(mySurface);
      RevolutionData aRevData;
      aRevData.BasisCurve = std::make_unique<GeomAdaptor_CurveCore>(aRevSurf->BasisCurve());
      aRevData.Axis       = aRevSurf->Axis();
      myEvalData          = std::move(aRevData);
    }
    else if (aType == STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion))
    {
      mySurfaceType = GeomAbs_SurfaceOfExtrusion;
      Handle(Geom_SurfaceOfLinearExtrusion) anExtSurf =
        Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(mySurface);
      ExtrusionData anExtData;
      anExtData.BasisCurve = std::make_unique<GeomAdaptor_CurveCore>(anExtSurf->BasisCurve());
      anExtData.Direction  = anExtSurf->Direction().XYZ();
      myEvalData           = std::move(anExtData);
    }
    else if (aType == STANDARD_TYPE(Geom_BezierSurface))
    {
      mySurfaceType = GeomAbs_BezierSurface;
      myEvalData    = BezierData{};
    }
    else if (aType == STANDARD_TYPE(Geom_BSplineSurface))
    {
      mySurfaceType = GeomAbs_BSplineSurface;
      BSplineData aBSplineData;
      aBSplineData.Surface = Handle(Geom_BSplineSurface)::DownCast(mySurface);
      myEvalData           = std::move(aBSplineData);
    }
    else if (aType == STANDARD_TYPE(Geom_OffsetSurface))
    {
      mySurfaceType                        = GeomAbs_OffsetSurface;
      Handle(Geom_OffsetSurface) anOffSurf = Handle(Geom_OffsetSurface)::DownCast(mySurface);
      OffsetData                 anOffsetData;
      anOffsetData.BasisCore =
        std::make_unique<GeomAdaptor_SurfaceCore>(anOffSurf->BasisSurface(),
                                                  myUFirst,
                                                  myULast,
                                                  myVFirst,
                                                  myVLast,
                                                  myTolU,
                                                  myTolV);
      anOffsetData.OffsetSurface = anOffSurf;
      anOffsetData.Offset        = anOffSurf->Offset();
      // Check if equivalent canonical surface exists for faster evaluation
      Handle(Geom_Surface) anEquivSurf = anOffSurf->Surface();
      if (!anEquivSurf.IsNull())
      {
        anOffsetData.EquivalentCore =
          std::make_unique<GeomAdaptor_SurfaceCore>(anEquivSurf,
                                                    myUFirst,
                                                    myULast,
                                                    myVFirst,
                                                    myVLast,
                                                    myTolU,
                                                    myTolV);
      }
      myEvalData = std::move(anOffsetData);
    }
    else
    {
      mySurfaceType = GeomAbs_OtherSurface;
    }
  }
  else
  {
    // Same surface, but need to invalidate cache if bounds changed
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

void GeomAdaptor_SurfaceCore::rebuildCache(double theU, double theV) const
{
  if (mySurfaceType == GeomAbs_BezierSurface)
  {
    auto&                      aBezData = std::get<BezierData>(myEvalData);
    Handle(Geom_BezierSurface) aBezier  = Handle(Geom_BezierSurface)::DownCast(mySurface);
    int                        aDegU    = aBezier->UDegree();
    int                        aDegV    = aBezier->VDegree();
    TColStd_Array1OfReal       aFlatKnotsU(BSplCLib::FlatBezierKnots(aDegU), 1, 2 * (aDegU + 1));
    TColStd_Array1OfReal       aFlatKnotsV(BSplCLib::FlatBezierKnots(aDegV), 1, 2 * (aDegV + 1));
    if (aBezData.Cache.IsNull())
    {
      aBezData.Cache = new BSplSLib_Cache(aDegU,
                                          aBezier->IsUPeriodic(),
                                          aFlatKnotsU,
                                          aDegV,
                                          aBezier->IsVPeriodic(),
                                          aFlatKnotsV,
                                          aBezier->Weights());
    }
    aBezData.Cache->BuildCache(theU, theV, aFlatKnotsU, aFlatKnotsV, aBezier->Poles(), aBezier->Weights());
  }
  else if (mySurfaceType == GeomAbs_BSplineSurface)
  {
    auto&       aBSplData = std::get<BSplineData>(myEvalData);
    const auto& aBSpl     = aBSplData.Surface;
    if (aBSplData.Cache.IsNull())
    {
      aBSplData.Cache = new BSplSLib_Cache(aBSpl->UDegree(),
                                           aBSpl->IsUPeriodic(),
                                           aBSpl->UKnotSequence(),
                                           aBSpl->VDegree(),
                                           aBSpl->IsVPeriodic(),
                                           aBSpl->VKnotSequence(),
                                           aBSpl->Weights());
    }
    aBSplData.Cache->BuildCache(theU,
                                theV,
                                aBSpl->UKnotSequence(),
                                aBSpl->VKnotSequence(),
                                aBSpl->Poles(),
                                aBSpl->Weights());
  }
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceCore::UContinuity() const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto&             aBSpl = std::get<BSplineData>(myEvalData).Surface;
      const int               N     = aBSpl->NbUKnots();
      TColStd_Array1OfReal    TK(1, N);
      TColStd_Array1OfInteger TM(1, N);
      aBSpl->UKnots(TK);
      aBSpl->UMultiplicities(TM);
      return LocalContinuity(aBSpl->UDegree(), aBSpl->NbUKnots(), TK, TM, myUFirst, myULast, IsUPeriodic());
    }
    case GeomAbs_OffsetSurface: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        switch (anOffsetData.BasisCore->UContinuity())
        {
          case GeomAbs_CN:
          case GeomAbs_C3:
            return GeomAbs_CN;
          case GeomAbs_G2:
          case GeomAbs_C2:
            return GeomAbs_C1;
          case GeomAbs_G1:
          case GeomAbs_C1:
          case GeomAbs_C0:
            return GeomAbs_C0;
        }
      }
      throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::UContinuity");
    }
    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      if (anExtData.BasisCurve)
      {
        return anExtData.BasisCurve->Continuity();
      }
      break;
    }
    case GeomAbs_OtherSurface:
      throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::UContinuity");
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_SurfaceOfRevolution:
      break;
  }
  return GeomAbs_CN;
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceCore::VContinuity() const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto&             aBSpl = std::get<BSplineData>(myEvalData).Surface;
      const int               N     = aBSpl->NbVKnots();
      TColStd_Array1OfReal    TK(1, N);
      TColStd_Array1OfInteger TM(1, N);
      aBSpl->VKnots(TK);
      aBSpl->VMultiplicities(TM);
      return LocalContinuity(aBSpl->VDegree(), aBSpl->NbVKnots(), TK, TM, myVFirst, myVLast, IsVPeriodic());
    }
    case GeomAbs_OffsetSurface: {
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        switch (anOffsetData.BasisCore->VContinuity())
        {
          case GeomAbs_CN:
          case GeomAbs_C3:
            return GeomAbs_CN;
          case GeomAbs_G2:
          case GeomAbs_C2:
            return GeomAbs_C1;
          case GeomAbs_G1:
          case GeomAbs_C1:
          case GeomAbs_C0:
            return GeomAbs_C0;
        }
      }
      throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::VContinuity");
    }
    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      if (aRevData.BasisCurve)
      {
        return aRevData.BasisCurve->Continuity();
      }
      break;
    }
    case GeomAbs_OtherSurface:
      throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::VContinuity");
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_SurfaceOfExtrusion:
      break;
  }
  return GeomAbs_CN;
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbUIntervals(GeomAbs_Shape theS) const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((!aBSpl->IsUPeriodic() && theS <= UContinuity()) || theS == GeomAbs_C0)
      {
        return 1;
      }
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
          aCont = aBSpl->UDegree();
          break;
        default:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::NbUIntervals");
      }
      double anEps = std::min(UResolution(Precision::Confusion()), Precision::PConfusion());
      return BSplCLib::Intervals(aBSpl->UKnots(),
                                 aBSpl->UMultiplicities(),
                                 aBSpl->UDegree(),
                                 aBSpl->IsUPeriodic(),
                                 aCont,
                                 myUFirst,
                                 myULast,
                                 anEps,
                                 nullptr);
    }
    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      if (anExtData.BasisCurve && anExtData.BasisCurve->GetType() == GeomAbs_BSplineCurve)
      {
        return anExtData.BasisCurve->NbIntervals(theS);
      }
      break;
    }
    case GeomAbs_OffsetSurface: {
      GeomAbs_Shape aBaseS = GeomAbs_CN;
      switch (theS)
      {
        case GeomAbs_G1:
        case GeomAbs_G2:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::NbUIntervals");
        case GeomAbs_C0:
          aBaseS = GeomAbs_C1;
          break;
        case GeomAbs_C1:
          aBaseS = GeomAbs_C2;
          break;
        case GeomAbs_C2:
          aBaseS = GeomAbs_C3;
          break;
        case GeomAbs_C3:
        case GeomAbs_CN:
          break;
      }
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        return anOffsetData.BasisCore->NbUIntervals(aBaseS);
      }
      break;
    }
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_OtherSurface:
    case GeomAbs_SurfaceOfRevolution:
      break;
  }
  return 1;
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbVIntervals(GeomAbs_Shape theS) const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((!aBSpl->IsVPeriodic() && theS <= VContinuity()) || theS == GeomAbs_C0)
      {
        return 1;
      }
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
          aCont = aBSpl->VDegree();
          break;
        default:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::NbVIntervals");
      }
      double anEps = std::min(VResolution(Precision::Confusion()), Precision::PConfusion());
      return BSplCLib::Intervals(aBSpl->VKnots(),
                                 aBSpl->VMultiplicities(),
                                 aBSpl->VDegree(),
                                 aBSpl->IsVPeriodic(),
                                 aCont,
                                 myVFirst,
                                 myVLast,
                                 anEps,
                                 nullptr);
    }
    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      if (aRevData.BasisCurve && aRevData.BasisCurve->GetType() == GeomAbs_BSplineCurve)
      {
        return aRevData.BasisCurve->NbIntervals(theS);
      }
      break;
    }
    case GeomAbs_OffsetSurface: {
      GeomAbs_Shape aBaseS = GeomAbs_CN;
      switch (theS)
      {
        case GeomAbs_G1:
        case GeomAbs_G2:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::NbVIntervals");
        case GeomAbs_C0:
          aBaseS = GeomAbs_C1;
          break;
        case GeomAbs_C1:
          aBaseS = GeomAbs_C2;
          break;
        case GeomAbs_C2:
          aBaseS = GeomAbs_C3;
          break;
        case GeomAbs_C3:
        case GeomAbs_CN:
          break;
      }
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        return anOffsetData.BasisCore->NbVIntervals(aBaseS);
      }
      break;
    }
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_OtherSurface:
    case GeomAbs_SurfaceOfExtrusion:
      break;
  }
  return 1;
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::UIntervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((!aBSpl->IsUPeriodic() && theS <= UContinuity()) || theS == GeomAbs_C0)
      {
        theT(theT.Lower())     = myUFirst;
        theT(theT.Lower() + 1) = myULast;
        return;
      }
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
          aCont = aBSpl->UDegree();
          break;
        default:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::UIntervals");
      }
      double anEps = std::min(UResolution(Precision::Confusion()), Precision::PConfusion());
      BSplCLib::Intervals(aBSpl->UKnots(),
                          aBSpl->UMultiplicities(),
                          aBSpl->UDegree(),
                          aBSpl->IsUPeriodic(),
                          aCont,
                          myUFirst,
                          myULast,
                          anEps,
                          &theT);
      return;
    }
    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      if (anExtData.BasisCurve && anExtData.BasisCurve->GetType() == GeomAbs_BSplineCurve)
      {
        anExtData.BasisCurve->Intervals(theT, theS);
        return;
      }
      break;
    }
    case GeomAbs_OffsetSurface: {
      GeomAbs_Shape aBaseS = GeomAbs_CN;
      switch (theS)
      {
        case GeomAbs_G1:
        case GeomAbs_G2:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::UIntervals");
        case GeomAbs_C0:
          aBaseS = GeomAbs_C1;
          break;
        case GeomAbs_C1:
          aBaseS = GeomAbs_C2;
          break;
        case GeomAbs_C2:
          aBaseS = GeomAbs_C3;
          break;
        case GeomAbs_C3:
        case GeomAbs_CN:
          break;
      }
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        anOffsetData.BasisCore->UIntervals(theT, aBaseS);
        return;
      }
      break;
    }
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_OtherSurface:
    case GeomAbs_SurfaceOfRevolution:
      break;
  }

  theT(theT.Lower())     = myUFirst;
  theT(theT.Lower() + 1) = myULast;
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::VIntervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const
{
  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((!aBSpl->IsVPeriodic() && theS <= VContinuity()) || theS == GeomAbs_C0)
      {
        theT(theT.Lower())     = myVFirst;
        theT(theT.Lower() + 1) = myVLast;
        return;
      }
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
          aCont = aBSpl->VDegree();
          break;
        default:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::VIntervals");
      }
      double anEps = std::min(VResolution(Precision::Confusion()), Precision::PConfusion());
      BSplCLib::Intervals(aBSpl->VKnots(),
                          aBSpl->VMultiplicities(),
                          aBSpl->VDegree(),
                          aBSpl->IsVPeriodic(),
                          aCont,
                          myVFirst,
                          myVLast,
                          anEps,
                          &theT);
      return;
    }
    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      if (aRevData.BasisCurve && aRevData.BasisCurve->GetType() == GeomAbs_BSplineCurve)
      {
        aRevData.BasisCurve->Intervals(theT, theS);
        return;
      }
      break;
    }
    case GeomAbs_OffsetSurface: {
      GeomAbs_Shape aBaseS = GeomAbs_CN;
      switch (theS)
      {
        case GeomAbs_G1:
        case GeomAbs_G2:
          throw Standard_DomainError("GeomAdaptor_SurfaceCore::VIntervals");
        case GeomAbs_C0:
          aBaseS = GeomAbs_C1;
          break;
        case GeomAbs_C1:
          aBaseS = GeomAbs_C2;
          break;
        case GeomAbs_C2:
          aBaseS = GeomAbs_C3;
          break;
        case GeomAbs_C3:
        case GeomAbs_CN:
          break;
      }
      const auto& anOffsetData = std::get<OffsetData>(myEvalData);
      if (anOffsetData.BasisCore)
      {
        anOffsetData.BasisCore->VIntervals(theT, aBaseS);
        return;
      }
      break;
    }
    case GeomAbs_Plane:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Sphere:
    case GeomAbs_Torus:
    case GeomAbs_BezierSurface:
    case GeomAbs_OtherSurface:
    case GeomAbs_SurfaceOfExtrusion:
      break;
  }

  theT(theT.Lower())     = myVFirst;
  theT(theT.Lower() + 1) = myVLast;
}

//==================================================================================================

gp_Pnt GeomAdaptor_SurfaceCore::Value(double theU, double theV) const
{
  gp_Pnt aP;
  D0(theU, theV, aP);
  return aP;
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::D0(double theU, double theV, gp_Pnt& theP) const
{
  // Apply parameter modifier (pre-evaluation)
  double aU = theU, aV = theV;
  applyParamModifier(aU, aV);

  switch (mySurfaceType)
  {
    case GeomAbs_BezierSurface: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(aU, aV))
        rebuildCache(aU, aV);
      aCache->D0(aU, aV, theP);
      break;
    }

    case GeomAbs_BSplineSurface: {
      auto& aCache = std::get<BSplineData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(aU, aV))
        rebuildCache(aU, aV);
      aCache->D0(aU, aV, theP);
      break;
    }

    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      Geom_ExtrusionUtils::D0(aU, aV, *anExtData.BasisCurve, anExtData.Direction, theP);
      break;
    }

    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      Geom_RevolutionUtils::D0(aU, aV, *aRevData.BasisCurve, aRevData.Axis, theP);
      break;
    }

    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      offsetD0(aU, aV, anOffData, theP);
      break;
    }

    default:
      mySurface->D0(aU, aV, theP);
  }

  applyTransform(theP);
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::D1(double   theU,
                                 double   theV,
                                 gp_Pnt&  theP,
                                 gp_Vec&  theD1U,
                                 gp_Vec&  theD1V) const
{
  // Apply parameter modifier (pre-evaluation)
  double aU = theU, aV = theV;
  applyParamModifier(aU, aV);

  int    Ideb, Ifin, IVdeb, IVfin, USide = 0, VSide = 0;
  double u = aU, v = aV;
  if (std::abs(aU - myUFirst) <= myTolU)
  {
    USide = 1;
    u     = myUFirst;
  }
  else if (std::abs(aU - myULast) <= myTolU)
  {
    USide = -1;
    u     = myULast;
  }
  if (std::abs(aV - myVFirst) <= myTolV)
  {
    VSide = 1;
    v     = myVFirst;
  }
  else if (std::abs(aV - myVLast) <= myTolV)
  {
    VSide = -1;
    v     = myVLast;
  }

  switch (mySurfaceType)
  {
    case GeomAbs_BezierSurface: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(aU, aV))
        rebuildCache(aU, aV);
      aCache->D1(aU, aV, theP, theD1U, theD1V);
      break;
    }

    case GeomAbs_BSplineSurface: {
      auto&       aBSplData = std::get<BSplineData>(myEvalData);
      const auto& aBSpl     = aBSplData.Surface;
      if ((USide != 0 || VSide != 0) && ifUVBound(aBSpl, u, v, Ideb, Ifin, IVdeb, IVfin, USide, VSide))
        aBSpl->LocalD1(u, v, Ideb, Ifin, IVdeb, IVfin, theP, theD1U, theD1V);
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(aU, aV))
          rebuildCache(aU, aV);
        aBSplData.Cache->D1(aU, aV, theP, theD1U, theD1V);
      }
      break;
    }

    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      Geom_ExtrusionUtils::D1(u, v, *anExtData.BasisCurve, anExtData.Direction, theP, theD1U, theD1V);
      break;
    }

    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      Geom_RevolutionUtils::D1(u, v, *aRevData.BasisCurve, aRevData.Axis, theP, theD1U, theD1V);
      break;
    }

    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      offsetD1(u, v, anOffData, theP, theD1U, theD1V);
      break;
    }

    default:
      mySurface->D1(u, v, theP, theD1U, theD1V);
  }

  applyTransform(theP);
  applyTransform(theD1U);
  applyTransform(theD1V);
  applyPostProcessorU(theD1U, 1);
  applyPostProcessorV(theD1V, 1);
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::D2(double   theU,
                                 double   theV,
                                 gp_Pnt&  theP,
                                 gp_Vec&  theD1U,
                                 gp_Vec&  theD1V,
                                 gp_Vec&  theD2U,
                                 gp_Vec&  theD2V,
                                 gp_Vec&  theD2UV) const
{
  // Apply parameter modifier (pre-evaluation)
  double aU = theU, aV = theV;
  applyParamModifier(aU, aV);

  int    Ideb, Ifin, IVdeb, IVfin, USide = 0, VSide = 0;
  double u = aU, v = aV;
  if (std::abs(aU - myUFirst) <= myTolU)
  {
    USide = 1;
    u     = myUFirst;
  }
  else if (std::abs(aU - myULast) <= myTolU)
  {
    USide = -1;
    u     = myULast;
  }
  if (std::abs(aV - myVFirst) <= myTolV)
  {
    VSide = 1;
    v     = myVFirst;
  }
  else if (std::abs(aV - myVLast) <= myTolV)
  {
    VSide = -1;
    v     = myVLast;
  }

  switch (mySurfaceType)
  {
    case GeomAbs_BezierSurface: {
      auto& aCache = std::get<BezierData>(myEvalData).Cache;
      if (aCache.IsNull() || !aCache->IsCacheValid(aU, aV))
        rebuildCache(aU, aV);
      aCache->D2(aU, aV, theP, theD1U, theD1V, theD2U, theD2V, theD2UV);
      break;
    }

    case GeomAbs_BSplineSurface: {
      auto&       aBSplData = std::get<BSplineData>(myEvalData);
      const auto& aBSpl     = aBSplData.Surface;
      if ((USide != 0 || VSide != 0) && ifUVBound(aBSpl, u, v, Ideb, Ifin, IVdeb, IVfin, USide, VSide))
        aBSpl->LocalD2(u, v, Ideb, Ifin, IVdeb, IVfin, theP, theD1U, theD1V, theD2U, theD2V, theD2UV);
      else
      {
        if (aBSplData.Cache.IsNull() || !aBSplData.Cache->IsCacheValid(aU, aV))
          rebuildCache(aU, aV);
        aBSplData.Cache->D2(aU, aV, theP, theD1U, theD1V, theD2U, theD2V, theD2UV);
      }
      break;
    }

    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      Geom_ExtrusionUtils::D2(u,
                              v,
                              *anExtData.BasisCurve,
                              anExtData.Direction,
                              theP,
                              theD1U,
                              theD1V,
                              theD2U,
                              theD2V,
                              theD2UV);
      break;
    }

    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      Geom_RevolutionUtils::D2(u,
                               v,
                               *aRevData.BasisCurve,
                               aRevData.Axis,
                               theP,
                               theD1U,
                               theD1V,
                               theD2U,
                               theD2V,
                               theD2UV);
      break;
    }

    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      offsetD2(u, v, anOffData, theP, theD1U, theD1V, theD2U, theD2V, theD2UV);
      break;
    }

    default:
      mySurface->D2(u, v, theP, theD1U, theD1V, theD2U, theD2V, theD2UV);
  }

  applyTransform(theP);
  applyTransform(theD1U);
  applyTransform(theD1V);
  applyTransform(theD2U);
  applyTransform(theD2V);
  applyTransform(theD2UV);
  applyPostProcessorU(theD1U, 1);
  applyPostProcessorV(theD1V, 1);
  applyPostProcessorU(theD2U, 2);
  applyPostProcessorV(theD2V, 2);
  applyPostProcessorUV(theD2UV, 1, 1);
}

//==================================================================================================

void GeomAdaptor_SurfaceCore::D3(double   theU,
                                 double   theV,
                                 gp_Pnt&  theP,
                                 gp_Vec&  theD1U,
                                 gp_Vec&  theD1V,
                                 gp_Vec&  theD2U,
                                 gp_Vec&  theD2V,
                                 gp_Vec&  theD2UV,
                                 gp_Vec&  theD3U,
                                 gp_Vec&  theD3V,
                                 gp_Vec&  theD3UUV,
                                 gp_Vec&  theD3UVV) const
{
  // Apply parameter modifier (pre-evaluation)
  double aU = theU, aV = theV;
  applyParamModifier(aU, aV);

  int    Ideb, Ifin, IVdeb, IVfin, USide = 0, VSide = 0;
  double u = aU, v = aV;
  if (std::abs(aU - myUFirst) <= myTolU)
  {
    USide = 1;
    u     = myUFirst;
  }
  else if (std::abs(aU - myULast) <= myTolU)
  {
    USide = -1;
    u     = myULast;
  }
  if (std::abs(aV - myVFirst) <= myTolV)
  {
    VSide = 1;
    v     = myVFirst;
  }
  else if (std::abs(aV - myVLast) <= myTolV)
  {
    VSide = -1;
    v     = myVLast;
  }

  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((USide == 0) && (VSide == 0))
        aBSpl->D3(u, v, theP, theD1U, theD1V, theD2U, theD2V, theD2UV, theD3U, theD3V, theD3UUV, theD3UVV);
      else
      {
        if (ifUVBound(aBSpl, u, v, Ideb, Ifin, IVdeb, IVfin, USide, VSide))
          aBSpl->LocalD3(u,
                         v,
                         Ideb,
                         Ifin,
                         IVdeb,
                         IVfin,
                         theP,
                         theD1U,
                         theD1V,
                         theD2U,
                         theD2V,
                         theD2UV,
                         theD3U,
                         theD3V,
                         theD3UUV,
                         theD3UVV);
        else
          aBSpl->D3(u, v, theP, theD1U, theD1V, theD2U, theD2V, theD2UV, theD3U, theD3V, theD3UUV, theD3UVV);
      }
      break;
    }

    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      Geom_ExtrusionUtils::D3(u,
                              v,
                              *anExtData.BasisCurve,
                              anExtData.Direction,
                              theP,
                              theD1U,
                              theD1V,
                              theD2U,
                              theD2V,
                              theD2UV,
                              theD3U,
                              theD3V,
                              theD3UUV,
                              theD3UVV);
      break;
    }

    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      Geom_RevolutionUtils::D3(u,
                               v,
                               *aRevData.BasisCurve,
                               aRevData.Axis,
                               theP,
                               theD1U,
                               theD1V,
                               theD2U,
                               theD2V,
                               theD2UV,
                               theD3U,
                               theD3V,
                               theD3UUV,
                               theD3UVV);
      break;
    }

    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      offsetD3(u, v, anOffData, theP, theD1U, theD1V, theD2U, theD2V, theD2UV, theD3U, theD3V, theD3UUV, theD3UVV);
      break;
    }

    default:
      mySurface->D3(u, v, theP, theD1U, theD1V, theD2U, theD2V, theD2UV, theD3U, theD3V, theD3UUV, theD3UVV);
  }

  applyTransform(theP);
  applyTransform(theD1U);
  applyTransform(theD1V);
  applyTransform(theD2U);
  applyTransform(theD2V);
  applyTransform(theD2UV);
  applyTransform(theD3U);
  applyTransform(theD3V);
  applyTransform(theD3UUV);
  applyTransform(theD3UVV);
  applyPostProcessorU(theD1U, 1);
  applyPostProcessorV(theD1V, 1);
  applyPostProcessorU(theD2U, 2);
  applyPostProcessorV(theD2V, 2);
  applyPostProcessorUV(theD2UV, 1, 1);
  applyPostProcessorU(theD3U, 3);
  applyPostProcessorV(theD3V, 3);
  applyPostProcessorUV(theD3UUV, 2, 1);
  applyPostProcessorUV(theD3UVV, 1, 2);
}

//==================================================================================================

gp_Vec GeomAdaptor_SurfaceCore::DN(double theU, double theV, int theNu, int theNv) const
{
  // Apply parameter modifier (pre-evaluation)
  double aU = theU, aV = theV;
  applyParamModifier(aU, aV);

  int    Ideb, Ifin, IVdeb, IVfin, USide = 0, VSide = 0;
  double u = aU, v = aV;
  if (std::abs(aU - myUFirst) <= myTolU)
  {
    USide = 1;
    u     = myUFirst;
  }
  else if (std::abs(aU - myULast) <= myTolU)
  {
    USide = -1;
    u     = myULast;
  }
  if (std::abs(aV - myVFirst) <= myTolV)
  {
    VSide = 1;
    v     = myVFirst;
  }
  else if (std::abs(aV - myVLast) <= myTolV)
  {
    VSide = -1;
    v     = myVLast;
  }

  gp_Vec aResult;

  switch (mySurfaceType)
  {
    case GeomAbs_BSplineSurface: {
      const auto& aBSpl = std::get<BSplineData>(myEvalData).Surface;
      if ((USide == 0) && (VSide == 0))
        aResult = aBSpl->DN(u, v, theNu, theNv);
      else
      {
        if (ifUVBound(aBSpl, u, v, Ideb, Ifin, IVdeb, IVfin, USide, VSide))
          aResult = aBSpl->LocalDN(u, v, Ideb, Ifin, IVdeb, IVfin, theNu, theNv);
        else
          aResult = aBSpl->DN(u, v, theNu, theNv);
      }
      break;
    }

    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      aResult = Geom_ExtrusionUtils::DN(u, *anExtData.BasisCurve, anExtData.Direction, theNu, theNv);
      break;
    }

    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      aResult = Geom_RevolutionUtils::DN(u, v, *aRevData.BasisCurve, aRevData.Axis, theNu, theNv);
      break;
    }

    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      aResult = offsetDN(u, v, anOffData, theNu, theNv);
      break;
    }

    default:
      aResult = mySurface->DN(u, v, theNu, theNv);
  }

  applyTransform(aResult);
  applyPostProcessorUV(aResult, theNu, theNv);
  return aResult;
}

//==================================================================================================

double GeomAdaptor_SurfaceCore::UResolution(double theR3d) const
{
  double aRes = 0.0;

  switch (mySurfaceType)
  {
    case GeomAbs_SurfaceOfExtrusion: {
      const auto& anExtData = std::get<ExtrusionData>(myEvalData);
      if (anExtData.BasisCurve)
      {
        return anExtData.BasisCurve->Resolution(theR3d);
      }
      return Precision::Parametric(theR3d);
    }
    case GeomAbs_Torus: {
      Handle(Geom_ToroidalSurface) S(Handle(Geom_ToroidalSurface)::DownCast(mySurface));
      const double                 R = S->MajorRadius() + S->MinorRadius();
      if (R > Precision::Confusion())
        aRes = theR3d / (2.0 * R);
      break;
    }
    case GeomAbs_Sphere: {
      Handle(Geom_SphericalSurface) S(Handle(Geom_SphericalSurface)::DownCast(mySurface));
      const double                  R = S->Radius();
      if (R > Precision::Confusion())
        aRes = theR3d / (2.0 * R);
      break;
    }
    case GeomAbs_Cylinder: {
      Handle(Geom_CylindricalSurface) S(Handle(Geom_CylindricalSurface)::DownCast(mySurface));
      const double                    R = S->Radius();
      if (R > Precision::Confusion())
        aRes = theR3d / (2.0 * R);
      break;
    }
    case GeomAbs_Cone: {
      if (myVLast - myVFirst > 1.e10)
      {
        return Precision::Parametric(theR3d);
      }
      Handle(Geom_ConicalSurface) S(Handle(Geom_ConicalSurface)::DownCast(mySurface));
      Handle(Geom_Curve)          C      = S->VIso(myVLast);
      const double                Rayon1 = Handle(Geom_Circle)::DownCast(C)->Radius();
      C                                  = S->VIso(myVFirst);
      const double Rayon2                = Handle(Geom_Circle)::DownCast(C)->Radius();
      const double R                     = (Rayon1 > Rayon2) ? Rayon1 : Rayon2;
      return (R > Precision::Confusion() ? (theR3d / R) : 0.0);
    }
    case GeomAbs_Plane: {
      return theR3d;
    }
    case GeomAbs_BezierSurface: {
      double Ures, Vres;
      Handle(Geom_BezierSurface)::DownCast(mySurface)->Resolution(theR3d, Ures, Vres);
      return Ures;
    }
    case GeomAbs_BSplineSurface: {
      double Ures, Vres;
      std::get<BSplineData>(myEvalData).Surface->Resolution(theR3d, Ures, Vres);
      return Ures;
    }
    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      if (anOffData.BasisCore)
      {
        return anOffData.BasisCore->UResolution(theR3d);
      }
      return Precision::Parametric(theR3d);
    }
    default:
      return Precision::Parametric(theR3d);
  }

  if (aRes <= 1.0)
    return 2.0 * std::asin(aRes);

  return 2.0 * M_PI;
}

//==================================================================================================

double GeomAdaptor_SurfaceCore::VResolution(double theR3d) const
{
  double aRes = 0.0;

  switch (mySurfaceType)
  {
    case GeomAbs_SurfaceOfRevolution: {
      const auto& aRevData = std::get<RevolutionData>(myEvalData);
      if (aRevData.BasisCurve)
      {
        return aRevData.BasisCurve->Resolution(theR3d);
      }
      return Precision::Parametric(theR3d);
    }
    case GeomAbs_Torus: {
      Handle(Geom_ToroidalSurface) S(Handle(Geom_ToroidalSurface)::DownCast(mySurface));
      const double                 R = S->MinorRadius();
      if (R > Precision::Confusion())
        aRes = theR3d / (2.0 * R);
      break;
    }
    case GeomAbs_Sphere: {
      Handle(Geom_SphericalSurface) S(Handle(Geom_SphericalSurface)::DownCast(mySurface));
      const double                  R = S->Radius();
      if (R > Precision::Confusion())
        aRes = theR3d / (2.0 * R);
      break;
    }
    case GeomAbs_SurfaceOfExtrusion:
    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
    case GeomAbs_Plane: {
      return theR3d;
    }
    case GeomAbs_BezierSurface: {
      double Ures, Vres;
      Handle(Geom_BezierSurface)::DownCast(mySurface)->Resolution(theR3d, Ures, Vres);
      return Vres;
    }
    case GeomAbs_BSplineSurface: {
      double Ures, Vres;
      std::get<BSplineData>(myEvalData).Surface->Resolution(theR3d, Ures, Vres);
      return Vres;
    }
    case GeomAbs_OffsetSurface: {
      const auto& anOffData = std::get<OffsetData>(myEvalData);
      if (anOffData.BasisCore)
      {
        return anOffData.BasisCore->VResolution(theR3d);
      }
      return Precision::Parametric(theR3d);
    }
    default:
      return Precision::Parametric(theR3d);
  }

  if (aRes <= 1.0)
    return 2.0 * std::asin(aRes);

  return 2.0 * M_PI;
}

//==================================================================================================

gp_Pln GeomAdaptor_SurfaceCore::Plane() const
{
  if (mySurfaceType != GeomAbs_Plane)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Plane");
  gp_Pln aPlane = Handle(Geom_Plane)::DownCast(mySurface)->Pln();
  if (myTrsf.has_value())
  {
    aPlane.Transform(*myTrsf);
  }
  return aPlane;
}

//==================================================================================================

gp_Cylinder GeomAdaptor_SurfaceCore::Cylinder() const
{
  if (mySurfaceType != GeomAbs_Cylinder)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Cylinder");
  gp_Cylinder aCyl = Handle(Geom_CylindricalSurface)::DownCast(mySurface)->Cylinder();
  if (myTrsf.has_value())
  {
    aCyl.Transform(*myTrsf);
  }
  return aCyl;
}

//==================================================================================================

gp_Cone GeomAdaptor_SurfaceCore::Cone() const
{
  if (mySurfaceType != GeomAbs_Cone)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Cone");
  gp_Cone aCone = Handle(Geom_ConicalSurface)::DownCast(mySurface)->Cone();
  if (myTrsf.has_value())
  {
    aCone.Transform(*myTrsf);
  }
  return aCone;
}

//==================================================================================================

gp_Sphere GeomAdaptor_SurfaceCore::Sphere() const
{
  if (mySurfaceType != GeomAbs_Sphere)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Sphere");
  gp_Sphere aSphere = Handle(Geom_SphericalSurface)::DownCast(mySurface)->Sphere();
  if (myTrsf.has_value())
  {
    aSphere.Transform(*myTrsf);
  }
  return aSphere;
}

//==================================================================================================

gp_Torus GeomAdaptor_SurfaceCore::Torus() const
{
  if (mySurfaceType != GeomAbs_Torus)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Torus");
  gp_Torus aTorus = Handle(Geom_ToroidalSurface)::DownCast(mySurface)->Torus();
  if (myTrsf.has_value())
  {
    aTorus.Transform(*myTrsf);
  }
  return aTorus;
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::UDegree() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->UDegree();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->UDegree();
  if (mySurfaceType == GeomAbs_SurfaceOfExtrusion)
  {
    const auto& anExtData = std::get<ExtrusionData>(myEvalData);
    if (anExtData.BasisCurve)
    {
      return anExtData.BasisCurve->Degree();
    }
  }
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::UDegree");
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::VDegree() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->VDegree();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->VDegree();
  if (mySurfaceType == GeomAbs_SurfaceOfRevolution)
  {
    const auto& aRevData = std::get<RevolutionData>(myEvalData);
    if (aRevData.BasisCurve)
    {
      return aRevData.BasisCurve->Degree();
    }
  }
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::VDegree");
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbUPoles() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->NbUPoles();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->NbUPoles();
  if (mySurfaceType == GeomAbs_SurfaceOfExtrusion)
  {
    const auto& anExtData = std::get<ExtrusionData>(myEvalData);
    if (anExtData.BasisCurve)
    {
      return anExtData.BasisCurve->NbPoles();
    }
  }
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::NbUPoles");
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbVPoles() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->NbVPoles();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->NbVPoles();
  if (mySurfaceType == GeomAbs_SurfaceOfRevolution)
  {
    const auto& aRevData = std::get<RevolutionData>(myEvalData);
    if (aRevData.BasisCurve)
    {
      return aRevData.BasisCurve->NbPoles();
    }
  }
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::NbVPoles");
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbUKnots() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->NbUKnots();
  if (mySurfaceType == GeomAbs_SurfaceOfExtrusion)
  {
    const auto& anExtData = std::get<ExtrusionData>(myEvalData);
    if (anExtData.BasisCurve)
    {
      return anExtData.BasisCurve->NbKnots();
    }
  }
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::NbUKnots");
}

//==================================================================================================

int GeomAdaptor_SurfaceCore::NbVKnots() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->NbVKnots();
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::NbVKnots");
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsURational() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->IsURational();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->IsURational();
  return false;
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsVRational() const
{
  if (mySurfaceType == GeomAbs_BSplineSurface)
    return std::get<BSplineData>(myEvalData).Surface->IsVRational();
  if (mySurfaceType == GeomAbs_BezierSurface)
    return Handle(Geom_BezierSurface)::DownCast(mySurface)->IsVRational();
  return false;
}

//==================================================================================================

Handle(Geom_BezierSurface) GeomAdaptor_SurfaceCore::Bezier() const
{
  if (mySurfaceType != GeomAbs_BezierSurface)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Bezier");
  return Handle(Geom_BezierSurface)::DownCast(mySurface);
}

//==================================================================================================

Handle(Geom_BSplineSurface) GeomAdaptor_SurfaceCore::BSpline() const
{
  if (mySurfaceType != GeomAbs_BSplineSurface)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::BSpline");
  return std::get<BSplineData>(myEvalData).Surface;
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsUClosed() const
{
  if (mySurface.IsNull())
    return false;
  if (!mySurface->IsUClosed())
    return false;

  double U1, U2, V1, V2;
  mySurface->Bounds(U1, U2, V1, V2);
  if (mySurface->IsUPeriodic())
    return (std::abs(std::abs(U1 - U2) - std::abs(myUFirst - myULast)) < Precision::PConfusion());

  return (std::abs(U1 - myUFirst) < Precision::PConfusion()
          && std::abs(U2 - myULast) < Precision::PConfusion());
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsVClosed() const
{
  if (mySurface.IsNull())
    return false;
  if (!mySurface->IsVClosed())
    return false;

  double U1, U2, V1, V2;
  mySurface->Bounds(U1, U2, V1, V2);
  if (mySurface->IsVPeriodic())
    return (std::abs(std::abs(V1 - V2) - std::abs(myVFirst - myVLast)) < Precision::PConfusion());

  return (std::abs(V1 - myVFirst) < Precision::PConfusion()
          && std::abs(V2 - myVLast) < Precision::PConfusion());
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsUPeriodic() const
{
  return mySurface.IsNull() ? false : mySurface->IsUPeriodic();
}

//==================================================================================================

bool GeomAdaptor_SurfaceCore::IsVPeriodic() const
{
  return mySurface.IsNull() ? false : mySurface->IsVPeriodic();
}

//==================================================================================================

double GeomAdaptor_SurfaceCore::UPeriod() const
{
  if (mySurface.IsNull() || !mySurface->IsUPeriodic())
  {
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::UPeriod - surface is not U-periodic");
  }
  return mySurface->UPeriod();
}

//==================================================================================================

double GeomAdaptor_SurfaceCore::VPeriod() const
{
  if (mySurface.IsNull() || !mySurface->IsVPeriodic())
  {
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::VPeriod - surface is not V-periodic");
  }
  return mySurface->VPeriod();
}

//==================================================================================================

gp_Ax1 GeomAdaptor_SurfaceCore::AxeOfRevolution() const
{
  if (mySurfaceType != GeomAbs_SurfaceOfRevolution)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::AxeOfRevolution");
  gp_Ax1 anAxis = Handle(Geom_SurfaceOfRevolution)::DownCast(mySurface)->Axis();
  if (myTrsf.has_value())
  {
    anAxis.Transform(*myTrsf);
  }
  return anAxis;
}

//==================================================================================================

gp_Dir GeomAdaptor_SurfaceCore::Direction() const
{
  if (mySurfaceType != GeomAbs_SurfaceOfExtrusion)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::Direction");
  gp_Dir aDir = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(mySurface)->Direction();
  if (myTrsf.has_value())
  {
    aDir.Transform(*myTrsf);
  }
  return aDir;
}

//==================================================================================================

double GeomAdaptor_SurfaceCore::OffsetValue() const
{
  if (mySurfaceType != GeomAbs_OffsetSurface)
    throw Standard_NoSuchObject("GeomAdaptor_SurfaceCore::OffsetValue");
  return Handle(Geom_OffsetSurface)::DownCast(mySurface)->Offset();
}
