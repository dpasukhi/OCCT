// Copyright (c) 2025 OPEN CASCADE SAS
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

#include <ExtremaPS_GridEvaluator.hxx>

#include <Adaptor3d_Surface.hxx>
#include <GeomAbs_Shape.hxx>
#include <MathOpt_BFGS.hxx>
#include <MathSys_Newton2D.hxx>
#include <MathUtils_Config.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_LocalArray.hxx>
#include <Precision.hxx>
#include <Standard_DimensionError.hxx>
#include <Standard_Failure.hxx>
#include <Standard_RangeError.hxx>

#include <algorithm>
#include <cmath>

namespace
{
class DistanceFunction
{
public:
  DistanceFunction(const Adaptor3d_Surface& theSurface, const gp_Pnt& thePoint)
      : mySurface(theSurface),
        myPoint(thePoint)
  {
  }

  bool Value(const double theU, const double theV, double& theFu, double& theFv) const
  {
    const Geom_Surface::ResD1 aD1 = mySurface.EvalD1(theU, theV);
    const gp_Vec              aDelta(myPoint, aD1.Point);
    theFu = aDelta.Dot(aD1.D1U);
    theFv = aDelta.Dot(aD1.D1V);
    return true;
  }

  bool Value(const math_Vector& theUV, double& theSquareDistance) const
  {
    theSquareDistance = myPoint.SquareDistance(mySurface.EvalD0(theUV.At(0), theUV.At(1)));
    return true;
  }

  bool Gradient(const math_Vector& theUV, math_Vector& theGradient) const
  {
    double aFu = 0.0;
    double aFv = 0.0;
    Value(theUV.At(0), theUV.At(1), aFu, aFv);
    theGradient.ChangeAt(0) = 2.0 * aFu;
    theGradient.ChangeAt(1) = 2.0 * aFv;
    return true;
  }

  bool ValueAndJacobian(const double theU,
                        const double theV,
                        double&      theFu,
                        double&      theFv,
                        double&      theDFuu,
                        double&      theDFuv,
                        double&      theDFvv) const
  {
    const Geom_Surface::ResD2 aD2 = mySurface.EvalD2(theU, theV);
    const gp_Vec              aDelta(myPoint, aD2.Point);
    theFu   = aDelta.Dot(aD2.D1U);
    theFv   = aDelta.Dot(aD2.D1V);
    theDFuu = aD2.D1U.Dot(aD2.D1U) + aDelta.Dot(aD2.D2U);
    theDFuv = aD2.D1U.Dot(aD2.D1V) + aDelta.Dot(aD2.D2UV);
    theDFvv = aD2.D1V.Dot(aD2.D1V) + aDelta.Dot(aD2.D2V);
    return true;
  }

private:
  const Adaptor3d_Surface& mySurface;
  gp_Pnt                   myPoint;
};

MathSys::NewtonResultN<2> solveNewton(const DistanceFunction&    theFunction,
                                      const double               theU,
                                      const double               theV,
                                      const ExtremaPS::Domain2D& theDomain,
                                      const double               theTolerance)
{
  const MathSys::NewtonBoundsN<2> aBounds{{theDomain.UMin, theDomain.VMin},
                                          {theDomain.UMax, theDomain.VMax},
                                          true};
  MathSys::NewtonOptions          anOptions;
  anOptions.FTolerance    = theTolerance;
  anOptions.XTolerance    = theTolerance;
  anOptions.MaxIterations = 30;
  return MathSys::Solve2DSymmetric(theFunction, {theU, theV}, aBounds, anOptions);
}

bool isContinuityBoundary(const Adaptor3d_Surface& theSurface,
                          const double             theParameter,
                          const bool               theIsU)
{
  const int aNbIntervals =
    theIsU ? theSurface.NbUIntervals(GeomAbs_C1) : theSurface.NbVIntervals(GeomAbs_C1);
  if (aNbIntervals <= 1)
  {
    return false;
  }
  NCollection_LocalArray<double, 16> aStorage(static_cast<size_t>(aNbIntervals + 1));
  NCollection_Array1<double>         anIntervals(aStorage[0], 1, aNbIntervals + 1);
  if (theIsU)
  {
    theSurface.UIntervals(anIntervals, GeomAbs_C1);
  }
  else
  {
    theSurface.VIntervals(anIntervals, GeomAbs_C1);
  }
  for (size_t anIndex = 1; anIndex + 1 < anIntervals.Size(); ++anIndex)
  {
    if (std::abs(theParameter - anIntervals.At(anIndex)) <= Precision::PConfusion())
    {
      return true;
    }
  }
  return false;
}

} // namespace

ExtremaPS::Result& ExtremaPS_GridEvaluator::ChangeResult() const
{
  return myResult;
}

//==================================================================================================

