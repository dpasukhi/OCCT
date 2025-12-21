// Created on: 1993-06-04
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

#include <Geom2dAdaptor_Curve.hxx>

#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom2dAdaptor_Curve, Standard_Transient)

//==================================================================================================

Handle(Geom2dAdaptor_Curve) Geom2dAdaptor_Curve::ShallowCopy() const
{
  Handle(Geom2dAdaptor_Curve) aCopy = new Geom2dAdaptor_Curve();
  aCopy->myCore                     = myCore;  // Uses copy constructor/assignment of Core
  return aCopy;
}

//==================================================================================================

GeomAbs_Shape Geom2dAdaptor_Curve::Continuity() const
{
  return myCore.Continuity();
}

//==================================================================================================

Standard_Integer Geom2dAdaptor_Curve::NbIntervals(const GeomAbs_Shape S) const
{
  return myCore.NbIntervals(S);
}

//==================================================================================================

void Geom2dAdaptor_Curve::Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const
{
  myCore.Intervals(T, S);
}

//==================================================================================================

Handle(Geom2dAdaptor_Curve) Geom2dAdaptor_Curve::Trim(const Standard_Real First,
                                                      const Standard_Real Last,
                                                      const Standard_Real) const
{
  return new Geom2dAdaptor_Curve(myCore.Curve(), First, Last);
}

//==================================================================================================

Standard_Boolean Geom2dAdaptor_Curve::IsClosed() const
{
  return myCore.IsClosed();
}

//==================================================================================================

Standard_Boolean Geom2dAdaptor_Curve::IsPeriodic() const
{
  return myCore.IsPeriodic();
}

//==================================================================================================

Standard_Real Geom2dAdaptor_Curve::Period() const
{
  return myCore.Period();
}

//==================================================================================================

gp_Pnt2d Geom2dAdaptor_Curve::Value(const Standard_Real U) const
{
  return myCore.Value(U);
}

//==================================================================================================

void Geom2dAdaptor_Curve::D0(const Standard_Real U, gp_Pnt2d& P) const
{
  myCore.D0(U, P);
}

//==================================================================================================

void Geom2dAdaptor_Curve::D1(const Standard_Real U, gp_Pnt2d& P, gp_Vec2d& V) const
{
  myCore.D1(U, P, V);
}

//==================================================================================================

void Geom2dAdaptor_Curve::D2(const Standard_Real U, gp_Pnt2d& P, gp_Vec2d& V1, gp_Vec2d& V2) const
{
  myCore.D2(U, P, V1, V2);
}

//==================================================================================================

void Geom2dAdaptor_Curve::D3(const Standard_Real U,
                             gp_Pnt2d&           P,
                             gp_Vec2d&           V1,
                             gp_Vec2d&           V2,
                             gp_Vec2d&           V3) const
{
  myCore.D3(U, P, V1, V2, V3);
}

//==================================================================================================

gp_Vec2d Geom2dAdaptor_Curve::DN(const Standard_Real U, const Standard_Integer N) const
{
  return myCore.DN(U, N);
}

//==================================================================================================

Standard_Real Geom2dAdaptor_Curve::Resolution(const Standard_Real Ruv) const
{
  return myCore.Resolution(Ruv);
}

//==================================================================================================

gp_Lin2d Geom2dAdaptor_Curve::Line() const
{
  return myCore.Line();
}

//==================================================================================================

gp_Circ2d Geom2dAdaptor_Curve::Circle() const
{
  return myCore.Circle();
}

//==================================================================================================

gp_Elips2d Geom2dAdaptor_Curve::Ellipse() const
{
  return myCore.Ellipse();
}

//==================================================================================================

gp_Hypr2d Geom2dAdaptor_Curve::Hyperbola() const
{
  return myCore.Hyperbola();
}

//==================================================================================================

gp_Parab2d Geom2dAdaptor_Curve::Parabola() const
{
  return myCore.Parabola();
}

//==================================================================================================

Standard_Integer Geom2dAdaptor_Curve::Degree() const
{
  return myCore.Degree();
}

//==================================================================================================

Standard_Boolean Geom2dAdaptor_Curve::IsRational() const
{
  return myCore.IsRational();
}

//==================================================================================================

Standard_Integer Geom2dAdaptor_Curve::NbPoles() const
{
  return myCore.NbPoles();
}

//==================================================================================================

Standard_Integer Geom2dAdaptor_Curve::NbKnots() const
{
  return myCore.NbKnots();
}

//==================================================================================================

Standard_Integer Geom2dAdaptor_Curve::NbSamples() const
{
  const Handle(Geom2d_Curve)& aCurve = myCore.Curve();
  if (aCurve.IsNull())
  {
    return 20;
  }

  GeomAbs_CurveType aType = myCore.GetType();
  switch (aType)
  {
    case GeomAbs_Line:
      return 2;

    case GeomAbs_BezierCurve: {
      Handle(Geom2d_BezierCurve) aBezier = Handle(Geom2d_BezierCurve)::DownCast(aCurve);
      if (!aBezier.IsNull())
      {
        return 3 + aBezier->NbPoles();
      }
      break;
    }

    case GeomAbs_BSplineCurve: {
      Handle(Geom2d_BSplineCurve) aBSpline = Handle(Geom2d_BSplineCurve)::DownCast(aCurve);
      if (!aBSpline.IsNull())
      {
        int nbs = aBSpline->NbKnots() * aBSpline->Degree();
        if (nbs < 2)
          nbs = 2;
        if (nbs > 300)
          nbs = 300;
        return nbs;
      }
      break;
    }

    default:
      break;
  }

  return 20;
}

//==================================================================================================

Handle(Geom2d_BezierCurve) Geom2dAdaptor_Curve::Bezier() const
{
  return myCore.Bezier();
}

//==================================================================================================

Handle(Geom2d_BSplineCurve) Geom2dAdaptor_Curve::BSpline() const
{
  return myCore.BSpline();
}
