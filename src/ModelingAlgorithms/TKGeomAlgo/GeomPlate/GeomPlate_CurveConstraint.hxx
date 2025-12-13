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

#ifndef _GeomPlate_CurveConstraint_HeaderFile
#define _GeomPlate_CurveConstraint_HeaderFile

#include <GeomAdaptor_Curve.hxx>
#include <GeomLProp_SLProps.hxx>

#include <memory>

class Adaptor2d_Curve2d;
class Geom2d_Curve;
class Geom2dAdaptor_Curve;
class GeomAdaptor_Surface;
class Law_Function;
class gp_Pnt;
class gp_Vec;

class GeomPlate_CurveConstraint;
DEFINE_STANDARD_HANDLE(GeomPlate_CurveConstraint, Standard_Transient)

//! Defines curves as constraints to be used to deform a surface.
class GeomPlate_CurveConstraint : public Standard_Transient
{

public:
  //! Initializes an empty curve constraint object.
  Standard_EXPORT GeomPlate_CurveConstraint();

  //! Create a constraint from a GeomAdaptor_Curve.
  //! If the curve has a CurveOnSurface modifier, it will be used for surface constraint evaluation.
  //! Order is the order of the constraint. The possible values for order are -1,0,1,2.
  //! Order i means constraints Gi
  //! Npt is the number of points associated with the constraint.
  //! TolDist is the maximum error to satisfy for G0 constraints
  //! TolAng is the maximum error to satisfy for G1 constraints
  //! TolCurv is the maximum error to satisfy for G2 constraints
  //! These errors can be replaced by laws of criterion.
  //! Raises ConstructionError if Order is not -1 , 0, 1, 2
  Standard_EXPORT GeomPlate_CurveConstraint(GeomAdaptor_Curve&&    theBoundary,
                                            const Standard_Integer theOrder,
                                            const Standard_Integer theNPt     = 10,
                                            const Standard_Real    theTolDist = 0.0001,
                                            const Standard_Real    theTolAng  = 0.01,
                                            const Standard_Real    theTolCurv = 0.1);

  //! Create a constraint from an Adaptor3d_Curve.
  //! If the curve is an Adaptor3d_CurveOnSurface, its surface will be used for constraint evaluation.
  //! Otherwise, it must be a GeomAdaptor_Curve or a derived class.
  //! @deprecated Use the constructor taking GeomAdaptor_Curve&& instead.
  Standard_EXPORT GeomPlate_CurveConstraint(const Handle(Adaptor3d_Curve)& theBoundary,
                                            const Standard_Integer         theOrder,
                                            const Standard_Integer         theNPt     = 10,
                                            const Standard_Real            theTolDist = 0.0001,
                                            const Standard_Real            theTolAng  = 0.01,
                                            const Standard_Real            theTolCurv = 0.1);

  //! Allows you to set the order of continuity required for
  //! the constraints: G0, G1, and G2, controlled
  //! respectively by G0Criterion G1Criterion and G2Criterion.
  Standard_EXPORT void SetOrder(const Standard_Integer Order);

  //! Returns the order of constraint, one of G0, G1 or G2.
  Standard_EXPORT Standard_Integer Order() const;

  //! Returns the number of points on the curve used as a
  //! constraint. The default setting is 10. This parameter
  //! affects computation time, which increases by the cube of
  //! the number of points.
  Standard_EXPORT Standard_Integer NbPoints() const;

  //! Allows you to set the number of points on the curve
  //! constraint. The default setting is 10. This parameter
  //! affects computation time, which increases by the cube of
  //! the number of points.
  Standard_EXPORT void SetNbPoints(const Standard_Integer NewNb);

  //! Allows you to set the G0 criterion. This is the law
  //! defining the greatest distance allowed between the
  //! constraint and the target surface for each point of the
  //! constraint. If this criterion is not set, TolDist, the
  //! distance tolerance from the constructor, is used.
  Standard_EXPORT void SetG0Criterion(const Handle(Law_Function)& G0Crit);

  //! Allows you to set the G1 criterion. This is the law
  //! defining the greatest angle allowed between the
  //! constraint and the target surface. If this criterion is not
  //! set, TolAng, the angular tolerance from the constructor, is used.
  //! Raises ConstructionError if the curve is not on a surface
  Standard_EXPORT void SetG1Criterion(const Handle(Law_Function)& G1Crit);

