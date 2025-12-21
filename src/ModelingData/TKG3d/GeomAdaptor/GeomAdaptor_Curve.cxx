// Created on: 1993-04-29
// Created by: Bruno DUMORTIER
// Copyright (c) 1993-1999 Matra Datavision
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

#include <GeomAdaptor_Curve.hxx>

#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Standard_NoSuchObject.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_Curve, Standard_Transient)

//==================================================================================================

Handle(GeomAdaptor_Curve) GeomAdaptor_Curve::ShallowCopy() const
{
  Handle(GeomAdaptor_Curve) aCopy = new GeomAdaptor_Curve();
  aCopy->myCore                   = myCore; // Uses copy constructor of Core
  return aCopy;
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_Curve::Continuity() const
{
  return myCore.Continuity();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Curve::NbIntervals(const GeomAbs_Shape S) const
{
  return myCore.NbIntervals(S);
}

//==================================================================================================

void GeomAdaptor_Curve::Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const
{
  myCore.Intervals(T, S);
}

//==================================================================================================

Handle(GeomAdaptor_Curve) GeomAdaptor_Curve::Trim(const Standard_Real First,
                                                  const Standard_Real Last,
                                                  const Standard_Real /*Tol*/) const
{
  return new GeomAdaptor_Curve(myCore.Curve(), First, Last);
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Curve::IsClosed() const
{
  return myCore.IsClosed();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Curve::IsPeriodic() const
{
  return myCore.IsPeriodic();
}

//==================================================================================================

Standard_Real GeomAdaptor_Curve::Period() const
{
  return myCore.Period();
}

//==================================================================================================

gp_Pnt GeomAdaptor_Curve::Value(const Standard_Real U) const
{
  return myCore.Value(U);
}

//==================================================================================================

void GeomAdaptor_Curve::D0(const Standard_Real U, gp_Pnt& P) const
{
  myCore.D0(U, P);
}

//==================================================================================================

void GeomAdaptor_Curve::D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V) const
{
  myCore.D1(U, P, V);
}

//==================================================================================================

void GeomAdaptor_Curve::D2(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const
{
  myCore.D2(U, P, V1, V2);
}

//==================================================================================================

void GeomAdaptor_Curve::D3(const Standard_Real U,
                           gp_Pnt&             P,
                           gp_Vec&             V1,
                           gp_Vec&             V2,
                           gp_Vec&             V3) const
{
  myCore.D3(U, P, V1, V2, V3);
}

//==================================================================================================

gp_Vec GeomAdaptor_Curve::DN(const Standard_Real U, const Standard_Integer N) const
{
  return myCore.DN(U, N);
}

//==================================================================================================

Standard_Real GeomAdaptor_Curve::Resolution(const Standard_Real R3D) const
{
  return myCore.Resolution(R3D);
}

//==================================================================================================

gp_Lin GeomAdaptor_Curve::Line() const
{
  return myCore.Line();
}

//==================================================================================================

gp_Circ GeomAdaptor_Curve::Circle() const
{
  return myCore.Circle();
}

//==================================================================================================

gp_Elips GeomAdaptor_Curve::Ellipse() const
{
  return myCore.Ellipse();
}

//==================================================================================================

gp_Hypr GeomAdaptor_Curve::Hyperbola() const
{
  return myCore.Hyperbola();
}

//==================================================================================================

gp_Parab GeomAdaptor_Curve::Parabola() const
{
  return myCore.Parabola();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Curve::Degree() const
{
  return myCore.Degree();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Curve::IsRational() const
{
  return myCore.IsRational();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Curve::NbPoles() const
{
  return myCore.NbPoles();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Curve::NbKnots() const
{
  return myCore.NbKnots();
}

//==================================================================================================

Handle(Geom_BezierCurve) GeomAdaptor_Curve::Bezier() const
{
  return myCore.Bezier();
}

//==================================================================================================

Handle(Geom_BSplineCurve) GeomAdaptor_Curve::BSpline() const
{
  return myCore.BSpline();
}

//==================================================================================================

Handle(Geom_OffsetCurve) GeomAdaptor_Curve::OffsetCurve() const
{
  return myCore.OffsetCurve();
}
