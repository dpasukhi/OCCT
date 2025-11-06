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

#include <GeomEvaluator_OffsetSurface.hxx>

#include <CSLib.hxx>
#include <CSLib_NormalStatus.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_UndefinedValue.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Vec.hxx>
#include <Standard_RangeError.hxx>
#include <Standard_NumericError.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomEvaluator_OffsetSurface, GeomEvaluator_Surface)

namespace
{

// tolerance for considering derivative to be null
const Standard_Real the_D1MagTol = 1.e-9;

// If calculation of normal fails, try shifting the point towards the center
// of the parametric space of the surface, in the hope that derivatives
// are better defined there.
//
// This shift is iterative, starting with Precision::PConfusion()
// and increasing by multiple of 2 on each step.
//
// NB: temporarily this is made as static function and not class method,
// hence code duplications
static Standard_Boolean shiftPoint(const Standard_Real                theUStart,
                                   const Standard_Real                theVStart,
                                   Standard_Real&                     theU,
                                   Standard_Real&                     theV,
                                   const Handle(Geom_Surface)&        theSurf,
                                   const Handle(GeomAdaptor_Surface)& theAdaptor,
                                   const gp_Vec&                      theD1U,
                                   const gp_Vec&                      theD1V)
{
  // Get parametric bounds and closure status
  Standard_Real    aUMin, aUMax, aVMin, aVMax;
  Standard_Boolean isUPeriodic, isVPeriodic;
  if (!theSurf.IsNull())
  {
    theSurf->Bounds(aUMin, aUMax, aVMin, aVMax);
    isUPeriodic = theSurf->IsUPeriodic();
    isVPeriodic = theSurf->IsVPeriodic();
  }
  else
  {
    aUMin       = theAdaptor->FirstUParameter();
    aUMax       = theAdaptor->LastUParameter();
    aVMin       = theAdaptor->FirstVParameter();
    aVMax       = theAdaptor->LastVParameter();
    isUPeriodic = theAdaptor->IsUPeriodic();
    isVPeriodic = theAdaptor->IsVPeriodic();
  }

  // check if either U or V is singular (normally one of them is)
  Standard_Boolean isUSingular = (theD1U.SquareMagnitude() < the_D1MagTol * the_D1MagTol);
  Standard_Boolean isVSingular = (theD1V.SquareMagnitude() < the_D1MagTol * the_D1MagTol);

  // compute vector to shift from start point to center of the surface;
  // if surface is periodic or singular in some direction, take shift in that direction zero
  Standard_Real aDirU =
    (isUPeriodic || (isUSingular && !isVSingular) ? 0. : 0.5 * (aUMin + aUMax) - theUStart);
  Standard_Real aDirV =
    (isVPeriodic || (isVSingular && !isUSingular) ? 0. : 0.5 * (aVMin + aVMax) - theVStart);
  Standard_Real aDist = Sqrt(aDirU * aDirU + aDirV * aDirV);

  // shift current point from its current position towards center, by value of twice
  // current distance from it to start (but not less than Precision::PConfusion());
  // fail if center is overpassed.
  Standard_Real aDU   = theU - theUStart;
  Standard_Real aDV   = theV - theVStart;
  Standard_Real aStep = Max(2. * Sqrt(aDU * aDU + aDV * aDV), Precision::PConfusion());
  if (aStep >= aDist)
  {
    return Standard_False;
  }

  aStep /= aDist;
  theU += aDirU * aStep;
  theV += aDirV * aStep;

  //  std::cout << "Normal calculation failed at (" << theUStart << ", " << theVStart << "),
  //  shifting to (" << theU << ", " << theV << ")" << std::endl;

  return Standard_True;
}

// auxiliary function
template <class SurfOrAdapt>
static void derivatives(Standard_Integer                   theMaxOrder,
                        Standard_Integer                   theMinOrder,
                        const Standard_Real                theU,
                        const Standard_Real                theV,
                        const SurfOrAdapt&                 theBasisSurf,
                        const Standard_Integer             theNU,
                        const Standard_Integer             theNV,
                        const Standard_Boolean             theAlongU,
                        const Standard_Boolean             theAlongV,
                        const Handle(Geom_BSplineSurface)& theL,
                        TColgp_Array2OfVec&                theDerNUV,
                        TColgp_Array2OfVec&                theDerSurf)
{
  Standard_Integer i, j;
  gp_Pnt           P;
  gp_Vec           DL1U, DL1V, DL2U, DL2V, DL2UV, DL3U, DL3UUV, DL3UVV, DL3V;

  if (theAlongU || theAlongV)
  {
    theMaxOrder = 0;
    TColgp_Array2OfVec DerSurfL(0, theMaxOrder + theNU + 1, 0, theMaxOrder + theNV + 1);
    switch (theMinOrder)
    {
      case 1:
        theL->D1(theU, theV, P, DL1U, DL1V);
        DerSurfL.SetValue(1, 0, DL1U);
        DerSurfL.SetValue(0, 1, DL1V);
        break;
      case 2:
        theL->D2(theU, theV, P, DL1U, DL1V, DL2U, DL2V, DL2UV);
        DerSurfL.SetValue(1, 0, DL1U);
        DerSurfL.SetValue(0, 1, DL1V);
        DerSurfL.SetValue(1, 1, DL2UV);
        DerSurfL.SetValue(2, 0, DL2U);
        DerSurfL.SetValue(0, 2, DL2V);
        break;
      case 3:
        theL->D3(theU, theV, P, DL1U, DL1V, DL2U, DL2V, DL2UV, DL3U, DL3V, DL3UUV, DL3UVV);
        DerSurfL.SetValue(1, 0, DL1U);
        DerSurfL.SetValue(0, 1, DL1V);
        DerSurfL.SetValue(1, 1, DL2UV);
        DerSurfL.SetValue(2, 0, DL2U);
        DerSurfL.SetValue(0, 2, DL2V);
        DerSurfL.SetValue(3, 0, DL3U);
        DerSurfL.SetValue(2, 1, DL3UUV);
        DerSurfL.SetValue(1, 2, DL3UVV);
        DerSurfL.SetValue(0, 3, DL3V);
        break;
      default:
        break;
    }

    if (theNU <= theNV)
    {
      for (i = 0; i <= theMaxOrder + 1 + theNU; i++)
        for (j = i; j <= theMaxOrder + theNV + 1; j++)
          if (i + j > theMinOrder)
          {
            DerSurfL.SetValue(i, j, theL->DN(theU, theV, i, j));
            theDerSurf.SetValue(i, j, theBasisSurf->DN(theU, theV, i, j));
            if (i != j && j <= theNU + 1)
            {
              theDerSurf.SetValue(j, i, theBasisSurf->DN(theU, theV, j, i));
              DerSurfL.SetValue(j, i, theL->DN(theU, theV, j, i));
            }
          }
    }
    else
    {
      for (j = 0; j <= theMaxOrder + 1 + theNV; j++)
        for (i = j; i <= theMaxOrder + theNU + 1; i++)
          if (i + j > theMinOrder)
          {
            DerSurfL.SetValue(i, j, theL->DN(theU, theV, i, j));
            theDerSurf.SetValue(i, j, theBasisSurf->DN(theU, theV, i, j));
            if (i != j && i <= theNV + 1)
            {
              theDerSurf.SetValue(j, i, theBasisSurf->DN(theU, theV, j, i));
              DerSurfL.SetValue(j, i, theL->DN(theU, theV, j, i));
            }
          }
    }
    for (i = 0; i <= theMaxOrder + theNU; i++)
      for (j = 0; j <= theMaxOrder + theNV; j++)
      {
        if (theAlongU)
          theDerNUV.SetValue(i, j, CSLib::DNNUV(i, j, DerSurfL, theDerSurf));
        if (theAlongV)
          theDerNUV.SetValue(i, j, CSLib::DNNUV(i, j, theDerSurf, DerSurfL));
      }
  }
  else
  {
    for (i = 0; i <= theMaxOrder + theNU + 1; i++)
    {
      for (j = i; j <= theMaxOrder + theNV + 1; j++)
      {
        if (i + j > theMinOrder)
        {
          theDerSurf.SetValue(i, j, theBasisSurf->DN(theU, theV, i, j));
          if (i != j && j <= theDerSurf.UpperRow() && i <= theDerSurf.UpperCol())
          {
            theDerSurf.SetValue(j, i, theBasisSurf->DN(theU, theV, j, i));
          }
        }
      }
    }
    for (i = 0; i <= theMaxOrder + theNU; i++)
      for (j = 0; j <= theMaxOrder + theNV; j++)
        theDerNUV.SetValue(i, j, CSLib::DNNUV(i, j, theDerSurf));
  }
}

inline Standard_Boolean IsInfiniteCoord(const gp_Vec& theVec)
{
  return Precision::IsInfinite(theVec.X()) || Precision::IsInfinite(theVec.Y())
         || Precision::IsInfinite(theVec.Z());
}

inline void CheckInfinite(const gp_Vec& theVecU, const gp_Vec& theVecV)
{
  if (IsInfiniteCoord(theVecU) || IsInfiniteCoord(theVecV))
  {
    throw Standard_NumericError("GeomEvaluator_OffsetSurface: Evaluation of infinite parameters");
  }
}

} // end of anonymous namespace

