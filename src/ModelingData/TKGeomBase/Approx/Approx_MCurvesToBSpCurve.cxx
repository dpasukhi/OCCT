// Copyright (c) 1995-1999 Matra Datavision
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

#include <AppParCurves_MultiPoint.hxx>
#include <Approx_MCurvesToBSpCurve.hxx>
#include <BSplCLib.hxx>
#include <Convert_CompBezierCurvesToBSplineCurve.hxx>
#include <Convert_CompBezierCurves2dToBSplineCurve2d.hxx>
#include <Standard_Integer.hxx>
#include <NCollection_Array1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

Approx_MCurvesToBSpCurve::Approx_MCurvesToBSpCurve()
{
  myDone = false;
}

void Approx_MCurvesToBSpCurve::Reset()
{
  myDone = false;
  myCurves.Clear();
}

void Approx_MCurvesToBSpCurve::Append(const AppParCurves_MultiCurve& MC)
{
  myCurves.Append(MC);
}

void Approx_MCurvesToBSpCurve::Perform()
{
  Perform(myCurves);
}

void Approx_MCurvesToBSpCurve::Perform(
  const NCollection_Sequence<AppParCurves_MultiCurve>& theCurves)
{
  // All components must retain the same parameterization. Smoothing the first
  // 3D component alone and copying its knot multiplicities to the PCurves changes
  // those PCurves, even when each individual fit meets its tolerance.
  const int aCount  = theCurves.Length();
  int       aDegree = 0;
  for (const auto& aCurve : theCurves)
  {
    aDegree = std::max(aDegree, aCurve.Degree());
  }
  const auto&                                 aFirst = theCurves.First().Value(1);
  const int                                   aNb3d  = aFirst.NbPoints();
  const int                                   aNb2d  = aFirst.NbPoints2d();
  NCollection_Array1<AppParCurves_MultiPoint> aPoles(1, aCount * aDegree + 1);
  for (auto& aPole : aPoles)
  {
    aPole = AppParCurves_MultiPoint(aNb3d, aNb2d);
  }
  NCollection_Array1<double> aKnots(1, aCount + 1);
  NCollection_Array1<int>    aMults(1, aCount + 1);
  aMults.Init(aDegree);
  aMults(1) = aMults(aCount + 1) = aDegree + 1;
  // Retain the speed-based parameter spacing, without adopting the primary
  // component's knot removal for unrelated components.
  NCollection_Array1<int> aSuggestedMults(1, aCount + 1);
  if (aNb3d != 0)
  {
    Convert_CompBezierCurvesToBSplineCurve aSpacing;
    for (const auto& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt> aPoints(1, aCurve.NbPoles());
      aCurve.Curve(1, aPoints);
      aSpacing.AddCurve(aPoints);
    }
    aSpacing.Perform();
    aSpacing.KnotsAndMults(aKnots, aSuggestedMults);
  }
  else
  {
    Convert_CompBezierCurves2dToBSplineCurve2d aSpacing;
    for (const auto& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt2d> aPoints(1, aCurve.NbPoles());
      aCurve.Curve(1, aPoints);
      aSpacing.AddCurve(aPoints);
    }
    aSpacing.Perform();
    aSpacing.KnotsAndMults(aKnots, aSuggestedMults);
  }
  for (int i = 1; i <= aCount; ++i)
  {
    const auto& aCurve     = theCurves.Value(i);
    const int   aFirstPole = i == 1 ? 1 : 2;
    for (int c = 1; c <= aNb3d; ++c)
    {
      NCollection_Array1<gp_Pnt> aSource(1, aCurve.NbPoles());
      NCollection_Array1<gp_Pnt> aTarget(1, aDegree + 1);
      aCurve.Curve(c, aSource);
      if (aCurve.Degree() == aDegree)
      {
        aTarget = aSource;
      }
      else
      {
        BSplCLib::IncreaseDegree(aDegree,
                                 aSource,
                                 BSplCLib::NoWeights(),
                                 aTarget,
                                 BSplCLib::NoWeights());
      }
      for (int k = aFirstPole; k <= aDegree + 1; ++k)
      {
        aPoles((i - 1) * aDegree + k).SetPoint(c, aTarget(k));
      }
    }
    for (int c = 1; c <= aNb2d; ++c)
    {
      NCollection_Array1<gp_Pnt2d> aSource(1, aCurve.NbPoles());
      NCollection_Array1<gp_Pnt2d> aTarget(1, aDegree + 1);
      aCurve.Curve(aNb3d + c, aSource);
      if (aCurve.Degree() == aDegree)
      {
        aTarget = aSource;
      }
      else
      {
        BSplCLib::IncreaseDegree(aDegree,
                                 aSource,
                                 BSplCLib::NoWeights(),
                                 aTarget,
                                 BSplCLib::NoWeights());
      }
      for (int k = aFirstPole; k <= aDegree + 1; ++k)
      {
        aPoles((i - 1) * aDegree + k).SetPoint2d(aNb3d + c, aTarget(k));
      }
    }
  }
  mySpline = AppParCurves_MultiBSpCurve(aPoles, aKnots, aMults);
  myDone = true;
}

const AppParCurves_MultiBSpCurve& Approx_MCurvesToBSpCurve::Value() const
{
  return mySpline;
}

const AppParCurves_MultiBSpCurve& Approx_MCurvesToBSpCurve::ChangeValue()
{
  return mySpline;
}
