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
#include <NCollection_Array1.hxx>
#include <Precision.hxx>
#include <Standard_Integer.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

#include <algorithm>
#include <cmath>

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
  myDone   = false;
  mySpline = AppParCurves_MultiBSpCurve();

  const int aNbSegments = theCurves.Length();
  if (aNbSegments == 0)
  {
    return;
  }

  const AppParCurves_MultiCurve& aFirstCurve = theCurves.First();
  if (aFirstCurve.NbPoles() == 0)
  {
    return;
  }

  const AppParCurves_MultiPoint& aFirstPoint = aFirstCurve.Value(1);
  const int                      aNb3d        = aFirstPoint.NbPoints();
  const int                      aNb2d        = aFirstPoint.NbPoints2d();
  if (aNb3d + aNb2d == 0)
  {
    return;
  }

  int aDegree = 0;
  for (const AppParCurves_MultiCurve& aCurve : theCurves)
  {
    if (aCurve.NbPoles() == 0 || aCurve.NbCurves() != aNb3d + aNb2d)
    {
      return;
    }

    const AppParCurves_MultiPoint& aPoint = aCurve.Value(1);
    if (aPoint.NbPoints() != aNb3d || aPoint.NbPoints2d() != aNb2d)
    {
      return;
    }
    aDegree = std::max(aDegree, aCurve.Degree());
  }

  // Convert_CompBezierCurves* works with adjacent Bezier segments. Check all
  // components, otherwise sharing a B-spline junction would alter geometry.
  for (int aSegment = 2; aSegment <= aNbSegments; ++aSegment)
  {
    const AppParCurves_MultiCurve& aPrevious = theCurves.Value(aSegment - 1);
    const AppParCurves_MultiCurve& aCurrent  = theCurves.Value(aSegment);
    const AppParCurves_MultiPoint& aPreviousEnd = aPrevious.Value(aPrevious.NbPoles());
    const AppParCurves_MultiPoint& aCurrentStart = aCurrent.Value(1);

    for (int aComponent = 1; aComponent <= aNb3d; ++aComponent)
    {
      if (aPreviousEnd.Point(aComponent).Distance(aCurrentStart.Point(aComponent))
          > Precision::Confusion())
      {
        return;
      }
    }
    for (int aComponent = 1; aComponent <= aNb2d; ++aComponent)
    {
      const int aPointIndex = aNb3d + aComponent;
      if (aPreviousEnd.Point2d(aPointIndex).Distance(aCurrentStart.Point2d(aPointIndex))
          > Precision::PConfusion())
      {
        return;
      }
    }
  }

  if (aNbSegments == 1)
  {
    NCollection_Array1<double> aKnots(1, 2);
    NCollection_Array1<int>    aMults(1, 2);
    aKnots(1) = 0.0;
    aKnots(2) = 1.0;
    aMults.Init(aDegree + 1);
    mySpline = AppParCurves_MultiBSpCurve(aFirstCurve, aKnots, aMults);
    myDone   = true;
    return;
  }

  // Use one component to define a candidate smooth knot structure. It may be
  // shared only if every other component independently produces the same one.
  NCollection_Array1<double> aKnots(1, aNbSegments + 1);
  NCollection_Array1<int>    aReferenceMults(1, aNbSegments + 1);
  if (aNb3d != 0)
  {
    Convert_CompBezierCurvesToBSplineCurve aConverter;
    for (const AppParCurves_MultiCurve& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt> aPoles(1, aCurve.NbPoles());
      aCurve.Curve(1, aPoles);
      aConverter.AddCurve(aPoles);
    }
    aConverter.Perform();
    aConverter.KnotsAndMults(aKnots, aReferenceMults);
  }
  else
  {
    Convert_CompBezierCurves2dToBSplineCurve2d aConverter;
    for (const AppParCurves_MultiCurve& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt2d> aPoles(1, aCurve.NbPoles());
      aCurve.Curve(1, aPoles);
      aConverter.AddCurve(aPoles);
    }
    aConverter.Perform();
    aConverter.KnotsAndMults(aKnots, aReferenceMults);
  }

  const auto hasSameStructure = [&](const auto& theConverter) {
    if (theConverter.Degree() != aDegree || theConverter.NbKnots() != aKnots.Length())
    {
      return false;
    }

    NCollection_Array1<double> aComponentKnots(1, aKnots.Length());
    NCollection_Array1<int>    aComponentMults(1, aReferenceMults.Length());
    theConverter.KnotsAndMults(aComponentKnots, aComponentMults);
    for (int aKnot = 1; aKnot <= aKnots.Length(); ++aKnot)
    {
      if (aComponentMults(aKnot) != aReferenceMults(aKnot)
          || std::abs(aComponentKnots(aKnot) - aKnots(aKnot)) > Precision::PConfusion())
      {
        return false;
      }
    }
    return true;
  };

  bool isCommonStructure = true;
  for (int aComponent = 1; aComponent <= aNb3d && isCommonStructure; ++aComponent)
  {
    if (aNb3d != 0 && aComponent == 1)
    {
      continue;
    }

    Convert_CompBezierCurvesToBSplineCurve aConverter;
    for (const AppParCurves_MultiCurve& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt> aPoles(1, aCurve.NbPoles());
      aCurve.Curve(aComponent, aPoles);
      aConverter.AddCurve(aPoles);
    }
    aConverter.Perform();
    isCommonStructure = hasSameStructure(aConverter);
  }

  for (int aComponent = 1; aComponent <= aNb2d && isCommonStructure; ++aComponent)
  {
    if (aNb3d == 0 && aComponent == 1)
    {
      continue;
    }

    Convert_CompBezierCurves2dToBSplineCurve2d aConverter;
    for (const AppParCurves_MultiCurve& aCurve : theCurves)
    {
      NCollection_Array1<gp_Pnt2d> aPoles(1, aCurve.NbPoles());
      aCurve.Curve(aNb3d + aComponent, aPoles);
      aConverter.AddCurve(aPoles);
    }
    aConverter.Perform();
    isCommonStructure = hasSameStructure(aConverter);
  }

  NCollection_Array1<int> aMults(1, aReferenceMults.Length());
  if (isCommonStructure)
  {
    aMults = aReferenceMults;
  }
  else
  {
    // Different components require different C1 parameterizations. Keep each
    // Bezier segment exact and use C0 junctions with the reference knot spacing.
    aMults.Init(aDegree);
    aMults(1) = aMults(aMults.Upper()) = aDegree + 1;
  }

  int aNbPoles = -aDegree - 1;
  for (int aMultiplicity : aMults)
  {
    aNbPoles += aMultiplicity;
  }

  NCollection_Array1<AppParCurves_MultiPoint> aPoles(1, aNbPoles);
  for (AppParCurves_MultiPoint& aPole : aPoles)
  {
    aPole = AppParCurves_MultiPoint(aNb3d, aNb2d);
  }

  for (int aComponent = 1; aComponent <= aNb3d; ++aComponent)
  {
    int aResultPole = 1;
    for (int aSegment = 1; aSegment <= aNbSegments; ++aSegment)
    {
      const AppParCurves_MultiCurve& aCurve = theCurves.Value(aSegment);
      NCollection_Array1<gp_Pnt>     aSource(1, aCurve.NbPoles());
      NCollection_Array1<gp_Pnt>     aTarget(1, aDegree + 1);
      aCurve.Curve(aComponent, aSource);
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

      int aFirstPole = 1;
      if (aSegment > 1 && (aMults(aSegment) == aDegree - 1 || aMults(aSegment) == aDegree))
      {
        aFirstPole = 2;
      }
      int aLastPole = aDegree;
      if (aSegment == aNbSegments || aMults(aSegment + 1) == aDegree)
      {
        aLastPole = aDegree + 1;
      }
      for (int aPole = aFirstPole; aPole <= aLastPole; ++aPole)
      {
        aPoles(aResultPole++).SetPoint(aComponent, aTarget(aPole));
      }
    }
  }

  for (int aComponent = 1; aComponent <= aNb2d; ++aComponent)
  {
    int aResultPole = 1;
    for (int aSegment = 1; aSegment <= aNbSegments; ++aSegment)
    {
      const AppParCurves_MultiCurve& aCurve = theCurves.Value(aSegment);
      NCollection_Array1<gp_Pnt2d>   aSource(1, aCurve.NbPoles());
      NCollection_Array1<gp_Pnt2d>   aTarget(1, aDegree + 1);
      aCurve.Curve(aNb3d + aComponent, aSource);
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

      int aFirstPole = 1;
      if (aSegment > 1 && (aMults(aSegment) == aDegree - 1 || aMults(aSegment) == aDegree))
      {
        aFirstPole = 2;
      }
      int aLastPole = aDegree;
      if (aSegment == aNbSegments || aMults(aSegment + 1) == aDegree)
      {
        aLastPole = aDegree + 1;
      }
      for (int aPole = aFirstPole; aPole <= aLastPole; ++aPole)
      {
        aPoles(aResultPole++).SetPoint2d(aNb3d + aComponent, aTarget(aPole));
      }
    }
  }

  mySpline = AppParCurves_MultiBSpCurve(aPoles, aKnots, aMults);
  myDone   = true;
}

bool Approx_MCurvesToBSpCurve::IsDone() const
{
  return myDone;
}

const AppParCurves_MultiBSpCurve& Approx_MCurvesToBSpCurve::Value() const
{
  return mySpline;
}

const AppParCurves_MultiBSpCurve& Approx_MCurvesToBSpCurve::ChangeValue()
{
  return mySpline;
}
