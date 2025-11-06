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

#include <GeomEvaluator_SurfaceOfRevolution.hxx>

#include <Adaptor3d_Curve.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomEvaluator_SurfaceOfRevolution, GeomEvaluator_Surface)

GeomEvaluator_SurfaceOfRevolution::GeomEvaluator_SurfaceOfRevolution(
  const Handle(Geom_Curve)& theBase,
  const gp_Dir&             theRevolDir,
  const gp_Pnt&             theRevolLoc)
    : GeomEvaluator_Surface(),
      myBaseCurve(theBase),
      myRotAxis(theRevolLoc, theRevolDir)
{
}

GeomEvaluator_SurfaceOfRevolution::GeomEvaluator_SurfaceOfRevolution(
  const Handle(Adaptor3d_Curve)& theBase,
  const gp_Dir&                  theRevolDir,
  const gp_Pnt&                  theRevolLoc)
    : GeomEvaluator_Surface(),
      myBaseAdaptor(theBase),
      myRotAxis(theRevolLoc, theRevolDir)
{
}

std::optional<gp_Pnt> GeomEvaluator_SurfaceOfRevolution::D0(const Standard_Real theU,
                                                             const Standard_Real theV) const
{
  gp_Pnt aValue;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D0(theV, aValue);
  else
    myBaseCurve->D0(theV, aValue);

  gp_Trsf aRotation;
  aRotation.SetRotation(myRotAxis, theU);
  aValue.Transform(aRotation);
  return aValue;
}

