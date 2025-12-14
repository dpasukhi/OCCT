// Created on: 1993-08-25
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

#ifndef _ProjLib_ProjectedCurve_HeaderFile
#define _ProjLib_ProjectedCurve_HeaderFile

#include <Adaptor3d_Surface.hxx>
#include <Adaptor3d_Curve.hxx>
#include <AppParCurves_Constraint.hxx>
#include <GeomAbs_CurveType.hxx>
#include <ProjLib_Projector.hxx>
#include <Standard_Transient.hxx>

class gp_Pnt2d;
class gp_Vec2d;
class gp_Lin2d;
class gp_Circ2d;
class gp_Elips2d;
class gp_Hypr2d;
class gp_Parab2d;
class Geom2d_BezierCurve;
class Geom2d_BSplineCurve;
class Geom2d_Curve;

DEFINE_STANDARD_HANDLE(ProjLib_ProjectedCurve, Standard_Transient)

//! Compute the 2d-curve. Try to solve the particular
//! case if possible. Otherwise, an approximation is
//! done. For approximation some parameters are used, including
//! required tolerance of approximation.
//! Tolerance is maximal possible value of 3d deviation of 3d projection of projected curve from
//! "exact" 3d projection. Since algorithm searches 2d curve on surface, required 2d tolerance is
//! computed from 3d tolerance with help of U,V resolutions of surface. 3d and 2d tolerances have
//! sense only for curves on surface, it defines precision of projecting and approximation and have
//! nothing to do with distance between the projected curve and the surface.
class ProjLib_ProjectedCurve : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(ProjLib_ProjectedCurve, Standard_Transient)
public:
  //! Empty constructor, it only sets some initial values for class fields.
  Standard_EXPORT ProjLib_ProjectedCurve();

  //! Constructor with initialisation field mySurface
  Standard_EXPORT ProjLib_ProjectedCurve(const Handle(Adaptor3d_Surface)& S);

  //! Constructor, which performs projecting.
  //! If projecting uses approximation, default parameters are used, in particular, 3d tolerance of
  //! approximation is Precision::Confusion()
  Standard_EXPORT ProjLib_ProjectedCurve(const Handle(Adaptor3d_Surface)& S,
                                         const Handle(Adaptor3d_Curve)&   C);

  //! Constructor, which performs projecting.
  //! If projecting uses approximation, 3d tolerance is Tol, default parameters are used,
  Standard_EXPORT ProjLib_ProjectedCurve(const Handle(Adaptor3d_Surface)& S,
                                         const Handle(Adaptor3d_Curve)&   C,
                                         const Standard_Real              Tol);

  //! Shallow copy of projector
  Standard_EXPORT Handle(ProjLib_ProjectedCurve) ShallowCopy() const;

  //! Changes the tolerance used to project
  //! the curve on the surface
  Standard_EXPORT void Load(const Standard_Real Tolerance);

  //! Changes the Surface.
  Standard_EXPORT void Load(const Handle(Adaptor3d_Surface)& S);

  //! Performs projecting for given curve.
  //! If projecting uses approximation,
  //! approximation parameters can be set before by corresponding methods
  //! SetDegree(...), SetMaxSegmets(...), SetBndPnt(...), SetMaxDist(...)
  Standard_EXPORT void Perform(const Handle(Adaptor3d_Curve)& C);

  //! Set min and max possible degree of result BSpline curve2d, which is got by approximation.
  //! If theDegMin/Max < 0, algorithm uses values that are chosen depending of types curve 3d
  //! and surface.
  Standard_EXPORT void SetDegree(const Standard_Integer theDegMin, const Standard_Integer theDegMax);

  //! Set the parameter, which defines maximal value of parametric intervals the projected
  //! curve can be cut for approximation. If theMaxSegments < 0, algorithm uses default
  //! value = 1000.
  Standard_EXPORT void SetMaxSegments(const Standard_Integer theMaxSegments);

  //! Set the parameter, which defines type of boundary condition between segments during
  //! approximation. It can be AppParCurves_PassPoint or AppParCurves_TangencyPoint. Default value
  //! is AppParCurves_TangencyPoint;
  Standard_EXPORT void SetBndPnt(const AppParCurves_Constraint theBndPnt);

  //! Set the parameter, which degines maximal possible distance between projected curve and
  //! surface. It uses only for projecting on not analytical surfaces. If theMaxDist < 0, algorithm
  //! uses default value 100.*Tolerance. If real distance between curve and surface more then
  //! theMaxDist, algorithm stops working.
  Standard_EXPORT void SetMaxDist(const Standard_Real theMaxDist);

  Standard_EXPORT const Handle(Adaptor3d_Surface)& GetSurface() const;

  Standard_EXPORT const Handle(Adaptor3d_Curve)& GetCurve() const;

  //! returns the tolerance reached if an approximation
  //! is Done.
  Standard_EXPORT Standard_Real GetTolerance() const;

  //! Returns true if the projection was successful.
  Standard_Boolean IsDone() const { return myResult.IsDone(); }

  //! Returns the first parameter of the curve.
  Standard_EXPORT Standard_Real FirstParameter() const;

  //! Returns the last parameter of the curve.
  Standard_EXPORT Standard_Real LastParameter() const;

  //! Returns true if the result is periodic.
  Standard_EXPORT Standard_Boolean IsPeriodic() const;

  //! Returns the type of the curve in the current
  //! interval: Line, Circle, Ellipse, Hyperbola,
  //! Parabola, BezierCurve, BSplineCurve, OtherCurve.
  Standard_EXPORT GeomAbs_CurveType GetType() const;

  Standard_EXPORT gp_Lin2d Line() const;

  Standard_EXPORT gp_Circ2d Circle() const;

  Standard_EXPORT gp_Elips2d Ellipse() const;

  Standard_EXPORT gp_Hypr2d Hyperbola() const;

  Standard_EXPORT gp_Parab2d Parabola() const;

  Standard_EXPORT Standard_Integer Degree() const;

  Standard_EXPORT Standard_Boolean IsRational() const;

  Standard_EXPORT Standard_Integer NbPoles() const;

  Standard_EXPORT Standard_Integer NbKnots() const;

  //! Warning! This will NOT make a copy of the Bezier Curve
  //! If you want to modify the Curve please make a copy
  //! yourself. Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT Handle(Geom2d_BezierCurve) Bezier() const;

  //! Warning! This will NOT make a copy of the BSpline Curve
  //! If you want to modify the Curve please make a copy
  //! yourself. Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT Handle(Geom2d_BSplineCurve) BSpline() const;

  //! Returns the result 2D curve as Geom2d_Curve.
  //! Returns null if projection was not done or failed.
  Standard_EXPORT Handle(Geom2d_Curve) GetCurve2d() const;

private:
  Standard_Real             myTolerance;
  Handle(Adaptor3d_Surface) mySurface;
  Handle(Adaptor3d_Curve)   myCurve;
  ProjLib_Projector         myResult;
  Standard_Integer          myDegMin;
  Standard_Integer          myDegMax;
  Standard_Integer          myMaxSegments;
  Standard_Real             myMaxDist;
  AppParCurves_Constraint   myBndPnt;
};

#endif // _ProjLib_ProjectedCurve_HeaderFile
