// Created on: 1993-04-21
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

#include <GeomAdaptor_SurfaceOfLinearExtrusion.hxx>

#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <gp_Ax3.hxx>
#include <Standard_NoSuchObject.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_SurfaceOfLinearExtrusion, GeomAdaptor_Surface)

//=================================================================================================

GeomAdaptor_SurfaceOfLinearExtrusion::GeomAdaptor_SurfaceOfLinearExtrusion()
    : myHaveDir(Standard_False)
{
}

//=================================================================================================

GeomAdaptor_SurfaceOfLinearExtrusion::GeomAdaptor_SurfaceOfLinearExtrusion(
  const Handle(GeomAdaptor_Curve)& C)
    : myHaveDir(Standard_False)
{
  Load(C);
}

//=================================================================================================

GeomAdaptor_SurfaceOfLinearExtrusion::GeomAdaptor_SurfaceOfLinearExtrusion(
  const Handle(GeomAdaptor_Curve)& C,
  const gp_Dir&                  V)
    : myHaveDir(Standard_False)
{
  Load(C);
  Load(V);
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfLinearExtrusion::ShallowCopy() const
{
  Handle(GeomAdaptor_SurfaceOfLinearExtrusion) aCopy = new GeomAdaptor_SurfaceOfLinearExtrusion();

  if (!myBasisCurve.IsNull())
  {
    aCopy->myBasisCurve = myBasisCurve->ShallowCopy();
  }
  aCopy->myDirection = myDirection;
  aCopy->myHaveDir   = myHaveDir;

  // Copy core (which handles surface data internally)
  aCopy->Core() = Core();

  return aCopy;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::Load(const Handle(GeomAdaptor_Curve)& C)
{
  myBasisCurve = C;
  if (myHaveDir)
    Load(myDirection);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::Load(const gp_Dir& V)
{
  myHaveDir   = Standard_True;
  myDirection = V;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::FirstUParameter() const
{
  return myBasisCurve->FirstParameter();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::LastUParameter() const
{
  return myBasisCurve->LastParameter();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::FirstVParameter() const
{
  return RealFirst();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::LastVParameter() const
{
  return RealLast();
}

//=================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceOfLinearExtrusion::UContinuity() const
{
  return myBasisCurve->Continuity();
}

//=================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceOfLinearExtrusion::VContinuity() const
{
  return GeomAbs_CN;
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfLinearExtrusion::NbUIntervals(const GeomAbs_Shape S) const
{
  return myBasisCurve->NbIntervals(S);
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfLinearExtrusion::NbVIntervals(const GeomAbs_Shape) const
{
  return 1;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::UIntervals(TColStd_Array1OfReal& T,
                                                      const GeomAbs_Shape   S) const
{
  myBasisCurve->Intervals(T, S);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::VIntervals(TColStd_Array1OfReal& T,
                                                      const GeomAbs_Shape) const
{
  T(T.Lower())     = FirstVParameter();
  T(T.Lower() + 1) = LastVParameter();
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfLinearExtrusion::VTrim(const Standard_Real First,
                                                                      const Standard_Real Last,
                                                                      const Standard_Real Tol) const
{
  Handle(GeomAdaptor_Curve)                      HC = BasisCurve()->Trim(First, Last, Tol);
  Handle(GeomAdaptor_SurfaceOfLinearExtrusion) HR =
    new GeomAdaptor_SurfaceOfLinearExtrusion(GeomAdaptor_SurfaceOfLinearExtrusion(HC, myDirection));
  return HR;
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfLinearExtrusion::UTrim(const Standard_Real,
                                                                      const Standard_Real,
                                                                      const Standard_Real) const
{
  Handle(GeomAdaptor_SurfaceOfLinearExtrusion) HR = new GeomAdaptor_SurfaceOfLinearExtrusion(
    GeomAdaptor_SurfaceOfLinearExtrusion(myBasisCurve, myDirection));
  return HR;
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsUClosed() const
{
  return myBasisCurve->IsClosed();
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsVClosed() const
{
  return Standard_False;
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsUPeriodic() const
{
  return myBasisCurve->IsPeriodic();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::UPeriod() const
{
  return myBasisCurve->Period();
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsVPeriodic() const
{
  return Standard_False;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::VPeriod() const
{
  throw Standard_DomainError("GeomAdaptor_SurfaceOfLinearExtrusion::VPeriod");
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::UResolution(const Standard_Real R3d) const
{
  return myBasisCurve->Resolution(R3d);
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfLinearExtrusion::VResolution(const Standard_Real R3d) const
{
  return R3d;
}

//=================================================================================================

GeomAbs_SurfaceType GeomAdaptor_SurfaceOfLinearExtrusion::GetType() const
{
  switch (myBasisCurve->GetType())
  {

    case GeomAbs_Line: {
      gp_Dir D = myBasisCurve->Line().Direction();
      if (!myDirection.IsParallel(D, Precision::Angular()))
        return GeomAbs_Plane;
      break;
    }

    case GeomAbs_Circle: {
      gp_Dir D = (myBasisCurve->Circle()).Axis().Direction();
      if (myDirection.IsParallel(D, Precision::Angular()))
        return GeomAbs_Cylinder;
      else if (myDirection.IsNormal(D, Precision::Angular()))
        return GeomAbs_Plane;
      break;
    }

    case GeomAbs_Ellipse: {
      gp_Dir D = (myBasisCurve->Ellipse()).Axis().Direction();
      if (myDirection.IsNormal(D, Precision::Angular()))
        return GeomAbs_Plane;
      break;
    }

    case GeomAbs_Parabola: {
      gp_Dir D = (myBasisCurve->Parabola()).Axis().Direction();
      if (myDirection.IsNormal(D, Precision::Angular()))
        return GeomAbs_Plane;
      break;
    }

    case GeomAbs_Hyperbola: {
      gp_Dir D = (myBasisCurve->Hyperbola()).Axis().Direction();
      if (myDirection.IsNormal(D, Precision::Angular()))
        return GeomAbs_Plane;
      break;
    }

    default:
      break;
  }

  return GeomAbs_SurfaceOfExtrusion;
}

//=================================================================================================

gp_Pln GeomAdaptor_SurfaceOfLinearExtrusion::Plane() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Plane,
                                 "GeomAdaptor_SurfaceOfLinearExtrusion::Plane");

  gp_Pnt        P;
  gp_Vec        D1u, newZ;
  Standard_Real UFirst = myBasisCurve->FirstParameter();
  Standard_Real ULast  = myBasisCurve->LastParameter();
  if (Precision::IsNegativeInfinite(UFirst) && Precision::IsPositiveInfinite(ULast))
  {
    UFirst = -100.;
    ULast  = 100.;
  }
  else if (Precision::IsNegativeInfinite(UFirst))
  {
    UFirst = ULast - 200.;
  }
  else if (Precision::IsPositiveInfinite(ULast))
  {
    ULast = UFirst + 200.;
  }
  Standard_Real deltau = (ULast - UFirst) / 20.;
  for (Standard_Integer i = 1; i <= 21; i++)
  {
    Standard_Real prm = UFirst + (i - 1) * deltau;
    myBasisCurve->D1(prm, P, D1u);
    newZ = D1u.Normalized().Crossed(myDirection);
    if (newZ.Magnitude() > 1.e-12)
      break;
  }
  gp_Ax3 Ax3(P, gp_Dir(newZ), gp_Dir(D1u));
  if (myDirection.Dot(Ax3.YDirection()) < 0.)
  {
    Ax3.YReverse();
  }
  return gp_Pln(Ax3);
}

//=================================================================================================

gp_Cylinder GeomAdaptor_SurfaceOfLinearExtrusion::Cylinder() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Cylinder,
                                 "GeomAdaptor_SurfaceOfLinearExtrusion::Cylinder");

  gp_Circ C = myBasisCurve->Circle();
  gp_Ax3  Ax3(C.Position());
  if (myDirection.Dot((C.Axis()).Direction()) < 0.)
  {
    Ax3.ZReverse();
  }
  return gp_Cylinder(Ax3, C.Radius());
}

//=================================================================================================

gp_Cone GeomAdaptor_SurfaceOfLinearExtrusion::Cone() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::Cone");
}

//=================================================================================================

gp_Sphere GeomAdaptor_SurfaceOfLinearExtrusion::Sphere() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::Sphere");
}

//=================================================================================================

gp_Torus GeomAdaptor_SurfaceOfLinearExtrusion::Torus() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::Torus");
}

//=================================================================================================

gp_Ax1 GeomAdaptor_SurfaceOfLinearExtrusion::AxeOfRevolution() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::Axes");
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfLinearExtrusion::UDegree() const
{
  return myBasisCurve->Degree();
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfLinearExtrusion::NbUPoles() const
{
  return myBasisCurve->NbPoles();
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsURational() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::IsURational");
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfLinearExtrusion::IsVRational() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::IsVRational");
}

//=================================================================================================

Handle(Geom_BezierSurface) GeomAdaptor_SurfaceOfLinearExtrusion::Bezier() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::Bezier");
}

//=================================================================================================

Handle(Geom_BSplineSurface) GeomAdaptor_SurfaceOfLinearExtrusion::BSpline() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfLinearExtrusion::BSpline");
}

//=================================================================================================

gp_Dir GeomAdaptor_SurfaceOfLinearExtrusion::Direction() const
{
  return myDirection;
}

//=================================================================================================

Handle(GeomAdaptor_Curve) GeomAdaptor_SurfaceOfLinearExtrusion::BasisCurve() const
{
  return myBasisCurve;
}

//=================================================================================================

gp_Pnt GeomAdaptor_SurfaceOfLinearExtrusion::Value(const Standard_Real U, const Standard_Real V) const
{
  gp_Pnt aP;
  D0(U, V, aP);
  return aP;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::D0(const Standard_Real U,
                                              const Standard_Real V,
                                              gp_Pnt&             P) const
{
  // S(u,v) = C(u) + v * Direction
  myBasisCurve->D0(U, P);
  P.SetXYZ(P.XYZ() + V * myDirection.XYZ());
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::D1(const Standard_Real U,
                                              const Standard_Real V,
                                              gp_Pnt&             P,
                                              gp_Vec&             D1U,
                                              gp_Vec&             D1V) const
{
  // S(u,v) = C(u) + v * Direction
  // D1U = C'(u)
  // D1V = Direction
  myBasisCurve->D1(U, P, D1U);
  P.SetXYZ(P.XYZ() + V * myDirection.XYZ());
  D1V.SetXYZ(myDirection.XYZ());
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::D2(const Standard_Real U,
                                              const Standard_Real V,
                                              gp_Pnt&             P,
                                              gp_Vec&             D1U,
                                              gp_Vec&             D1V,
                                              gp_Vec&             D2U,
                                              gp_Vec&             D2V,
                                              gp_Vec&             D2UV) const
{
  // S(u,v) = C(u) + v * Direction
  // D2U = C''(u), D2V = 0, D2UV = 0
  myBasisCurve->D2(U, P, D1U, D2U);
  P.SetXYZ(P.XYZ() + V * myDirection.XYZ());
  D1V.SetXYZ(myDirection.XYZ());
  D2V  = gp_Vec(0., 0., 0.);
  D2UV = gp_Vec(0., 0., 0.);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfLinearExtrusion::D3(const Standard_Real U,
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
                                              gp_Vec&             D3UVV) const
{
  // S(u,v) = C(u) + v * Direction
  // D3U = C'''(u), all other third derivatives are 0
  myBasisCurve->D3(U, P, D1U, D2U, D3U);
  P.SetXYZ(P.XYZ() + V * myDirection.XYZ());
  D1V   = gp_Vec(myDirection.XYZ());
  D2V   = gp_Vec(0., 0., 0.);
  D2UV  = gp_Vec(0., 0., 0.);
  D3V   = gp_Vec(0., 0., 0.);
  D3UUV = gp_Vec(0., 0., 0.);
  D3UVV = gp_Vec(0., 0., 0.);
}

//=================================================================================================

gp_Vec GeomAdaptor_SurfaceOfLinearExtrusion::DN(const Standard_Real    U,
                                                const Standard_Real    V,
                                                const Standard_Integer Nu,
                                                const Standard_Integer Nv) const
{
  (void)V; // V parameter doesn't affect derivatives
  if (Nu == 0 && Nv == 1)
  {
    return gp_Vec(myDirection);
  }
  else if (Nv >= 2)
  {
    // All derivatives of order >= 2 in V direction are 0
    return gp_Vec(0., 0., 0.);
  }
  else if (Nv == 1)
  {
    // Mixed derivatives with Nv=1 and Nu>=1 are 0
    return gp_Vec(0., 0., 0.);
  }
  else
  {
    // Nv == 0, pure U derivatives
    return myBasisCurve->DN(U, Nu);
  }
}
