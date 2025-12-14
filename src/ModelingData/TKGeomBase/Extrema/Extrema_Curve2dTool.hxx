// Created on: 1995-07-18
// Created by: Modelistation
// Copyright (c) 1995-1999 Matra Datavision
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

#ifndef _Extrema_Curve2dTool_HeaderFile
#define _Extrema_Curve2dTool_HeaderFile

#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Hypr2d.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Parab2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_HArray1OfReal.hxx>

class Extrema_Curve2dTool
{
public:
  DEFINE_STANDARD_ALLOC

  static double FirstParameter(const Geom2dAdaptor_Curve& theC) { return theC.FirstParameter(); }

  static double LastParameter(const Geom2dAdaptor_Curve& theC) { return theC.LastParameter(); }

  static GeomAbs_Shape Continuity(const Geom2dAdaptor_Curve& theC) { return theC.Continuity(); }

  //! If necessary, breaks the curve in intervals of continuity <S>.
  //! And returns the number of intervals.
  static int NbIntervals(const Geom2dAdaptor_Curve& theC, const GeomAbs_Shape theS)
  {
    return theC.NbIntervals(theS);
  }

  //! Stores in <T> the parameters bounding the intervals of continuity <S>.
  static void Intervals(const Geom2dAdaptor_Curve& theC,
                        TColStd_Array1OfReal&    theT,
                        const GeomAbs_Shape      theS)
  {
    theC.Intervals(theT, theS);
  }

  //! Returns the parameters bounding the intervals of subdivision of curve
  //! according to Curvature deflection. Value of deflection is defined in method.
  Standard_EXPORT static Handle(TColStd_HArray1OfReal) DeflCurvIntervals(
    const Geom2dAdaptor_Curve& theC);

  static bool IsClosed(const Geom2dAdaptor_Curve& theC) { return theC.IsClosed(); }

  static bool IsPeriodic(const Geom2dAdaptor_Curve& theC) { return theC.IsPeriodic(); }

  static double Period(const Geom2dAdaptor_Curve& theC) { return theC.Period(); }

  //! Computes the point of parameter U on the curve.
  static gp_Pnt2d Value(const Geom2dAdaptor_Curve& theC, const double theU)
  {
    return theC.Value(theU);
  }

  //! Computes the point of parameter U on the curve.
  static void D0(const Geom2dAdaptor_Curve& theC, const double theU, gp_Pnt2d& theP)
  {
    theC.D0(theU, theP);
  }

  //! Computes the point of parameter U on the curve with its first derivative.
  static void D1(const Geom2dAdaptor_Curve& theC, const double theU, gp_Pnt2d& theP, gp_Vec2d& theV)
  {
    theC.D1(theU, theP, theV);
  }

  //! Returns the point P of parameter U, the first and second derivatives V1 and V2.
  static void D2(const Geom2dAdaptor_Curve& theC,
                 const double             theU,
                 gp_Pnt2d&                theP,
                 gp_Vec2d&                theV1,
                 gp_Vec2d&                theV2)
  {
    theC.D2(theU, theP, theV1, theV2);
  }

  //! Returns the point P of parameter U, the first, the second and the third derivative.
  static void D3(const Geom2dAdaptor_Curve& theC,
                 const double             theU,
                 gp_Pnt2d&                theP,
                 gp_Vec2d&                theV1,
                 gp_Vec2d&                theV2,
                 gp_Vec2d&                theV3)
  {
    theC.D3(theU, theP, theV1, theV2, theV3);
  }

  //! The returned vector gives the value of the derivative for the order of derivation N.
  static gp_Vec2d DN(const Geom2dAdaptor_Curve& theC, const double theU, const int theN)
  {
    return theC.DN(theU, theN);
  }

  //! Returns the parametric resolution corresponding to the real space resolution <R3d>.
  static double Resolution(const Geom2dAdaptor_Curve& theC, const double theR3d)
  {
    return theC.Resolution(theR3d);
  }

  //! Returns the type of the curve in the current interval:
  //! Line, Circle, Ellipse, Hyperbola, Parabola, BezierCurve, BSplineCurve, OtherCurve.
  static GeomAbs_CurveType GetType(const Geom2dAdaptor_Curve& theC) { return theC.GetType(); }

  static gp_Lin2d Line(const Geom2dAdaptor_Curve& theC) { return theC.Line(); }

  static gp_Circ2d Circle(const Geom2dAdaptor_Curve& theC) { return theC.Circle(); }

  static gp_Elips2d Ellipse(const Geom2dAdaptor_Curve& theC) { return theC.Ellipse(); }

  static gp_Hypr2d Hyperbola(const Geom2dAdaptor_Curve& theC) { return theC.Hyperbola(); }

  static gp_Parab2d Parabola(const Geom2dAdaptor_Curve& theC) { return theC.Parabola(); }

  static int Degree(const Geom2dAdaptor_Curve& theC) { return theC.Degree(); }

  static bool IsRational(const Geom2dAdaptor_Curve& theC) { return theC.IsRational(); }

  static int NbPoles(const Geom2dAdaptor_Curve& theC) { return theC.NbPoles(); }

  static int NbKnots(const Geom2dAdaptor_Curve& theC) { return theC.NbKnots(); }

  static Handle(Geom2d_BezierCurve) Bezier(const Geom2dAdaptor_Curve& theC) { return theC.Bezier(); }

  static Handle(Geom2d_BSplineCurve) BSpline(const Geom2dAdaptor_Curve& theC)
  {
    return theC.BSpline();
  }
};

#endif // _Extrema_Curve2dTool_HeaderFile
