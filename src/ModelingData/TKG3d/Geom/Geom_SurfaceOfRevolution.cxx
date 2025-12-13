// Created on: 1993-03-10
// Created by: JCV
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

#include <BSplCLib.hxx>
#include <BSplSLib.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Geometry.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_UndefinedDerivative.hxx>
#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Dir.hxx>
#include <gp_GTrsf2d.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>
#include <Precision.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_RangeError.hxx>
#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom_SurfaceOfRevolution, Geom_SweptSurface)

#define POLES (poles->Array2())
#define WEIGHTS (weights->Array2())
#define UKNOTS (uknots->Array1())
#define VKNOTS (vknots->Array1())
#define UFKNOTS (ufknots->Array1())
#define VFKNOTS (vfknots->Array1())
#define FMULTS (BSplCLib::NoMults())

typedef Geom_SurfaceOfRevolution SurfaceOfRevolution;
typedef Geom_Curve               Curve;
typedef gp_Ax1                   Ax1;
typedef gp_Ax2                   Ax2;
typedef gp_Dir                   Dir;
typedef gp_Lin                   Lin;
typedef gp_Pnt                   Pnt;
typedef gp_Trsf                  Trsf;
typedef gp_Vec                   Vec;
typedef gp_XYZ                   XYZ;

//=================================================================================================

Handle(Geom_Geometry) Geom_SurfaceOfRevolution::Copy() const
{

  return new Geom_SurfaceOfRevolution(basisCurve, Axis());
}

//=================================================================================================

Geom_SurfaceOfRevolution::Geom_SurfaceOfRevolution(const Handle(Geom_Curve)& C, const Ax1& A1)
    : loc(A1.Location())
{

  direction = A1.Direction();
  SetBasisCurve(C);
}

//=================================================================================================

void Geom_SurfaceOfRevolution::UReverse()
{
  direction.Reverse();
}

//=================================================================================================

Standard_Real Geom_SurfaceOfRevolution::UReversedParameter(const Standard_Real U) const
{

  return (2. * M_PI - U);
}

//=================================================================================================

void Geom_SurfaceOfRevolution::VReverse()
{

  basisCurve->Reverse();
}

//=================================================================================================

Standard_Real Geom_SurfaceOfRevolution::VReversedParameter(const Standard_Real V) const
{

  return basisCurve->ReversedParameter(V);
}

//=================================================================================================

