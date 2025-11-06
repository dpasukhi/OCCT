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

#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Geom_UndefinedDerivative.hxx>
#include <Geom_UndefinedValue.hxx>
#include <gp_GTrsf2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom_Surface, Geom_Geometry)

typedef Geom_Surface Surface;

//=================================================================================================

Handle(Geom_Surface) Geom_Surface::UReversed() const
{
  Handle(Geom_Surface) S = Handle(Geom_Surface)::DownCast(Copy());
  S->UReverse();
  return S;
}

//=================================================================================================

Handle(Geom_Surface) Geom_Surface::VReversed() const
{
  Handle(Geom_Surface) S = Handle(Geom_Surface)::DownCast(Copy());
  S->VReverse();
  return S;
}

//=================================================================================================

void Geom_Surface::TransformParameters(Standard_Real&, Standard_Real&, const gp_Trsf&) const {}

//=================================================================================================

gp_GTrsf2d Geom_Surface::ParametricTransformation(const gp_Trsf&) const
{
  gp_GTrsf2d dummy;
  return dummy;
}

//=================================================================================================

Standard_Real Geom_Surface::UPeriod() const
{
  Standard_NoSuchObject_Raise_if(!IsUPeriodic(), "Geom_Surface::UPeriod");

  Standard_Real U1, U2, V1, V2;
  Bounds(U1, U2, V1, V2);
  return (U2 - U1);
}

//=================================================================================================

Standard_Real Geom_Surface::VPeriod() const
{
  Standard_NoSuchObject_Raise_if(!IsVPeriodic(), "Geom_Surface::VPeriod");

  Standard_Real U1, U2, V1, V2;
  Bounds(U1, U2, V1, V2);
  return (V2 - V1);
}

//=================================================================================================

void Geom_Surface::D0(const Standard_Real U, const Standard_Real V, gp_Pnt& P) const
{
  if (auto aResult = D0(U, V))
  {
    P = *aResult;
  }
  else
  {
    throw Geom_UndefinedValue("Geom_Surface::D0 - computation failed");
  }
}

//=================================================================================================

void Geom_Surface::D1(const Standard_Real U,
                      const Standard_Real V,
                      gp_Pnt&             P,
                      gp_Vec&             D1U,
                      gp_Vec&             D1V) const
{
  if (auto aResult = D1(U, V))
  {
    P   = aResult->theValue;
    D1U = aResult->theD1U;
    D1V = aResult->theD1V;
  }
  else
  {
    throw Geom_UndefinedValue("Geom_Surface::D1 - computation failed");
  }
}

//=================================================================================================

void Geom_Surface::D2(const Standard_Real U,
                      const Standard_Real V,
                      gp_Pnt&             P,
                      gp_Vec&             D1U,
                      gp_Vec&             D1V,
                      gp_Vec&             D2U,
                      gp_Vec&             D2V,
                      gp_Vec&             D2UV) const
{
  if (auto aResult = D2(U, V))
  {
    P    = aResult->theValue;
    D1U  = aResult->theD1U;
    D1V  = aResult->theD1V;
    D2U  = aResult->theD2U;
    D2V  = aResult->theD2V;
    D2UV = aResult->theD2UV;
  }
  else
  {
    throw Geom_UndefinedValue("Geom_Surface::D2 - computation failed");
  }
}

//=================================================================================================

void Geom_Surface::D3(const Standard_Real U,
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
  if (auto aResult = D3(U, V))
  {
    P     = aResult->theValue;
    D1U   = aResult->theD1U;
    D1V   = aResult->theD1V;
    D2U   = aResult->theD2U;
    D2V   = aResult->theD2V;
    D2UV  = aResult->theD2UV;
    D3U   = aResult->theD3U;
    D3V   = aResult->theD3V;
    D3UUV = aResult->theD3UUV;
    D3UVV = aResult->theD3UVV;
  }
  else
  {
    throw Geom_UndefinedValue("Geom_Surface::D3 - computation failed");
  }
}

//=================================================================================================

gp_Pnt Geom_Surface::Value(const Standard_Real U, const Standard_Real V) const
{
  gp_Pnt P;
  D0(U, V, P);
  return P;
}

//=================================================================================================

void Geom_Surface::DumpJson(Standard_OStream& theOStream, Standard_Integer theDepth) const
{
  OCCT_DUMP_TRANSIENT_CLASS_BEGIN(theOStream)

  OCCT_DUMP_BASE_CLASS(theOStream, theDepth, Geom_Geometry)
}