  Standard_EXPORT void SetG2Criterion(const Handle(Law_Function)& G2Crit);

  //! Returns the G0 criterion at the parametric point U on
  //! the curve. This is the greatest distance allowed between
  //! the constraint and the target surface at U.
  Standard_EXPORT Standard_Real G0Criterion(const Standard_Real U) const;

  //! Returns the G1 criterion at the parametric point U on
  //! the curve. This is the greatest angle allowed between
  //! the constraint and the target surface at U.
  //! Raises ConstructionError if the curve is not on a surface
  Standard_EXPORT Standard_Real G1Criterion(const Standard_Real U) const;

  //! Returns the G2 criterion at the parametric point U on
  //! the curve. This is the greatest difference in curvature
  //! allowed between the constraint and the target surface at U.
  //! Raises ConstructionError if the curve is not on a surface
  Standard_EXPORT Standard_Real G2Criterion(const Standard_Real U) const;

  Standard_EXPORT Standard_Real FirstParameter() const;

  Standard_EXPORT Standard_Real LastParameter() const;

  Standard_EXPORT Standard_Real Length() const;

  Standard_EXPORT GeomLProp_SLProps& LPropSurf(const Standard_Real U);

  Standard_EXPORT void D0(const Standard_Real U, gp_Pnt& P) const;

  Standard_EXPORT void D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const;

  Standard_EXPORT void D2(const Standard_Real U,
                          gp_Pnt&             P,
                          gp_Vec&             V1,
                          gp_Vec&             V2,
                          gp_Vec&             V3,
                          gp_Vec&             V4,
                          gp_Vec&             V5) const;

  //! Returns the 3D curve of this constraint as a Handle.
  //! @return handle to a shallow copy of the internal GeomAdaptor_Curve
  Standard_EXPORT Handle(Adaptor3d_Curve) Curve3d() const;

  //! loads a 2d curve associated the surface resulting of the constraints
  Standard_EXPORT void SetCurve2dOnSurf(const Handle(Geom2d_Curve)& Curve2d);

  //! Returns a 2d curve associated the surface resulting of the constraints
  Standard_EXPORT Handle(Geom2d_Curve) Curve2dOnSurf() const;

  //! loads a 2d curve resulting from the normal projection of
  //! the curve on the initial surface
  Standard_EXPORT void SetProjectedCurve(const Handle(Adaptor2d_Curve2d)& Curve2d,
                                         const Standard_Real              TolU,
                                         const Standard_Real              TolV);

  //! Returns the projected curve resulting from the normal projection of the
  //! curve on the initial surface
  Standard_EXPORT Handle(Adaptor2d_Curve2d) ProjectedCurve() const;

  DEFINE_STANDARD_RTTIEXT(GeomPlate_CurveConstraint, Standard_Transient)

protected:
  //! Returns true if this constraint has a curve-on-surface (frontiere).
  Standard_Boolean HasCurveOnSurface() const
  {
    return myCurve != nullptr && myCurve->HasCurveOnSurface();
  }

  //! Returns the PCurve of the curve-on-surface constraint.
  //! @return reference to the 2D curve adaptor
  const Geom2dAdaptor_Curve& GetPCurve() const { return myCurve->GetPCurve(); }

  //! Returns the surface of the curve-on-surface constraint.
  //! @return reference to the surface adaptor
  const GeomAdaptor_Surface& GetSurface() const { return myCurve->GetSurface(); }

protected:
  std::unique_ptr<GeomAdaptor_Curve> myCurve;     //!< The constraint curve (may have COS modifier)
  Standard_Integer                   myNbPoints;
  Standard_Integer                   myOrder;
  Standard_Integer                   myTang;
  Handle(Geom2d_Curve)               my2dCurve;
  Handle(Adaptor2d_Curve2d)          myHCurve2d;
  Handle(Law_Function)               myG0Crit;
  Handle(Law_Function)               myG1Crit;
  Handle(Law_Function)               myG2Crit;
  Standard_Boolean                   myConstG0;
  Standard_Boolean                   myConstG1;
  Standard_Boolean                   myConstG2;
  GeomLProp_SLProps                  myLProp;
  Standard_Real                      myTolDist;
  Standard_Real                      myTolAng;
  Standard_Real                      myTolCurv;
  Standard_Real                      myTolU;
  Standard_Real                      myTolV;
};

#endif // _GeomPlate_CurveConstraint_HeaderFile
