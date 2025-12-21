// Created on: 1993-06-03
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

#ifndef _Geom2dAdaptor_Curve_HeaderFile
#define _Geom2dAdaptor_Curve_HeaderFile

#include <Geom2dAdaptor_CurveCore.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <Standard_Transient.hxx>
#include <TColStd_Array1OfReal.hxx>

class Geom2d_BezierCurve;
class Geom2d_BSplineCurve;
class gp_Circ2d;
class gp_Elips2d;
class gp_Hypr2d;
class gp_Lin2d;
class gp_Parab2d;
class gp_Pnt2d;
class gp_Vec2d;

DEFINE_STANDARD_HANDLE(Geom2dAdaptor_Curve, Standard_Transient)

//! An interface between the services provided by any
//! curve from the package Geom2d and those required
//! of the curve by algorithms which use it.
//!
//! This is the base class for 2D curve adaptors in OCCT. Derived classes include:
//! - BRepAdaptor_Curve2d for curves from topological edges
//! - Geom2dAdaptor_OffsetCurve for offset curves
//!
//! Internally delegates all evaluation to Geom2dAdaptor_CurveCore for efficient
//! non-virtual dispatch. The Core class handles caching for BSpline/Bezier curves
//! and supports optional coordinate transformation.
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class Geom2dAdaptor_Curve : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Geom2dAdaptor_Curve, Standard_Transient)
public:

  //! Default constructor.
  Geom2dAdaptor_Curve() {}

  //! Constructor with curve.
  Geom2dAdaptor_Curve(const Handle(Geom2d_Curve)& theCurve)
      : myCore(theCurve)
  {
  }

  //! Constructor with curve and parameter bounds.
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  Geom2dAdaptor_Curve(const Handle(Geom2d_Curve)& theCurve,
                      const Standard_Real         theUFirst,
                      const Standard_Real         theULast)
      : myCore(theCurve, theUFirst, theULast)
  {
  }

  //! Shallow copy of adaptor
  Standard_EXPORT virtual Handle(Geom2dAdaptor_Curve) ShallowCopy() const;

  //! Reset currently loaded curve (undone Load()).
  void Reset() { myCore.Reset(); }

  //! Load a curve.
  //! @throw Standard_NullObject if theCurve is null
  void Load(const Handle(Geom2d_Curve)& theCurve) { myCore.Load(theCurve); }

  //! Load a curve with parameter bounds.
  //! @throw Standard_NullObject if theCurve is null
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  void Load(const Handle(Geom2d_Curve)& theCurve,
            const Standard_Real         theUFirst,
            const Standard_Real         theULast)
  {
    myCore.Load(theCurve, theUFirst, theULast);
  }

  //! Returns the underlying curve.
  const Handle(Geom2d_Curve)& Curve() const { return myCore.Curve(); }

  virtual Standard_Real FirstParameter() const { return myCore.FirstParameter(); }

  virtual Standard_Real LastParameter() const { return myCore.LastParameter(); }

  Standard_EXPORT virtual GeomAbs_Shape Continuity() const;

  //! If necessary, breaks the curve in intervals of
  //! continuity <S>. And returns the number of intervals.
  Standard_EXPORT virtual Standard_Integer NbIntervals(const GeomAbs_Shape S) const;

  //! Stores in <T> the parameters bounding the intervals of continuity <S>.
  //! The array must provide enough room to accommodate for the parameters.
  //! i.e. T.Length() > NbIntervals()
  Standard_EXPORT virtual void Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const;

  //! Returns a curve equivalent of <me> between
  //! parameters <First> and <Last>. <Tol> is used to
  //! test for 2d points confusion.
  Standard_EXPORT virtual Handle(Geom2dAdaptor_Curve) Trim(const Standard_Real First,
                                                           const Standard_Real Last,
                                                           const Standard_Real Tol) const;

  Standard_EXPORT virtual Standard_Boolean IsClosed() const;

  Standard_EXPORT virtual Standard_Boolean IsPeriodic() const;

  Standard_EXPORT virtual Standard_Real Period() const;

  //! Computes the point of parameter U on the curve
  Standard_EXPORT virtual gp_Pnt2d Value(const Standard_Real U) const;

  //! Computes the point of parameter U.
  Standard_EXPORT virtual void D0(const Standard_Real U, gp_Pnt2d& P) const;

  //! Computes the point of parameter U on the curve with its first derivative.
  Standard_EXPORT virtual void D1(const Standard_Real U, gp_Pnt2d& P, gp_Vec2d& V) const;

  //! Returns the point P of parameter U, the first and second derivatives V1 and V2.
  Standard_EXPORT virtual void D2(const Standard_Real U,
                                  gp_Pnt2d&           P,
                                  gp_Vec2d&           V1,
                                  gp_Vec2d&           V2) const;

  //! Returns the point P of parameter U, the first, the second and the third derivative.
  Standard_EXPORT virtual void D3(const Standard_Real U,
                                  gp_Pnt2d&           P,
                                  gp_Vec2d&           V1,
                                  gp_Vec2d&           V2,
                                  gp_Vec2d&           V3) const;

  //! The returned vector gives the value of the derivative for the order of derivation N.
  //! @throw Standard_OutOfRange if N < 1
  Standard_EXPORT virtual gp_Vec2d DN(const Standard_Real U, const Standard_Integer N) const;

  //! Returns the parametric resolution
  Standard_EXPORT virtual Standard_Real Resolution(const Standard_Real Ruv) const;

  virtual GeomAbs_CurveType GetType() const { return myCore.GetType(); }

  Standard_EXPORT virtual gp_Lin2d Line() const;

  Standard_EXPORT virtual gp_Circ2d Circle() const;

  Standard_EXPORT virtual gp_Elips2d Ellipse() const;

  Standard_EXPORT virtual gp_Hypr2d Hyperbola() const;

  Standard_EXPORT virtual gp_Parab2d Parabola() const;

  Standard_EXPORT virtual Standard_Integer Degree() const;

  Standard_EXPORT virtual Standard_Boolean IsRational() const;

  Standard_EXPORT virtual Standard_Integer NbPoles() const;

  Standard_EXPORT virtual Standard_Integer NbKnots() const;

  Standard_EXPORT virtual Standard_Integer NbSamples() const;

  Standard_EXPORT virtual Handle(Geom2d_BezierCurve) Bezier() const;

  Standard_EXPORT virtual Handle(Geom2d_BSplineCurve) BSpline() const;

  //! Returns the internal Core object for direct access.
  //! Use with caution - modifications affect this adaptor.
  Geom2dAdaptor_CurveCore& Core() { return myCore; }

  //! Returns the internal Core object (const version).
  const Geom2dAdaptor_CurveCore& Core() const { return myCore; }

private:
  Geom2dAdaptor_CurveCore myCore; //!< Core evaluation implementation
};

#endif // _Geom2dAdaptor_Curve_HeaderFile