GeomEvaluator_OffsetSurface::GeomEvaluator_OffsetSurface(
  const Handle(Geom_Surface)&           theBase,
  const Standard_Real                   theOffset,
  const Handle(Geom_OsculatingSurface)& theOscSurf)
    : GeomEvaluator_Surface(),
      myBaseSurf(theBase),
      myOffset(theOffset),
      myOscSurf(theOscSurf)
{
  if (!myOscSurf.IsNull())
    return; // osculating surface already exists

  // Create osculating surface for B-spline and Besier surfaces only
  if (myBaseSurf->IsKind(STANDARD_TYPE(Geom_BSplineSurface))
      || myBaseSurf->IsKind(STANDARD_TYPE(Geom_BezierSurface)))
    myOscSurf = new Geom_OsculatingSurface(myBaseSurf, Precision::Confusion());
}

GeomEvaluator_OffsetSurface::GeomEvaluator_OffsetSurface(
  const Handle(GeomAdaptor_Surface)&    theBase,
  const Standard_Real                   theOffset,
  const Handle(Geom_OsculatingSurface)& theOscSurf)
    : GeomEvaluator_Surface(),
      myBaseAdaptor(theBase),
      myOffset(theOffset),
      myOscSurf(theOscSurf)
{
}

std::optional<gp_Pnt> GeomEvaluator_OffsetSurface::D0(const Standard_Real theU,
                                                       const Standard_Real theV) const
{
  Standard_Real aU = theU, aV = theV;
  for (;;)
  {
    gp_Pnt aValue;
    gp_Vec aD1U, aD1V;
    BaseD1(aU, aV, aValue, aD1U, aD1V);

    CheckInfinite(aD1U, aD1V);

    if (auto aResult = CalculateD0(aU, aV, aValue, aD1U, aD1V))
    {
      return aResult;
    }
    else
    {
      // if failed at parametric boundary, try taking derivative at shifted point
      if (!shiftPoint(theU, theV, aU, aV, myBaseSurf, myBaseAdaptor, aD1U, aD1V))
      {
        return std::nullopt;
      }
    }
  }
}

std::optional<GeomEvaluator_D1Result> GeomEvaluator_OffsetSurface::D1(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  Standard_Real aU = theU, aV = theV;
  for (;;)
  {
    gp_Pnt aValue;
    gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV;
    BaseD2(aU, aV, aValue, aD1U, aD1V, aD2U, aD2V, aD2UV);

    CheckInfinite(aD1U, aD1V);

    if (auto aResult = CalculateD1(aU, aV, aValue, aD1U, aD1V, aD2U, aD2V, aD2UV))
    {
      return aResult;
    }
    else
    {
      // if failed at parametric boundary, try taking derivative at shifted point
      if (!shiftPoint(theU, theV, aU, aV, myBaseSurf, myBaseAdaptor, aD1U, aD1V))
      {
        return std::nullopt;
      }
    }
  }
}