void ExtremaPS_GridEvaluator::SetGrid(const NCollection_Array2<GeomGridEval::SurfD1>& theD1Grid,
                                      const math_Vector&                              theUParams,
                                      const math_Vector&                              theVParams)
{
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  Standard_DimensionError_Raise_if(theD1Grid.RowSize() != aNbU || theD1Grid.ColSize() != aNbV,
                                   "ExtremaPS_GridEvaluator::SetGrid");

  myGrid.Resize(aNbU, aNbV, false);
  myRowData[0].Resize(aNbV, false);
  myRowData[1].Resize(aNbV, false);
  myUParams.Resize(aNbU, false);
  myVParams.Resize(aNbV, false);
  myCacheCount         = 0;
  myCacheIndex         = 0;
  myHasPreviousQuery   = false;
  myHasAffineDirection = false;
  myHasAffineGrid      = false;
  for (size_t anIndex = 0; anIndex < aNbU * aNbV; ++anIndex)
  {
    const GeomGridEval::SurfD1& aD1     = theD1Grid.Data()[anIndex];
    GridSample&                 aSample = myGrid.Data()[anIndex];
    aSample.PointX                      = aD1.Point.X();
    aSample.PointY                      = aD1.Point.Y();
    aSample.PointZ                      = aD1.Point.Z();
    aSample.DUX                         = aD1.D1U.X();
    aSample.DUY                         = aD1.D1U.Y();
    aSample.DUZ                         = aD1.D1U.Z();
    aSample.DVX                         = aD1.D1V.X();
    aSample.DVY                         = aD1.D1V.Y();
    aSample.DVZ                         = aD1.D1V.Z();
  }
  for (size_t anIndex = 0; anIndex < aNbU; ++anIndex)
  {
    myUParams.ChangeAt(anIndex) = theUParams.At(anIndex);
  }
  for (size_t anIndex = 0; anIndex < aNbV; ++anIndex)
  {
    myVParams.ChangeAt(anIndex) = theVParams.At(anIndex);
  }
}

//==================================================================================================

