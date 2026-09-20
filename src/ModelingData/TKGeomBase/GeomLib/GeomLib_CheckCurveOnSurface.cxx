// Created by: Nikolai BUKHALOV
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

#include <GeomLib_CheckCurveOnSurface.hxx>

#include <Adaptor3d_Curve.hxx>
#include <Adaptor3d_CurveOnSurface.hxx>
#include <ElCLib.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_XYZ.hxx>
#include <math_TrigonometricFunctionRoots.hxx>
#include <MathOpt_PSO.hxx>
#include <OSD_Parallel.hxx>
#include <Standard_ErrorHandler.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_HArray1.hxx>

#include <algorithm>

typedef NCollection_Array1<occ::handle<Adaptor3d_Curve>> Array1OfHCurve;

class GeomLib_CheckCurveOnSurface_TargetFunc;

static bool MinComputing(GeomLib_CheckCurveOnSurface_TargetFunc& theFunction,
                         const int                               theNbParticles,
                         double&                                 theBestValue,
                         double&                                 theBestParameter);

static int FillSubIntervals(const occ::handle<Adaptor3d_Curve>&   theCurve3d,
                            const occ::handle<Adaptor2d_Curve2d>& theCurve2d,
                            const double                          theFirst,
                            const double                          theLast,
                            int&                                  theNbParticles,
                            NCollection_Array1<double>* const     theSubIntervals = nullptr);

//=================================================================================================

class GeomLib_CheckCurveOnSurface_TargetFunc
{
public:
  GeomLib_CheckCurveOnSurface_TargetFunc(const Adaptor3d_Curve& theC3D,
                                         const Adaptor3d_Curve& theCurveOnSurface,
                                         const double           theFirst,
                                         const double           theLast)
      : myCurve1(theC3D),
        myCurve2(theCurveOnSurface),
        myFirst(theFirst),
        myLast(theLast)
  {
  }

  //! The optimizer works on [0, 1], independently of the curve's parameter scale.
  bool Value(const math_Vector& theX, double& theValue) const
  {
    return Value(Parameter(theX.At(0)), theValue);
  }

  double Parameter(const double theUnitParameter) const
  {
    return theUnitParameter == 1
             ? myLast
             : std::clamp(myFirst + theUnitParameter * (myLast - myFirst), myFirst, myLast);
  }

  // returns value of the one-dimension-function when parameter
  // is equal to theX
  bool Value(const double theX, double& theFVal) const
  {
    if (!CheckParameter(theX))
    {
      return false;
    }
    theFVal = -myCurve1.Value(theX).SquareDistance(myCurve2.Value(theX));
    return Precision::IsFinite(theFVal);
  }