std::optional<GeomEvaluator_D2Result> GeomEvaluator_OffsetSurface::D2(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  Standard_Real aU = theU, aV = theV;
  for (;;)
  {
    gp_Pnt aValue;
    gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV;
    BaseD3(aU, aV, aValue, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV);

    CheckInfinite(aD1U, aD1V);

    if (auto aResult = CalculateD2(aU,
                                    aV,
                                    aValue,
                                    aD1U,
                                    aD1V,
                                    aD2U,
                                    aD2V,
                                    aD2UV,
                                    aD3U,
                                    aD3V,
                                    aD3UUV,
                                    aD3UVV))
    {
      return aResult;
    }
    else
    {
      // if failed at parametric boundary, try taking derivative at shifted point
      if (!shiftPoint(theU, theV, aU, aV, myBaseSurf, myBaseAdaptor, aD1U, aD1V))
      {
        return std::nullopt;
      }
    }
  }
}

std::optional<GeomEvaluator_D3Result> GeomEvaluator_OffsetSurface::D3(
  const Standard_Real theU,
  const Standard_Real theV) const
{
  Standard_Real aU = theU, aV = theV;
  for (;;)
  {
    gp_Pnt aValue;
    gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV;
    BaseD3(aU,
           aV,
           aValue,
           aD1U,
           aD1V,
           aD2U,
           aD2V,
           aD2UV,
           aD3U,
           aD3V,
           aD3UUV,
           aD3UVV);

    CheckInfinite(aD1U, aD1V);

    if (auto aResult = CalculateD3(aU,
                                    aV,
                                    aValue,
                                    aD1U,
                                    aD1V,
                                    aD2U,
                                    aD2V,
                                    aD2UV,
                                    aD3U,
                                    aD3V,
                                    aD3UUV,
                                    aD3UVV))
    {
      return aResult;
    }
    else
    {
      // if failed at parametric boundary, try taking derivative at shifted point
      if (!shiftPoint(theU, theV, aU, aV, myBaseSurf, myBaseAdaptor, aD1U, aD1V))
      {
        return std::nullopt;
      }
    }
  }
}

std::optional<gp_Vec> GeomEvaluator_OffsetSurface::DN(const Standard_Real    theU,
                                                       const Standard_Real    theV,
                                                       const Standard_Integer theDerU,
                                                       const Standard_Integer theDerV) const
{
  Standard_RangeError_Raise_if(theDerU < 0, "GeomEvaluator_OffsetSurface::DN(): theDerU < 0");
  Standard_RangeError_Raise_if(theDerV < 0, "GeomEvaluator_OffsetSurface::DN(): theDerV < 0");
  Standard_RangeError_Raise_if(theDerU + theDerV < 1,
                               "GeomEvaluator_OffsetSurface::DN(): theDerU + theDerV < 1");

  Standard_Real aU = theU, aV = theV;
  for (;;)
  {
    gp_Pnt aP;
    gp_Vec aD1U, aD1V;
    BaseD1(aU, aV, aP, aD1U, aD1V);

    CheckInfinite(aD1U, aD1V);

    if (auto aResult = CalculateDN(aU, aV, theDerU, theDerV, aD1U, aD1V))
    {
      return aResult;
    }
    else
    {
      // if failed at parametric boundary, try taking derivative at shifted point
      if (!shiftPoint(theU, theV, aU, aV, myBaseSurf, myBaseAdaptor, aD1U, aD1V))
      {
        return std::nullopt;
      }
    }
  }
}

Handle(GeomEvaluator_Surface) GeomEvaluator_OffsetSurface::ShallowCopy() const
{
  Handle(GeomEvaluator_OffsetSurface) aCopy;
  if (!myBaseAdaptor.IsNull())
  {
    aCopy = new GeomEvaluator_OffsetSurface(
      Handle(GeomAdaptor_Surface)::DownCast(myBaseAdaptor->ShallowCopy()),
      myOffset,
      myOscSurf);
  }
  else
  {
    aCopy = new GeomEvaluator_OffsetSurface(myBaseSurf, myOffset, myOscSurf);
  }

  return aCopy;
}

void GeomEvaluator_OffsetSurface::BaseD0(const Standard_Real theU,
                                         const Standard_Real theV,
                                         gp_Pnt&             theValue) const
{
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D0(theU, theV, theValue);
  else
    myBaseSurf->D0(theU, theV, theValue);
}

void GeomEvaluator_OffsetSurface::BaseD1(const Standard_Real theU,
                                         const Standard_Real theV,
                                         gp_Pnt&             theValue,
                                         gp_Vec&             theD1U,
                                         gp_Vec&             theD1V) const
{
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D1(theU, theV, theValue, theD1U, theD1V);
  else
    myBaseSurf->D1(theU, theV, theValue, theD1U, theD1V);
}

void GeomEvaluator_OffsetSurface::BaseD2(const Standard_Real theU,
                                         const Standard_Real theV,
                                         gp_Pnt&             theValue,
                                         gp_Vec&             theD1U,
                                         gp_Vec&             theD1V,
                                         gp_Vec&             theD2U,
                                         gp_Vec&             theD2V,
                                         gp_Vec&             theD2UV) const
{
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D2(theU, theV, theValue, theD1U, theD1V, theD2U, theD2V, theD2UV);
  else
    myBaseSurf->D2(theU, theV, theValue, theD1U, theD1V, theD2U, theD2V, theD2UV);
}

