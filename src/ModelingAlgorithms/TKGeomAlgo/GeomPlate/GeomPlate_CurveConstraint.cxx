// Created on: 1997-05-05
// Created by: Joelle CHAUVET
// Copyright (c) 1997-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

// Modified:	Mon Nov  3 10:24:07 1997
//		ne traite que les GeomAdaptor_Surface;
//              plus de reference a BRepAdaptor

#include <GeomPlate_CurveConstraint.hxx>

#include <Approx_Curve2d.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <Law_Function.hxx>
#include <ProjLib_ProjectedCurve.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomPlate_CurveConstraint, Standard_Transient)

//---------------------------------------------------------
//         Constructeur vide
//---------------------------------------------------------
GeomPlate_CurveConstraint ::GeomPlate_CurveConstraint()
    : myNbPoints(0),
      myOrder(0),
      myTang(0),
      myConstG0(Standard_False),
      myConstG1(Standard_False),
      myConstG2(Standard_False),
      myLProp(2, 1.e-4),
      myTolDist(0.0),
      myTolAng(0.0),
      myTolCurv(0.0),
      myTolU(0.0),
      myTolV(0.0)
{
}

//==================================================================================================

GeomPlate_CurveConstraint::GeomPlate_CurveConstraint(GeomAdaptor_Curve&&    theBoundary,
                                                     const Standard_Integer theOrder,
                                                     const Standard_Integer theNPt,
                                                     const Standard_Real    theTolDist,
                                                     const Standard_Real    theTolAng,
                                                     const Standard_Real    theTolCurv)
    : myCurve(std::make_unique<GeomAdaptor_Curve>(std::move(theBoundary))),
      myNbPoints(theNPt),
      myOrder(theOrder),
      myTang(0),
      myConstG0(Standard_True),
      myConstG1(Standard_True),
      myConstG2(Standard_True),
      myLProp(2, theTolDist),
      myTolDist(theTolDist),
      myTolAng(theTolAng),
      myTolCurv(theTolCurv),
      myTolU(0.0),
      myTolV(0.0)
{
  if ((theOrder < -1) || (theOrder > 2))
  {
    throw Standard_Failure("GeomPlate : The continuity is not G0 G1 or G2");
  }

  // If the curve has a curve-on-surface modifier, set up LProp with the surface
  if (myCurve->HasCurveOnSurface())
  {
    const GeomAdaptor_Surface& aSurfAdaptor = myCurve->GetSurface();
    Handle(Geom_Surface)       aSurf        = aSurfAdaptor.Surface();
    if (!aSurf.IsNull())
    {
      myLProp.SetSurface(aSurf);
    }
  }
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::FirstParameter() const
{
  if (!myHCurve2d.IsNull())
  {
    return myHCurve2d->FirstParameter();
  }
  if (myCurve != nullptr)
  {
    return myCurve->FirstParameter();
  }
  return 0.0;
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::LastParameter() const
{
  if (!myHCurve2d.IsNull())
  {
    return myHCurve2d->LastParameter();
  }
  if (myCurve != nullptr)
  {
    return myCurve->LastParameter();
  }
  return 0.0;
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::Length() const
{
  if (myCurve == nullptr)
  {
    return 0.0;
  }
  GCPnts_AbscissaPoint AP;
  return AP.Length(*myCurve);
}

//==================================================================================================

void GeomPlate_CurveConstraint::D0(const Standard_Real U, gp_Pnt& P) const
{
  if (myCurve == nullptr)
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::D0 - no curve loaded");
  }

  if (HasCurveOnSurface())
  {
    gp_Pnt2d P2d = GetPCurve().Value(U);
    GetSurface().D0(P2d.X(), P2d.Y(), P);
  }
  else
  {
    myCurve->D0(U, P);
  }
}

//==================================================================================================

void GeomPlate_CurveConstraint::D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::D1 - Curve must be on a Surface");
  }

  gp_Pnt2d P2d = GetPCurve().Value(U);
  GetSurface().D1(P2d.X(), P2d.Y(), P, V1, V2);
}

//==================================================================================================

void GeomPlate_CurveConstraint::D2(const Standard_Real U,
                                   gp_Pnt&             P,
                                   gp_Vec&             V1,
                                   gp_Vec&             V2,
                                   gp_Vec&             V3,
                                   gp_Vec&             V4,
                                   gp_Vec&             V5) const
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::D2 - Curve must be on a Surface");
  }

  gp_Pnt2d P2d = GetPCurve().Value(U);
  GetSurface().D2(P2d.X(), P2d.Y(), P, V1, V2, V3, V4, V5);
}

//==================================================================================================