  // Computes the exact maximum for elementary compositions supported by the adaptors.
  bool ElementaryMaximum(double& theBestValue, double& theBestParameter) const
  {
    const GeomAbs_CurveType aType1 = myCurve1.GetType();
    const GeomAbs_CurveType aType2 = myCurve2.GetType();
    if (aType1 == GeomAbs_Line && aType2 == GeomAbs_Line)
    {
      double aFirstValue = RealLast(), aLastValue = RealLast();
      if (!Value(myFirst, aFirstValue) || !Value(myLast, aLastValue))
      {
        return false;
      }

      if (aFirstValue <= aLastValue)
      {
        theBestValue     = aFirstValue;
        theBestParameter = myFirst;
      }
      else
      {
        theBestValue     = aLastValue;
        theBestParameter = myLast;
      }
      return true;
    }

    const bool isTrigType1 = aType1 == GeomAbs_Circle || aType1 == GeomAbs_Ellipse;
    const bool isTrigType2 = aType2 == GeomAbs_Circle || aType2 == GeomAbs_Ellipse;
    if (!isTrigType1 || !isTrigType2)
    {
      return false;
    }

    struct Coefficients
    {
      gp_XYZ Origin;
      gp_XYZ Cos;
      gp_XYZ Sin;
    };

    const auto getCoefficients = [](const Adaptor3d_Curve&  theCurve,
                                    const GeomAbs_CurveType theType) -> Coefficients {
      if (theType == GeomAbs_Circle)
      {
        const gp_Circ aCircle = theCurve.Circle();
        return {aCircle.Location().XYZ(),
                aCircle.XAxis().Direction().XYZ() * aCircle.Radius(),
                aCircle.YAxis().Direction().XYZ() * aCircle.Radius()};
      }

      const gp_Elips anEllipse = theCurve.Ellipse();
      return {anEllipse.Location().XYZ(),
              anEllipse.XAxis().Direction().XYZ() * anEllipse.MajorRadius(),
              anEllipse.YAxis().Direction().XYZ() * anEllipse.MinorRadius()};
    };

    const auto [anOrigin1, aCosCoeff1, aSinCoeff1] = getCoefficients(myCurve1, aType1);
    const auto [anOrigin2, aCosCoeff2, aSinCoeff2] = getCoefficients(myCurve2, aType2);

    const gp_XYZ anOffset  = anOrigin1 - anOrigin2;
    const gp_XYZ aCosCoeff = aCosCoeff1 - aCosCoeff2;
    const gp_XYZ aSinCoeff = aSinCoeff1 - aSinCoeff2;

    // Coefficients of the derivative of the squared distance in trigonometric form.
    const double aCosCos   = 2.0 * aCosCoeff.Dot(aSinCoeff);
    const double aCosSin   = 0.5 * (aSinCoeff.SquareModulus() - aCosCoeff.SquareModulus());
    const double aCos      = anOffset.Dot(aSinCoeff);
    const double aSin      = -anOffset.Dot(aCosCoeff);
    const double aConstant = -aCosCoeff.Dot(aSinCoeff);

    // Exact zero is required here: a tolerance could classify a varying distance as constant.
    if (aCosCos == 0.0 && aCosSin == 0.0 && aCos == 0.0 && aSin == 0.0 && aConstant == 0.0)
    {
      theBestParameter = myFirst;
      return Value(myFirst, theBestValue);
    }

    const double aPeriod = 2.0 * M_PI;
    const double aFirst  = ElCLib::InPeriod(myFirst, 0.0, aPeriod);
    const double aShift  = aFirst - myFirst;
    const double aLast   = aFirst + std::min(myLast - myFirst, aPeriod);
    const double aScale  = std::max(
      {std::abs(aCosCos), std::abs(aCosSin), std::abs(aCos), std::abs(aSin), std::abs(aConstant)});
    math_TrigonometricFunctionRoots aRoots(aCosCos / aScale,
                                           aCosSin / aScale,
                                           aCos / aScale,
                                           aSin / aScale,
                                           aConstant / aScale,
                                           aFirst,
                                           aLast);
    if (!aRoots.IsDone() || aRoots.InfiniteRoots()
        || (myLast - myFirst >= aPeriod && aRoots.NbSolutions() == 0))
    {
      return false;
    }

    theBestValue          = RealLast();
    theBestParameter      = myFirst;
    const auto updateBest = [&](const double theParameter) {
      double aValue = RealLast();
      if (!Value(theParameter, aValue))
      {
        return false;
      }
      if (aValue < theBestValue)
      {
        theBestValue     = aValue;
        theBestParameter = theParameter;
      }
      return true;
    };

    if (!updateBest(myFirst) || !updateBest(myLast))
    {
      return false;
    }
    for (int anIndex = 1; anIndex <= aRoots.NbSolutions(); ++anIndex)
    {
      if (!updateBest(aRoots.Value(anIndex) - aShift))
      {
        return false;
      }
    }
    return true;
  }

private:
  GeomLib_CheckCurveOnSurface_TargetFunc operator=(GeomLib_CheckCurveOnSurface_TargetFunc&) =
    delete;

  // checks if the function can be computed when its parameter is
  // equal to theParam
  bool CheckParameter(const double theParam) const
  {
    return ((myFirst <= theParam) && (theParam <= myLast));
  }

  const Adaptor3d_Curve& myCurve1;
  const Adaptor3d_Curve& myCurve2;
  const double           myFirst;
  const double           myLast;
};

//=================================================================================================

