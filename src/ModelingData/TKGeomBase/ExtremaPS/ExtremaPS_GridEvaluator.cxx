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
#include <math_Vector.hxx>
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
                                      const NCollection_Array1<double>&                theUParams,
                                      const NCollection_Array1<double>&                theVParams)
{
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  Standard_DimensionError_Raise_if(theD1Grid.RowSize() != aNbU || theD1Grid.ColSize() != aNbV,
                                   "ExtremaPS_GridEvaluator::SetGrid");

  myGrid.Resize(aNbU, aNbV, false);
  initializeGridState(theUParams, theVParams);
  for (size_t anIndex = 0; anIndex < aNbU * aNbV; ++anIndex)
  {
    const GeomGridEval::SurfD1& aD1     = theD1Grid.Data()[anIndex];
    GridSample&                 aSample = myGrid.Data()[anIndex];
    aD1.Point.Coord(aSample.Point[0], aSample.Point[1], aSample.Point[2]);
    aD1.D1U.Coord(aSample.D1U[0], aSample.D1U[1], aSample.D1U[2]);
    aD1.D1V.Coord(aSample.D1V[0], aSample.D1V[1], aSample.D1V[2]);
  }
}

//==================================================================================================

void ExtremaPS_GridEvaluator::SetGrid(NCollection_Array2<GeomGridEval::SurfD1Coords>&& theD1Grid,
                                      NCollection_Array1<double>&&                      theUParams,
                                      NCollection_Array1<double>&&                      theVParams)
{
  Standard_DimensionError_Raise_if(theD1Grid.RowSize() != theUParams.Size()
                                     || theD1Grid.ColSize() != theVParams.Size(),
                                   "ExtremaPS_GridEvaluator::SetGrid");
  myGrid = std::move(theD1Grid);
  initializeGridState(std::move(theUParams), std::move(theVParams));
}

//==================================================================================================