std::optional<GeomEvaluator_D1Result> GeomEvaluator_SurfaceOfRevolution::D1(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D1Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D1(theV, aResult.theValue, aResult.theD1V);
  else
    myBaseCurve->D1(theV, aResult.theValue, aResult.theD1V);

  // vector from center of rotation to the point on rotated curve
  gp_XYZ aCQ      = aResult.theValue.XYZ() - myRotAxis.Location().XYZ();
  aResult.theD1U = gp_Vec(myRotAxis.Direction().XYZ().Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (aResult.theD1U.SquareMagnitude() < Precision::SquareConfusion())
    aResult.theD1U.SetCoord(0.0, 0.0, 0.0);

  gp_Trsf aRotation;
  aRotation.SetRotation(myRotAxis, theU);
  aResult.theValue.Transform(aRotation);
  aResult.theD1U.Transform(aRotation);
  aResult.theD1V.Transform(aRotation);
  return aResult;
}

std::optional<GeomEvaluator_D2Result> GeomEvaluator_SurfaceOfRevolution::D2(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D2Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D2(theV, aResult.theValue, aResult.theD1V, aResult.theD2V);
  else
    myBaseCurve->D2(theV, aResult.theValue, aResult.theD1V, aResult.theD2V);

  // vector from center of rotation to the point on rotated curve
  gp_XYZ        aCQ  = aResult.theValue.XYZ() - myRotAxis.Location().XYZ();
  const gp_XYZ& aDir = myRotAxis.Direction().XYZ();
  aResult.theD1U     = gp_Vec(aDir.Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (aResult.theD1U.SquareMagnitude() < Precision::SquareConfusion())
    aResult.theD1U.SetCoord(0.0, 0.0, 0.0);
  aResult.theD2U  = gp_Vec(aDir.Dot(aCQ) * aDir - aCQ);
  aResult.theD2UV = gp_Vec(aDir.Crossed(aResult.theD1V.XYZ()));

  gp_Trsf aRotation;
  aRotation.SetRotation(myRotAxis, theU);
  aResult.theValue.Transform(aRotation);
  aResult.theD1U.Transform(aRotation);
  aResult.theD1V.Transform(aRotation);
  aResult.theD2U.Transform(aRotation);
  aResult.theD2V.Transform(aRotation);
  aResult.theD2UV.Transform(aRotation);
  return aResult;
}

std::optional<GeomEvaluator_D3Result> GeomEvaluator_SurfaceOfRevolution::D3(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  GeomEvaluator_D3Result aResult;
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D3(theV, aResult.theValue, aResult.theD1V, aResult.theD2V, aResult.theD3V);
  else
    myBaseCurve->D3(theV, aResult.theValue, aResult.theD1V, aResult.theD2V, aResult.theD3V);

  // vector from center of rotation to the point on rotated curve
  gp_XYZ        aCQ  = aResult.theValue.XYZ() - myRotAxis.Location().XYZ();
  const gp_XYZ& aDir = myRotAxis.Direction().XYZ();
  aResult.theD1U     = gp_Vec(aDir.Crossed(aCQ));
  // If the point is placed on the axis of revolution then derivatives on U are undefined.
  // Manually set them to zero.
  if (aResult.theD1U.SquareMagnitude() < Precision::SquareConfusion())
    aResult.theD1U.SetCoord(0.0, 0.0, 0.0);
  aResult.theD2U   = gp_Vec(aDir.Dot(aCQ) * aDir - aCQ);
  aResult.theD2UV  = gp_Vec(aDir.Crossed(aResult.theD1V.XYZ()));
  aResult.theD3U   = -aResult.theD1U;
  aResult.theD3UUV = gp_Vec(aDir.Dot(aResult.theD1V.XYZ()) * aDir - aResult.theD1V.XYZ());
  aResult.theD3UVV = gp_Vec(aDir.Crossed(aResult.theD2V.XYZ()));

  gp_Trsf aRotation;
  aRotation.SetRotation(myRotAxis, theU);
  aResult.theValue.Transform(aRotation);
  aResult.theD1U.Transform(aRotation);
  aResult.theD1V.Transform(aRotation);
  aResult.theD2U.Transform(aRotation);
  aResult.theD2V.Transform(aRotation);
  aResult.theD2UV.Transform(aRotation);
  aResult.theD3U.Transform(aRotation);
  aResult.theD3V.Transform(aRotation);
  aResult.theD3UUV.Transform(aRotation);
  aResult.theD3UVV.Transform(aRotation);
  return aResult;
}

std::optional<gp_Vec> GeomEvaluator_SurfaceOfRevolution::DN(const Standard_Real    theU,
                                                             const Standard_Real    theV,
                                                             const Standard_Integer theDerU,
                                                             const Standard_Integer theDerV) const
{
  Standard_RangeError_Raise_if(theDerU < 0, "GeomEvaluator_SurfaceOfRevolution::DN(): theDerU < 0");
  Standard_RangeError_Raise_if(theDerV < 0, "GeomEvaluator_SurfaceOfRevolution::DN(): theDerV < 0");
  Standard_RangeError_Raise_if(theDerU + theDerV < 1,
                               "GeomEvaluator_SurfaceOfRevolution::DN(): theDerU + theDerV < 1");

  gp_Trsf aRotation;
  aRotation.SetRotation(myRotAxis, theU);

  gp_Pnt aP;
  gp_Vec aDV;
  gp_Vec aResult;
  if (theDerU == 0)
  {
    if (!myBaseAdaptor.IsNull())
      aResult = myBaseAdaptor->DN(theV, theDerV);
    else
      aResult = myBaseCurve->DN(theV, theDerV);
  }
  else
  {
    if (theDerV == 0)
    {
      if (!myBaseAdaptor.IsNull())
        myBaseAdaptor->D0(theV, aP);
      else
        myBaseCurve->D0(theV, aP);
      aDV = gp_Vec(aP.XYZ() - myRotAxis.Location().XYZ());
    }
    else
    {
      if (!myBaseAdaptor.IsNull())
        aDV = myBaseAdaptor->DN(theV, theDerV);
      else
        aDV = myBaseCurve->DN(theV, theDerV);
    }

    const gp_XYZ& aDir = myRotAxis.Direction().XYZ();
    if (theDerU % 4 == 1)
      aResult = gp_Vec(aDir.Crossed(aDV.XYZ()));
    else if (theDerU % 4 == 2)
      aResult = gp_Vec(aDir.Dot(aDV.XYZ()) * aDir - aDV.XYZ());
    else if (theDerU % 4 == 3)
      aResult = gp_Vec(aDir.Crossed(aDV.XYZ())) * (-1.0);
    else
      aResult = gp_Vec(aDV.XYZ() - aDir.Dot(aDV.XYZ()) * aDir);
  }

  aResult.Transform(aRotation);
  return aResult;
}

Handle(GeomEvaluator_Surface) GeomEvaluator_SurfaceOfRevolution::ShallowCopy() const
{
  Handle(GeomEvaluator_SurfaceOfRevolution) aCopy;
  if (!myBaseAdaptor.IsNull())
  {
    aCopy = new GeomEvaluator_SurfaceOfRevolution(myBaseAdaptor->ShallowCopy(),
                                                  myRotAxis.Direction(),
                                                  myRotAxis.Location());
  }
  else
  {
    aCopy = new GeomEvaluator_SurfaceOfRevolution(myBaseCurve,
                                                  myRotAxis.Direction(),
                                                  myRotAxis.Location());
  }

  return aCopy;
}