class GeomLib_CheckCurveOnSurface_Local
{
public:
  GeomLib_CheckCurveOnSurface_Local(const Array1OfHCurve&             theCurveArray,
                                    const Array1OfHCurve&             theCurveOnSurfaceArray,
                                    const NCollection_Array1<double>& theIntervalsArr,
                                    const int                         theNbParticles)
      : myCurveArray(theCurveArray),
        myCurveOnSurfaceArray(theCurveOnSurfaceArray),
        mySubIntervals(theIntervalsArr),
        myNbParticles(theNbParticles),
        myArrOfDist(theIntervalsArr.Lower(), theIntervalsArr.Upper() - 1),
        myArrOfParam(theIntervalsArr.Lower(), theIntervalsArr.Upper() - 1)
  {
  }

  void operator()(int theThreadIndex, int theElemIndex) const
  {
    // For every sub-interval (which is set by mySubIntervals array) this method
    // computes optimal value of GeomLib_CheckCurveOnSurface_TargetFunc function.
    // This optimal value will be put in corresponding (depending on theIndex - the
    // identificator of the current interval in mySubIntervals array) cell of
    // myArrOfDist and myArrOfParam arrays.
    GeomLib_CheckCurveOnSurface_TargetFunc aFunc(
      *(myCurveArray.Value(theThreadIndex).get()),
      *(myCurveOnSurfaceArray.Value(theThreadIndex).get()),
      mySubIntervals.Value(theElemIndex),
      mySubIntervals.Value(theElemIndex + 1));

    double aMinDist = RealLast(), aPar = 0.0;
    if (!MinComputing(aFunc, myNbParticles, aMinDist, aPar))
    {
      myArrOfDist(theElemIndex)  = RealLast();
      myArrOfParam(theElemIndex) = mySubIntervals.Value(theElemIndex);
      return;
    }

    myArrOfDist(theElemIndex)  = aMinDist;
    myArrOfParam(theElemIndex) = aPar;
  }

  // Returns optimal value (inverse of square of maximal distance)
  bool OptimalValues(double& theMinimalValue, double& theParameter) const
  {
    // This method looks for the minimal value of myArrOfDist.

    const int aStartInd = myArrOfDist.Lower();
    theMinimalValue     = myArrOfDist(aStartInd);
    theParameter        = myArrOfParam(aStartInd);
    for (int i = aStartInd; i <= myArrOfDist.Upper(); i++)
    {
      if (myArrOfDist(i) == RealLast())
      {
        return false;
      }
      if (myArrOfDist(i) < theMinimalValue)
      {
        theMinimalValue = myArrOfDist(i);
        theParameter    = myArrOfParam(i);
      }
    }
    return true;
  }

private:
  GeomLib_CheckCurveOnSurface_Local operator=(const GeomLib_CheckCurveOnSurface_Local&) = delete;

private:
  const Array1OfHCurve& myCurveArray;
  const Array1OfHCurve& myCurveOnSurfaceArray;

  const NCollection_Array1<double>&  mySubIntervals;
  const int                          myNbParticles;
  mutable NCollection_Array1<double> myArrOfDist;
  mutable NCollection_Array1<double> myArrOfParam;
};

//=================================================================================================

GeomLib_CheckCurveOnSurface::GeomLib_CheckCurveOnSurface()
    : myErrorStatus(0),
      myMaxDistance(RealLast()),
      myMaxParameter(0.),
      myTolRange(Precision::PConfusion()),
      myIsParallel(false)
{
}

//=================================================================================================

GeomLib_CheckCurveOnSurface::GeomLib_CheckCurveOnSurface(
  const occ::handle<Adaptor3d_Curve>& theCurve,
  const double                        theTolRange)
    : myCurve(theCurve),
      myErrorStatus(0),
      myMaxDistance(RealLast()),
      myMaxParameter(0.),
      myTolRange(theTolRange),
      myIsParallel(false)
{
}

//=================================================================================================

void GeomLib_CheckCurveOnSurface::Init()
{
  myCurve.Nullify();
  myErrorStatus  = 0;
  myMaxDistance  = RealLast();
  myMaxParameter = 0.0;
  myTolRange     = Precision::PConfusion();
}

//=================================================================================================

