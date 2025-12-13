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

#ifndef _BRepFill_CurveConstraint_HeaderFile
#define _BRepFill_CurveConstraint_HeaderFile

#include <Standard.hxx>
#include <Standard_Type.hxx>

#include <GeomPlate_CurveConstraint.hxx>
#include <Standard_Integer.hxx>

class BRepFill_CurveConstraint;
DEFINE_STANDARD_HANDLE(BRepFill_CurveConstraint, GeomPlate_CurveConstraint)

//! Same as CurveConstraint from GeomPlate, specialized for BRep usage.
//! The curve is provided as a GeomAdaptor_Curve which may have a
//! CurveOnSurface modifier for surface constraint evaluation.
class BRepFill_CurveConstraint : public GeomPlate_CurveConstraint
{

public:
  //! Create a constraint from a GeomAdaptor_Curve.
  //! The curve may have a CurveOnSurface modifier for G1/G2 continuity.
  //! @param theBoundary curve adaptor (moved into the constraint)
  //! @param theOrder continuity order (-1, 0, 1, 2)
  //! @param theNPt number of points on the curve
  //! @param theTolDist distance tolerance (G0)
  //! @param theTolAng angular tolerance (G1)
  //! @param theTolCurv curvature tolerance (G2)
  Standard_EXPORT BRepFill_CurveConstraint(GeomAdaptor_Curve&&    theBoundary,
                                           const Standard_Integer theOrder,
                                           const Standard_Integer theNPt     = 10,
                                           const Standard_Real    theTolDist = 0.0001,
                                           const Standard_Real    theTolAng  = 0.01,
                                           const Standard_Real    theTolCurv = 0.1);

  DEFINE_STANDARD_RTTIEXT(BRepFill_CurveConstraint, GeomPlate_CurveConstraint)

protected:
private:
};

#endif // _BRepFill_CurveConstraint_HeaderFile
