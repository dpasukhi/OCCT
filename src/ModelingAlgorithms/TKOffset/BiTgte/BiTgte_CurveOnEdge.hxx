// Created on: 1997-01-10
// Created by: Bruno DUMORTIER
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

#ifndef _BiTgte_CurveOnEdge_HeaderFile
#define _BiTgte_CurveOnEdge_HeaderFile

#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>

#include <TopoDS_Edge.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Circ.hxx>

class Geom_Curve;
class gp_Pnt;

//! Private class used to create a filler rolling on an edge.
//! This class represents a curve projected from one edge onto another.
class BiTgte_CurveOnEdge
{
public:
  DEFINE_STANDARD_ALLOC

  Standard_EXPORT BiTgte_CurveOnEdge();

  Standard_EXPORT BiTgte_CurveOnEdge(const TopoDS_Edge& EonF, const TopoDS_Edge& Edge);

  Standard_EXPORT void Init(const TopoDS_Edge& EonF, const TopoDS_Edge& Edge);

  //! Returns the first parameter of the curve.
  Standard_EXPORT double FirstParameter() const;

  //! Returns the last parameter of the curve.
  Standard_EXPORT double LastParameter() const;

  //! Computes the point of parameter U on the curve.
  Standard_EXPORT gp_Pnt Value(double U) const;

  //! Returns the type of the curve in the current interval:
  //! Circle or OtherCurve.
  Standard_EXPORT GeomAbs_CurveType GetType() const;

  //! Returns the circle if GetType() == GeomAbs_Circle.
  //! @throw Standard_NoSuchObject if GetType() != GeomAbs_Circle
  Standard_EXPORT gp_Circ Circle() const;

private:
  TopoDS_Edge            myEdge;
  TopoDS_Edge            myEonF;
  Handle(Geom_Curve)     myCurv;
  Handle(Geom_Curve)     myConF;
  GeomAbs_CurveType      myType;
  gp_Circ                myCirc;
};

#endif // _BiTgte_CurveOnEdge_HeaderFile
