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

#include <GeomAdaptor_HVertex.hxx>

#include <Geom2dAdaptor_Curve.hxx>
#include <ElCLib.hxx>
#include <gp_Pnt2d.hxx>
#include <Precision.hxx>
#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_HVertex, Standard_Transient)

GeomAdaptor_HVertex::GeomAdaptor_HVertex()
    : myTol(0.0)
{
}

GeomAdaptor_HVertex::GeomAdaptor_HVertex(const gp_Pnt2d&          P,
                                     const TopAbs_Orientation Or,
                                     const Standard_Real      Resolution)
    : myPnt(P),
      myTol(Resolution),
      myOri(Or)
{
}

gp_Pnt2d GeomAdaptor_HVertex::Value()
{
  return myPnt;
}

Standard_Real GeomAdaptor_HVertex::Parameter(const Handle(Geom2dAdaptor_Curve)& C)
{
  return ElCLib::Parameter(C->Line(), myPnt);
}

Standard_Real GeomAdaptor_HVertex::Resolution(const Handle(Geom2dAdaptor_Curve)&)
{
  return myTol;
}

TopAbs_Orientation GeomAdaptor_HVertex::Orientation()
{
  return myOri;
}

Standard_Boolean GeomAdaptor_HVertex::IsSame(const Handle(GeomAdaptor_HVertex)& Other)
{
  return (myPnt.Distance(Other->Value()) <= Precision::Confusion());
}