void GeomEvaluator_OffsetSurface::BaseD3(const Standard_Real theU,
                                         const Standard_Real theV,
                                         gp_Pnt&             theValue,
                                         gp_Vec&             theD1U,
                                         gp_Vec&             theD1V,
                                         gp_Vec&             theD2U,
                                         gp_Vec&             theD2V,
                                         gp_Vec&             theD2UV,
                                         gp_Vec&             theD3U,
                                         gp_Vec&             theD3V,
                                         gp_Vec&             theD3UUV,
                                         gp_Vec&             theD3UVV) const
{
  if (!myBaseAdaptor.IsNull())
    myBaseAdaptor->D3(theU,
                      theV,
                      theValue,
                      theD1U,
                      theD1V,
                      theD2U,
                      theD2V,
                      theD2UV,
                      theD3U,
                      theD3V,
                      theD3UUV,
                      theD3UVV);
  else
    myBaseSurf->D3(theU,
                   theV,
                   theValue,
                   theD1U,
                   theD1V,
                   theD2U,
                   theD2V,
                   theD2UV,
                   theD3U,
                   theD3V,
                   theD3UUV,
                   theD3UVV);
}

std::optional<gp_Pnt> GeomEvaluator_OffsetSurface::CalculateD0(const Standard_Real theU,
                                                                const Standard_Real theV,
                                                                const gp_Pnt&       theBaseValue,
                                                                const gp_Vec&       theD1U,
                                                                const gp_Vec&       theD1V) const
{
  gp_Pnt aValue = theBaseValue;

  // Normalize derivatives before normal calculation because it gives more stable result.
  // There will be normalized only derivatives greater than 1.0 to avoid differences in last
  // significant digit
  gp_Vec        aD1U(theD1U);
  gp_Vec        aD1V(theD1V);
  Standard_Real aD1UNorm2 = aD1U.SquareMagnitude();
  Standard_Real aD1VNorm2 = aD1V.SquareMagnitude();
  if (aD1UNorm2 > 1.0)
    aD1U /= Sqrt(aD1UNorm2);
  if (aD1VNorm2 > 1.0)
    aD1V /= Sqrt(aD1VNorm2);

  gp_Vec aNorm = aD1U.Crossed(aD1V);
  if (aNorm.SquareMagnitude() > the_D1MagTol * the_D1MagTol)
  {
    // Non singular case. Simple computations.
    aNorm.Normalize();
    aValue.SetXYZ(aValue.XYZ() + myOffset * aNorm.XYZ());
  }
  else
  {
    const Standard_Integer MaxOrder = 3;

    Handle(Geom_BSplineSurface) L;
    Standard_Boolean            isOpposite = Standard_False;
    Standard_Boolean            AlongU     = Standard_False;
    Standard_Boolean            AlongV     = Standard_False;
    if (!myOscSurf.IsNull())
    {
      AlongU = myOscSurf->UOscSurf(theU, theV, isOpposite, L);
      AlongV = myOscSurf->VOscSurf(theU, theV, isOpposite, L);
    }
    const Standard_Real aSign = ((AlongV || AlongU) && isOpposite) ? -1. : 1.;

    TColgp_Array2OfVec DerNUV(0, MaxOrder, 0, MaxOrder);
    TColgp_Array2OfVec DerSurf(0, MaxOrder + 1, 0, MaxOrder + 1);
    Standard_Integer   OrderU, OrderV;
    Standard_Real      Umin = 0, Umax = 0, Vmin = 0, Vmax = 0;
    Bounds(Umin, Umax, Vmin, Vmax);

    DerSurf.SetValue(1, 0, theD1U);
    DerSurf.SetValue(0, 1, theD1V);
    if (!myBaseSurf.IsNull())
      derivatives(MaxOrder, 1, theU, theV, myBaseSurf, 0, 0, AlongU, AlongV, L, DerNUV, DerSurf);
    else
      derivatives(MaxOrder, 1, theU, theV, myBaseAdaptor, 0, 0, AlongU, AlongV, L, DerNUV, DerSurf);

    gp_Dir             Normal;
    CSLib_NormalStatus NStatus = CSLib_Singular;
    CSLib::Normal(MaxOrder,
                  DerNUV,
                  the_D1MagTol,
                  theU,
                  theV,
                  Umin,
                  Umax,
                  Vmin,
                  Vmax,
                  NStatus,
                  Normal,
                  OrderU,
                  OrderV);
    if (NStatus == CSLib_InfinityOfSolutions)
    {
      // Replace zero derivative and try to calculate normal
      gp_Vec aNewDU = theD1U;
      gp_Vec aNewDV = theD1V;
      if (ReplaceDerivative(theU, theV, aNewDU, aNewDV, the_D1MagTol * the_D1MagTol))
        CSLib::Normal(aNewDU, aNewDV, the_D1MagTol, NStatus, Normal);
    }

    if (NStatus != CSLib_Defined)
      return std::nullopt;

    aValue.SetXYZ(aValue.XYZ() + myOffset * aSign * Normal.XYZ());
  }
  return aValue;
}