void GeomPlate_CurveConstraint::SetG0Criterion(const Handle(Law_Function)& G0Crit)
{
  myG0Crit  = G0Crit;
  myConstG0 = Standard_False;
}

//==================================================================================================

void GeomPlate_CurveConstraint::SetG1Criterion(const Handle(Law_Function)& G1Crit)
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::SetG1Criterion - Curve must be on a Surface");
  }
  myG1Crit  = G1Crit;
  myConstG1 = Standard_False;
}

//==================================================================================================

void GeomPlate_CurveConstraint::SetG2Criterion(const Handle(Law_Function)& G2Crit)
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::SetG2Criterion - Curve must be on a Surface");
  }
  myG2Crit  = G2Crit;
  myConstG2 = Standard_False;
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::G0Criterion(const Standard_Real U) const
{
  if (myConstG0)
  {
    return myTolDist;
  }
  return myG0Crit->Value(U);
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::G1Criterion(const Standard_Real U) const
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::G1Criterion - Curve must be on a Surface");
  }
  if (myConstG1)
  {
    return myTolAng;
  }
  return myG1Crit->Value(U);
}

//==================================================================================================

Standard_Real GeomPlate_CurveConstraint::G2Criterion(const Standard_Real U) const
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::G2Criterion - Curve must be on a Surface");
  }
  if (myConstG2)
  {
    return myTolCurv;
  }
  return myG2Crit->Value(U);
}

//---------------------------------------------------------
// Fonction : Curve2dOnSurf
//---------------------------------------------------------
Handle(Geom2d_Curve) GeomPlate_CurveConstraint ::Curve2dOnSurf() const
{
  if (my2dCurve.IsNull() && !myHCurve2d.IsNull())
  {
    Handle(Geom2d_Curve) C2d;
    GeomAbs_Shape        Continuity = GeomAbs_C1;
    Standard_Integer     MaxDegree  = 10;
    Standard_Integer     MaxSeg     = 20 + myHCurve2d->NbIntervals(GeomAbs_C3);
    Approx_Curve2d       appr(myHCurve2d,
                        myHCurve2d->FirstParameter(),
                        myHCurve2d->LastParameter(),
                        myTolU,
                        myTolV,
                        Continuity,
                        MaxDegree,
                        MaxSeg);
    C2d = appr.Curve();
    return C2d;
  }
  else
    return my2dCurve;
}

//---------------------------------------------------------
// Fonction : SetCurve2dOnSurf
//---------------------------------------------------------
void GeomPlate_CurveConstraint ::SetCurve2dOnSurf(const Handle(Geom2d_Curve)& Curve)
{
  my2dCurve = Curve;
}

//---------------------------------------------------------
// Fonction : ProjectedCurve
//---------------------------------------------------------
Handle(Adaptor2d_Curve2d) GeomPlate_CurveConstraint ::ProjectedCurve() const
{
  return myHCurve2d;
}

//---------------------------------------------------------
// Fonction : SetProjectedCurve
//---------------------------------------------------------
void GeomPlate_CurveConstraint ::SetProjectedCurve(const Handle(Adaptor2d_Curve2d)& Curve,
                                                   const Standard_Real              TolU,
                                                   const Standard_Real              TolV)
{
  myHCurve2d = Curve;
  myTolU     = TolU;
  myTolV     = TolV;
}

//==================================================================================================

const GeomAdaptor_Curve& GeomPlate_CurveConstraint::Curve3d() const
{
  if (myCurve == nullptr)
  {
    throw Standard_NoSuchObject("GeomPlate_CurveConstraint::Curve3d - no curve loaded");
  }
  return *myCurve;
}

//==================================================================================================

Standard_Integer GeomPlate_CurveConstraint::NbPoints() const
{
  return myNbPoints;
}

//==================================================================================================

Standard_Integer GeomPlate_CurveConstraint::Order() const
{
  return myOrder;
}

//==================================================================================================

void GeomPlate_CurveConstraint::SetNbPoints(const Standard_Integer NewNb)
{
  myNbPoints = NewNb;
}

//==================================================================================================

void GeomPlate_CurveConstraint::SetOrder(const Standard_Integer Order)
{
  myOrder = Order;
}

//==================================================================================================

GeomLProp_SLProps& GeomPlate_CurveConstraint::LPropSurf(const Standard_Real U)
{
  if (!HasCurveOnSurface())
  {
    throw Standard_Failure("GeomPlate_CurveConstraint::LPropSurf - Curve must be on a Surface");
  }
  gp_Pnt2d P2d = GetPCurve().Value(U);
  myLProp.SetParameters(P2d.X(), P2d.Y());
  return myLProp;
}