const gp_Pnt& Geom_SurfaceOfRevolution::Location() const
{

  return loc;
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsUPeriodic() const
{

  return Standard_True;
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsCNu(const Standard_Integer) const
{

  return Standard_True;
}

//=================================================================================================

Ax1 Geom_SurfaceOfRevolution::Axis() const
{

  return Ax1(loc, direction);
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsCNv(const Standard_Integer N) const
{

  Standard_RangeError_Raise_if(N < 0, " ");
  return basisCurve->IsCN(N);
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsUClosed() const
{

  return Standard_True;
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsVClosed() const
{
  return basisCurve->IsClosed();
}

//=================================================================================================

Standard_Boolean Geom_SurfaceOfRevolution::IsVPeriodic() const
{

  return basisCurve->IsPeriodic();
}

//=================================================================================================

void Geom_SurfaceOfRevolution::SetAxis(const Ax1& A1)
{
  direction = A1.Direction();
  loc       = A1.Location();
}

//=================================================================================================

void Geom_SurfaceOfRevolution::SetDirection(const Dir& V)
{
  direction = V;
}

//=================================================================================================

void Geom_SurfaceOfRevolution::SetBasisCurve(const Handle(Geom_Curve)& C)
{
  basisCurve = Handle(Geom_Curve)::DownCast(C->Copy());
  smooth     = C->Continuity();
}

//=================================================================================================

void Geom_SurfaceOfRevolution::SetLocation(const Pnt& P)
{
  loc = P;
}

//=================================================================================================

void Geom_SurfaceOfRevolution::Bounds(Standard_Real& U1,
                                      Standard_Real& U2,
                                      Standard_Real& V1,
                                      Standard_Real& V2) const
{

  U1 = 0.0;
  U2 = 2.0 * M_PI;
  V1 = basisCurve->FirstParameter();
  V2 = basisCurve->LastParameter();
}

//=================================================================================================

void Geom_SurfaceOfRevolution::D0(const Standard_Real U, const Standard_Real V, Pnt& P) const
{
  basisCurve->D0(V, P);

  gp_Trsf aRotation;
  aRotation.SetRotation(Ax1(loc, direction), U);
  P.Transform(aRotation);
}

//=================================================================================================

void Geom_SurfaceOfRevolution::D1(const Standard_Real U,
                                  const Standard_Real V,
                                  Pnt&                P,
                                  Vec&                D1U,
                                  Vec&                D1V) const
{
  basisCurve->D1(V, P, D1V);

  // Vector from center of rotation to the point on rotated curve
  gp_XYZ aCQ = P.XYZ() - loc.XYZ();
  D1U        = Vec(direction.XYZ().Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (D1U.SquareMagnitude() < Precision::SquareConfusion())
    D1U.SetCoord(0.0, 0.0, 0.0);

  gp_Trsf aRotation;
  aRotation.SetRotation(Ax1(loc, direction), U);
  P.Transform(aRotation);
  D1U.Transform(aRotation);
  D1V.Transform(aRotation);
}

//=================================================================================================

void Geom_SurfaceOfRevolution::D2(const Standard_Real U,
                                  const Standard_Real V,
                                  Pnt&                P,
                                  Vec&                D1U,
                                  Vec&                D1V,
                                  Vec&                D2U,
                                  Vec&                D2V,
                                  Vec&                D2UV) const
{
  basisCurve->D2(V, P, D1V, D2V);

  // Vector from center of rotation to the point on rotated curve
  gp_XYZ        aCQ  = P.XYZ() - loc.XYZ();
  const gp_XYZ& aDir = direction.XYZ();
  D1U                = Vec(aDir.Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (D1U.SquareMagnitude() < Precision::SquareConfusion())
    D1U.SetCoord(0.0, 0.0, 0.0);
  D2U  = Vec(aDir.Dot(aCQ) * aDir - aCQ);
  D2UV = Vec(aDir.Crossed(D1V.XYZ()));

  gp_Trsf aRotation;
  aRotation.SetRotation(Ax1(loc, direction), U);
  P.Transform(aRotation);
  D1U.Transform(aRotation);
  D1V.Transform(aRotation);
  D2U.Transform(aRotation);
  D2V.Transform(aRotation);
  D2UV.Transform(aRotation);
}

//=================================================================================================

void Geom_SurfaceOfRevolution::D3(const Standard_Real U,
                                  const Standard_Real V,
                                  Pnt&                P,
                                  Vec&                D1U,
                                  Vec&                D1V,
                                  Vec&                D2U,
                                  Vec&                D2V,
                                  Vec&                D2UV,
                                  Vec&                D3U,
                                  Vec&                D3V,
                                  Vec&                D3UUV,
                                  Vec&                D3UVV) const
{
  basisCurve->D3(V, P, D1V, D2V, D3V);

  // Vector from center of rotation to the point on rotated curve
  gp_XYZ        aCQ  = P.XYZ() - loc.XYZ();
  const gp_XYZ& aDir = direction.XYZ();
  D1U                = Vec(aDir.Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (D1U.SquareMagnitude() < Precision::SquareConfusion())
    D1U.SetCoord(0.0, 0.0, 0.0);
  D2U   = Vec(aDir.Dot(aCQ) * aDir - aCQ);
  D2UV  = Vec(aDir.Crossed(D1V.XYZ()));
  D3U   = -D1U;
  D3UUV = Vec(aDir.Dot(D1V.XYZ()) * aDir - D1V.XYZ());
  D3UVV = Vec(aDir.Crossed(D2V.XYZ()));

  gp_Trsf aRotation;
  aRotation.SetRotation(Ax1(loc, direction), U);
  P.Transform(aRotation);
  D1U.Transform(aRotation);
  D1V.Transform(aRotation);
  D2U.Transform(aRotation);
  D2V.Transform(aRotation);
  D2UV.Transform(aRotation);
  D3U.Transform(aRotation);
  D3V.Transform(aRotation);
  D3UUV.Transform(aRotation);
  D3UVV.Transform(aRotation);
}

//=================================================================================================

Vec Geom_SurfaceOfRevolution::DN(const Standard_Real    U,
                                 const Standard_Real    V,
                                 const Standard_Integer Nu,
                                 const Standard_Integer Nv) const
{
  Standard_RangeError_Raise_if(Nu < 0, "Geom_SurfaceOfRevolution::DN(): Nu < 0");
  Standard_RangeError_Raise_if(Nv < 0, "Geom_SurfaceOfRevolution::DN(): Nv < 0");
  Standard_RangeError_Raise_if(Nu + Nv < 1, "Geom_SurfaceOfRevolution::DN(): Nu + Nv < 1");

  gp_Trsf aRotation;
  aRotation.SetRotation(Ax1(loc, direction), U);

  Pnt aP;
  Vec aDV;
  Vec aResult;
  if (Nu == 0)
  {
    aResult = basisCurve->DN(V, Nv);
  }
  else
  {
    if (Nv == 0)
    {
      basisCurve->D0(V, aP);
      aDV = Vec(aP.XYZ() - loc.XYZ());
    }
    else
    {
      aDV = basisCurve->DN(V, Nv);
    }

    const gp_XYZ& aDir = direction.XYZ();
    if (Nu % 4 == 1)
      aResult = Vec(aDir.Crossed(aDV.XYZ()));
    else if (Nu % 4 == 2)
      aResult = Vec(aDir.Dot(aDV.XYZ()) * aDir - aDV.XYZ());
    else if (Nu % 4 == 3)
      aResult = Vec(aDir.Crossed(aDV.XYZ())) * (-1.0);
    else
      aResult = Vec(aDV.XYZ() - aDir.Dot(aDV.XYZ()) * aDir);
  }

  aResult.Transform(aRotation);
  return aResult;
}

//=================================================================================================

Ax2 Geom_SurfaceOfRevolution::ReferencePlane() const
{

  throw Standard_NotImplemented();
}

//=================================================================================================

Handle(Geom_Curve) Geom_SurfaceOfRevolution::UIso(const Standard_Real U) const
{

  Handle(Geom_Curve) C       = Handle(Geom_Curve)::DownCast(basisCurve->Copy());
  Ax1                RotAxis = Ax1(loc, direction);
  C->Rotate(RotAxis, U);
  return C;
}

//=================================================================================================

Handle(Geom_Curve) Geom_SurfaceOfRevolution::VIso(const Standard_Real V) const
{

  Handle(Geom_Circle) Circ;
  Pnt                 Pc = basisCurve->Value(V);
  gp_Lin              L1(loc, direction);
  Standard_Real       Rad = L1.Distance(Pc);

  Ax2 Rep;
  if (Rad > gp::Resolution())
  {
    XYZ P = Pc.XYZ();
    XYZ C;
    C.SetLinearForm((P - loc.XYZ()).Dot(direction.XYZ()), direction.XYZ(), loc.XYZ());
    P = P - C;
    if (P.Modulus() > gp::Resolution())
    {
      gp_Dir D = P.Normalized();
      Rep      = gp_Ax2(C, direction, D);
    }
    else
      Rep = gp_Ax2(C, direction);
  }
  else
    Rep = gp_Ax2(Pc, direction);

  Circ = new Geom_Circle(Rep, Rad);
  return Circ;
}

//=================================================================================================

void Geom_SurfaceOfRevolution::Transform(const Trsf& T)
{
  loc.Transform(T);
  direction.Transform(T);
  basisCurve->Transform(T);
  if (T.ScaleFactor() * T.HVectorialPart().Determinant() < 0.)
    UReverse();
}

//=================================================================================================

void Geom_SurfaceOfRevolution::TransformParameters(Standard_Real&,
                                                   Standard_Real& V,
                                                   const gp_Trsf& T) const
{
  V = basisCurve->TransformedParameter(V, T);
}

//=================================================================================================

gp_GTrsf2d Geom_SurfaceOfRevolution::ParametricTransformation(const gp_Trsf& T) const
{
  gp_GTrsf2d T2;
  gp_Ax2d    Axis(gp::Origin2d(), gp::DX2d());
  T2.SetAffinity(Axis, basisCurve->ParametricTransformation(T));
  return T2;
}

//=================================================================================================

void Geom_SurfaceOfRevolution::DumpJson(Standard_OStream& theOStream,
                                        Standard_Integer  theDepth) const
{
  OCCT_DUMP_TRANSIENT_CLASS_BEGIN(theOStream)
  OCCT_DUMP_BASE_CLASS(theOStream, theDepth, Geom_SweptSurface)

  OCCT_DUMP_FIELD_VALUES_DUMPED(theOStream, theDepth, &loc)
}