void GeomLib_CheckCurveOnSurface::Init(const occ::handle<Adaptor3d_Curve>& theCurve,
                                       const double                        theTolRange)
{
  myCurve        = theCurve;
  myErrorStatus  = 0;
  myMaxDistance  = RealLast();
  myMaxParameter = 0.0;
  myTolRange     = theTolRange;
}

//=================================================================================================

void GeomLib_CheckCurveOnSurface::Perform(
  const occ::handle<Adaptor3d_CurveOnSurface>& theCurveOnSurface)
{
  if (myCurve.IsNull() || theCurveOnSurface.IsNull())
  {
    myErrorStatus = 1;
    return;
  }

  const double aFirst = myCurve->FirstParameter();
  const double aLast  = myCurve->LastParameter();
  if (!Precision::IsFinite(aFirst) || !Precision::IsFinite(aLast) || !(aFirst < aLast)
      || (aFirst - theCurveOnSurface->FirstParameter() > myTolRange)
      || (aLast - theCurveOnSurface->LastParameter() < -myTolRange))
  {
    myErrorStatus = 2;
    return;
  }

  int aNbParticles = 3;

  // Polynomial degree guides the initial search density on each knot interval.
  const int aNbSubIntervals =
    FillSubIntervals(myCurve, theCurveOnSurface->GetCurve(), aFirst, aLast, aNbParticles);

  if (!aNbSubIntervals)
  {
    myErrorStatus = 3;
    return;
  }

  try
  {
    OCC_CATCH_SIGNALS

    NCollection_Array1<double> anIntervals(1, aNbSubIntervals + 1);
    FillSubIntervals(myCurve,
                     theCurveOnSurface->GetCurve(),
                     aFirst,
                     aLast,
                     aNbParticles,
                     &anIntervals);

    const int aNbThreads =
      myIsParallel
        ? std::min(anIntervals.Length(), OSD_ThreadPool::DefaultPool()->NbDefaultThreadsToLaunch())
        : 1;
    Array1OfHCurve aCurveArray(0, aNbThreads - 1);
    Array1OfHCurve aCurveOnSurfaceArray(0, aNbThreads - 1);
    for (int anI = 0; anI < aNbThreads; ++anI)
    {
      aCurveArray.SetValue(anI, aNbThreads > 1 ? myCurve->ShallowCopy() : myCurve);
      aCurveOnSurfaceArray.SetValue(
        anI,
        aNbThreads > 1 ? theCurveOnSurface->ShallowCopy()
                       : static_cast<const occ::handle<Adaptor3d_Curve>&>(theCurveOnSurface));
    }
    GeomLib_CheckCurveOnSurface_Local aComp(aCurveArray,
                                            aCurveOnSurfaceArray,
                                            anIntervals,
                                            aNbParticles);
    if (aNbThreads > 1)
    {
      const occ::handle<OSD_ThreadPool>& aThreadPool = OSD_ThreadPool::DefaultPool();
      OSD_ThreadPool::Launcher           aLauncher(*aThreadPool, aNbThreads);
      aLauncher.Perform(anIntervals.Lower(), anIntervals.Upper(), aComp);
    }
    else
    {
      for (int anI = anIntervals.Lower(); anI < anIntervals.Upper(); ++anI)
      {
        aComp(0, anI);
      }
    }
    if (!aComp.OptimalValues(myMaxDistance, myMaxParameter))
    {
      myErrorStatus = 3;
      return;
    }

    myMaxDistance = sqrt(std::abs(myMaxDistance));
  }
  catch (Standard_Failure const&)
  {
    myErrorStatus = 3;
  }
}

//=================================================================================================

