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

#include <GeomAdaptor_SurfaceOfRevolution.hxx>

#include <GeomAdaptor_Curve.hxx>
#include <ElCLib.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>

#include <cmath>

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_SurfaceOfRevolution, GeomAdaptor_Surface)

//=================================================================================================

GeomAdaptor_SurfaceOfRevolution::GeomAdaptor_SurfaceOfRevolution()
    : myHaveAxis(Standard_False)
{
}

//=================================================================================================

GeomAdaptor_SurfaceOfRevolution::GeomAdaptor_SurfaceOfRevolution(const Handle(GeomAdaptor_Curve)& C)
    : myHaveAxis(Standard_False)
{
  Load(C);
}

//=================================================================================================

GeomAdaptor_SurfaceOfRevolution::GeomAdaptor_SurfaceOfRevolution(const Handle(GeomAdaptor_Curve)& C,
                                                                 const gp_Ax1&                  V)
    : myHaveAxis(Standard_False)
{
  Load(C);
  Load(V);
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfRevolution::ShallowCopy() const
{
  Handle(GeomAdaptor_SurfaceOfRevolution) aCopy = new GeomAdaptor_SurfaceOfRevolution();

  if (!myBasisCurve.IsNull())
  {
    aCopy->myBasisCurve = myBasisCurve->ShallowCopy();
  }
  aCopy->myAxis     = myAxis;
  aCopy->myHaveAxis = myHaveAxis;
  aCopy->myAxeRev   = myAxeRev;

  // Copy core (which handles surface data internally)
  aCopy->Core() = Core();

  return aCopy;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::Load(const Handle(GeomAdaptor_Curve)& C)
{
  myBasisCurve = C;
  if (myHaveAxis)
    Load(myAxis); // to evaluate the new myAxeRev.
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::Load(const gp_Ax1& V)
{
  myHaveAxis = Standard_True;
  myAxis     = V;

  // Eval myAxeRev : axe of revolution ( Determination de Ox).
  gp_Pnt           P, Q;
  gp_Pnt           O = myAxis.Location();
  gp_Dir           Ox;
  gp_Dir           Oz   = myAxis.Direction();
  Standard_Boolean yrev = Standard_False;
  if (myBasisCurve->GetType() == GeomAbs_Line)
  {
    if ((myBasisCurve->Line().Direction()).Dot(Oz) < 0.)
    {
      yrev = Standard_True;
      Oz.Reverse();
    }
  }

  if (myBasisCurve->GetType() == GeomAbs_Circle)
  {
    Q = P = (myBasisCurve->Circle()).Location();
  }
  else
  {
    Standard_Real First = myBasisCurve->FirstParameter();
    P                   = Value(0., 0.); // ce qui ne veut pas dire grand chose
    if (GetType() == GeomAbs_Cone)
    {
      if (gp_Lin(myAxis).Distance(P) <= Precision::Confusion())
        Q = ElCLib::Value(1., myBasisCurve->Line());
      else
        Q = P;
    }
    else if (Precision::IsInfinite(First))
      Q = P;
    else
      Q = Value(0., First);
  }

  gp_Dir DZ = myAxis.Direction();
  O.SetXYZ(O.XYZ() + (gp_Vec(O, P) * DZ) * DZ.XYZ());
  if (gp_Lin(myAxis).Distance(Q) > Precision::Confusion())
  {
    Ox = gp_Dir(Q.XYZ() - O.XYZ());
  }
  else
  {
    Standard_Real    First = myBasisCurve->FirstParameter();
    Standard_Real    Last  = myBasisCurve->LastParameter();
    Standard_Integer Ratio = 1;
    Standard_Real    Dist;
    gp_Pnt           PP;
    do
    {
      PP   = myBasisCurve->Value(First + (Last - First) / Ratio);
      Dist = gp_Lin(myAxis).Distance(PP);
      Ratio++;
    } while (Dist < Precision::Confusion() && Ratio < 100);

    if (Ratio >= 100)
    {
      throw Standard_ConstructionError(
        "GeomAdaptor_SurfaceOfRevolution : Axe and meridian are confused");
    }
    Ox = ((Oz ^ gp_Vec(PP.XYZ() - O.XYZ())) ^ Oz);
  }

  myAxeRev = gp_Ax3(O, Oz, Ox);

  if (yrev)
  {
    myAxeRev.YReverse();
  }
  else if (myBasisCurve->GetType() == GeomAbs_Circle)
  {
    gp_Dir DC = (myBasisCurve->Circle()).Axis().Direction();
    if ((Ox.Crossed(Oz)).Dot(DC) < 0.)
      myAxeRev.ZReverse();
  }
}

//=================================================================================================

gp_Ax1 GeomAdaptor_SurfaceOfRevolution::AxeOfRevolution() const
{
  return myAxis;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::FirstUParameter() const
{
  return 0.;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::LastUParameter() const
{
  return 2 * M_PI;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::FirstVParameter() const
{
  return myBasisCurve->FirstParameter();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::LastVParameter() const
{
  return myBasisCurve->LastParameter();
}

//=================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceOfRevolution::UContinuity() const
{
  return GeomAbs_CN;
}

//=================================================================================================

GeomAbs_Shape GeomAdaptor_SurfaceOfRevolution::VContinuity() const
{
  return myBasisCurve->Continuity();
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfRevolution::NbUIntervals(const GeomAbs_Shape) const
{
  return 1;
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfRevolution::NbVIntervals(const GeomAbs_Shape S) const
{
  return myBasisCurve->NbIntervals(S);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::UIntervals(TColStd_Array1OfReal& T, const GeomAbs_Shape) const
{
  T(T.Lower())     = 0.;
  T(T.Lower() + 1) = 2 * M_PI;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::VIntervals(TColStd_Array1OfReal& T,
                                                 const GeomAbs_Shape   S) const
{
  myBasisCurve->Intervals(T, S);
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfRevolution::UTrim(const Standard_Real First,
                                                                 const Standard_Real Last,
                                                                 const Standard_Real Tol) const
{
  constexpr Standard_Real Eps = Precision::PConfusion();
  (void)Eps;
  (void)First;
  (void)Last;
  (void)Tol;
  Standard_OutOfRange_Raise_if(std::abs(First) > Eps || std::abs(Last - 2. * M_PI) > Eps,
                               "GeomAdaptor_SurfaceOfRevolution : UTrim : Parameters out of range");

  Handle(GeomAdaptor_SurfaceOfRevolution) HR =
    new GeomAdaptor_SurfaceOfRevolution(GeomAdaptor_SurfaceOfRevolution(myBasisCurve, myAxis));
  return HR;
}

//=================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_SurfaceOfRevolution::VTrim(const Standard_Real First,
                                                                 const Standard_Real Last,
                                                                 const Standard_Real Tol) const
{
  Handle(GeomAdaptor_Curve)                 HC = BasisCurve()->Trim(First, Last, Tol);
  Handle(GeomAdaptor_SurfaceOfRevolution) HR =
    new GeomAdaptor_SurfaceOfRevolution(GeomAdaptor_SurfaceOfRevolution(HC, myAxis));
  return HR;
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsUClosed() const
{
  return Standard_True;
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsVClosed() const
{
  return myBasisCurve->IsClosed();
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsUPeriodic() const
{
  return Standard_True;
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::UPeriod() const
{
  return 2 * M_PI;
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsVPeriodic() const
{
  return myBasisCurve->IsPeriodic();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::VPeriod() const
{
  return myBasisCurve->Period();
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::UResolution(const Standard_Real R3d) const
{
  return Precision::Parametric(R3d);
}

//=================================================================================================

Standard_Real GeomAdaptor_SurfaceOfRevolution::VResolution(const Standard_Real R3d) const
{
  return myBasisCurve->Resolution(R3d);
}

//=================================================================================================

GeomAbs_SurfaceType GeomAdaptor_SurfaceOfRevolution::GetType() const
{
  constexpr Standard_Real TolConf        = Precision::Confusion();
  constexpr Standard_Real TolAng         = Precision::Angular();
  constexpr Standard_Real TolConeSemiAng = Precision::Confusion();

  switch (myBasisCurve->GetType())
  {
    case GeomAbs_Line: {
      gp_Ax1 Axe = myBasisCurve->Line().Position();

      if (myAxis.IsParallel(Axe, TolAng))
      {
        gp_Pnt        P = Value(0., 0.);
        Standard_Real R = gp_Vec(myAxeRev.Location(), P) * myAxeRev.XDirection();
        if (R > TolConf)
        {
          return GeomAbs_Cylinder;
        }
      }
      else if (myAxis.IsNormal(Axe, TolAng))
        return GeomAbs_Plane;
      else
      {
        Standard_Real    uf     = myBasisCurve->FirstParameter();
        Standard_Real    ul     = myBasisCurve->LastParameter();
        Standard_Boolean istrim = (!Precision::IsInfinite(uf) && !Precision::IsInfinite(ul));
        if (istrim)
        {
          gp_Pnt        pf  = myBasisCurve->Value(uf);
          gp_Pnt        pl  = myBasisCurve->Value(ul);
          Standard_Real len = pf.Distance(pl);
          // on calcule la distance projetee sur l axe.
          gp_Vec        vlin(pf, pl);
          gp_Vec        vaxe(myAxis.Direction());
          Standard_Real projlen = std::abs(vaxe.Dot(vlin));
          if ((len - projlen) <= TolConf)
          {
            gp_Pnt        P = Value(0., 0.);
            Standard_Real R = gp_Vec(myAxeRev.Location(), P) * myAxeRev.XDirection();
            if (R > TolConf)
            {
              return GeomAbs_Cylinder;
            }
          }
          else if (projlen <= TolConf)
            return GeomAbs_Plane;
        }
        gp_Vec        V(myAxis.Location(), myBasisCurve->Line().Location());
        gp_Vec        W(Axe.Direction());
        gp_Vec        AxisDir(myAxis.Direction());
        Standard_Real proj = std::abs(W.Dot(AxisDir));
        if (std::abs(V.DotCross(AxisDir, W)) <= TolConf
            && (proj >= TolConeSemiAng && proj <= 1. - TolConeSemiAng))
        {
          return GeomAbs_Cone;
        }
      }
      break;
    } // case GeomAbs_Line:
    //
    case GeomAbs_Circle: {
      Standard_Real MajorRadius, aR;
      gp_Lin        aLin(myAxis);
      //
      gp_Circ       C   = myBasisCurve->Circle();
      const gp_Pnt& aLC = C.Location();
      aR                = C.Radius();
      //

      if (!C.Position().IsCoplanar(myAxis, TolConf, TolAng))
        return GeomAbs_SurfaceOfRevolution;
      else if (aLin.Distance(aLC) <= TolConf)
        return GeomAbs_Sphere;
      else
      {
        MajorRadius = aLin.Distance(aLC);
        if (MajorRadius > aR)
        {
          return GeomAbs_Torus;
        }
      }
      break;
    }
    //
    default:
      break;
  }

  return GeomAbs_SurfaceOfRevolution;
}

//=================================================================================================

gp_Pln GeomAdaptor_SurfaceOfRevolution::Plane() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Plane,
                                 "GeomAdaptor_SurfaceOfRevolution:Plane");

  gp_Ax3        Axe       = myAxeRev;
  gp_Pnt        aPonCurve = Value(0., 0.);
  Standard_Real aDot = (aPonCurve.XYZ() - myAxis.Location().XYZ()).Dot(myAxis.Direction().XYZ());

  gp_Pnt P(myAxis.Location().XYZ() + aDot * myAxis.Direction().XYZ());
  Axe.SetLocation(P);
  if (Axe.XDirection().Dot(myBasisCurve->Line().Direction()) >= -Precision::Confusion())
    Axe.XReverse();

  return gp_Pln(Axe);
}

//=================================================================================================

gp_Cylinder GeomAdaptor_SurfaceOfRevolution::Cylinder() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Cylinder,
                                 "GeomAdaptor_SurfaceOfRevolution::Cylinder");

  gp_Pnt        P = Value(0., 0.);
  Standard_Real R = gp_Vec(myAxeRev.Location(), P) * myAxeRev.XDirection();
  return gp_Cylinder(myAxeRev, R);
}

//=================================================================================================

gp_Cone GeomAdaptor_SurfaceOfRevolution::Cone() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Cone, "GeomAdaptor_SurfaceOfRevolution:Cone");

  gp_Ax3        Axe   = myAxeRev;
  gp_Dir        ldir  = (myBasisCurve->Line()).Direction();
  Standard_Real Angle = (Axe.Direction()).Angle(ldir);
  gp_Pnt        P0    = Value(0., 0.);
  Standard_Real R     = (Axe.Location()).Distance(P0);
  if (R >= Precision::Confusion())
  {
    gp_Pnt        O = Axe.Location();
    gp_Vec        OP0(O, P0);
    Standard_Real t = OP0.Dot(Axe.XDirection());
    t /= ldir.Dot(Axe.XDirection());
    OP0.Add(-t * gp_Vec(ldir));
    if (OP0.Dot(Axe.Direction()) > 0.)
      Angle = -Angle;
  }
  return gp_Cone(Axe, Angle, R);
}

//=================================================================================================

gp_Sphere GeomAdaptor_SurfaceOfRevolution::Sphere() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Sphere,
                                 "GeomAdaptor_SurfaceOfRevolution:Sphere");

  gp_Circ C   = myBasisCurve->Circle();
  gp_Ax3  Axe = myAxeRev;
  Axe.SetLocation(C.Location());
  return gp_Sphere(Axe, C.Radius());
}

//=================================================================================================

gp_Torus GeomAdaptor_SurfaceOfRevolution::Torus() const
{
  Standard_NoSuchObject_Raise_if(GetType() != GeomAbs_Torus,
                                 "GeomAdaptor_SurfaceOfRevolution:Torus");

  gp_Circ       C           = myBasisCurve->Circle();
  Standard_Real MajorRadius = gp_Lin(myAxis).Distance(C.Location());
  return gp_Torus(myAxeRev, MajorRadius, C.Radius());
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfRevolution::VDegree() const
{
  return myBasisCurve->Degree();
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfRevolution::NbVPoles() const
{
  return myBasisCurve->NbPoles();
}

//=================================================================================================

Standard_Integer GeomAdaptor_SurfaceOfRevolution::NbVKnots() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfRevolution::NbVKnots");
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsURational() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfRevolution::IsURational");
}

//=================================================================================================

Standard_Boolean GeomAdaptor_SurfaceOfRevolution::IsVRational() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfRevolution::IsVRational");
}

//=================================================================================================

Handle(Geom_BezierSurface) GeomAdaptor_SurfaceOfRevolution::Bezier() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfRevolution::Bezier");
}

//=================================================================================================

Handle(Geom_BSplineSurface) GeomAdaptor_SurfaceOfRevolution::BSpline() const
{
  throw Standard_NoSuchObject("GeomAdaptor_SurfaceOfRevolution::BSpline");
}

//=================================================================================================

const gp_Ax3& GeomAdaptor_SurfaceOfRevolution::Axis() const
{
  return myAxeRev;
}

//=================================================================================================

Handle(GeomAdaptor_Curve) GeomAdaptor_SurfaceOfRevolution::BasisCurve() const
{
  return myBasisCurve;
}

//=================================================================================================

gp_Pnt GeomAdaptor_SurfaceOfRevolution::Value(const Standard_Real U, const Standard_Real V) const
{
  gp_Pnt aP;
  D0(U, V, aP);
  return aP;
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::D0(const Standard_Real U,
                                         const Standard_Real V,
                                         gp_Pnt&             P) const
{
  // Get point on curve
  gp_Pnt aPOnCurve = myBasisCurve->Value(V);

  // Rotate around axis
  const gp_XYZ& aO  = myAxeRev.Location().XYZ();
  const gp_XYZ& aX  = myAxeRev.XDirection().XYZ();
  const gp_XYZ& aY  = myAxeRev.YDirection().XYZ();
  const gp_XYZ& aZ  = myAxeRev.Direction().XYZ();
  gp_XYZ        aV  = aPOnCurve.XYZ() - aO;
  double        aXr = aV.Dot(aX);
  double        aYr = aV.Dot(aY);
  double        aZr = aV.Dot(aZ);
  double        aCos = std::cos(U);
  double        aSin = std::sin(U);
  double        aXn  = aXr * aCos - aYr * aSin;
  double        aYn  = aXr * aSin + aYr * aCos;
  P.SetXYZ(aO + aXn * aX + aYn * aY + aZr * aZ);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::D1(const Standard_Real U,
                                         const Standard_Real V,
                                         gp_Pnt&             P,
                                         gp_Vec&             D1U,
                                         gp_Vec&             D1V) const
{
  // Get point and first derivative on curve
  gp_Pnt aPOnCurve;
  gp_Vec aD1Curve;
  myBasisCurve->D1(V, aPOnCurve, aD1Curve);

  const gp_XYZ& aO  = myAxeRev.Location().XYZ();
  const gp_XYZ& aX  = myAxeRev.XDirection().XYZ();
  const gp_XYZ& aY  = myAxeRev.YDirection().XYZ();
  const gp_XYZ& aZ  = myAxeRev.Direction().XYZ();
  gp_XYZ        aRel = aPOnCurve.XYZ() - aO;
  double        aXr = aRel.Dot(aX);
  double        aYr = aRel.Dot(aY);
  double        aZr = aRel.Dot(aZ);
  double        aCos = std::cos(U);
  double        aSin = std::sin(U);
  double        aXn  = aXr * aCos - aYr * aSin;
  double        aYn  = aXr * aSin + aYr * aCos;
  P.SetXYZ(aO + aXn * aX + aYn * aY + aZr * aZ);

  // D1U = dS/du: derivative of rotation
  // dXn/du = -Xr*sin - Yr*cos, dYn/du = Xr*cos - Yr*sin
  double aD1UX = -aXr * aSin - aYr * aCos;
  double aD1UY = aXr * aCos - aYr * aSin;
  D1U.SetXYZ(aD1UX * aX + aD1UY * aY);

  // D1V = dS/dv: derivative through curve
  double aDXr = aD1Curve.XYZ().Dot(aX);
  double aDYr = aD1Curve.XYZ().Dot(aY);
  double aDZr = aD1Curve.XYZ().Dot(aZ);
  double aD1VX = aDXr * aCos - aDYr * aSin;
  double aD1VY = aDXr * aSin + aDYr * aCos;
  D1V.SetXYZ(aD1VX * aX + aD1VY * aY + aDZr * aZ);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::D2(const Standard_Real U,
                                         const Standard_Real V,
                                         gp_Pnt&             P,
                                         gp_Vec&             D1U,
                                         gp_Vec&             D1V,
                                         gp_Vec&             D2U,
                                         gp_Vec&             D2V,
                                         gp_Vec&             D2UV) const
{
  // Get point and first two derivatives on curve
  gp_Pnt aPOnCurve;
  gp_Vec aD1Curve, aD2Curve;
  myBasisCurve->D2(V, aPOnCurve, aD1Curve, aD2Curve);

  const gp_XYZ& aO  = myAxeRev.Location().XYZ();
  const gp_XYZ& aX  = myAxeRev.XDirection().XYZ();
  const gp_XYZ& aY  = myAxeRev.YDirection().XYZ();
  const gp_XYZ& aZ  = myAxeRev.Direction().XYZ();
  gp_XYZ        aRel = aPOnCurve.XYZ() - aO;
  double        aXr = aRel.Dot(aX);
  double        aYr = aRel.Dot(aY);
  double        aZr = aRel.Dot(aZ);
  double        aCos = std::cos(U);
  double        aSin = std::sin(U);
  double        aXn  = aXr * aCos - aYr * aSin;
  double        aYn  = aXr * aSin + aYr * aCos;
  P.SetXYZ(aO + aXn * aX + aYn * aY + aZr * aZ);

  // D1U
  double aD1UX = -aXr * aSin - aYr * aCos;
  double aD1UY = aXr * aCos - aYr * aSin;
  D1U.SetXYZ(aD1UX * aX + aD1UY * aY);

  // D1V
  double aDXr = aD1Curve.XYZ().Dot(aX);
  double aDYr = aD1Curve.XYZ().Dot(aY);
  double aDZr = aD1Curve.XYZ().Dot(aZ);
  double aD1VX = aDXr * aCos - aDYr * aSin;
  double aD1VY = aDXr * aSin + aDYr * aCos;
  D1V.SetXYZ(aD1VX * aX + aD1VY * aY + aDZr * aZ);

  // D2U = d2S/du2 = -Xn*X - Yn*Y = -aXn*X - aYn*Y
  D2U.SetXYZ(-aXn * aX - aYn * aY);

  // D2V = d2S/dv2: second derivative through curve
  double aD2Xr = aD2Curve.XYZ().Dot(aX);
  double aD2Yr = aD2Curve.XYZ().Dot(aY);
  double aD2Zr = aD2Curve.XYZ().Dot(aZ);
  double aD2VX = aD2Xr * aCos - aD2Yr * aSin;
  double aD2VY = aD2Xr * aSin + aD2Yr * aCos;
  D2V.SetXYZ(aD2VX * aX + aD2VY * aY + aD2Zr * aZ);

  // D2UV = d2S/dudv
  double aD2UVX = -aDXr * aSin - aDYr * aCos;
  double aD2UVY = aDXr * aCos - aDYr * aSin;
  D2UV.SetXYZ(aD2UVX * aX + aD2UVY * aY);
}

//=================================================================================================

void GeomAdaptor_SurfaceOfRevolution::D3(const Standard_Real U,
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
  // Get point and first three derivatives on curve
  gp_Pnt aPOnCurve;
  gp_Vec aD1Curve, aD2Curve, aD3Curve;
  myBasisCurve->D3(V, aPOnCurve, aD1Curve, aD2Curve, aD3Curve);

  const gp_XYZ& aO  = myAxeRev.Location().XYZ();
  const gp_XYZ& aX  = myAxeRev.XDirection().XYZ();
  const gp_XYZ& aY  = myAxeRev.YDirection().XYZ();
  const gp_XYZ& aZ  = myAxeRev.Direction().XYZ();
  gp_XYZ        aRel = aPOnCurve.XYZ() - aO;
  double        aXr = aRel.Dot(aX);
  double        aYr = aRel.Dot(aY);
  double        aZr = aRel.Dot(aZ);
  double        aCos = std::cos(U);
  double        aSin = std::sin(U);
  double        aXn  = aXr * aCos - aYr * aSin;
  double        aYn  = aXr * aSin + aYr * aCos;
  P.SetXYZ(aO + aXn * aX + aYn * aY + aZr * aZ);

  // D1U
  double aD1UX = -aXr * aSin - aYr * aCos;
  double aD1UY = aXr * aCos - aYr * aSin;
  D1U.SetXYZ(aD1UX * aX + aD1UY * aY);

  // D1V
  double aDXr = aD1Curve.XYZ().Dot(aX);
  double aDYr = aD1Curve.XYZ().Dot(aY);
  double aDZr = aD1Curve.XYZ().Dot(aZ);
  double aD1VX = aDXr * aCos - aDYr * aSin;
  double aD1VY = aDXr * aSin + aDYr * aCos;
  D1V.SetXYZ(aD1VX * aX + aD1VY * aY + aDZr * aZ);

  // D2U
  D2U.SetXYZ(-aXn * aX - aYn * aY);

  // D2V
  double aD2Xr = aD2Curve.XYZ().Dot(aX);
  double aD2Yr = aD2Curve.XYZ().Dot(aY);
  double aD2Zr = aD2Curve.XYZ().Dot(aZ);
  double aD2VX = aD2Xr * aCos - aD2Yr * aSin;
  double aD2VY = aD2Xr * aSin + aD2Yr * aCos;
  D2V.SetXYZ(aD2VX * aX + aD2VY * aY + aD2Zr * aZ);

  // D2UV
  double aD2UVX = -aDXr * aSin - aDYr * aCos;
  double aD2UVY = aDXr * aCos - aDYr * aSin;
  D2UV.SetXYZ(aD2UVX * aX + aD2UVY * aY);

  // D3U = d3S/du3 = -D1U
  D3U.SetXYZ(-aD1UX * aX - aD1UY * aY);

  // D3V
  double aD3Xr = aD3Curve.XYZ().Dot(aX);
  double aD3Yr = aD3Curve.XYZ().Dot(aY);
  double aD3Zr = aD3Curve.XYZ().Dot(aZ);
  double aD3VX = aD3Xr * aCos - aD3Yr * aSin;
  double aD3VY = aD3Xr * aSin + aD3Yr * aCos;
  D3V.SetXYZ(aD3VX * aX + aD3VY * aY + aD3Zr * aZ);

  // D3UUV = d3S/du2dv = -D1V (XY part only)
  D3UUV.SetXYZ(-aD1VX * aX - aD1VY * aY);

  // D3UVV = d3S/dudv2
  double aD3UVVX = -aD2Xr * aSin - aD2Yr * aCos;
  double aD3UVVY = aD2Xr * aCos - aD2Yr * aSin;
  D3UVV.SetXYZ(aD3UVVX * aX + aD3UVVY * aY);
}

//=================================================================================================

gp_Vec GeomAdaptor_SurfaceOfRevolution::DN(const Standard_Real    U,
                                           const Standard_Real    V,
                                           const Standard_Integer Nu,
                                           const Standard_Integer Nv) const
{
  // General DN is complex for revolution surfaces.
  // For now, handle common cases and use finite differences for others.
  if (Nu + Nv <= 3)
  {
    gp_Pnt aP;
    gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV;
    if (Nu + Nv == 1)
    {
      D1(U, V, aP, aD1U, aD1V);
      return (Nu == 1) ? aD1U : aD1V;
    }
    else if (Nu + Nv == 2)
    {
      D2(U, V, aP, aD1U, aD1V, aD2U, aD2V, aD2UV);
      if (Nu == 2) return aD2U;
      if (Nv == 2) return aD2V;
      return aD2UV;
    }
    else // Nu + Nv == 3
    {
      D3(U, V, aP, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV);
      if (Nu == 3) return aD3U;
      if (Nv == 3) return aD3V;
      if (Nu == 2) return aD3UUV;
      return aD3UVV;
    }
  }

  // For higher orders, throw (or could implement using finite differences)
  throw Standard_NotImplemented("GeomAdaptor_SurfaceOfRevolution::DN for high orders");
}