std::optional<GeomEvaluator_D1Result> GeomEvaluator_OffsetSurface::CalculateD1(
  const Standard_Real theU,
  const Standard_Real theV,
  const gp_Pnt&       theBaseValue,
  const gp_Vec&       theBaseD1U,
  const gp_Vec&       theBaseD1V,
  const gp_Vec&       theD2U,
  const gp_Vec&       theD2V,
  const gp_Vec&       theD2UV) const
{
  GeomEvaluator_D1Result aResult;
  aResult.theValue = theBaseValue;
  aResult.theD1U   = theBaseD1U;
  aResult.theD1V   = theBaseD1V;

  // Check offset side.
  Handle(Geom_BSplineSurface) L;
  Standard_Boolean            isOpposite = Standard_False;
  Standard_Boolean            AlongU     = Standard_False;
  Standard_Boolean            AlongV     = Standard_False;

  // Normalize derivatives before normal calculation because it gives more stable result.
  // There will be normalized only derivatives greater than 1.0 to avoid differences in last
  // significant digit
  gp_Vec        aD1U(theBaseD1U);
  gp_Vec        aD1V(theBaseD1V);
  Standard_Real aD1UNorm2 = aD1U.SquareMagnitude();
  Standard_Real aD1VNorm2 = aD1V.SquareMagnitude();
  if (aD1UNorm2 > 1.0)
    aD1U /= Sqrt(aD1UNorm2);
  if (aD1VNorm2 > 1.0)
    aD1V /= Sqrt(aD1VNorm2);

  Standard_Boolean isSingular = Standard_False;
  Standard_Integer MaxOrder   = 0;
  gp_Vec           aNorm      = aD1U.Crossed(aD1V);
  if (aNorm.SquareMagnitude() <= the_D1MagTol * the_D1MagTol)
  {
    if (!myOscSurf.IsNull())
    {
      AlongU = myOscSurf->UOscSurf(theU, theV, isOpposite, L);
      AlongV = myOscSurf->VOscSurf(theU, theV, isOpposite, L);
    }

    MaxOrder   = 3;
    isSingular = Standard_True;
  }

  const Standard_Real aSign = ((AlongV || AlongU) && isOpposite) ? -1. : 1.;

  if (!isSingular)
  {
    // AlongU or AlongV leads to more complex D1 computation
    // Try to compute D0 and D1 much simpler
    aNorm.Normalize();
    aResult.theValue.SetXYZ(aResult.theValue.XYZ() + myOffset * aSign * aNorm.XYZ());

    gp_Vec        aN0(aNorm.XYZ()), aN1U, aN1V;
    Standard_Real aScale = (theBaseD1U ^ theBaseD1V).Dot(aN0);
    aN1U.SetX(theD2U.Y() * theBaseD1V.Z() + theBaseD1U.Y() * theD2UV.Z()
              - theD2U.Z() * theBaseD1V.Y() - theBaseD1U.Z() * theD2UV.Y());
    aN1U.SetY((theD2U.X() * theBaseD1V.Z() + theBaseD1U.X() * theD2UV.Z()
               - theD2U.Z() * theBaseD1V.X() - theBaseD1U.Z() * theD2UV.X())
              * -1.0);
    aN1U.SetZ(theD2U.X() * theBaseD1V.Y() + theBaseD1U.X() * theD2UV.Y()
              - theD2U.Y() * theBaseD1V.X() - theBaseD1U.Y() * theD2UV.X());
    Standard_Real aScaleU = aN1U.Dot(aN0);
    aN1U.Subtract(aScaleU * aN0);
    aN1U /= aScale;

    aN1V.SetX(theD2UV.Y() * theBaseD1V.Z() + theD2V.Z() * theBaseD1U.Y()
              - theD2UV.Z() * theBaseD1V.Y() - theD2V.Y() * theBaseD1U.Z());
    aN1V.SetY((theD2UV.X() * theBaseD1V.Z() + theD2V.Z() * theBaseD1U.X()
               - theD2UV.Z() * theBaseD1V.X() - theD2V.X() * theBaseD1U.Z())
              * -1.0);
    aN1V.SetZ(theD2UV.X() * theBaseD1V.Y() + theD2V.Y() * theBaseD1U.X()
              - theD2UV.Y() * theBaseD1V.X() - theD2V.X() * theBaseD1U.Y());
    Standard_Real aScaleV = aN1V.Dot(aN0);
    aN1V.Subtract(aScaleV * aN0);
    aN1V /= aScale;

    aResult.theD1U += myOffset * aSign * aN1U;
    aResult.theD1V += myOffset * aSign * aN1V;

    return aResult;
  }

  Standard_Integer   OrderU, OrderV;
  TColgp_Array2OfVec DerNUV(0, MaxOrder + 1, 0, MaxOrder + 1);
  TColgp_Array2OfVec DerSurf(0, MaxOrder + 2, 0, MaxOrder + 2);
  Standard_Real      Umin = 0, Umax = 0, Vmin = 0, Vmax = 0;
  Bounds(Umin, Umax, Vmin, Vmax);

  DerSurf.SetValue(1, 0, theBaseD1U);
  DerSurf.SetValue(0, 1, theBaseD1V);
  DerSurf.SetValue(1, 1, theD2UV);
  DerSurf.SetValue(2, 0, theD2U);
  DerSurf.SetValue(0, 2, theD2V);
  if (!myBaseSurf.IsNull())
    derivatives(MaxOrder, 2, theU, theV, myBaseSurf, 1, 1, AlongU, AlongV, L, DerNUV, DerSurf);
  else
    derivatives(MaxOrder, 2, theU, theV, myBaseAdaptor, 1, 1, AlongU, AlongV, L, DerNUV, DerSurf);

  gp_Dir             Normal;
  CSLib_NormalStatus NStatus;
  CSLib::Normal(MaxOrder,
                DerNUV,
                the_D1MagTol,
                theU,
                theV,
                Umin,
                Umax,
                Vmin,
                Vmax,
                NStatus,
                Normal,
                OrderU,
                OrderV);
  if (NStatus == CSLib_InfinityOfSolutions)
  {
    gp_Vec aNewDU = theBaseD1U;
    gp_Vec aNewDV = theBaseD1V;
    // Replace zero derivative and try to calculate normal
    if (ReplaceDerivative(theU, theV, aNewDU, aNewDV, the_D1MagTol * the_D1MagTol))
    {
      DerSurf.SetValue(1, 0, aNewDU);
      DerSurf.SetValue(0, 1, aNewDV);
      if (!myBaseSurf.IsNull())
        derivatives(MaxOrder, 2, theU, theV, myBaseSurf, 1, 1, AlongU, AlongV, L, DerNUV, DerSurf);
      else
        derivatives(MaxOrder,
                    2,
                    theU,
                    theV,
                    myBaseAdaptor,
                    1,
                    1,
                    AlongU,
                    AlongV,
                    L,
                    DerNUV,
                    DerSurf);
      CSLib::Normal(MaxOrder,
                    DerNUV,
                    the_D1MagTol,
                    theU,
                    theV,
                    Umin,
                    Umax,
                    Vmin,
                    Vmax,
                    NStatus,
                    Normal,
                    OrderU,
                    OrderV);
    }
  }

  if (NStatus != CSLib_Defined)
    return std::nullopt;

  aResult.theValue.SetXYZ(aResult.theValue.XYZ() + myOffset * aSign * Normal.XYZ());

  aResult.theD1U = DerSurf(1, 0) + myOffset * aSign * CSLib::DNNormal(1, 0, DerNUV, OrderU, OrderV);
  aResult.theD1V = DerSurf(0, 1) + myOffset * aSign * CSLib::DNNormal(0, 1, DerNUV, OrderU, OrderV);
  return aResult;
}

