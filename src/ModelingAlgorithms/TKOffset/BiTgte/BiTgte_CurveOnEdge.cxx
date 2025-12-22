// Created on: 1997-01-10
// Created by: Bruno DUMORTIER
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

#include <BiTgte_CurveOnEdge.hxx>

#include <BRep_Tool.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <Standard_NoSuchObject.hxx>
#include <gp_Pnt.hxx>

//=================================================================================================

BiTgte_CurveOnEdge::BiTgte_CurveOnEdge()
    : myType(GeomAbs_OtherCurve)
{
}

//=================================================================================================

BiTgte_CurveOnEdge::BiTgte_CurveOnEdge(const TopoDS_Edge& theEonF, const TopoDS_Edge& theEdge)
    : myEdge(theEdge),
      myEonF(theEonF),
      myType(GeomAbs_OtherCurve)
{
  Init(theEonF, theEdge);
}

//=================================================================================================

void BiTgte_CurveOnEdge::Init(const TopoDS_Edge& EonF, const TopoDS_Edge& Edge)
{
  double f, l;

  myEdge = Edge;
  myCurv = BRep_Tool::Curve(myEdge, f, l);
  myCurv = new Geom_TrimmedCurve(myCurv, f, l);

  myEonF = EonF;
  myConF = BRep_Tool::Curve(myEonF, f, l);
  myConF = new Geom_TrimmedCurve(myConF, f, l);

  // Check if we can generate a zero-radius circle
  GeomAdaptor_Curve Curv(myCurv);
  GeomAdaptor_Curve ConF(myConF);

  myType = GeomAbs_OtherCurve;
  if (Curv.GetType() == GeomAbs_Line && ConF.GetType() == GeomAbs_Circle)
  {
    gp_Ax1 a1 = Curv.Line().Position();
    gp_Ax1 a2 = ConF.Circle().Axis();
    if (a1.IsCoaxial(a2, Precision::Angular(), Precision::Confusion()))
    {
      myType = GeomAbs_Circle;
      myCirc = gp_Circ(ConF.Circle().Position(), 0.);
    }
  }
}

//=================================================================================================

double BiTgte_CurveOnEdge::FirstParameter() const
{
  return myConF->FirstParameter();
}

//=================================================================================================

double BiTgte_CurveOnEdge::LastParameter() const
{
  return myConF->LastParameter();
}

//=================================================================================================

gp_Pnt BiTgte_CurveOnEdge::Value(double U) const
{
  GeomAPI_ProjectPointOnCurve Projector;
  gp_Pnt                      P = myConF->Value(U);
  Projector.Init(P, myCurv);
  return Projector.NearestPoint();
}

//=================================================================================================

GeomAbs_CurveType BiTgte_CurveOnEdge::GetType() const
{
  return myType;
}

//=================================================================================================

gp_Circ BiTgte_CurveOnEdge::Circle() const
{
  if (myType != GeomAbs_Circle)
  {
    throw Standard_NoSuchObject("BiTgte_CurveOnEdge::Circle");
  }

  return myCirc;
}
