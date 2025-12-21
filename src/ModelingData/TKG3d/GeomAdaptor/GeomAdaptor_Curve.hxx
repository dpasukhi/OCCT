// Created on: 1992-09-01
// Created by: Modelistation
// Copyright (c) 1992-1999 Matra Datavision
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

#ifndef _GeomAdaptor_Curve_HeaderFile
#define _GeomAdaptor_Curve_HeaderFile

#include <GeomAdaptor_CurveCore.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <Standard_Transient.hxx>
#include <TColStd_Array1OfReal.hxx>

class Geom_BezierCurve;
class Geom_BSplineCurve;
class Geom_OffsetCurve;
class gp_Circ;
class gp_Elips;
class gp_Hypr;
class gp_Lin;
class gp_Parab;
class gp_Pnt;
class gp_Vec;

DEFINE_STANDARD_HANDLE(GeomAdaptor_Curve, Standard_Transient)

//! This class provides an interface between the services provided by any
//! curve from the package Geom and those required of the curve by algorithms which use it.
//! Creation of the loaded curve the curve is C1 by piece.
//!
//! This is the base class for 3D curve adaptors in OCCT. Derived classes include:
//! - BRepAdaptor_Curve for curves from topological edges
//! - GeomAdaptor_CurveOnSurface for curves lying on surfaces
//! - GeomAdaptor_IsoCurve for iso-parametric curves on surfaces
//!
//! Internally delegates all evaluation to GeomAdaptor_CurveCore for efficient
//! non-virtual dispatch. The Core class handles caching for BSpline/Bezier curves
//! and supports optional coordinate transformation.
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class GeomAdaptor_Curve : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(GeomAdaptor_Curve, Standard_Transient)
public:

  //! Default constructor.
  GeomAdaptor_Curve() {}

  //! Constructor with curve.
  GeomAdaptor_Curve(const Handle(Geom_Curve)& theCurve)
      : myCore(theCurve)
  {
  }

  //! Constructor with curve and parameter bounds.
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  GeomAdaptor_Curve(const Handle(Geom_Curve)& theCurve,
                    const Standard_Real       theUFirst,
                    const Standard_Real       theULast)
      : myCore(theCurve, theUFirst, theULast)
  {
  }

  //! Shallow copy of adaptor
  Standard_EXPORT virtual Handle(GeomAdaptor_Curve) ShallowCopy() const;

  //! Reset currently loaded curve (undone Load()).
  void Reset() { myCore.Reset(); }

  //! Load a curve.
  //! @throw Standard_NullObject if theCurve is null
  void Load(const Handle(Geom_Curve)& theCurve) { myCore.Load(theCurve); }

  //! Load a curve with parameter bounds.
  //! @throw Standard_NullObject if theCurve is null
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  void Load(const Handle(Geom_Curve)& theCurve,
            const Standard_Real       theUFirst,
            const Standard_Real       theULast)
  {
    myCore.Load(theCurve, theUFirst, theULast);
  }

  //! Returns the underlying curve.
  const Handle(Geom_Curve)& Curve() const { return myCore.Curve(); }

  virtual Standard_Real FirstParameter() const { return myCore.FirstParameter(); }

  virtual Standard_Real LastParameter() const { return myCore.LastParameter(); }

  Standard_EXPORT virtual GeomAbs_Shape Continuity() const;

  //! Returns the number of intervals for continuity <S>.
  //! May be one if Continuity(me) >= <S>
  Standard_EXPORT virtual Standard_Integer NbIntervals(const GeomAbs_Shape S) const;

  //! Stores in <T> the parameters bounding the intervals of continuity <S>.
  //! The array must provide enough room to accommodate for the parameters,
  //! i.e. T.Length() > NbIntervals()
  Standard_EXPORT virtual void Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const;

  //! Returns a curve equivalent of <me> between parameters <First> and <Last>.
  //! <Tol> is used to test for 3d points confusion.
  Standard_EXPORT virtual Handle(GeomAdaptor_Curve) Trim(const Standard_Real First,
                                                         const Standard_Real Last,
                                                         const Standard_Real Tol) const;

  Standard_EXPORT virtual Standard_Boolean IsClosed() const;

  Standard_EXPORT virtual Standard_Boolean IsPeriodic() const;

  Standard_EXPORT virtual Standard_Real Period() const;

  //! Computes the point of parameter U on the curve
  Standard_EXPORT virtual gp_Pnt Value(const Standard_Real U) const;

  //! Computes the point of parameter U.
  Standard_EXPORT virtual void D0(const Standard_Real U, gp_Pnt& P) const;

  //! Computes the point of parameter U on the curve with its first derivative.
  Standard_EXPORT virtual void D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V) const;

  //! Returns the point P of parameter U, the first and second derivatives V1 and V2.
  Standard_EXPORT virtual void D2(const Standard_Real U,
                                  gp_Pnt&             P,
                                  gp_Vec&             V1,
                                  gp_Vec&             V2) const;

  //! Returns the point P of parameter U, the first, the second and the third derivative.
  Standard_EXPORT virtual void D3(const Standard_Real U,
                                  gp_Pnt&             P,
                                  gp_Vec&             V1,
                                  gp_Vec&             V2,
                                  gp_Vec&             V3) const;

  //! The returned vector gives the value of the derivative for the order of derivation N.
  //! @throw Standard_OutOfRange if N < 1
  Standard_EXPORT virtual gp_Vec DN(const Standard_Real U, const Standard_Integer N) const;

  //! Returns the parametric resolution
  Standard_EXPORT virtual Standard_Real Resolution(const Standard_Real R3d) const;

  virtual GeomAbs_CurveType GetType() const { return myCore.GetType(); }

  Standard_EXPORT virtual gp_Lin Line() const;

  Standard_EXPORT virtual gp_Circ Circle() const;

  Standard_EXPORT virtual gp_Elips Ellipse() const;

  Standard_EXPORT virtual gp_Hypr Hyperbola() const;

  Standard_EXPORT virtual gp_Parab Parabola() const;

  Standard_EXPORT virtual Standard_Integer Degree() const;

  Standard_EXPORT virtual Standard_Boolean IsRational() const;

  Standard_EXPORT virtual Standard_Integer NbPoles() const;

  Standard_EXPORT virtual Standard_Integer NbKnots() const;

  //! This will NOT make a copy of the Bezier Curve.
  //! If you want to modify the Curve please make a copy yourself.
  //! Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT virtual Handle(Geom_BezierCurve) Bezier() const;

  //! This will NOT make a copy of the BSpline Curve.
  //! If you want to modify the Curve please make a copy yourself.
  //! Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT virtual Handle(Geom_BSplineCurve) BSpline() const;

  Standard_EXPORT virtual Handle(Geom_OffsetCurve) OffsetCurve() const;

  //! Returns the internal Core object for direct access.
  //! Use with caution - modifications affect this adaptor.
  GeomAdaptor_CurveCore& Core() { return myCore; }

  //! Returns the internal Core object (const version).
  const GeomAdaptor_CurveCore& Core() const { return myCore; }

  friend class GeomAdaptor_Surface;

private:
  GeomAdaptor_CurveCore myCore; //!< Core evaluation implementation
};

#endif // _GeomAdaptor_Curve_HeaderFile