int FillSubIntervals(const occ::handle<Adaptor3d_Curve>&   theCurve3d,
                     const occ::handle<Adaptor2d_Curve2d>& theCurve2d,
                     const double                          theFirst,
                     const double                          theLast,
                     int&                                  theNbParticles,
                     NCollection_Array1<double>* const     theSubIntervals)
{
  const int                        aMaxKnots     = 101;
  const double                     anArrTempC[2] = {theFirst, theLast};
  const NCollection_Array1<double> anArrTemp(anArrTempC[0], 1, 2);

  theNbParticles = 3;
  occ::handle<Geom2d_BSplineCurve> aBS2DCurv;
  occ::handle<Geom_BSplineCurve>   aBS3DCurv;
  bool                             isTrimmed3D = false, isTrimmed2D = false;

  //
  if (theCurve3d->GetType() == GeomAbs_BSplineCurve)
  {
    aBS3DCurv = theCurve3d->BSpline();
  }
  if (theCurve2d->GetType() == GeomAbs_BSplineCurve)
  {
    aBS2DCurv = theCurve2d->BSpline();
  }

  occ::handle<NCollection_HArray1<double>> anArrKnots3D, anArrKnots2D;

  if (!aBS3DCurv.IsNull())
  {
    if (aBS3DCurv->NbKnots() <= aMaxKnots)
    {
      anArrKnots3D = new NCollection_HArray1<double>(aBS3DCurv->Knots());
    }
    else
    {
      int KnotCount;
      if (isTrimmed3D)
      {
        int i;
        KnotCount                                = 0;
        const NCollection_Array1<double>& aKnots = aBS3DCurv->Knots();
        for (i = aBS3DCurv->FirstUKnotIndex(); i <= aBS3DCurv->LastUKnotIndex(); ++i)
        {
          if (aKnots(i) > theFirst && aKnots(i) < theLast)
          {
            ++KnotCount;
          }
        }
        KnotCount += 2;
      }
      else
      {
        KnotCount = aBS3DCurv->LastUKnotIndex() - aBS3DCurv->FirstUKnotIndex() + 1;
      }
      if (KnotCount <= aMaxKnots)
      {
        anArrKnots3D = new NCollection_HArray1<double>(aBS3DCurv->Knots());
      }
      else
      {
        anArrKnots3D = new NCollection_HArray1<double>(1, aMaxKnots);
        anArrKnots3D->SetValue(1, theFirst);
        anArrKnots3D->SetValue(aMaxKnots, theLast);
        int    i;
        double dt = (theLast - theFirst) / (aMaxKnots - 1);
        double t  = theFirst + dt;
        for (i = 2; i < aMaxKnots; ++i, t += dt)
        {
          anArrKnots3D->SetValue(i, t);
        }
      }
    }
  }
  else
  {
    anArrKnots3D = new NCollection_HArray1<double>(anArrTemp);
  }
  if (!aBS2DCurv.IsNull())
  {
    if (aBS2DCurv->NbKnots() <= aMaxKnots)
    {
      anArrKnots2D = new NCollection_HArray1<double>(aBS2DCurv->Knots());
    }
    else
    {
      int KnotCount;
      if (isTrimmed2D)
      {
        int i;
        KnotCount                                = 0;
        const NCollection_Array1<double>& aKnots = aBS2DCurv->Knots();
        for (i = aBS2DCurv->FirstUKnotIndex(); i <= aBS2DCurv->LastUKnotIndex(); ++i)
        {
          if (aKnots(i) > theFirst && aKnots(i) < theLast)
          {
            ++KnotCount;
          }
        }
        KnotCount += 2;
      }
      else
      {
        KnotCount = aBS2DCurv->LastUKnotIndex() - aBS2DCurv->FirstUKnotIndex() + 1;
      }
      if (KnotCount <= aMaxKnots)
      {
        anArrKnots2D = new NCollection_HArray1<double>(aBS2DCurv->Knots());
      }
      else
      {
        anArrKnots2D = new NCollection_HArray1<double>(1, aMaxKnots);
        anArrKnots2D->SetValue(1, theFirst);
        anArrKnots2D->SetValue(aMaxKnots, theLast);
        int    i;
        double dt = (theLast - theFirst) / (aMaxKnots - 1);
        double t  = theFirst + dt;
        for (i = 2; i < aMaxKnots; ++i, t += dt)
        {
          anArrKnots2D->SetValue(i, t);
        }
      }
    }
  }
  else
  {
    anArrKnots2D = new NCollection_HArray1<double>(anArrTemp);
  }

  int aNbSubIntervals = 1;

  try
  {
    OCC_CATCH_SIGNALS
    const int anIndMax3D = anArrKnots3D->Upper(), anIndMax2D = anArrKnots2D->Upper();

    int anIndex3D = anArrKnots3D->Lower(), anIndex2D = anArrKnots2D->Lower();

    if (theSubIntervals)
    {
      theSubIntervals->ChangeValue(aNbSubIntervals) = theFirst;
    }

    while ((anIndex3D <= anIndMax3D) && (anIndex2D <= anIndMax2D))
    {
      const double aVal3D = anArrKnots3D->Value(anIndex3D), aVal2D = anArrKnots2D->Value(anIndex2D);
      const double aDelta = aVal3D - aVal2D;

      if (aDelta < Precision::PConfusion())
      { // aVal3D <= aVal2D
        if ((aVal3D > theFirst) && (aVal3D < theLast))
        {
          aNbSubIntervals++;

          if (theSubIntervals)
          {
            theSubIntervals->ChangeValue(aNbSubIntervals) = aVal3D;
          }
        }

        anIndex3D++;

        if (-aDelta < Precision::PConfusion())
        { // aVal3D == aVal2D
          anIndex2D++;
        }
      }
      else
      { // aVal2D < aVal3D
        if ((aVal2D > theFirst) && (aVal2D < theLast))
        {
          aNbSubIntervals++;

          if (theSubIntervals)
          {
            theSubIntervals->ChangeValue(aNbSubIntervals) = aVal2D;
          }
        }

        anIndex2D++;
      }
    }

    if (theSubIntervals)
    {
      theSubIntervals->ChangeValue(aNbSubIntervals + 1) = theLast;
    }

    if (!aBS3DCurv.IsNull() || theCurve3d->GetType() == GeomAbs_BezierCurve)
    {
      theNbParticles =
        std::max(theNbParticles, aBS3DCurv.IsNull() ? theCurve3d->Degree() : aBS3DCurv->Degree());
    }

    if (!aBS2DCurv.IsNull() || theCurve2d->GetType() == GeomAbs_BezierCurve)
    {
      theNbParticles =
        std::max(theNbParticles, aBS2DCurv.IsNull() ? theCurve2d->Degree() : aBS2DCurv->Degree());
    }
  }
  catch (Standard_Failure const&)
  {
#ifdef OCCT_DEBUG
    std::cout << "ERROR! BRepLib_CheckCurveOnSurface.cxx, "
                 "FillSubIntervals(): Incorrect filling!"
              << std::endl;
#endif

    aNbSubIntervals = 0;
  }

  return aNbSubIntervals;
}