[[nodiscard]] const ExtremaPS::Result& ExtremaPS_GridEvaluator::Perform(
  const Adaptor3d_Surface&    theSurface,
  const gp_Pnt&               theP,
  const ExtremaPS::Domain2D&  theDomain,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  myResult.Clear();

  if (!ExtremaPC::IsFinitePoint(theP) || !ExtremaPC::IsValidTolerance(theTol)
      || !theDomain.IsValid())
  {
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  if (myGrid.IsEmpty())
  {
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  try
  {
    scanGrid(theP, theTol, theMode);
    addCachedAsCandidate(theP, theDomain);
    refineCandidates(theSurface, theP, theDomain, theTol, theMode);
    addVerifiedGridMinimum(theSurface, theP, theDomain, theTol, theMode);
  }
  catch (const Standard_Failure&)
  {
    myResult.Clear();
    myResult.Status = ExtremaPS::Status::NumericalError;
    return myResult;
  }

  if (!myResult.Extrema.IsEmpty())
  {
    myResult.Status = ExtremaPS::Status::OK;
    updateSolutionCache(theP, theMode);
  }
  else
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
  }
  return myResult;
}

//==================================================================================================

math_Vector ExtremaPS_GridEvaluator::BuildUniformParams(const double theMin,
                                                        const double theMax,
                                                        const size_t theNbSamples)
{
  Standard_RangeError_Raise_if(theNbSamples < 2, "ExtremaPS_GridEvaluator::BuildUniformParams");
  math_Vector  aParams(theNbSamples);
  const double aStep = (theMax - theMin) / static_cast<double>(theNbSamples - 1);

  for (size_t anIndex = 0; anIndex < theNbSamples; ++anIndex)
  {
    aParams.ChangeAt(anIndex) = theMin + static_cast<double>(anIndex) * aStep;
  }
  aParams.ChangeAt(theNbSamples - 1) = theMax;

  return aParams;
}

//==================================================================================================

ExtremaPS_GridEvaluator::GridPointData ExtremaPS_GridEvaluator::evaluateGridPoint(
  const GridSample& theSample,
  const double      thePx,
  const double      thePy,
  const double      thePz)
{
  const double aDx = theSample.PointX - thePx;
  const double aDy = theSample.PointY - thePy;
  const double aDz = theSample.PointZ - thePz;
  const double aFu = aDx * theSample.DUX + aDy * theSample.DUY + aDz * theSample.DUZ;
  const double aFv = aDx * theSample.DVX + aDy * theSample.DVY + aDz * theSample.DVZ;

  GridPointData aData;
  aData.GradSq = aFu * aFu + aFv * aFv;
  aData.Dist   = aDx * aDx + aDy * aDy + aDz * aDz;
  aData.SignU  = aFu < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFu > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  aData.SignV  = aFv < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFv > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  return aData;
}

//==================================================================================================

ExtremaPS_GridEvaluator::GridPointData ExtremaPS_GridEvaluator::evaluateAffineGridPoint(
  const AffineSample& theSample,
  const double        theParameter) const
{
  const double aFu = theSample.FuOrigin - theParameter * theSample.FuDirection;
  const double aFv = theSample.FvOrigin - theParameter * theSample.FvDirection;

  GridPointData aData;
  aData.GradSq = aFu * aFu + aFv * aFv;
  aData.Dist =
    std::max(0.0,
             theSample.DistOrigin
               + theParameter * (theSample.DistDirection + theParameter * myAffineDirectionSq));
  aData.SignU = aFu < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFu > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  aData.SignV = aFv < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFv > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  return aData;
}

//==================================================================================================

void ExtremaPS_GridEvaluator::buildAffineGrid(const gp_Pnt& theOrigin,
                                              const gp_Vec& theDirection) const
{
  myAffineGrid.Resize(myGrid.Size(), false);
  const GridSample* aGrid = myGrid.Data();
  for (size_t anIndex = 0; anIndex < myGrid.Size(); ++anIndex)
  {
    const GridSample& aSample = aGrid[anIndex];
    const double      aDx     = aSample.PointX - theOrigin.X();
    const double      aDy     = aSample.PointY - theOrigin.Y();
    const double      aDz     = aSample.PointZ - theOrigin.Z();
    AffineSample&     aAffine = myAffineGrid.ChangeAt(anIndex);
    aAffine.FuOrigin          = aDx * aSample.DUX + aDy * aSample.DUY + aDz * aSample.DUZ;
    aAffine.FvOrigin          = aDx * aSample.DVX + aDy * aSample.DVY + aDz * aSample.DVZ;
    aAffine.FuDirection       = theDirection.X() * aSample.DUX + theDirection.Y() * aSample.DUY
                                + theDirection.Z() * aSample.DUZ;
    aAffine.FvDirection       = theDirection.X() * aSample.DVX + theDirection.Y() * aSample.DVY
                                + theDirection.Z() * aSample.DVZ;
    aAffine.DistOrigin        = aDx * aDx + aDy * aDy + aDz * aDz;
    aAffine.DistDirection =
      -2.0 * (aDx * theDirection.X() + aDy * theDirection.Y() + aDz * theDirection.Z());
  }
}

//==================================================================================================

bool ExtremaPS_GridEvaluator::prepareAffineEvaluation(const gp_Pnt& thePoint,
                                                      double&       theParameter) const
{
  if (!myHasPreviousQuery)
  {
    myPreviousQuery    = thePoint;
    myHasPreviousQuery = true;
    return false;
  }

  if (!myHasAffineDirection)
  {
    const gp_Vec aDirection(myPreviousQuery, thePoint);
    const double aDirectionSq = aDirection.SquareMagnitude();
    if (aDirectionSq <= Precision::SquareConfusion())
    {
      myPreviousQuery = thePoint;
      return false;
    }

    const double aDx     = std::abs(aDirection.X());
    const double aDy     = std::abs(aDirection.Y());
    const double aDz     = std::abs(aDirection.Z());
    myAffineAxis         = aDx >= aDy && aDx >= aDz ? 0 : (aDy >= aDz ? 1 : 2);
    myAffineOrigin       = myPreviousQuery;
    myAffineDirection    = aDirection;
    myAffineDirectionSq  = aDirectionSq;
    myPreviousQuery      = thePoint;
    myHasAffineDirection = true;
    return false;
  }

  const double aDirectionComponent =
    myAffineAxis == 0 ? myAffineDirection.X()
                      : (myAffineAxis == 1 ? myAffineDirection.Y() : myAffineDirection.Z());
  const double aPointComponent =
    myAffineAxis == 0 ? thePoint.X() : (myAffineAxis == 1 ? thePoint.Y() : thePoint.Z());
  const double anOriginComponent =
    myAffineAxis == 0 ? myAffineOrigin.X()
                      : (myAffineAxis == 1 ? myAffineOrigin.Y() : myAffineOrigin.Z());
  theParameter = (aPointComponent - anOriginComponent) / aDirectionComponent;

  const double aPredictedX = myAffineOrigin.X() + theParameter * myAffineDirection.X();
  const double aPredictedY = myAffineOrigin.Y() + theParameter * myAffineDirection.Y();
  const double aPredictedZ = myAffineOrigin.Z() + theParameter * myAffineDirection.Z();
  const double aTolX       = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                             * std::max({1.0, std::abs(thePoint.X()), std::abs(aPredictedX)});
  const double aTolY       = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                             * std::max({1.0, std::abs(thePoint.Y()), std::abs(aPredictedY)});
  const double aTolZ       = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                             * std::max({1.0, std::abs(thePoint.Z()), std::abs(aPredictedZ)});
  if (std::abs(thePoint.X() - aPredictedX) <= aTolX && std::abs(thePoint.Y() - aPredictedY) <= aTolY
      && std::abs(thePoint.Z() - aPredictedZ) <= aTolZ)
  {
    if (!myHasAffineGrid)
    {
      buildAffineGrid(myAffineOrigin, myAffineDirection);
      myHasAffineGrid = true;
    }
    myPreviousQuery = thePoint;
    return true;
  }

  const gp_Vec aDirection(myPreviousQuery, thePoint);
  const double aDirectionSq = aDirection.SquareMagnitude();
  myAffineOrigin            = myPreviousQuery;
  myPreviousQuery           = thePoint;
  myHasAffineGrid           = false;
  if (aDirectionSq <= Precision::SquareConfusion())
  {
    myHasAffineDirection = false;
    return false;
  }

  const double aDx     = std::abs(aDirection.X());
  const double aDy     = std::abs(aDirection.Y());
  const double aDz     = std::abs(aDirection.Z());
  myAffineAxis         = aDx >= aDy && aDx >= aDz ? 0 : (aDy >= aDz ? 1 : 2);
  myAffineDirection    = aDirection;
  myAffineDirectionSq  = aDirectionSq;
  myHasAffineDirection = true;
  return false;
}

//==================================================================================================

void ExtremaPS_GridEvaluator::scanGrid(const gp_Pnt&                 theP,
                                       [[maybe_unused]] const double theTol,
                                       const ExtremaPS::SearchMode   theMode) const
{
  myCandidates.Clear();

  const size_t aNbU = myGrid.RowSize();
  const size_t aNbV = myGrid.ColSize();

  if (aNbU < 2 || aNbV < 2)
  {
    return;
  }

  const double aPx = theP.X();
  const double aPy = theP.Y();
  const double aPz = theP.Z();

  const double aTolF               = Precision::Confusion() * THE_GRADIENT_TOL_FACTOR;
  const double aTolFSq             = aTolF * aTolF;
  double       anAffineParameter   = 0.0;
  const bool   useAffineEvaluation = prepareAffineEvaluation(theP, anAffineParameter);

  const GridSample*   aGrid              = myGrid.Data();
  const AffineSample* anAffineGrid       = useAffineEvaluation ? myAffineGrid.Data() : nullptr;
  size_t              aMinIndex          = 0;
  size_t              aMaxIndex          = 0;
  double              aMinSquareDistance = 0.0;
  double              aMaxSquareDistance = 0.0;

  for (size_t i = 0; i < aNbU; ++i)
  {
    GridPointData* aCurrentRow = myRowData[i % 2].Data();
    const size_t   aRowOffset  = i * aNbV;
    if (useAffineEvaluation)
    {
      for (size_t j = 0; j < aNbV; ++j)
      {
        const size_t anIndex = aRowOffset + j;
        aCurrentRow[j]       = evaluateAffineGridPoint(anAffineGrid[anIndex], anAffineParameter);
        if (anIndex == 0)
        {
          aMinSquareDistance = aCurrentRow[j].Dist;
          aMaxSquareDistance = aCurrentRow[j].Dist;
        }
        else if (aCurrentRow[j].Dist < aMinSquareDistance)
        {
          aMinSquareDistance = aCurrentRow[j].Dist;
          aMinIndex          = anIndex;
        }
        else if (aCurrentRow[j].Dist > aMaxSquareDistance)
        {
          aMaxSquareDistance = aCurrentRow[j].Dist;
          aMaxIndex          = anIndex;
        }
      }
    }
    else
    {
      for (size_t j = 0; j < aNbV; ++j)
      {
        const size_t anIndex = aRowOffset + j;
        aCurrentRow[j]       = evaluateGridPoint(aGrid[anIndex], aPx, aPy, aPz);
        if (anIndex == 0)
        {
          aMinSquareDistance = aCurrentRow[j].Dist;
          aMaxSquareDistance = aCurrentRow[j].Dist;
        }
        else if (aCurrentRow[j].Dist < aMinSquareDistance)
        {
          aMinSquareDistance = aCurrentRow[j].Dist;
          aMinIndex          = anIndex;
        }
        else if (aCurrentRow[j].Dist > aMaxSquareDistance)
        {
          aMaxSquareDistance = aCurrentRow[j].Dist;
          aMaxIndex          = anIndex;
        }
      }
    }

    if (i == 0)
    {
      continue;
    }

    const size_t         aCellI       = i - 1;
    const GridPointData* aPreviousRow = myRowData[aCellI % 2].Data();
    for (size_t j = 0; j + 1 < aNbV; ++j)
    {
      const GridPointData& aD00 = aPreviousRow[j];
      const GridPointData& aD10 = aCurrentRow[j];
      const GridPointData& aD01 = aPreviousRow[j + 1];
      const GridPointData& aD11 = aCurrentRow[j + 1];

      double aMinDist    = aD00.Dist;
      double aMinGradMag = aD00.GradSq;
      if (aD10.Dist < aMinDist)
        aMinDist = aD10.Dist;
      if (aD01.Dist < aMinDist)
        aMinDist = aD01.Dist;
      if (aD11.Dist < aMinDist)
        aMinDist = aD11.Dist;
      if (aD10.GradSq < aMinGradMag)
        aMinGradMag = aD10.GradSq;
      if (aD01.GradSq < aMinGradMag)
        aMinGradMag = aD01.GradSq;
      if (aD11.GradSq < aMinGradMag)
        aMinGradMag = aD11.GradSq;

      bool aAddCandidate = aMinGradMag < aTolFSq;
      if (!aAddCandidate)
      {
        const bool aFuSignChange =
          (aD00.SignU | aD10.SignU | aD01.SignU | aD11.SignU) == THE_BOTH_SIGNS_MASK;
        if (aFuSignChange)
        {
          const bool aFvSignChange =
            (aD00.SignV | aD10.SignV | aD01.SignV | aD11.SignV) == THE_BOTH_SIGNS_MASK;
          aAddCandidate = aFvSignChange;
        }
      }

      if (aAddCandidate)
      {
        Candidate aCand;
        aCand.IdxU    = aCellI;
        aCand.IdxV    = j;
        aCand.StartU  = (myUParams.At(aCellI) + myUParams.At(i)) * 0.5;
        aCand.StartV  = (myVParams.At(j) + myVParams.At(j + 1)) * 0.5;
        aCand.EstDist = aMinDist;
        aCand.GradMag = aMinGradMag;
        myCandidates.Append(aCand);
      }
    }
  }

  myGridMinimum = GridDistanceExtremum{aMinIndex / aNbV, aMinIndex % aNbV, aMinSquareDistance};
  myGridMaximum = GridDistanceExtremum{aMaxIndex / aNbV, aMaxIndex % aNbV, aMaxSquareDistance};

  if (theMode == ExtremaPS::SearchMode::Min || theMode == ExtremaPS::SearchMode::MinMax)
  {
    const size_t aCellI = std::min(aNbU - 2, myGridMinimum->I);
    const size_t aCellJ = std::min(aNbV - 2, myGridMinimum->J);

    Candidate aCand;
    aCand.IdxU    = aCellI;
    aCand.IdxV    = aCellJ;
    aCand.StartU  = myUParams.At(myGridMinimum->I);
    aCand.StartV  = myVParams.At(myGridMinimum->J);
    aCand.EstDist = myGridMinimum->SquareDistance;
    aCand.GradMag = 0.0;
    myCandidates.Append(aCand);
  }

  if (theMode == ExtremaPS::SearchMode::Max || theMode == ExtremaPS::SearchMode::MinMax)
  {
    const size_t aCellI = std::min(aNbU - 2, myGridMaximum->I);
    const size_t aCellJ = std::min(aNbV - 2, myGridMaximum->J);

    Candidate aCand;
    aCand.IdxU    = aCellI;
    aCand.IdxV    = aCellJ;
    aCand.StartU  = myUParams.At(myGridMaximum->I);
    aCand.StartV  = myVParams.At(myGridMaximum->J);
    aCand.EstDist = myGridMaximum->SquareDistance;
    aCand.GradMag = 0.0;
    myCandidates.Append(aCand);
  }
}

std::optional<std::pair<double, double>> ExtremaPS_GridEvaluator::cachedStart(
  const gp_Pnt&              thePoint,
  const ExtremaPS::Domain2D& theDomain) const
{
  if (myCacheCount == 0)
  {
    return std::nullopt;
  }

  size_t aNearestIdx  = (myCacheIndex + THE_CACHE_SIZE - 1) % THE_CACHE_SIZE;
  double aNearestDist = thePoint.SquareDistance(myCachedSolutions[aNearestIdx].QueryPoint);

  for (size_t i = 1; i < myCacheCount; ++i)
  {
    size_t aIdx  = (myCacheIndex + THE_CACHE_SIZE - 1 - i) % THE_CACHE_SIZE;
    double aDist = thePoint.SquareDistance(myCachedSolutions[aIdx].QueryPoint);
    if (aDist < aNearestDist)
    {
      aNearestDist = aDist;
      aNearestIdx  = aIdx;
    }
  }

  if (aNearestDist > THE_COHERENCE_THRESHOLD_SQ)
  {
    return std::nullopt;
  }

  const CachedSolution& aNearest = myCachedSolutions[aNearestIdx];

  double aStartU = aNearest.U;
  double aStartV = aNearest.V;

  if (myCacheCount >= 3)
  {
    const size_t aIdx2 = (myCacheIndex + THE_CACHE_SIZE - 1) % THE_CACHE_SIZE;
    const size_t aIdx1 = (myCacheIndex + THE_CACHE_SIZE - 2) % THE_CACHE_SIZE;
    const size_t aIdx0 = (myCacheIndex + THE_CACHE_SIZE - 3) % THE_CACHE_SIZE;

    const CachedSolution& aS0 = myCachedSolutions[aIdx0];
    const CachedSolution& aS1 = myCachedSolutions[aIdx1];
    const CachedSolution& aS2 = myCachedSolutions[aIdx2];

    const gp_Vec aV01(aS0.QueryPoint, aS1.QueryPoint);
    const gp_Vec aV12(aS1.QueryPoint, aS2.QueryPoint);
    const double aMag01 = aV01.Magnitude();
    const double aMag12 = aV12.Magnitude();

    if (aMag01 > Precision::Confusion() && aMag12 > Precision::Confusion())
    {
      const double aRatio = aMag12 / aMag01;
      if (aRatio > THE_TRAJECTORY_MIN_RATIO && aRatio < THE_TRAJECTORY_MAX_RATIO)
      {
        const double aCos = aV01.Dot(aV12) / (aMag01 * aMag12);
        if (aCos > THE_TRAJECTORY_MIN_COS)
        {
          if (myCacheCount >= 4 && aCos > THE_TRAJECTORY_QUADRATIC_COS)
          {
            aStartU = 3.0 * aS2.U - 3.0 * aS1.U + aS0.U;
            aStartV = 3.0 * aS2.V - 3.0 * aS1.V + aS0.V;
          }
          else
          {
            const double aDeltaU = aS2.U - aS1.U;
            const double aDeltaV = aS2.V - aS1.V;
            aStartU              = aS2.U + aDeltaU;
            aStartV              = aS2.V + aDeltaV;
          }
          aStartU = theDomain.U().Clamp(aStartU);
          aStartV = theDomain.V().Clamp(aStartV);
        }
      }
    }
  }

  return std::make_pair(aStartU, aStartV);
}

//==================================================================================================

void ExtremaPS_GridEvaluator::addCachedAsCandidate(const gp_Pnt&              theP,
                                                   const ExtremaPS::Domain2D& theDomain) const
{
  const std::optional<std::pair<double, double>> aStart = cachedStart(theP, theDomain);
  if (!aStart)
  {
    return;
  }
  const double aStartU = aStart->first;
  const double aStartV = aStart->second;

  const size_t aNbU = myGrid.RowSize();
  const size_t aNbV = myGrid.ColSize();

  size_t aCellI = 0;
  {
    size_t aLo = 0;
    size_t aHi = aNbU - 1;
    while (aLo + 1 < aHi)
    {
      const size_t aMid = aLo + (aHi - aLo) / 2;
      if (aStartU < myUParams.At(aMid))
      {
        aHi = aMid;
      }
      else
      {
        aLo = aMid;
      }
    }
    aCellI = aLo;
  }

  size_t aCellJ = 0;
  {
    size_t aLo = 0;
    size_t aHi = aNbV - 1;
    while (aLo + 1 < aHi)
    {
      const size_t aMid = aLo + (aHi - aLo) / 2;
      if (aStartV < myVParams.At(aMid))
      {
        aHi = aMid;
      }
      else
      {
        aLo = aMid;
      }
    }
    aCellJ = aLo;
  }

  const double        aPx  = theP.X();
  const double        aPy  = theP.Y();
  const double        aPz  = theP.Z();
  const GridPointData aD00 = evaluateGridPoint(myGrid.At(aCellI, aCellJ), aPx, aPy, aPz);
  const GridPointData aD10 = evaluateGridPoint(myGrid.At(aCellI + 1, aCellJ), aPx, aPy, aPz);
  const GridPointData aD01 = evaluateGridPoint(myGrid.At(aCellI, aCellJ + 1), aPx, aPy, aPz);
  const GridPointData aD11 = evaluateGridPoint(myGrid.At(aCellI + 1, aCellJ + 1), aPx, aPy, aPz);

  double aMinDist = aD00.Dist;
  if (aD10.Dist < aMinDist)
    aMinDist = aD10.Dist;
  if (aD01.Dist < aMinDist)
    aMinDist = aD01.Dist;
  if (aD11.Dist < aMinDist)
    aMinDist = aD11.Dist;

  const double aMinGradMag =
    std::min(std::min(aD00.GradSq, aD10.GradSq), std::min(aD01.GradSq, aD11.GradSq));

  for (size_t anIndex = 0; anIndex < myCandidates.Size(); ++anIndex)
  {
    Candidate& aCandidate = myCandidates.ChangeValue(anIndex);
    if (aCandidate.IdxU == aCellI && aCandidate.IdxV == aCellJ)
    {
      aCandidate.StartU = aStartU;
      aCandidate.StartV = aStartV;
      return;
    }
  }

  Candidate aCand;
  aCand.IdxU    = aCellI;
  aCand.IdxV    = aCellJ;
  aCand.StartU  = aStartU;
  aCand.StartV  = aStartV;
  aCand.EstDist = aMinDist;    // Actual distance estimate from grid
  aCand.GradMag = aMinGradMag; // Actual gradient magnitude from grid
  myCandidates.Append(aCand);
}

//==================================================================================================

void ExtremaPS_GridEvaluator::refineCandidates(const Adaptor3d_Surface&    theSurface,
                                               const gp_Pnt&               theP,
                                               const ExtremaPS::Domain2D&  theDomain,
                                               const double                theTol,
                                               const ExtremaPS::SearchMode theMode) const
{
  myResult.Status = ExtremaPS::Status::OK;
  myFoundRoots.Clear();
  mySortedEntries.Clear();

  DistanceFunction aFunc(theSurface, theP);

  for (size_t c = 0; c < myCandidates.Size(); ++c)
  {
    const Candidate& aCand = myCandidates.Value(c);
    SortEntry        anEntry;
    anEntry.Idx     = c;
    anEntry.Dist    = aCand.EstDist;
    anEntry.GradMag = aCand.GradMag;
    mySortedEntries.Append(anEntry);
  }

  if (theMode == ExtremaPS::SearchMode::Min)
  {
    std::sort(mySortedEntries.begin(),
              mySortedEntries.end(),
              [](const SortEntry& a, const SortEntry& b) {
                if (std::abs(a.Dist - b.Dist) > Precision::Confusion() * std::max(a.Dist, b.Dist))
                  return a.Dist < b.Dist;
                return a.GradMag < b.GradMag;
              });
  }
  else if (theMode == ExtremaPS::SearchMode::Max)
  {
    std::sort(mySortedEntries.begin(),
              mySortedEntries.end(),
              [](const SortEntry& a, const SortEntry& b) {
                if (std::abs(a.Dist - b.Dist) > Precision::Confusion() * std::max(a.Dist, b.Dist))
                  return a.Dist > b.Dist;
                return a.GradMag < b.GradMag;
              });
  }

  for (size_t s = 0; s < mySortedEntries.Size(); ++s)
  {
    const Candidate& aCand = myCandidates.Value(mySortedEntries.Value(s).Idx);

    double aBestRootU = 0.0;
    double aBestRootV = 0.0;
    bool   aConverged = false;

    const ExtremaPS::Domain2D       aCellDomain(myUParams.At(aCand.IdxU),
                                                myUParams.At(aCand.IdxU + 1),
                                                myVParams.At(aCand.IdxV),
                                                myVParams.At(aCand.IdxV + 1));
    const MathSys::NewtonResultN<2> aNewtonRes =
      solveNewton(aFunc, aCand.StartU, aCand.StartV, aCellDomain, theTol);
    if (aNewtonRes.IsDone())
    {
      aBestRootU = aNewtonRes.X[0];
      aBestRootV = aNewtonRes.X[1];
      aConverged = true;
    }

    if (!aConverged)
    {
      std::optional<double> aBestGridDist;
      size_t                aBestI = aCand.IdxU;
      size_t                aBestJ = aCand.IdxV;

      for (size_t di = 0; di <= 1; ++di)
      {
        for (size_t dj = 0; dj <= 1; ++dj)
        {
          const size_t      i       = aCand.IdxU + di;
          const size_t      j       = aCand.IdxV + dj;
          const GridSample& aSample = myGrid.At(i, j);
          const double      aDist =
            theP.SquareDistance(gp_Pnt(aSample.PointX, aSample.PointY, aSample.PointZ));
          if (!aBestGridDist || aDist < *aBestGridDist)
          {
            aBestGridDist = aDist;
            aBestI        = i;
            aBestJ        = j;
          }
        }
      }

      aBestRootU = myUParams.At(aBestI);
      aBestRootV = myVParams.At(aBestJ);

      double aFu = 0.0;
      double aFv = 0.0;
      if (!aFunc.Value(aBestRootU, aBestRootV, aFu, aFv))
      {
        continue;
      }
      double aFNorm = std::sqrt(aFu * aFu + aFv * aFv);
      if (aFNorm < Precision::Confusion() * THE_GRADIENT_TOL_FACTOR)
      {
        aConverged = true;
      }
    }

    double aRootU = aBestRootU;
    double aRootV = aBestRootV;

    if (!aConverged)
      continue;

    if (aRootU <= theDomain.UMin + Precision::PConfusion()
        || aRootU >= theDomain.UMax - Precision::PConfusion()
        || aRootV <= theDomain.VMin + Precision::PConfusion()
        || aRootV >= theDomain.VMax - Precision::PConfusion())
    {
      continue;
    }

    bool aSkip = false;
    for (size_t r = 0; r < myFoundRoots.Size(); ++r)
    {
      const std::pair<double, double>& aRoot = myFoundRoots.Value(r);
      if (std::abs(aRootU - aRoot.first) <= Precision::PConfusion()
          && std::abs(aRootV - aRoot.second) <= Precision::PConfusion())
      {
        aSkip = true;
        break;
      }
    }
    if (aSkip)
      continue;

    double aFu, aFv, aDFuu, aDFuv, aDFvv;
    if (!aFunc.ValueAndJacobian(aRootU, aRootV, aFu, aFv, aDFuu, aDFuv, aDFvv))
    {
      continue;
    }

    const double aDet = aDFuu * aDFvv - aDFuv * aDFuv;
    const double aHessianScale =
      std::max(std::abs(aDFuu) * std::abs(aDFvv), MathUtils::THE_HESSIAN_DEGENERACY_ABS);
    const bool aIsDegenerate =
      std::abs(aDet) < MathUtils::THE_HESSIAN_DEGENERACY_REL * aHessianScale;

    if (aIsDegenerate || aDet < 0.0)
      continue;

    const bool aIsMin = (aDFuu > 0.0 && aDet > 0.0);
    if ((theMode == ExtremaPS::SearchMode::Min && !aIsMin)
        || (theMode == ExtremaPS::SearchMode::Max && aIsMin))
    {
      continue;
    }

    ExtremaPS::ExtremumResult anExt;
    anExt.U              = aRootU;
    anExt.V              = aRootV;
    anExt.Point          = theSurface.EvalD0(aRootU, aRootV);
    anExt.SquareDistance = theP.SquareDistance(anExt.Point);
    anExt.IsMinimum      = aIsMin;
    myResult.Extrema.Append(anExt);

    myFoundRoots.Append(std::make_pair(aRootU, aRootV));
  }

  if (myResult.Extrema.IsEmpty() && myCandidates.IsEmpty())
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
  }
}