std::optional<GeomEvaluator_D2Result> GeomEvaluator_OffsetSurface::CalculateD2(
  const Standard_Real theU,
  const Standard_Real theV,
  const gp_Pnt&       theBaseValue,
  const gp_Vec&       theBaseD1U,
  const gp_Vec&       theBaseD1V,
  const gp_Vec&       theBaseD2U,
  const gp_Vec&       theBaseD2V,
  const gp_Vec&       theBaseD2UV,
  const gp_Vec&       theD3U,
  const gp_Vec&       theD3V,
  const gp_Vec&       theD3UUV,
  const gp_Vec&       theD3UVV) const
{
  GeomEvaluator_D2Result aResult;
  aResult.theValue = theBaseValue;

  gp_Dir             Normal;
  CSLib_NormalStatus NStatus;
  CSLib::Normal(theBaseD1U, theBaseD1V, the_D1MagTol, NStatus, Normal);

  const Standard_Integer MaxOrder = (NStatus == CSLib_Defined) ? 0 : 3;
  Standard_Integer       OrderU, OrderV;
  TColgp_Array2OfVec     DerNUV(0, MaxOrder + 2, 0, MaxOrder + 2);
  TColgp_Array2OfVec     DerSurf(0, MaxOrder + 3, 0, MaxOrder + 3);

  Standard_Real Umin = 0, Umax = 0, Vmin = 0, Vmax = 0;
  Bounds(Umin, Umax, Vmin, Vmax);

  DerSurf.SetValue(1, 0, theBaseD1U);
  DerSurf.SetValue(0, 1, theBaseD1V);
  DerSurf.SetValue(1, 1, theBaseD2UV);
  DerSurf.SetValue(2, 0, theBaseD2U);
  DerSurf.SetValue(0, 2, theBaseD2V);
  DerSurf.SetValue(3, 0, theD3U);
  DerSurf.SetValue(2, 1, theD3UUV);
  DerSurf.SetValue(1, 2, theD3UVV);
  DerSurf.SetValue(0, 3, theD3V);
  //*********************

  Handle(Geom_BSplineSurface) L;
  Standard_Boolean            isOpposite = Standard_False;
  Standard_Boolean            AlongU     = Standard_False;
  Standard_Boolean            AlongV     = Standard_False;
  if ((NStatus != CSLib_Defined) && !myOscSurf.IsNull())
  {
    AlongU = myOscSurf->UOscSurf(theU, theV, isOpposite, L);
    AlongV = myOscSurf->VOscSurf(theU, theV, isOpposite, L);
  }
  const Standard_Real aSign = ((AlongV || AlongU) && isOpposite) ? -1. : 1.;

  if (!myBaseSurf.IsNull())
    derivatives(MaxOrder, 3, theU, theV, myBaseSurf, 2, 2, AlongU, AlongV, L, DerNUV, DerSurf);
  else
    derivatives(MaxOrder, 3, theU, theV, myBaseAdaptor, 2, 2, AlongU, AlongV, L, DerNUV, DerSurf);

  CSLib::Normal(MaxOrder,
                DerNUV,
                the_D1MagTol,
                theU,
                theV,
                Umin,
                Umax,
                Vmin,
                Vmax,
                NStatus,
                Normal,
                OrderU,
                OrderV);
  if (NStatus != CSLib_Defined)
    return std::nullopt;

  aResult.theValue.SetXYZ(aResult.theValue.XYZ() + myOffset * aSign * Normal.XYZ());

  aResult.theD1U = DerSurf(1, 0) + myOffset * aSign * CSLib::DNNormal(1, 0, DerNUV, OrderU, OrderV);
  aResult.theD1V = DerSurf(0, 1) + myOffset * aSign * CSLib::DNNormal(0, 1, DerNUV, OrderU, OrderV);

  if (!myBaseSurf.IsNull())
  {
    aResult.theD2U  = myBaseSurf->DN(theU, theV, 2, 0);
    aResult.theD2V  = myBaseSurf->DN(theU, theV, 0, 2);
    aResult.theD2UV = myBaseSurf->DN(theU, theV, 1, 1);
  }
  else
  {
    aResult.theD2U  = myBaseAdaptor->DN(theU, theV, 2, 0);
    aResult.theD2V  = myBaseAdaptor->DN(theU, theV, 0, 2);
    aResult.theD2UV = myBaseAdaptor->DN(theU, theV, 1, 1);
  }

  aResult.theD2U += aSign * myOffset * CSLib::DNNormal(2, 0, DerNUV, OrderU, OrderV);
  aResult.theD2V += aSign * myOffset * CSLib::DNNormal(0, 2, DerNUV, OrderU, OrderV);
  aResult.theD2UV += aSign * myOffset * CSLib::DNNormal(1, 1, DerNUV, OrderU, OrderV);
  return aResult;
}

