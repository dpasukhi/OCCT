// Created on: 2015-09-21
// Copyright (c) 2015 OPEN CASCADE SAS
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

#include <GeomEvaluator_SurfaceOfExtrusion.hxx>

#include <GeomAdaptor_Curve.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomEvaluator_SurfaceOfExtrusion, GeomEvaluator_Surface)

GeomEvaluator_SurfaceOfExtrusion::GeomEvaluator_SurfaceOfExtrusion(
  const Handle(Geom_Curve)& theBase,
  const gp_Dir&             theExtrusionDir)
    : GeomEvaluator_Surface(),
      myBaseCurve(theBase),
      myDirection(theExtrusionDir)
{
}

GeomEvaluator_SurfaceOfExtrusion::GeomEvaluator_SurfaceOfExtrusion(
  const Handle(Adaptor3d_Curve)& theBase,
  const gp_Dir&                  theExtrusionDir)
    : GeomEvaluator_Surface(),
      myBaseAdaptor(theBase),
      myDirection(theExtrusionDir)
{
}

std::optional<gp_Pnt> GeomEvaluator_SurfaceOfExtrusion::D0(const Standard_Real theU,
                                                            const Standard_Real theV) const
{
  gp_Pnt aValue;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D0(theU, aValue);
  else
    myBaseCurve->D0(theU, aValue);

  Shift(theV, aValue);
  return aValue;
}

std::optional<GeomEvaluator_D1Result> GeomEvaluator_SurfaceOfExtrusion::D1(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D1Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D1(theU, aResult.theValue, aResult.theD1U);
  else
    myBaseCurve->D1(theU, aResult.theValue, aResult.theD1U);

  aResult.theD1V = myDirection;
  Shift(theV, aResult.theValue);
  return aResult;
}

std::optional<GeomEvaluator_D2Result> GeomEvaluator_SurfaceOfExtrusion::D2(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D2Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D2(theU, aResult.theValue, aResult.theD1U, aResult.theD2U);
  else
    myBaseCurve->D2(theU, aResult.theValue, aResult.theD1U, aResult.theD2U);

  aResult.theD1V = myDirection;
  aResult.theD2V.SetCoord(0.0, 0.0, 0.0);
  aResult.theD2UV.SetCoord(0.0, 0.0, 0.0);

  Shift(theV, aResult.theValue);
  return aResult;
}

std::optional<GeomEvaluator_D3Result> GeomEvaluator_SurfaceOfExtrusion::D3(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D3Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D3(theU, aResult.theValue, aResult.theD1U, aResult.theD2U, aResult.theD3U);
  else
    myBaseCurve->D3(theU, aResult.theValue, aResult.theD1U, aResult.theD2U, aResult.theD3U);

  aResult.theD1V = myDirection;
  aResult.theD2V.SetCoord(0.0, 0.0, 0.0);
  aResult.theD2UV.SetCoord(0.0, 0.0, 0.0);
  aResult.theD3V.SetCoord(0.0, 0.0, 0.0);
  aResult.theD3UUV.SetCoord(0.0, 0.0, 0.0);
  aResult.theD3UVV.SetCoord(0.0, 0.0, 0.0);

  Shift(theV, aResult.theValue);
  return aResult;
}

std::optional<gp_Vec> GeomEvaluator_SurfaceOfExtrusion::DN(const Standard_Real    theU,
                                                            const Standard_Real,
                                                            const Standard_Integer theDerU,
                                                            const Standard_Integer theDerV) const
{
  Standard_RangeError_Raise_if(theDerU < 0, "GeomEvaluator_SurfaceOfExtrusion::DN(): theDerU < 0");
  Standard_RangeError_Raise_if(theDerV < 0, "GeomEvaluator_SurfaceOfExtrusion::DN(): theDerV < 0");
  Standard_RangeError_Raise_if(theDerU + theDerV < 1,
                               "GeomEvaluator_SurfaceOfExtrusion::DN(): theDerU + theDerV < 1");

  gp_Vec aResult(0.0, 0.0, 0.0);
  if (theDerV == 0)
  {
    if (!myBaseAdaptor.IsNull())
      aResult = myBaseAdaptor->DN(theU, theDerU);
    else
      aResult = myBaseCurve->DN(theU, theDerU);
  }
  else if (theDerU == 0 && theDerV == 1)
    aResult = gp_Vec(myDirection);
  return aResult;
}

Handle(GeomEvaluator_Surface) GeomEvaluator_SurfaceOfExtrusion::ShallowCopy() const
{
  Handle(GeomEvaluator_SurfaceOfExtrusion) aCopy;
  if (!myBaseAdaptor.IsNull())
  {
    aCopy = new GeomEvaluator_SurfaceOfExtrusion(myBaseAdaptor->ShallowCopy(), myDirection);
  }
  else
  {
    aCopy = new GeomEvaluator_SurfaceOfExtrusion(myBaseCurve, myDirection);
  }

  return aCopy;
}
