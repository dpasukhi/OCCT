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

#include <GeomAdaptor_IsoCurveModifier.hxx>

#include <GeomAdaptor_Surface.hxx>
#include <Standard_NoSuchObject.hxx>

//==================================================================================================

GeomAdaptor_IsoCurveModifier::GeomAdaptor_IsoCurveModifier()
    : mySurface(nullptr),
      myIsoType(GeomAbs_NoneIso),
      myParameter(0.0),
      myFirst(0.0),
      myLast(0.0)
{
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier::GeomAdaptor_IsoCurveModifier(
  std::unique_ptr<GeomAdaptor_Surface> theSurface,
  GeomAbs_IsoType                      theIsoType,
  double                               theParam)
    : mySurface(std::move(theSurface)),
      myIsoType(GeomAbs_NoneIso),
      myParameter(0.0),
      myFirst(0.0),
      myLast(0.0)
{
  Load(theIsoType, theParam);
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier::GeomAdaptor_IsoCurveModifier(
  std::unique_ptr<GeomAdaptor_Surface> theSurface,
  GeomAbs_IsoType                      theIsoType,
  double                               theParam,
  double                               theFirst,
  double                               theLast)
    : mySurface(std::move(theSurface)),
      myIsoType(GeomAbs_NoneIso),
      myParameter(0.0),
      myFirst(0.0),
      myLast(0.0)
{
  Load(theIsoType, theParam, theFirst, theLast);
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier::GeomAdaptor_IsoCurveModifier(
  GeomAdaptor_IsoCurveModifier&& theOther) noexcept
    : mySurface(std::move(theOther.mySurface)),
      myIsoType(theOther.myIsoType),
      myParameter(theOther.myParameter),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast)
{
  theOther.myIsoType   = GeomAbs_NoneIso;
  theOther.myParameter = 0.0;
  theOther.myFirst     = 0.0;
  theOther.myLast      = 0.0;
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier& GeomAdaptor_IsoCurveModifier::operator=(
  GeomAdaptor_IsoCurveModifier&& theOther) noexcept
{
  if (this != &theOther)
  {
    mySurface   = std::move(theOther.mySurface);
    myIsoType   = theOther.myIsoType;
    myParameter = theOther.myParameter;
    myFirst     = theOther.myFirst;
    myLast      = theOther.myLast;

    theOther.myIsoType   = GeomAbs_NoneIso;
    theOther.myParameter = 0.0;
    theOther.myFirst     = 0.0;
    theOther.myLast      = 0.0;
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier GeomAdaptor_IsoCurveModifier::Copy() const
{
  GeomAdaptor_IsoCurveModifier aCopy;
  if (mySurface != nullptr)
  {
    aCopy.mySurface = std::make_unique<GeomAdaptor_Surface>(*mySurface);
  }
  aCopy.myIsoType   = myIsoType;
  aCopy.myParameter = myParameter;
  aCopy.myFirst     = myFirst;
  aCopy.myLast      = myLast;
  return aCopy;
}

//==================================================================================================

GeomAdaptor_IsoCurveModifier::~GeomAdaptor_IsoCurveModifier() = default;

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::Load(std::unique_ptr<GeomAdaptor_Surface> theSurface)
{
  mySurface   = std::move(theSurface);
  myIsoType   = GeomAbs_NoneIso;
  myParameter = 0.0;
  myFirst     = 0.0;
  myLast      = 0.0;
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::Load(GeomAbs_IsoType theIsoType, double theParam)
{
  if (mySurface == nullptr)
  {
    throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: No surface loaded");
  }

  switch (theIsoType)
  {
    case GeomAbs_IsoU:
      Load(theIsoType, theParam, mySurface->FirstVParameter(), mySurface->LastVParameter());
      break;

    case GeomAbs_IsoV:
      Load(theIsoType, theParam, mySurface->FirstUParameter(), mySurface->LastUParameter());
      break;

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso not allowed");
  }
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::Load(GeomAbs_IsoType theIsoType,
                                        double          theParam,
                                        double          theFirst,
                                        double          theLast)
{
  if (mySurface == nullptr)
  {
    throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: No surface loaded");
  }

  myIsoType   = theIsoType;
  myParameter = theParam;
  myFirst     = theFirst;
  myLast      = theLast;

  // Clamp bounds to surface parameter range
  if (myIsoType == GeomAbs_IsoU)
  {
    myFirst = std::max(myFirst, mySurface->FirstVParameter());
    myLast  = std::min(myLast, mySurface->LastVParameter());
  }
  else
  {
    myFirst = std::max(myFirst, mySurface->FirstUParameter());
    myLast  = std::min(myLast, mySurface->LastUParameter());
  }
}

//==================================================================================================

GeomAbs_CurveType GeomAdaptor_IsoCurveModifier::GetType() const
{
  if (mySurface == nullptr || myIsoType == GeomAbs_NoneIso)
  {
    return GeomAbs_OtherCurve;
  }

  switch (mySurface->GetType())
  {
    case GeomAbs_Plane:
      return GeomAbs_Line;

    case GeomAbs_Cylinder:
    case GeomAbs_Cone:
      switch (myIsoType)
      {
        case GeomAbs_IsoU:
          return GeomAbs_Line;
        case GeomAbs_IsoV:
          return GeomAbs_Circle;
        case GeomAbs_NoneIso:
          throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
      }
      break;

    case GeomAbs_Sphere:
    case GeomAbs_Torus:
      return GeomAbs_Circle;

    case GeomAbs_BezierSurface:
      return GeomAbs_BezierCurve;

    case GeomAbs_BSplineSurface:
      return GeomAbs_BSplineCurve;

    case GeomAbs_SurfaceOfRevolution:
      switch (myIsoType)
      {
        case GeomAbs_IsoU:
          return mySurface->BasisCurve()->GetType();
        case GeomAbs_IsoV:
          return GeomAbs_Circle;
        case GeomAbs_NoneIso:
          throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
      }
      break;

    case GeomAbs_SurfaceOfExtrusion:
      switch (myIsoType)
      {
        case GeomAbs_IsoU:
          return GeomAbs_Line;
        case GeomAbs_IsoV:
          return mySurface->BasisCurve()->GetType();
        case GeomAbs_NoneIso:
          throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
      }
      break;

    default:
      return GeomAbs_OtherCurve;
  }

  return GeomAbs_OtherCurve;
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::D0(double theT, gp_Pnt& theP) const
{
  switch (myIsoType)
  {
    case GeomAbs_IsoU:
      mySurface->D0(myParameter, theT, theP);
      break;

    case GeomAbs_IsoV:
      mySurface->D0(theT, myParameter, theP);
      break;

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
  }
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::D1(double theT, gp_Pnt& theP, gp_Vec& theV) const
{
  gp_Vec aDummy;
  switch (myIsoType)
  {
    case GeomAbs_IsoU:
      // For IsoU: P = S(U_fixed, V=T), derivative is dS/dV
      mySurface->D1(myParameter, theT, theP, aDummy, theV);
      break;

    case GeomAbs_IsoV:
      // For IsoV: P = S(U=T, V_fixed), derivative is dS/dU
      mySurface->D1(theT, myParameter, theP, theV, aDummy);
      break;

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
  }
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::D2(double theT, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const
{
  gp_Vec aDummy1, aDummy2, aDummy3;
  switch (myIsoType)
  {
    case GeomAbs_IsoU:
      // For IsoU: extract V derivatives (D1V, D2VV)
      // Surface D2 returns: P, D1U, D1V, D2UU, D2VV, D2UV
      mySurface->D2(myParameter, theT, theP, aDummy1, theV1, aDummy2, theV2, aDummy3);
      break;

    case GeomAbs_IsoV:
      // For IsoV: extract U derivatives (D1U, D2UU)
      mySurface->D2(theT, myParameter, theP, theV1, aDummy1, theV2, aDummy2, aDummy3);
      break;

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
  }
}

//==================================================================================================

void GeomAdaptor_IsoCurveModifier::D3(double  theT,
                                      gp_Pnt& theP,
                                      gp_Vec& theV1,
                                      gp_Vec& theV2,
                                      gp_Vec& theV3) const
{
  gp_Vec aDummy[6];
  switch (myIsoType)
  {
    case GeomAbs_IsoU:
      // For IsoU: extract V derivatives (D1V, D2VV, D3VVV)
      // Surface D3 returns: P, D1U, D1V, D2UU, D2VV, D2UV, D3UUU, D3VVV, D3UUV, D3UVV
      mySurface->D3(myParameter,
                    theT,
                    theP,
                    aDummy[0],
                    theV1,
                    aDummy[1],
                    theV2,
                    aDummy[2],
                    aDummy[3],
                    theV3,
                    aDummy[4],
                    aDummy[5]);
      break;

    case GeomAbs_IsoV:
      // For IsoV: extract U derivatives (D1U, D2UU, D3UUU)
      mySurface->D3(theT,
                    myParameter,
                    theP,
                    theV1,
                    aDummy[0],
                    theV2,
                    aDummy[1],
                    aDummy[2],
                    theV3,
                    aDummy[3],
                    aDummy[4],
                    aDummy[5]);
      break;

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
  }
}

//==================================================================================================

gp_Vec GeomAdaptor_IsoCurveModifier::DN(double theT, int theN) const
{
  switch (myIsoType)
  {
    case GeomAbs_IsoU:
      // For IsoU: N-th derivative in V direction
      return mySurface->DN(myParameter, theT, 0, theN);

    case GeomAbs_IsoV:
      // For IsoV: N-th derivative in U direction
      return mySurface->DN(theT, myParameter, theN, 0);

    case GeomAbs_NoneIso:
      throw Standard_NoSuchObject("GeomAdaptor_IsoCurveModifier: NoneIso");
  }

  return gp_Vec();
}
