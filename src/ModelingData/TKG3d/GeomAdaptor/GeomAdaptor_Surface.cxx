// Created on: 1993-05-14
// Created by: Joelle CHAUVET
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

#include <GeomAdaptor_Surface.hxx>

#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Standard_NoSuchObject.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_Surface, Standard_Transient)

//==================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_Surface::ShallowCopy() const
{
  Handle(GeomAdaptor_Surface) aCopy = new GeomAdaptor_Surface();
  aCopy->myCore                     = myCore; // Uses copy constructor of Core
  return aCopy;
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_Surface::UContinuity() const
{
  return myCore.UContinuity();
}

//==================================================================================================

GeomAbs_Shape GeomAdaptor_Surface::VContinuity() const
{
  return myCore.VContinuity();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbUIntervals(const GeomAbs_Shape S) const
{
  return myCore.NbUIntervals(S);
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbVIntervals(const GeomAbs_Shape S) const
{
  return myCore.NbVIntervals(S);
}

//==================================================================================================

void GeomAdaptor_Surface::UIntervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const
{
  myCore.UIntervals(T, S);
}

//==================================================================================================

void GeomAdaptor_Surface::VIntervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const
{
  myCore.VIntervals(T, S);
}

//==================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_Surface::UTrim(const Standard_Real First,
                                                       const Standard_Real Last,
                                                       const Standard_Real Tol) const
{
  return new GeomAdaptor_Surface(myCore.Surface(),
                                 First,
                                 Last,
                                 myCore.FirstVParameter(),
                                 myCore.LastVParameter(),
                                 Tol,
                                 0.0);
}

//==================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_Surface::VTrim(const Standard_Real First,
                                                       const Standard_Real Last,
                                                       const Standard_Real Tol) const
{
  return new GeomAdaptor_Surface(myCore.Surface(),
                                 myCore.FirstUParameter(),
                                 myCore.LastUParameter(),
                                 First,
                                 Last,
                                 0.0,
                                 Tol);
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsUClosed() const
{
  return myCore.IsUClosed();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsVClosed() const
{
  return myCore.IsVClosed();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsUPeriodic() const
{
  return myCore.IsUPeriodic();
}

//==================================================================================================

Standard_Real GeomAdaptor_Surface::UPeriod() const
{
  return myCore.UPeriod();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsVPeriodic() const
{
  return myCore.IsVPeriodic();
}

//==================================================================================================

Standard_Real GeomAdaptor_Surface::VPeriod() const
{
  return myCore.VPeriod();
}

//==================================================================================================

gp_Pnt GeomAdaptor_Surface::Value(const Standard_Real U, const Standard_Real V) const
{
  return myCore.Value(U, V);
}

//==================================================================================================

void GeomAdaptor_Surface::D0(const Standard_Real U, const Standard_Real V, gp_Pnt& P) const
{
  myCore.D0(U, V, P);
}

//==================================================================================================

void GeomAdaptor_Surface::D1(const Standard_Real U,
                             const Standard_Real V,
                             gp_Pnt&             P,
                             gp_Vec&             D1U,
                             gp_Vec&             D1V) const
{
  myCore.D1(U, V, P, D1U, D1V);
}

//==================================================================================================

void GeomAdaptor_Surface::D2(const Standard_Real U,
                             const Standard_Real V,
                             gp_Pnt&             P,
                             gp_Vec&             D1U,
                             gp_Vec&             D1V,
                             gp_Vec&             D2U,
                             gp_Vec&             D2V,
                             gp_Vec&             D2UV) const
{
  myCore.D2(U, V, P, D1U, D1V, D2U, D2V, D2UV);
}

//==================================================================================================

void GeomAdaptor_Surface::D3(const Standard_Real U,
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
  myCore.D3(U, V, P, D1U, D1V, D2U, D2V, D2UV, D3U, D3V, D3UUV, D3UVV);
}

//==================================================================================================

gp_Vec GeomAdaptor_Surface::DN(const Standard_Real    U,
                               const Standard_Real    V,
                               const Standard_Integer Nu,
                               const Standard_Integer Nv) const
{
  return myCore.DN(U, V, Nu, Nv);
}

//==================================================================================================

Standard_Real GeomAdaptor_Surface::UResolution(const Standard_Real R3d) const
{
  return myCore.UResolution(R3d);
}

//==================================================================================================

Standard_Real GeomAdaptor_Surface::VResolution(const Standard_Real R3d) const
{
  return myCore.VResolution(R3d);
}

//==================================================================================================

gp_Pln GeomAdaptor_Surface::Plane() const
{
  return myCore.Plane();
}

//==================================================================================================

gp_Cylinder GeomAdaptor_Surface::Cylinder() const
{
  return myCore.Cylinder();
}

//==================================================================================================

gp_Cone GeomAdaptor_Surface::Cone() const
{
  return myCore.Cone();
}

//==================================================================================================

gp_Sphere GeomAdaptor_Surface::Sphere() const
{
  return myCore.Sphere();
}

//==================================================================================================

gp_Torus GeomAdaptor_Surface::Torus() const
{
  return myCore.Torus();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::UDegree() const
{
  return myCore.UDegree();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbUPoles() const
{
  return myCore.NbUPoles();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::VDegree() const
{
  return myCore.VDegree();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbVPoles() const
{
  return myCore.NbVPoles();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbUKnots() const
{
  return myCore.NbUKnots();
}

//==================================================================================================

Standard_Integer GeomAdaptor_Surface::NbVKnots() const
{
  return myCore.NbVKnots();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsURational() const
{
  return myCore.IsURational();
}

//==================================================================================================

Standard_Boolean GeomAdaptor_Surface::IsVRational() const
{
  return myCore.IsVRational();
}

//==================================================================================================

Handle(Geom_BezierSurface) GeomAdaptor_Surface::Bezier() const
{
  return myCore.Bezier();
}

//==================================================================================================

Handle(Geom_BSplineSurface) GeomAdaptor_Surface::BSpline() const
{
  return myCore.BSpline();
}

//==================================================================================================

gp_Ax1 GeomAdaptor_Surface::AxeOfRevolution() const
{
  return myCore.AxeOfRevolution();
}

//==================================================================================================

gp_Dir GeomAdaptor_Surface::Direction() const
{
  return myCore.Direction();
}

//==================================================================================================

Handle(GeomAdaptor_Curve) GeomAdaptor_Surface::BasisCurve() const
{
  // Get basis curve from the underlying surface
  const Handle(Geom_Surface)& aSurf = myCore.Surface();
  if (aSurf.IsNull())
  {
    throw Standard_NoSuchObject("GeomAdaptor_Surface::BasisCurve");
  }

  Handle(Geom_Curve) aCurve;
  if (myCore.GetType() == GeomAbs_SurfaceOfExtrusion)
  {
    Handle(Geom_SurfaceOfLinearExtrusion) anExtSurf =
      Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(aSurf);
    if (!anExtSurf.IsNull())
    {
      aCurve = anExtSurf->BasisCurve();
    }
  }
  else if (myCore.GetType() == GeomAbs_SurfaceOfRevolution)
  {
    Handle(Geom_SurfaceOfRevolution) aRevSurf = Handle(Geom_SurfaceOfRevolution)::DownCast(aSurf);
    if (!aRevSurf.IsNull())
    {
      aCurve = aRevSurf->BasisCurve();
    }
  }

  if (aCurve.IsNull())
  {
    throw Standard_NoSuchObject("GeomAdaptor_Surface::BasisCurve");
  }

  return new GeomAdaptor_Curve(aCurve);
}

//==================================================================================================

Handle(GeomAdaptor_Surface) GeomAdaptor_Surface::BasisSurface() const
{
  if (myCore.GetType() != GeomAbs_OffsetSurface)
  {
    throw Standard_NoSuchObject("GeomAdaptor_Surface::BasisSurface");
  }

  const Handle(Geom_Surface)& aSurf = myCore.Surface();
  if (aSurf.IsNull())
  {
    throw Standard_NoSuchObject("GeomAdaptor_Surface::BasisSurface");
  }

  Handle(Geom_OffsetSurface) anOffSurf = Handle(Geom_OffsetSurface)::DownCast(aSurf);
  if (anOffSurf.IsNull())
  {
    throw Standard_NoSuchObject("GeomAdaptor_Surface::BasisSurface");
  }

  return new GeomAdaptor_Surface(anOffSurf->BasisSurface(),
                                 myCore.FirstUParameter(),
                                 myCore.LastUParameter(),
                                 myCore.FirstVParameter(),
                                 myCore.LastVParameter());
}

//==================================================================================================

Standard_Real GeomAdaptor_Surface::OffsetValue() const
{
  return myCore.OffsetValue();
}
