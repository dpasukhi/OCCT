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

#include <Adaptor3d_CurveOnSurface.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomPlate_CurveConstraint.hxx>
#include <ProjLib_ProjectedCurve.hxx>
#include <ProjLib_ProjectOnPlane.hxx>
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

//==================================================================================================

BRepFill_CurveConstraint::BRepFill_CurveConstraint(const Handle(Adaptor3d_CurveOnSurface)& Boundary,
                                                   const Standard_Integer                  Tang,
                                                   const Standard_Integer                  NPt,
                                                   const Standard_Real                     TolDist,
                                                   const Standard_Real                     TolAng,
                                                   const Standard_Real                     TolCurv)
    : GeomPlate_CurveConstraint()
{
  if (Boundary.IsNull())
  {
    throw Standard_Failure("BRepFill_CurveConstraint : Curve must be on a Surface");
  }
  if ((Tang < -1) || (Tang > 2))
  {
    throw Standard_Failure("BRepFill : The continuity is not G0 G1 or G2");
  }

  myOrder    = Tang;
  myNbPoints = NPt;
  myTolDist  = TolDist;
  myTolAng   = TolAng;
  myTolCurv  = TolCurv;
  myConstG0  = Standard_True;
  myConstG1  = Standard_True;
  myConstG2  = Standard_True;

  // Extract PCurve and Surface from Adaptor3d_CurveOnSurface
  Handle(Adaptor2d_Curve2d)   aPCurveHandle = Boundary->GetCurve();
  Handle(Adaptor3d_Surface)   aSurfHandle   = Boundary->GetSurface();
  Handle(Geom2dAdaptor_Curve) aGeomPCurve   = Handle(Geom2dAdaptor_Curve)::DownCast(aPCurveHandle);

  // Get the underlying Geom_Surface for LProp
  Handle(Geom_Surface) aSurf;

  // Try GeomAdaptor_Surface first
  Handle(GeomAdaptor_Surface) aGeomSurf = Handle(GeomAdaptor_Surface)::DownCast(aSurfHandle);
  if (!aGeomSurf.IsNull())
  {
    aSurf = aGeomSurf->Surface();

    // Create new GeomAdaptor_Curve with COS modifier
    if (!aGeomPCurve.IsNull())
    {
      myCurve    = std::make_unique<GeomAdaptor_Curve>();
      auto aPCrv = std::make_unique<Geom2dAdaptor_Curve>(*aGeomPCurve);
      auto aSrf  = std::make_unique<GeomAdaptor_Surface>(*aGeomSurf);
      myCurve->SetCurveOnSurface(std::move(aPCrv), std::move(aSrf));
    }
  }
  else
  {
    // Try BRepAdaptor_Surface
    Handle(BRepAdaptor_Surface) aBRepSurf = Handle(BRepAdaptor_Surface)::DownCast(aSurfHandle);
    if (!aBRepSurf.IsNull())
    {
      aSurf = BRep_Tool::Surface(aBRepSurf->Face());

      // Create GeomAdaptor_Surface from the underlying Geom_Surface
      if (!aGeomPCurve.IsNull() && !aSurf.IsNull())
      {
        myCurve    = std::make_unique<GeomAdaptor_Curve>();
        auto aPCrv = std::make_unique<Geom2dAdaptor_Curve>(*aGeomPCurve);
        auto aSrf  = std::make_unique<GeomAdaptor_Surface>(aSurf);
        myCurve->SetCurveOnSurface(std::move(aPCrv), std::move(aSrf));
      }
    }
  }

  // Set up LProp
  if (!aSurf.IsNull())
  {
    myLProp = GeomLProp_SLProps(2, TolDist);
    myLProp.SetSurface(aSurf);
  }
}

//==================================================================================================

BRepFill_CurveConstraint::BRepFill_CurveConstraint(const Handle(Adaptor3d_Curve)& Boundary,
                                                   const Standard_Integer         Tang,
                                                   const Standard_Integer         NPt,
                                                   const Standard_Real            TolDist)
    : GeomPlate_CurveConstraint(Boundary, Tang, NPt, TolDist)
{
}