void ExtremaPS_GridEvaluator::initializeGridState(const NCollection_Array1<double>& theUParams,
                                                  const NCollection_Array1<double>& theVParams)
{
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  myRowData[0].Resize(aNbV, false);
  myRowData[1].Resize(aNbV, false);
  myUParams.Resize(aNbU, false);
  myVParams.Resize(aNbV, false);
  myHasPreviousQuery   = false;
  myHasAffineDirection = false;
  myAffineGrid          = NCollection_Array1<AffineSample>();
  myGridMinimum.reset();
  myGridMaximum.reset();
  myGridBlocks.Clear();
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

void ExtremaPS_GridEvaluator::initializeGridState(NCollection_Array1<double>&& theUParams,
                                                  NCollection_Array1<double>&& theVParams)
{
  myRowData[0].Resize(theVParams.Size(), false);
  myRowData[1].Resize(theVParams.Size(), false);
  myUParams            = std::move(theUParams);
  myVParams            = std::move(theVParams);
  myHasPreviousQuery   = false;
  myHasAffineDirection = false;
  myAffineGrid          = NCollection_Array1<AffineSample>();
  myGridMinimum.reset();
  myGridMaximum.reset();
  myGridBlocks.Clear();
}

//==================================================================================================

void ExtremaPS_GridEvaluator::buildBlockHierarchy() const
{
  myGridBlocks.Clear();

  const size_t aNbU = myGrid.RowSize();
  const size_t aNbV = myGrid.ColSize();
  if (aNbU < 2 || aNbV < 2)
  {
    return;
  }

  const size_t aNbUBlocks = (aNbU - 2) / THE_BLOCK_NB_CELLS + 1;
  const size_t aNbVBlocks = (aNbV - 2) / THE_BLOCK_NB_CELLS + 1;
  myGridBlocks.Reserve(aNbUBlocks * aNbVBlocks);
  for (size_t aFirstU = 0; aFirstU + 1 < aNbU; aFirstU += THE_BLOCK_NB_CELLS)
  {
    const size_t aLastU = std::min(aFirstU + THE_BLOCK_NB_CELLS, aNbU - 1);
    for (size_t aFirstV = 0; aFirstV + 1 < aNbV; aFirstV += THE_BLOCK_NB_CELLS)
    {
      const size_t aLastV = std::min(aFirstV + THE_BLOCK_NB_CELLS, aNbV - 1);
      GridBlock    aBlock;
      aBlock.FirstU = aFirstU;
      aBlock.LastU  = aLastU;
      aBlock.FirstV = aFirstV;
      aBlock.LastV  = aLastV;
      bool isFirst = true;
      for (size_t i = aFirstU; i <= aLastU; ++i)
      {
        for (size_t j = aFirstV; j <= aLastV; ++j)
        {
          const GridSample& aSample = myGrid.At(i, j);
          const double aFu[4] = {aSample.Point[0] * aSample.D1U[0]
                                   + aSample.Point[1] * aSample.D1U[1]
                                   + aSample.Point[2] * aSample.D1U[2],
                                 aSample.D1U[0],
                                 aSample.D1U[1],
                                 aSample.D1U[2]};
          const double aFv[4] = {aSample.Point[0] * aSample.D1V[0]
                                   + aSample.Point[1] * aSample.D1V[1]
                                   + aSample.Point[2] * aSample.D1V[2],
                                 aSample.D1V[0],
                                 aSample.D1V[1],
                                 aSample.D1V[2]};
          for (size_t aCoord = 0; aCoord < 4; ++aCoord)
          {
            if (isFirst)
            {
              aBlock.FuMin[aCoord] = aBlock.FuMax[aCoord] = aFu[aCoord];
              aBlock.FvMin[aCoord] = aBlock.FvMax[aCoord] = aFv[aCoord];
            }
            else
            {
              aBlock.FuMin[aCoord] = std::min(aBlock.FuMin[aCoord], aFu[aCoord]);
              aBlock.FuMax[aCoord] = std::max(aBlock.FuMax[aCoord], aFu[aCoord]);
              aBlock.FvMin[aCoord] = std::min(aBlock.FvMin[aCoord], aFv[aCoord]);
              aBlock.FvMax[aCoord] = std::max(aBlock.FvMax[aCoord], aFv[aCoord]);
            }
          }
          for (size_t aCoord = 0; aCoord < 3; ++aCoord)
          {
            if (isFirst)
            {
              aBlock.PointMin[aCoord] = aBlock.PointMax[aCoord] = aSample.Point[aCoord];
            }
            else
            {
              aBlock.PointMin[aCoord] =
                std::min(aBlock.PointMin[aCoord], aSample.Point[aCoord]);
              aBlock.PointMax[aCoord] =
                std::max(aBlock.PointMax[aCoord], aSample.Point[aCoord]);
            }
          }
          isFirst = false;
        }
      }
      myGridBlocks.Append(aBlock);
    }
  }
}

//==================================================================================================

void ExtremaPS_GridEvaluator::buildAffineGrid() const
{
  myAffineGrid.Resize(myGrid.Size(), false);
  const double      aDx = myAffineDirection.X();
  const double      aDy = myAffineDirection.Y();
  const double      aDz = myAffineDirection.Z();
  const GridSample* aGrid = myGrid.Data();
  for (size_t anIndex = 0; anIndex < myGrid.Size(); ++anIndex)
  {
    const GridSample& aSample = aGrid[anIndex];
    const double aPx = aSample.Point[0] - myAffineOrigin.X();
    const double aPy = aSample.Point[1] - myAffineOrigin.Y();
    const double aPz = aSample.Point[2] - myAffineOrigin.Z();
    AffineSample& aAffine = myAffineGrid.ChangeAt(anIndex);
    aAffine.FuOrigin = aPx * aSample.D1U[0] + aPy * aSample.D1U[1] + aPz * aSample.D1U[2];
    aAffine.FvOrigin = aPx * aSample.D1V[0] + aPy * aSample.D1V[1] + aPz * aSample.D1V[2];
    aAffine.FuDirection = aDx * aSample.D1U[0] + aDy * aSample.D1U[1] + aDz * aSample.D1U[2];
    aAffine.FvDirection = aDx * aSample.D1V[0] + aDy * aSample.D1V[1] + aDz * aSample.D1V[2];
    aAffine.DistOrigin = aPx * aPx + aPy * aPy + aPz * aPz;
    aAffine.DistDirection = -2.0 * (aPx * aDx + aPy * aDy + aPz * aDz);
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

  if (myGrid.RowSize() < 2 || myGrid.ColSize() < 2)
  {
    myResult.Status = ExtremaPS::Status::InvalidInput;
    return myResult;
  }

  try
  {
    myCandidates.Clear();
    scanGrid(theP, theMode);
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
  }
  else
  {
    myResult.Status = ExtremaPS::Status::NoSolution;
  }
  return myResult;
}

//==================================================================================================

NCollection_Array1<double> ExtremaPS_GridEvaluator::BuildUniformParams(const double theMin,
                                                                      const double theMax,
                                                                      const size_t theNbSamples)
{
  Standard_RangeError_Raise_if(theNbSamples < 2, "ExtremaPS_GridEvaluator::BuildUniformParams");
  NCollection_Array1<double> aParams(theNbSamples);
  const double               aStep = (theMax - theMin) / static_cast<double>(theNbSamples - 1);

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
  const double aDx = theSample.Point[0] - thePx;
  const double aDy = theSample.Point[1] - thePy;
  const double aDz = theSample.Point[2] - thePz;
  const double aFu = aDx * theSample.D1U[0] + aDy * theSample.D1U[1] + aDz * theSample.D1U[2];
  const double aFv = aDx * theSample.D1V[0] + aDy * theSample.D1V[1] + aDz * theSample.D1V[2];

  GridPointData aData;
  aData.GradSq = aFu * aFu + aFv * aFv;
  aData.Dist   = aDx * aDx + aDy * aDy + aDz * aDz;
  aData.Fu     = aFu;
  aData.Fv     = aFv;
  aData.SignU  = aFu < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFu > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  aData.SignV  = aFv < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aFv > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  return aData;
}

//==================================================================================================

ExtremaPS_GridEvaluator::GridPointData ExtremaPS_GridEvaluator::evaluateAffineGridPoint(
  const AffineSample& theSample,
  const double        theParameter) const
{
  GridPointData aData;
  aData.Fu     = theSample.FuOrigin - theParameter * theSample.FuDirection;
  aData.Fv     = theSample.FvOrigin - theParameter * theSample.FvDirection;
  aData.GradSq = aData.Fu * aData.Fu + aData.Fv * aData.Fv;
  aData.Dist =
    std::max(0.0,
             theSample.DistOrigin
               + theParameter * (theSample.DistDirection + theParameter * myAffineDirectionSq));
  aData.SignU =
    aData.Fu < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aData.Fu > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  aData.SignV =
    aData.Fv < 0.0 ? THE_NEGATIVE_SIGN_MASK : (aData.Fv > 0.0 ? THE_POSITIVE_SIGN_MASK : 0);
  return aData;
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

  const double aDirectionComponent = myAffineDirection.Coord(myAffineAxis + 1);
  theParameter = (thePoint.Coord(myAffineAxis + 1) - myAffineOrigin.Coord(myAffineAxis + 1))
                 / aDirectionComponent;
  const gp_Pnt aPredicted = myAffineOrigin.Translated(theParameter * myAffineDirection);
  const double aTolX = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                       * std::max({1.0, std::abs(thePoint.X()), std::abs(aPredicted.X())});
  const double aTolY = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                       * std::max({1.0, std::abs(thePoint.Y()), std::abs(aPredicted.Y())});
  const double aTolZ = THE_AFFINE_COORD_TOL_FACTOR * RealEpsilon()
                       * std::max({1.0, std::abs(thePoint.Z()), std::abs(aPredicted.Z())});
  if (std::abs(thePoint.X() - aPredicted.X()) <= aTolX
      && std::abs(thePoint.Y() - aPredicted.Y()) <= aTolY
      && std::abs(thePoint.Z() - aPredicted.Z()) <= aTolZ)
  {
    myPreviousQuery = thePoint;
    return true;
  }

  const gp_Vec aDirection(myPreviousQuery, thePoint);
  const double aDirectionSq = aDirection.SquareMagnitude();
  myAffineOrigin            = myPreviousQuery;
  myPreviousQuery           = thePoint;
  myAffineGrid = NCollection_Array1<AffineSample>();
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

void ExtremaPS_GridEvaluator::scanGrid(const gp_Pnt&               theP,
                                       const ExtremaPS::SearchMode theMode) const
{
  double     anAffineParameter = 0.0;
  const bool isCoherentQuery   = prepareAffineEvaluation(theP, anAffineParameter);
  if (!isCoherentQuery)
  {
    scanGridDense(theP, theMode);
    return;
  }
  if (myGridBlocks.IsEmpty())
  {
    buildBlockHierarchy();
  }
  if (myAffineGrid.IsEmpty())
  {
    buildAffineGrid();
  }
  scanGridHierarchical(theP, anAffineParameter, theMode);
}

//==================================================================================================

void ExtremaPS_GridEvaluator::scanGridHierarchical(
  const gp_Pnt&               theP,
  const double                theAffineParameter,
  const ExtremaPS::SearchMode theMode) const
{
  const size_t aNbU = myGrid.RowSize();
  const size_t aNbV = myGrid.ColSize();

  if (aNbU < 2 || aNbV < 2)
  {
    return;
  }
  const double      aTolF               = Precision::Confusion() * THE_GRADIENT_TOL_FACTOR;
  const double      aTolFSq             = aTolF * aTolF;
  const AffineSample* anAffineGrid = myAffineGrid.Data();
  const double aPoint[3] = {theP.X(), theP.Y(), theP.Z()};
  const auto stationarityRange = [&](const double* theMin,
                                     const double* theMax,
                                     double&       theLower,
                                     double&       theUpper) {
    theLower = theMin[0];
    theUpper = theMax[0];
    for (size_t aCoord = 0; aCoord < 3; ++aCoord)
    {
      const double aTerm1 = -aPoint[aCoord] * theMin[aCoord + 1];
      const double aTerm2 = -aPoint[aCoord] * theMax[aCoord + 1];
      theLower += std::min(aTerm1, aTerm2);
      theUpper += std::max(aTerm1, aTerm2);
    }
  };
  const auto distanceBounds = [&](const GridBlock& theBlock,
                                  double&          theLower,
                                  double&          theUpper) {
    theLower = 0.0;
    theUpper = 0.0;
    for (size_t aCoord = 0; aCoord < 3; ++aCoord)
    {
      const double aLowDelta  = theBlock.PointMin[aCoord] - aPoint[aCoord];
      const double aHighDelta = theBlock.PointMax[aCoord] - aPoint[aCoord];
      double       aNear      = 0.0;
      if (aLowDelta > 0.0)
        aNear = aLowDelta;
      else if (aHighDelta < 0.0)
        aNear = aHighDelta;
      const double aFar = std::max(std::abs(aLowDelta), std::abs(aHighDelta));
      theLower += aNear * aNear;
      theUpper += aFar * aFar;
    }
  };

  size_t aMinIndex = 0;
  size_t aMaxIndex = 0;
  double aMinSquareDistance = 0.0;
  double aMaxSquareDistance = 0.0;
  bool   hasDistanceExtrema = false;
  constexpr size_t aBlockPointStride = THE_BLOCK_NB_CELLS + 1;
  GridPointData aBlockData[aBlockPointStride * aBlockPointStride];

  for (const GridBlock& aBlock : myGridBlocks)
  {
    double aFuLower = 0.0;
    double aFuUpper = 0.0;
    double aFvLower = 0.0;
    double aFvUpper = 0.0;
    stationarityRange(aBlock.FuMin, aBlock.FuMax, aFuLower, aFuUpper);
    stationarityRange(aBlock.FvMin, aBlock.FvMax, aFvLower, aFvUpper);
    const bool hasStationaryCell = aFuLower <= aTolF && aFuUpper >= -aTolF
                                   && aFvLower <= aTolF && aFvUpper >= -aTolF;
    double aDistanceLower = 0.0;
    double aDistanceUpper = 0.0;
    distanceBounds(aBlock, aDistanceLower, aDistanceUpper);
    const bool needsDistanceScan = !hasDistanceExtrema || aDistanceLower <= aMinSquareDistance
                                   || aDistanceUpper >= aMaxSquareDistance;
    if (!needsDistanceScan && !hasStationaryCell)
    {
      continue;
    }

    const size_t aBlockNbU = aBlock.LastU - aBlock.FirstU + 1;
    const size_t aBlockNbV = aBlock.LastV - aBlock.FirstV + 1;
    for (size_t i = 0; i < aBlockNbU; ++i)
    {
      for (size_t j = 0; j < aBlockNbV; ++j)
      {
        const size_t anIndex = (aBlock.FirstU + i) * aNbV + aBlock.FirstV + j;
        GridPointData& aData = aBlockData[i * aBlockPointStride + j];
        aData = evaluateAffineGridPoint(anAffineGrid[anIndex], theAffineParameter);
        if (!hasDistanceExtrema)
        {
          aMinIndex = aMaxIndex = anIndex;
          aMinSquareDistance = aMaxSquareDistance = aData.Dist;
          hasDistanceExtrema = true;
        }
        else
        {
          if (aData.Dist < aMinSquareDistance)
          {
            aMinSquareDistance = aData.Dist;
            aMinIndex          = anIndex;
          }
          if (aData.Dist > aMaxSquareDistance)
          {
            aMaxSquareDistance = aData.Dist;
            aMaxIndex          = anIndex;
          }
        }
      }
    }

    if (!hasStationaryCell)
    {
      continue;
    }
    for (size_t i = 0; i + 1 < aBlockNbU; ++i)
    {
      for (size_t j = 0; j + 1 < aBlockNbV; ++j)
      {
        const GridPointData& aD00 = aBlockData[i * aBlockPointStride + j];
        const GridPointData& aD10 = aBlockData[(i + 1) * aBlockPointStride + j];
        const GridPointData& aD01 = aBlockData[i * aBlockPointStride + j + 1];
        const GridPointData& aD11 = aBlockData[(i + 1) * aBlockPointStride + j + 1];
        const double aMinDist = std::min({aD00.Dist, aD10.Dist, aD01.Dist, aD11.Dist});
        const double aMinGradMag =
          std::min({aD00.GradSq, aD10.GradSq, aD01.GradSq, aD11.GradSq});
        bool aAddCandidate = aMinGradMag < aTolFSq;
        if (!aAddCandidate
            && (aD00.SignU | aD10.SignU | aD01.SignU | aD11.SignU) == THE_BOTH_SIGNS_MASK)
        {
          aAddCandidate =
            (aD00.SignV | aD10.SignV | aD01.SignV | aD11.SignV) == THE_BOTH_SIGNS_MASK;
        }
        if (aAddCandidate)
        {
          Candidate aCand;
          aCand.IdxU    = aBlock.FirstU + i;
          aCand.IdxV    = aBlock.FirstV + j;
          aCand.StartU  = (myUParams.At(aCand.IdxU) + myUParams.At(aCand.IdxU + 1)) * 0.5;
          aCand.StartV  = (myVParams.At(aCand.IdxV) + myVParams.At(aCand.IdxV + 1)) * 0.5;
          aCand.EstDist = aMinDist;
          aCand.GradMag = aMinGradMag;
          myCandidates.Append(aCand);
        }
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

//==================================================================================================

void ExtremaPS_GridEvaluator::scanGridDense(const gp_Pnt&               theP,
                                            const ExtremaPS::SearchMode theMode) const
{
  const size_t aNbU = myGrid.RowSize();
  const size_t aNbV = myGrid.ColSize();
  const double aPx   = theP.X();
  const double aPy   = theP.Y();
  const double aPz   = theP.Z();
  const double aTolF   = Precision::Confusion() * THE_GRADIENT_TOL_FACTOR;
  const double aTolFSq = aTolF * aTolF;
  const GridSample* aGrid = myGrid.Data();
  size_t aMinIndex = 0;
  size_t aMaxIndex = 0;
  double aMinSquareDistance = 0.0;
  double aMaxSquareDistance = 0.0;

  for (size_t i = 0; i < aNbU; ++i)
  {
    GridPointData* aCurrentRow = myRowData[i % 2].Data();
    const size_t   aRowOffset  = i * aNbV;
    for (size_t j = 0; j < aNbV; ++j)
    {
      const size_t anIndex = aRowOffset + j;
      aCurrentRow[j]       = evaluateGridPoint(aGrid[anIndex], aPx, aPy, aPz);
      if (anIndex == 0)
      {
        aMinSquareDistance = aCurrentRow[j].Dist;
        aMaxSquareDistance = aCurrentRow[j].Dist;
      }
      else
      {
        if (aCurrentRow[j].Dist < aMinSquareDistance)
        {
          aMinSquareDistance = aCurrentRow[j].Dist;
          aMinIndex          = anIndex;
        }
        if (aCurrentRow[j].Dist > aMaxSquareDistance)
        {
          aMaxSquareDistance = aCurrentRow[j].Dist;
          aMaxIndex          = anIndex;
        }
      }

      if (i == 0 || j == 0)
      {
        continue;
      }
      const size_t         aCellI       = i - 1;
      const size_t         aCellJ       = j - 1;
      const GridPointData* aPreviousRow = myRowData[aCellI % 2].Data();
      const GridPointData& aD00         = aPreviousRow[aCellJ];
      const GridPointData& aD10         = aCurrentRow[aCellJ];
      const GridPointData& aD01         = aPreviousRow[j];
      const GridPointData& aD11         = aCurrentRow[j];
      const double aMinDist = std::min({aD00.Dist, aD10.Dist, aD01.Dist, aD11.Dist});
      const double aMinGradMag =
        std::min({aD00.GradSq, aD10.GradSq, aD01.GradSq, aD11.GradSq});
      bool aAddCandidate = aMinGradMag < aTolFSq;
      if (!aAddCandidate
          && (aD00.SignU | aD10.SignU | aD01.SignU | aD11.SignU) == THE_BOTH_SIGNS_MASK)
      {
        aAddCandidate =
          (aD00.SignV | aD10.SignV | aD01.SignV | aD11.SignV) == THE_BOTH_SIGNS_MASK;
      }
      if (aAddCandidate)
      {
        Candidate aCand;
        aCand.IdxU    = aCellI;
        aCand.IdxV    = aCellJ;
        aCand.StartU  = (myUParams.At(aCellI) + myUParams.At(i)) * 0.5;
        aCand.StartV  = (myVParams.At(aCellJ) + myVParams.At(j)) * 0.5;
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
    Candidate aCand;
    aCand.IdxU    = std::min(aNbU - 2, myGridMinimum->I);
    aCand.IdxV    = std::min(aNbV - 2, myGridMinimum->J);
    aCand.StartU  = myUParams.At(myGridMinimum->I);
    aCand.StartV  = myVParams.At(myGridMinimum->J);
    aCand.EstDist = myGridMinimum->SquareDistance;
    aCand.GradMag = 0.0;
    myCandidates.Append(aCand);
  }
  if (theMode == ExtremaPS::SearchMode::Max || theMode == ExtremaPS::SearchMode::MinMax)
  {
    Candidate aCand;
    aCand.IdxU    = std::min(aNbU - 2, myGridMaximum->I);
    aCand.IdxV    = std::min(aNbV - 2, myGridMaximum->J);
    aCand.StartU  = myUParams.At(myGridMaximum->I);
    aCand.StartV  = myVParams.At(myGridMaximum->J);
    aCand.EstDist = myGridMaximum->SquareDistance;
    aCand.GradMag = 0.0;
    myCandidates.Append(aCand);
  }
}

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
            theP.SquareDistance(gp_Pnt(aSample.Point[0], aSample.Point[1], aSample.Point[2]));
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