//=================================================================================================

bool MinComputing(GeomLib_CheckCurveOnSurface_TargetFunc& theFunction,
                  const int                               theNbParticles,
                  double&                                 theBestValue,
                  double&                                 theBestParameter)
{
  try
  {
    OCC_CATCH_SIGNALS
    if (theFunction.ElementaryMaximum(theBestValue, theBestParameter))
    {
      return true;
    }

    // Preserve degree-dependent coverage, including both interval endpoints.
    MathOpt::PSOConfig aConfig(size_t(3 * theNbParticles), 100, Precision::PConfusion());
    aConfig.InitMode = MathOpt::PSOInitMode::SeededOnly;
    aConfig.AllowPartialDomain = false;
    NCollection_DynamicArray<MathOpt::PSOSeedParticle> aSeeds;
    math_Vector aPosition(size_t(1));
    for (size_t i = 0; i < aConfig.NbParticles; ++i)
    {
      aPosition.ChangeAt(0) = double(i) / double(aConfig.NbParticles - 1);
      aSeeds.Append(MathOpt::PSOSeedParticle(aPosition));
    }
    const math_Vector aLower(1, 1, 0), anUpper(1, 1, 1);
    const auto aResult = MathOpt::PSO(theFunction, aLower, anUpper, aConfig, &aSeeds);
    if (!aResult.IsDone() || !aResult.Value || !aResult.Solution)
    {
      return false;
    }
    theBestValue     = *aResult.Value;
    theBestParameter = theFunction.Parameter(aResult.Solution->At(0));
    return true;
  }
  catch (const Standard_Failure&)
  {
    return false;
  }
}
