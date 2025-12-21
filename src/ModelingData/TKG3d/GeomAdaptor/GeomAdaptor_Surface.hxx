// Created on: 1993-05-14
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

#ifndef _GeomAdaptor_Surface_HeaderFile
#define _GeomAdaptor_Surface_HeaderFile

#include <GeomAdaptor_SurfaceCore.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Ax1.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <gp_Vec.hxx>
#include <Standard_Transient.hxx>
#include <TColStd_Array1OfReal.hxx>

class GeomAdaptor_Curve;
class Geom_BezierSurface;
class Geom_BSplineSurface;

DEFINE_STANDARD_HANDLE(GeomAdaptor_Surface, Standard_Transient)

//! An interface between the services provided by any
//! surface from the package Geom and those required
//! of the surface by algorithms which use it.
//! Creation of the loaded surface the surface is C1 by piece
//!
//! This is the base class for 3D surface adaptors in OCCT. Derived classes include:
//! - BRepAdaptor_Surface for surfaces from topological faces
//!
//! Internally delegates all evaluation to GeomAdaptor_SurfaceCore for efficient
//! non-virtual dispatch. The Core class handles caching for BSpline/Bezier surfaces
//! and supports optional coordinate transformation.
//!
//! Polynomial coefficients of BSpline surfaces used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class GeomAdaptor_Surface : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(GeomAdaptor_Surface, Standard_Transient)
public:

  //! Default constructor.
  GeomAdaptor_Surface() {}

  //! Constructor with surface.
  GeomAdaptor_Surface(const Handle(Geom_Surface)& theSurf)
      : myCore(theSurf)
  {
  }

  //! Constructor with surface and parameter bounds.
  //! @throw Standard_ConstructionError if theUFirst > theULast or theVFirst > theVLast
  GeomAdaptor_Surface(const Handle(Geom_Surface)& theSurf,
                      const Standard_Real         theUFirst,
                      const Standard_Real         theULast,
                      const Standard_Real         theVFirst,
                      const Standard_Real         theVLast,
                      const Standard_Real         theTolU = 0.0,
                      const Standard_Real         theTolV = 0.0)
      : myCore(theSurf, theUFirst, theULast, theVFirst, theVLast, theTolU, theTolV)
  {
  }

  //! Shallow copy of adaptor
  Standard_EXPORT virtual Handle(GeomAdaptor_Surface) ShallowCopy() const;

  //! Load a surface.
  //! @throw Standard_NullObject if theSurf is null
  void Load(const Handle(Geom_Surface)& theSurf) { myCore.Load(theSurf); }

  //! Load a surface with parameter bounds.
  //! @throw Standard_NullObject if theSurf is null
  //! @throw Standard_ConstructionError if theUFirst > theULast or theVFirst > theVLast
  void Load(const Handle(Geom_Surface)& theSurf,
            const Standard_Real         theUFirst,
            const Standard_Real         theULast,
            const Standard_Real         theVFirst,
            const Standard_Real         theVLast,
            const Standard_Real         theTolU = 0.0,
            const Standard_Real         theTolV = 0.0)
  {
    myCore.Load(theSurf, theUFirst, theULast, theVFirst, theVLast, theTolU, theTolV);
  }

  //! Returns the underlying surface.
  const Handle(Geom_Surface)& Surface() const { return myCore.Surface(); }

  virtual Standard_Real FirstUParameter() const { return myCore.FirstUParameter(); }

  virtual Standard_Real LastUParameter() const { return myCore.LastUParameter(); }

  virtual Standard_Real FirstVParameter() const { return myCore.FirstVParameter(); }

  virtual Standard_Real LastVParameter() const { return myCore.LastVParameter(); }

  Standard_EXPORT virtual GeomAbs_Shape UContinuity() const;

  Standard_EXPORT virtual GeomAbs_Shape VContinuity() const;

  //! Returns the number of U intervals for continuity
  //! <S>. May be one if UContinuity(me) >= <S>
  Standard_EXPORT virtual Standard_Integer NbUIntervals(const GeomAbs_Shape S) const;

  //! Returns the number of V intervals for continuity
  //! <S>. May be one if VContinuity(me) >= <S>
  Standard_EXPORT virtual Standard_Integer NbVIntervals(const GeomAbs_Shape S) const;

  //! Returns the intervals with the requested continuity
  //! in the U direction.
  Standard_EXPORT virtual void UIntervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const;

  //! Returns the intervals with the requested continuity
  //! in the V direction.
  Standard_EXPORT virtual void VIntervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const;

  //! Returns a surface trimmed in the U direction
  //! equivalent of <me> between
  //! parameters <First> and <Last>. <Tol> is used to
  //! test for 3d points confusion.
  //! If <First> >= <Last>
  Standard_EXPORT virtual Handle(GeomAdaptor_Surface) UTrim(const Standard_Real First,
                                                            const Standard_Real Last,
                                                            const Standard_Real Tol) const;

  //! Returns a surface trimmed in the V direction between
  //! parameters <First> and <Last>. <Tol> is used to
  //! test for 3d points confusion.
  //! If <First> >= <Last>
  Standard_EXPORT virtual Handle(GeomAdaptor_Surface) VTrim(const Standard_Real First,
                                                            const Standard_Real Last,
                                                            const Standard_Real Tol) const;

  Standard_EXPORT virtual Standard_Boolean IsUClosed() const;

  Standard_EXPORT virtual Standard_Boolean IsVClosed() const;

  Standard_EXPORT virtual Standard_Boolean IsUPeriodic() const;

  Standard_EXPORT virtual Standard_Real UPeriod() const;

  Standard_EXPORT virtual Standard_Boolean IsVPeriodic() const;

  Standard_EXPORT virtual Standard_Real VPeriod() const;

  //! Computes the point of parameters U,V on the surface.
  Standard_EXPORT virtual gp_Pnt Value(const Standard_Real U, const Standard_Real V) const;

  //! Computes the point of parameters U,V on the surface.
  Standard_EXPORT virtual void D0(const Standard_Real U, const Standard_Real V, gp_Pnt& P) const;

  //! Computes the point and the first derivatives on
  //! the surface.
  //!
  //! Warning : On the specific case of BSplineSurface:
  //! if the surface is cut in interval of continuity at least C1,
  //! the derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis surface.
  Standard_EXPORT virtual void D1(const Standard_Real U,
                                  const Standard_Real V,
                                  gp_Pnt&             P,
                                  gp_Vec&             D1U,
                                  gp_Vec&             D1V) const;

  //! Computes the point, the first and second derivatives
  //! on the surface.
  //!
  //! Warning : On the specific case of BSplineSurface:
  //! if the surface is cut in interval of continuity at least C2,
  //! the derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis surface.
  Standard_EXPORT virtual void D2(const Standard_Real U,
                                  const Standard_Real V,
                                  gp_Pnt&             P,
                                  gp_Vec&             D1U,
                                  gp_Vec&             D1V,
                                  gp_Vec&             D2U,
                                  gp_Vec&             D2V,
                                  gp_Vec&             D2UV) const;

  //! Computes the point, the first, second and third
  //! derivatives on the surface.
  //!
  //! Warning : On the specific case of BSplineSurface:
  //! if the surface is cut in interval of continuity at least C3,
  //! the derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis surface.
  Standard_EXPORT virtual void D3(const Standard_Real U,
                                  const Standard_Real V,
                                  gp_Pnt&             P,
                                  gp_Vec&             D1U,
                                  gp_Vec&             D1V,
                                  gp_Vec&             D2U,
                                  gp_Vec&             D2V,
                                  gp_Vec&             D2UV,
                                  gp_Vec&             D3U,
                                  gp_Vec&             D3V,
                                  gp_Vec&             D3UUV,
                                  gp_Vec&             D3UVV) const;

  //! Computes the derivative of order Nu in the
  //! direction U and Nv in the direction V at the point P(U, V).
  //!
  //! Warning : On the specific case of BSplineSurface:
  //! if the surface is cut in interval of continuity CN,
  //! the derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis surface.
  //! Raised if Nu + Nv < 1 or Nu < 0 or Nv < 0.
  Standard_EXPORT virtual gp_Vec DN(const Standard_Real    U,
                                    const Standard_Real    V,
                                    const Standard_Integer Nu,
                                    const Standard_Integer Nv) const;

  //! Returns the parametric U resolution corresponding
  //! to the real space resolution <R3d>.
  Standard_EXPORT virtual Standard_Real UResolution(const Standard_Real R3d) const;

  //! Returns the parametric V resolution corresponding
  //! to the real space resolution <R3d>.
  Standard_EXPORT virtual Standard_Real VResolution(const Standard_Real R3d) const;

  //! Returns the type of the surface: Plane, Cylinder,
  //! Cone, Sphere, Torus, BezierSurface,
  //! BSplineSurface, SurfaceOfRevolution,
  //! SurfaceOfExtrusion, OtherSurface
  virtual GeomAbs_SurfaceType GetType() const { return myCore.GetType(); }

  Standard_EXPORT virtual gp_Pln Plane() const;

  Standard_EXPORT virtual gp_Cylinder Cylinder() const;

  Standard_EXPORT virtual gp_Cone Cone() const;

  Standard_EXPORT virtual gp_Sphere Sphere() const;

  Standard_EXPORT virtual gp_Torus Torus() const;

  Standard_EXPORT virtual Standard_Integer UDegree() const;

  Standard_EXPORT virtual Standard_Integer NbUPoles() const;

  Standard_EXPORT virtual Standard_Integer VDegree() const;

  Standard_EXPORT virtual Standard_Integer NbVPoles() const;

  Standard_EXPORT virtual Standard_Integer NbUKnots() const;

  Standard_EXPORT virtual Standard_Integer NbVKnots() const;

  Standard_EXPORT virtual Standard_Boolean IsURational() const;

  Standard_EXPORT virtual Standard_Boolean IsVRational() const;

  //! This will NOT make a copy of the
  //! Bezier Surface : If you want to modify
  //! the Surface please make a copy yourself
  //! Also it will NOT trim the surface to
  //! myU/VFirst/Last.
  Standard_EXPORT virtual Handle(Geom_BezierSurface) Bezier() const;

  //! This will NOT make a copy of the
  //! BSpline Surface : If you want to modify
  //! the Surface please make a copy yourself
  //! Also it will NOT trim the surface to
  //! myU/VFirst/Last.
  Standard_EXPORT virtual Handle(Geom_BSplineSurface) BSpline() const;

  Standard_EXPORT virtual gp_Ax1 AxeOfRevolution() const;

  Standard_EXPORT virtual gp_Dir Direction() const;

  Standard_EXPORT virtual Handle(GeomAdaptor_Curve) BasisCurve() const;

  Standard_EXPORT virtual Handle(GeomAdaptor_Surface) BasisSurface() const;

  Standard_EXPORT virtual Standard_Real OffsetValue() const;

  //! Returns the internal Core object for direct access.
  //! Use with caution - modifications affect this adaptor.
  GeomAdaptor_SurfaceCore& Core() { return myCore; }

  //! Returns the internal Core object (const version).
  const GeomAdaptor_SurfaceCore& Core() const { return myCore; }

private:
  GeomAdaptor_SurfaceCore myCore; //!< Core evaluation implementation
};

#endif // _GeomAdaptor_Surface_HeaderFile