//==================================================================================================

void ExtremaPS_GridEvaluator::addVerifiedGridMinimum(const Adaptor3d_Surface&    theSurface,
                                                     const gp_Pnt&               theP,
                                                     const ExtremaPS::Domain2D&  theDomain,
                                                     const double                theTol,
                                                     const ExtremaPS::SearchMode theMode) const
{
  if (theMode == ExtremaPS::SearchMode::Max || !myGridMinimum)
  {
    return;
  }

  double                          aU = myUParams.At(myGridMinimum->I);
  double                          aV = myVParams.At(myGridMinimum->J);
  DistanceFunction                aFunction(theSurface, theP);
  const MathSys::NewtonResultN<2> aNewton   = solveNewton(aFunction, aU, aV, theDomain, theTol);
  bool                            isRefined = aNewton.IsDone();
  if (aNewton.IsDone())
  {
    aU = aNewton.X[0];
    aV = aNewton.X[1];
  }
  else
  {
    math_Vector aStart(2), aLower(2), anUpper(2);
    aStart.ChangeAt(0)  = aU;
    aStart.ChangeAt(1)  = aV;
    aLower.ChangeAt(0)  = theDomain.UMin;
    aLower.ChangeAt(1)  = theDomain.VMin;
    anUpper.ChangeAt(0) = theDomain.UMax;
    anUpper.ChangeAt(1) = theDomain.VMax;
    MathUtils::Config           aConfig(theTol, 50);
    const MathOpt::VectorResult anOptimum =
      MathOpt::BFGSBounded(aFunction, aStart, aLower, anUpper, aConfig);
    if (anOptimum.IsDone() && anOptimum.Solution.has_value())
    {
      aU        = anOptimum.Solution->At(0);
      aV        = anOptimum.Solution->At(1);
      isRefined = true;
    }
  }

  double aFu = 0.0, aFv = 0.0, aDFuu = 0.0, aDFuv = 0.0, aDFvv = 0.0;
  if (!aFunction.ValueAndJacobian(aU, aV, aFu, aFv, aDFuu, aDFuv, aDFvv))
  {
    return;
  }
  const gp_Pnt aPoint          = theSurface.EvalD0(aU, aV);
  const double aSquareDistance = theP.SquareDistance(aPoint);
  const double aGradientTolerance =
    std::max(theTol, Precision::Confusion() * THE_GRADIENT_TOL_FACTOR);
  const double aGradientToleranceSq = aGradientTolerance * aGradientTolerance;
  const bool   isAtContinuityBoundary =
    isContinuityBoundary(theSurface, aU, true) || isContinuityBoundary(theSurface, aV, false);
  if (!isRefined && !isAtContinuityBoundary)
  {
    return;
  }
  if (!isAtContinuityBoundary && aFu * aFu + aFv * aFv > aGradientToleranceSq)
  {
    return;
  }

  const double aDet = aDFuu * aDFvv - aDFuv * aDFuv;
  const double aHessianScale =
    std::max(std::abs(aDFuu) * std::abs(aDFvv), MathUtils::THE_HESSIAN_DEGENERACY_ABS);
  const double aCurvatureTolerance = MathUtils::THE_HESSIAN_DEGENERACY_REL * aHessianScale;
  if (!isAtContinuityBoundary
      && (aDFuu < -aCurvatureTolerance || aDFvv < -aCurvatureTolerance
          || aDet < -aCurvatureTolerance))
  {
    return;
  }
  if (aU <= theDomain.UMin + Precision::PConfusion()
      || aU >= theDomain.UMax - Precision::PConfusion()
      || aV <= theDomain.VMin + Precision::PConfusion()
      || aV >= theDomain.VMax - Precision::PConfusion())
  {
    return;
  }

  for (size_t anIndex = 0; anIndex < myResult.Extrema.Size(); ++anIndex)
  {
    const ExtremaPS::ExtremumResult& anExisting = myResult.Extrema.Value(anIndex);
    if (std::abs(anExisting.U - aU) <= Precision::PConfusion()
        && std::abs(anExisting.V - aV) <= Precision::PConfusion())
    {
      return;
    }
  }

  ExtremaPS::ExtremumResult anExtremum;
  anExtremum.U              = aU;
  anExtremum.V              = aV;
  anExtremum.Point          = aPoint;
  anExtremum.SquareDistance = aSquareDistance;
  anExtremum.IsMinimum      = true;
  myResult.Extrema.Append(anExtremum);
}

//==================================================================================================

void ExtremaPS_GridEvaluator::addToCache(const gp_Pnt& theP,
                                         const double  theU,
                                         const double  theV) const
{
  myCachedSolutions[myCacheIndex].QueryPoint = theP;
  myCachedSolutions[myCacheIndex].U          = theU;
  myCachedSolutions[myCacheIndex].V          = theV;
  myCacheIndex                               = (myCacheIndex + 1) % THE_CACHE_SIZE;
  if (myCacheCount < THE_CACHE_SIZE)
    ++myCacheCount;
}

//==================================================================================================

void ExtremaPS_GridEvaluator::updateSolutionCache(const gp_Pnt&               theP,
                                                  const ExtremaPS::SearchMode theMode) const
{
  if (myResult.Extrema.IsEmpty())
  {
    return;
  }

  const size_t anIndex =
    theMode == ExtremaPS::SearchMode::Max ? myResult.MaxIndex() : myResult.MinIndex();
  addToCache(theP, myResult.Extrema.Value(anIndex).U, myResult.Extrema.Value(anIndex).V);
}