std::optional<GeomEvaluator_D3Result> GeomEvaluator_OffsetSurface::CalculateD3(
  const Standard_Real theU,
  const Standard_Real theV,
  const gp_Pnt&       theBaseValue,
  const gp_Vec&       theBaseD1U,
  const gp_Vec&       theBaseD1V,
  const gp_Vec&       theBaseD2U,
  const gp_Vec&       theBaseD2V,
  const gp_Vec&       theBaseD2UV,
  const gp_Vec&       theBaseD3U,
  const gp_Vec&       theBaseD3V,
  const gp_Vec&       theBaseD3UUV,
  const gp_Vec&       theBaseD3UVV) const
{
  GeomEvaluator_D3Result aResult;
  aResult.theValue = theBaseValue;

  gp_Dir             Normal;
  CSLib_NormalStatus NStatus;
  CSLib::Normal(theBaseD1U, theBaseD1V, the_D1MagTol, NStatus, Normal);
  const Standard_Integer MaxOrder = (NStatus == CSLib_Defined) ? 0 : 3;
  Standard_Integer       OrderU, OrderV;
  TColgp_Array2OfVec     DerNUV(0, MaxOrder + 3, 0, MaxOrder + 3);
  TColgp_Array2OfVec     DerSurf(0, MaxOrder + 4, 0, MaxOrder + 4);
  Standard_Real          Umin = 0, Umax = 0, Vmin = 0, Vmax = 0;
  Bounds(Umin, Umax, Vmin, Vmax);

  DerSurf.SetValue(1, 0, theBaseD1U);
  DerSurf.SetValue(0, 1, theBaseD1V);
  DerSurf.SetValue(1, 1, theBaseD2UV);
  DerSurf.SetValue(2, 0, theBaseD2U);
  DerSurf.SetValue(0, 2, theBaseD2V);
  DerSurf.SetValue(3, 0, theBaseD3U);
  DerSurf.SetValue(2, 1, theBaseD3UUV);
  DerSurf.SetValue(1, 2, theBaseD3UVV);
  DerSurf.SetValue(0, 3, theBaseD3V);

  //*********************
  Handle(Geom_BSplineSurface) L;
  Standard_Boolean            isOpposite = Standard_False;
  Standard_Boolean            AlongU     = Standard_False;
  Standard_Boolean            AlongV     = Standard_False;
  if ((NStatus != CSLib_Defined) && !myOscSurf.IsNull())
  {
    AlongU = myOscSurf->UOscSurf(theU, theV, isOpposite, L);
    AlongV = myOscSurf->VOscSurf(theU, theV, isOpposite, L);
  }
  const Standard_Real aSign = ((AlongV || AlongU) && isOpposite) ? -1. : 1.;

  if (!myBaseSurf.IsNull())
    derivatives(MaxOrder, 3, theU, theV, myBaseSurf, 3, 3, AlongU, AlongV, L, DerNUV, DerSurf);
  else
    derivatives(MaxOrder, 3, theU, theV, myBaseAdaptor, 3, 3, AlongU, AlongV, L, DerNUV, DerSurf);

  CSLib::Normal(MaxOrder,
                DerNUV,
                the_D1MagTol,
                theU,
                theV,
                Umin,
                Umax,
                Vmin,
                Vmax,
                NStatus,
                Normal,
                OrderU,
                OrderV);
  if (NStatus != CSLib_Defined)
    return std::nullopt;

  aResult.theValue.SetXYZ(aResult.theValue.XYZ() + myOffset * aSign * Normal.XYZ());

  aResult.theD1U = DerSurf(1, 0) + myOffset * aSign * CSLib::DNNormal(1, 0, DerNUV, OrderU, OrderV);
  aResult.theD1V = DerSurf(0, 1) + myOffset * aSign * CSLib::DNNormal(0, 1, DerNUV, OrderU, OrderV);

  if (!myBaseSurf.IsNull())
  {
    aResult.theD2U   = myBaseSurf->DN(theU, theV, 2, 0);
    aResult.theD2V   = myBaseSurf->DN(theU, theV, 0, 2);
    aResult.theD2UV  = myBaseSurf->DN(theU, theV, 1, 1);
    aResult.theD3U   = myBaseSurf->DN(theU, theV, 3, 0);
    aResult.theD3V   = myBaseSurf->DN(theU, theV, 0, 3);
    aResult.theD3UUV = myBaseSurf->DN(theU, theV, 2, 1);
    aResult.theD3UVV = myBaseSurf->DN(theU, theV, 1, 2);
  }
  else
  {
    aResult.theD2U   = myBaseAdaptor->DN(theU, theV, 2, 0);
    aResult.theD2V   = myBaseAdaptor->DN(theU, theV, 0, 2);
    aResult.theD2UV  = myBaseAdaptor->DN(theU, theV, 1, 1);
    aResult.theD3U   = myBaseAdaptor->DN(theU, theV, 3, 0);
    aResult.theD3V   = myBaseAdaptor->DN(theU, theV, 0, 3);
    aResult.theD3UUV = myBaseAdaptor->DN(theU, theV, 2, 1);
    aResult.theD3UVV = myBaseAdaptor->DN(theU, theV, 1, 2);
  }

  aResult.theD2U += aSign * myOffset * CSLib::DNNormal(2, 0, DerNUV, OrderU, OrderV);
  aResult.theD2V += aSign * myOffset * CSLib::DNNormal(0, 2, DerNUV, OrderU, OrderV);
  aResult.theD2UV += aSign * myOffset * CSLib::DNNormal(1, 1, DerNUV, OrderU, OrderV);
  aResult.theD3U += aSign * myOffset * CSLib::DNNormal(3, 0, DerNUV, OrderU, OrderV);
  aResult.theD3V += aSign * myOffset * CSLib::DNNormal(0, 3, DerNUV, OrderU, OrderV);
  aResult.theD3UUV += aSign * myOffset * CSLib::DNNormal(2, 1, DerNUV, OrderU, OrderV);
  aResult.theD3UVV += aSign * myOffset * CSLib::DNNormal(1, 2, DerNUV, OrderU, OrderV);
  return aResult;
}

