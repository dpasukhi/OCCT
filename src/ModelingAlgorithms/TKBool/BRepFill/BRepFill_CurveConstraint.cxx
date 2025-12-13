// Created on: 1997-10-31
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

#include <BRepFill_CurveConstraint.hxx>

#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(BRepFill_CurveConstraint, GeomPlate_CurveConstraint)

//==================================================================================================

BRepFill_CurveConstraint::BRepFill_CurveConstraint(GeomAdaptor_Curve&&    theBoundary,
                                                   const Standard_Integer theOrder,
                                                   const Standard_Integer theNPt,
                                                   const Standard_Real    theTolDist,
                                                   const Standard_Real    theTolAng,
                                                   const Standard_Real    theTolCurv)
    : GeomPlate_CurveConstraint(std::move(theBoundary), theOrder, theNPt, theTolDist, theTolAng, theTolCurv)
{
}
