// Created on: 1993-02-22
// Created by: Modelistation
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

#ifndef _GeomAdaptor_CurveOnSurface_HeaderFile
#define _GeomAdaptor_CurveOnSurface_HeaderFile

#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_HSequenceOfReal.hxx>

DEFINE_STANDARD_HANDLE(GeomAdaptor_CurveOnSurface, GeomAdaptor_Curve)

//! An interface between the services provided by a curve
//! lying on a surface from the package Geom and those
//! required of the curve by algorithms which use it. The
//! curve is defined as a 2D curve from the Geom2d
//! package, in the parametric space of the surface.
class GeomAdaptor_CurveOnSurface : public GeomAdaptor_Curve
{
  DEFINE_STANDARD_RTTIEXT(GeomAdaptor_CurveOnSurface, GeomAdaptor_Curve)
public:
  Standard_EXPORT GeomAdaptor_CurveOnSurface();

  Standard_EXPORT GeomAdaptor_CurveOnSurface(const Handle(GeomAdaptor_Surface)& S);

  //! Creates a CurveOnSurface from the 2d curve <C> and
  //! the surface <S>.
  Standard_EXPORT GeomAdaptor_CurveOnSurface(const Handle(Geom2dAdaptor_Curve)& C,
                                           const Handle(GeomAdaptor_Surface)& S);

  //! Shallow copy of adaptor
  Standard_EXPORT virtual Handle(GeomAdaptor_Curve) ShallowCopy() const Standard_OVERRIDE;

  //! Changes the surface.
  Standard_EXPORT void Load(const Handle(GeomAdaptor_Surface)& S);

  //! Changes the 2d curve.
  Standard_EXPORT void Load(const Handle(Geom2dAdaptor_Curve)& C);

  //! Load both curve and surface.
  Standard_EXPORT void Load(const Handle(Geom2dAdaptor_Curve)& C, const Handle(GeomAdaptor_Surface)& S);

  Standard_EXPORT const Handle(Geom2dAdaptor_Curve)& GetCurve() const;

  Standard_EXPORT const Handle(GeomAdaptor_Surface)& GetSurface() const;

  Standard_EXPORT Handle(Geom2dAdaptor_Curve)& ChangeCurve();

  Standard_EXPORT Handle(GeomAdaptor_Surface)& ChangeSurface();

  Standard_EXPORT Standard_Real FirstParameter() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Real LastParameter() const Standard_OVERRIDE;

  Standard_EXPORT GeomAbs_Shape Continuity() const Standard_OVERRIDE;

  //! Returns the number of intervals for continuity
  //! <S>. May be one if Continuity(me) >= <S>
  Standard_EXPORT Standard_Integer NbIntervals(const GeomAbs_Shape S) const Standard_OVERRIDE;

  //! Stores in <T> the parameters bounding the intervals
  //! of continuity <S>.
  //!
  //! The array must provide enough room to accommodate
  //! for the parameters. i.e. T.Length() > NbIntervals()
  Standard_EXPORT void Intervals(TColStd_Array1OfReal& T,
                                 const GeomAbs_Shape   S) const Standard_OVERRIDE;

  //! Returns a curve equivalent of <me> between
  //! parameters <First> and <Last>. <Tol> is used to
  //! test for 3d points confusion.
  //! If <First> >= <Last>
  Standard_EXPORT Handle(GeomAdaptor_Curve) Trim(const Standard_Real First,
                                               const Standard_Real Last,
                                               const Standard_Real Tol) const Standard_OVERRIDE;

  Standard_EXPORT Standard_Boolean IsClosed() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Boolean IsPeriodic() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Real Period() const Standard_OVERRIDE;

  //! D0-DN methods are inherited from GeomAdaptor_Curve and delegate to Core.
  //! The Core is configured with CurveOnSurfaceData at Load() time.

  //! Returns the parametric resolution corresponding
  //! to the real space resolution <R3d>.
  Standard_EXPORT Standard_Real Resolution(const Standard_Real R3d) const Standard_OVERRIDE;

  //! Returns the type of the curve in the current
  //! interval: Line, Circle, Ellipse, Hyperbola,
  //! Parabola, BezierCurve, BSplineCurve, OtherCurve.
  Standard_EXPORT GeomAbs_CurveType GetType() const Standard_OVERRIDE;

  Standard_EXPORT gp_Lin Line() const Standard_OVERRIDE;

  Standard_EXPORT gp_Circ Circle() const Standard_OVERRIDE;

  Standard_EXPORT gp_Elips Ellipse() const Standard_OVERRIDE;

  Standard_EXPORT gp_Hypr Hyperbola() const Standard_OVERRIDE;

  Standard_EXPORT gp_Parab Parabola() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Integer Degree() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Boolean IsRational() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Integer NbPoles() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Integer NbKnots() const Standard_OVERRIDE;

  Standard_EXPORT Handle(Geom_BezierCurve) Bezier() const Standard_OVERRIDE;

  Standard_EXPORT Handle(Geom_BSplineCurve) BSpline() const Standard_OVERRIDE;

private:
  Standard_EXPORT void EvalKPart();

  //! Evaluates myFirstSurf and myLastSurf
  //! for trimming the curve on surface.
  //! Following methods output left-bottom and right-top points
  //! of located part on surface
  //! for trimming the curve on surface.
  Standard_EXPORT void EvalFirstLastSurf();

  Standard_EXPORT void LocatePart(const gp_Pnt2d&                  UV,
                                  const gp_Vec2d&                  DUV,
                                  const Handle(GeomAdaptor_Surface)& S,
                                  gp_Pnt2d&                        LeftBot,
                                  gp_Pnt2d&                        RightTop) const;

  Standard_EXPORT Standard_Boolean LocatePart_RevExt(const gp_Pnt2d&                  UV,
                                                     const gp_Vec2d&                  DUV,
                                                     const Handle(GeomAdaptor_Surface)& S,
                                                     gp_Pnt2d&                        LeftBot,
                                                     gp_Pnt2d& RightTop) const;

  Standard_EXPORT Standard_Boolean LocatePart_Offset(const gp_Pnt2d&                  UV,
                                                     const gp_Vec2d&                  DUV,
                                                     const Handle(GeomAdaptor_Surface)& S,
                                                     gp_Pnt2d&                        LeftBot,
                                                     gp_Pnt2d& RightTop) const;

  //! Extracts the numbers of knots which equal
  //! the point and checks derivative components
  //! by zero equivalence.
  Standard_EXPORT void FindBounds(const TColStd_Array1OfReal& Arr,
                                  const Standard_Real         XYComp,
                                  const Standard_Real         DUVComp,
                                  Standard_Integer&           Bnd1,
                                  Standard_Integer&           Bnd2,
                                  Standard_Boolean&           DerIsNull) const;

private:
  Handle(GeomAdaptor_Surface)       mySurface;
  Handle(Geom2dAdaptor_Curve)       myCurve;
  GeomAbs_CurveType               myType;
  gp_Circ                         myCirc;
  gp_Lin                          myLin;
  Handle(GeomAdaptor_Surface)       myFirstSurf;
  Handle(GeomAdaptor_Surface)       myLastSurf;
  Handle(TColStd_HSequenceOfReal) myIntervals;
  GeomAbs_Shape                   myIntCont;
};

#endif // _GeomAdaptor_CurveOnSurface_HeaderFile