std::optional<gp_Vec> GeomEvaluator_OffsetSurface::CalculateDN(
  const Standard_Real    theU,
  const Standard_Real    theV,
  const Standard_Integer theNu,
  const Standard_Integer theNv,
  const gp_Vec&          theD1U,
  const gp_Vec&          theD1V) const
{
  gp_Dir             Normal;
  CSLib_NormalStatus NStatus;
  CSLib::Normal(theD1U, theD1V, the_D1MagTol, NStatus, Normal);
  const Standard_Integer MaxOrder = (NStatus == CSLib_Defined) ? 0 : 3;
  Standard_Integer       OrderU, OrderV;
  TColgp_Array2OfVec     DerNUV(0, MaxOrder + theNu, 0, MaxOrder + theNv);
  TColgp_Array2OfVec     DerSurf(0, MaxOrder + theNu + 1, 0, MaxOrder + theNv + 1);

  Standard_Real Umin = 0, Umax = 0, Vmin = 0, Vmax = 0;
  Bounds(Umin, Umax, Vmin, Vmax);

  DerSurf.SetValue(1, 0, theD1U);
  DerSurf.SetValue(0, 1, theD1V);

  //*********************
  Handle(Geom_BSplineSurface) L;
  //   Is there any osculatingsurface along U or V;
  Standard_Boolean isOpposite = Standard_False;
  Standard_Boolean AlongU     = Standard_False;
  Standard_Boolean AlongV     = Standard_False;
  if ((NStatus != CSLib_Defined) && !myOscSurf.IsNull())
  {
    AlongU = myOscSurf->UOscSurf(theU, theV, isOpposite, L);
    AlongV = myOscSurf->VOscSurf(theU, theV, isOpposite, L);
  }
  const Standard_Real aSign = ((AlongV || AlongU) && isOpposite) ? -1. : 1.;

  if (!myBaseSurf.IsNull())
    derivatives(MaxOrder,
                1,
                theU,
                theV,
                myBaseSurf,
                theNu,
                theNv,
                AlongU,
                AlongV,
                L,
                DerNUV,
                DerSurf);
  else
    derivatives(MaxOrder,
                1,
                theU,
                theV,
                myBaseAdaptor,
                theNu,
                theNv,
                AlongU,
                AlongV,
                L,
                DerNUV,
                DerSurf);

  CSLib::Normal(MaxOrder,
                DerNUV,
                the_D1MagTol,
                theU,
                theV,
                Umin,
                Umax,
                Vmin,
                Vmax,
                NStatus,
                Normal,
                OrderU,
                OrderV);
  if (NStatus != CSLib_Defined)
    return std::nullopt;

  gp_Vec aResult;
  if (!myBaseSurf.IsNull())
    aResult = myBaseSurf->DN(theU, theV, theNu, theNv);
  else
    aResult = myBaseAdaptor->DN(theU, theV, theNu, theNv);

  aResult += aSign * myOffset * CSLib::DNNormal(theNu, theNv, DerNUV, OrderU, OrderV);
  return aResult;
}

void GeomEvaluator_OffsetSurface::Bounds(Standard_Real& theUMin,
                                         Standard_Real& theUMax,
                                         Standard_Real& theVMin,
                                         Standard_Real& theVMax) const
{
  if (!myBaseSurf.IsNull())
    myBaseSurf->Bounds(theUMin, theUMax, theVMin, theVMax);
  else
  {
    theUMin = myBaseAdaptor->FirstUParameter();
    theUMax = myBaseAdaptor->LastUParameter();
    theVMin = myBaseAdaptor->FirstVParameter();
    theVMax = myBaseAdaptor->LastVParameter();
  }
}

Standard_Boolean GeomEvaluator_OffsetSurface::ReplaceDerivative(
  const Standard_Real theU,
  const Standard_Real theV,
  gp_Vec&             theDU,
  gp_Vec&             theDV,
  const Standard_Real theSquareTol) const
{
  Standard_Boolean isReplaceDU = theDU.SquareMagnitude() < theSquareTol;
  Standard_Boolean isReplaceDV = theDV.SquareMagnitude() < theSquareTol;
  Standard_Boolean isReplaced  = Standard_False;
  if (isReplaceDU ^ isReplaceDV)
  {
    Standard_Real aU    = theU;
    Standard_Real aV    = theV;
    Standard_Real aUMin = 0, aUMax = 0, aVMin = 0, aVMax = 0;
    Bounds(aUMin, aUMax, aVMin, aVMax);

    // Calculate step along non-zero derivative
    Standard_Real             aStep;
    Handle(Adaptor3d_Surface) aSurfAdapt;
    if (!myBaseAdaptor.IsNull())
      aSurfAdapt = myBaseAdaptor;
    else
      aSurfAdapt = new GeomAdaptor_Surface(myBaseSurf);
    if (isReplaceDV)
    {
      aStep = Precision::Confusion() * theDU.Magnitude();
      if (aStep > aUMax - aUMin)
        aStep = (aUMax - aUMin) / 100.;
    }
    else
    {
      aStep = Precision::Confusion() * theDV.Magnitude();
      if (aStep > aVMax - aVMin)
        aStep = (aVMax - aVMin) / 100.;
    }

    gp_Pnt aP;
    gp_Vec aDU, aDV;
    // Step away from current parametric coordinates and calculate derivatives once again.
    // Replace zero derivative by the obtained.
    for (Standard_Real aStepSign = -1.0; aStepSign <= 1.0 && !isReplaced; aStepSign += 2.0)
    {
      if (isReplaceDV)
      {
        aU = theU + aStepSign * aStep;
        if (aU < aUMin || aU > aUMax)
          continue;
      }
      else
      {
        aV = theV + aStepSign * aStep;
        if (aV < aVMin || aV > aVMax)
          continue;
      }

      BaseD1(aU, aV, aP, aDU, aDV);

      if (isReplaceDU && aDU.SquareMagnitude() > theSquareTol)
      {
        theDU      = aDU;
        isReplaced = Standard_True;
      }
      if (isReplaceDV && aDV.SquareMagnitude() > theSquareTol)
      {
        theDV      = aDV;
        isReplaced = Standard_True;
      }
    }
  }
  return isReplaced;
}
