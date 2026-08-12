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

#ifndef _ExtremaPC_GridEvaluator_HeaderFile
#define _ExtremaPC_GridEvaluator_HeaderFile

#include <Adaptor3d_Curve.hxx>
#include <Extrema_CurveTool.hxx>
#include <ExtremaPC.hxx>
#include <ExtremaPC_DistanceFunction.hxx>
#include <GeomAbs_Shape.hxx>
#include <math_Vector.hxx>
#include <MathRoot_Multiple.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_LinearVector.hxx>
#include <Standard_Failure.hxx>

#include <algorithm>
#include <cmath>

//! @brief Numerical point-curve extrema computation over a cached parameter partition.
//!
//! Each adjacent parameter pair is solved independently, making non-uniform knot- and
//! curvature-aware distributions effective. C2 continuity bounds are inserted into the
//! partition and classified separately from ordinary sampling boundaries.
class ExtremaPC_GridEvaluator
{
public:
  //! Default constructor.
  ExtremaPC_GridEvaluator() = default;

  //! Stores ordered parameter values defining the numerical search partition.
  //! @param[in] theParams ordered parameter values
  void SetParams(const math_Vector& theParams)
  {
    const int aNbParams = theParams.Length();
    if (aNbParams == 0)
    {
      myParams = NCollection_Array1<double>();
      return;
    }

    myParams = NCollection_Array1<double>(0, aNbParams - 1);
    for (int anIndex = 0; anIndex < aNbParams; ++anIndex)
    {
      myParams[anIndex] = theParams(theParams.Lower() + anIndex);
    }
  }

  //! Returns mutable reference to the result for post-processing.
  ExtremaPC::Result& Result() const { return myResult; }

  //! Performs extrema computation using the cached parameter partition.
  [[nodiscard]] const ExtremaPC::Result& Perform(const Adaptor3d_Curve&     theCurve,
                                                  const gp_Pnt&              theP,
                                                  const ExtremaPC::Domain1D& theDomain,
                                                  double                     theTol,
                                                  ExtremaPC::SearchMode      theMode) const
  {
    myResult.Clear();
    if (!theDomain.IsValid() || !ExtremaPC::IsValidTolerance(theTol))
    {
      myResult.Status = ExtremaPC::Status::InvalidInput;
      return myResult;
    }
    if (theDomain.Min == theDomain.Max)
    {
      myResult.Status = ExtremaPC::Status::NoSolution;
      return myResult;
    }
    if (!validateParams(theDomain))
    {
      myResult.Status = ExtremaPC::Status::InvalidInput;
      return myResult;
    }

    if (isConstantCurve(theCurve))
    {
      myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
      myResult.InfiniteSquareDistance = theP.SquareDistance(theCurve.Value(theDomain.Min));
      return myResult;
    }

    NCollection_LinearVector<double> aContinuityBounds;
    NCollection_LinearVector<double> aPartition;
    buildPartition(theCurve, theDomain, aContinuityBounds, aPartition);

    MathRoot::MultipleConfig aConfig;
    aConfig.NbSamples     = 2;
    aConfig.XTolerance    = ExtremaPC::THE_PARAM_TOLERANCE;
    aConfig.FTolerance    = theTol;
    aConfig.NullTolerance = theTol;

    NCollection_LinearVector<RootCandidate> aRoots;
    for (size_t anInterval = 0; anInterval + 1 < aPartition.Size(); ++anInterval)
    {
      const double aLower  = aPartition.Value(anInterval);
      const double anUpper = aPartition.Value(anInterval + 1);
      if (anUpper - aLower <= ExtremaPC::THE_PARAM_TOLERANCE)
      {
        continue;
      }
      occ::handle<Adaptor3d_Curve> aLocalCurve;
      try
      {
        aLocalCurve = theCurve.Trim(aLower, anUpper, theTol);
      }
      catch (const Standard_Failure&)
      {
        // Some custom adaptors support evaluation but do not implement Trim().
      }
      const Adaptor3d_Curve&     aCurveForInterval =
        aLocalCurve.IsNull() ? theCurve : *aLocalCurve;
      ExtremaPC_DistanceFunction aLocalFunction(aCurveForInterval, theP);
      MathRoot::MultipleResult   anIntervalRoots;
      try
      {
        anIntervalRoots =
          MathRoot::FindAllRootsWithDerivative(aLocalFunction, aLower, anUpper, aConfig);
      }
      catch (const Standard_Failure&)
      {
        myResult.Status = ExtremaPC::Status::NumericalError;
        return myResult;
      }
      if (!anIntervalRoots.IsDone())
      {
        myResult.Status = ExtremaPC::Status::NumericalError;
        return myResult;
      }
      if (anIntervalRoots.IsAllNull)
      {
        if (isConstantDistance(aCurveForInterval, theP, aLower, anUpper))
        {
          myResult.Status                 = ExtremaPC::Status::InfiniteSolutions;
          myResult.InfiniteSquareDistance =
            theP.SquareDistance(theCurve.Value((aLower + anUpper) * 0.5));
          return myResult;
        }

        MathRoot::MultipleConfig aRetryConfig = aConfig;
        aRetryConfig.FTolerance                 =
          std::min(theTol, ExtremaPC::THE_DEFAULT_TOLERANCE);
        aRetryConfig.NullTolerance = 0.0;
        try
        {
          anIntervalRoots = MathRoot::FindAllRootsWithDerivative(aLocalFunction,
                                                                 aLower,
                                                                 anUpper,
                                                                 aRetryConfig);
        }
        catch (const Standard_Failure&)
        {
          myResult.Status = ExtremaPC::Status::NumericalError;
          return myResult;
        }
        if (!anIntervalRoots.IsDone() || anIntervalRoots.IsAllNull)
        {
          myResult.Status = ExtremaPC::Status::NumericalError;
          return myResult;
        }
      }
      for (size_t aRootIndex = 0; aRootIndex < anIntervalRoots.NbRoots(); ++aRootIndex)
      {
        appendRoot(aRoots, RootCandidate{anIntervalRoots[aRootIndex], aLower, anUpper});
      }
    }
    ExtremaPC_DistanceFunction aFunction(theCurve, theP);
    const bool isClosed = IsClosedDomain(theCurve, theDomain);
    classifyRoots(theCurve,
                  theP,
                  theDomain,
                  aFunction,
                  aContinuityBounds,
                  aRoots,
                  theTol,
                  theMode);
    classifyJunctions(theCurve,
                      theP,
                      theDomain,
                       aFunction,
                       aContinuityBounds,
                       aPartition,
                       aRoots,
                       isClosed,
                      theTol,
                      theMode);

    myResult.Status = myResult.Extrema.IsEmpty() ? ExtremaPC::Status::NoSolution
                                                 : ExtremaPC::Status::OK;
    return myResult;
  }

  //! Builds a uniform parameter partition.
  static math_Vector BuildUniformParams(double theUMin, double theUMax, int theNbSamples)
  {
    math_Vector  aParams(1, theNbSamples);
    const double aStep = (theUMax - theUMin) / (theNbSamples - 1);
    for (int anIndex = 1; anIndex <= theNbSamples; ++anIndex)
    {
      aParams(anIndex) = theUMin + (anIndex - 1) * aStep;
    }
    aParams(theNbSamples) = theUMax;
    return aParams;
  }

  //! Builds a deflection-aware partition within each C2-smooth curve interval.
  static math_Vector BuildCurveAwareParams(const Adaptor3d_Curve&     theCurve,
                                           const ExtremaPC::Domain1D& theDomain)
  {
    try
    {
      NCollection_LinearVector<double> aParameters;
      insertParameter(aParameters, theDomain.Min);
      insertParameter(aParameters, theDomain.Max);

      const int aNbIntervals = theCurve.NbIntervals(GeomAbs_C2);
      NCollection_Array1<double> anIntervals(1, aNbIntervals + 1);
      theCurve.Intervals(anIntervals, GeomAbs_C2);
      for (int anInterval = anIntervals.Lower(); anInterval < anIntervals.Upper(); ++anInterval)
      {
        const double aLower = std::clamp(anIntervals.Value(anInterval),
                                         theDomain.Min,
                                         theDomain.Max);
        const double anUpper = std::clamp(anIntervals.Value(anInterval + 1),
                                          theDomain.Min,
                                          theDomain.Max);
        if (anUpper - aLower <= ExtremaPC::THE_PARAM_TOLERANCE)
        {
          continue;
        }

        insertParameter(aParameters, aLower);
        insertParameter(aParameters, anUpper);
        occ::handle<Adaptor3d_Curve> aLocalCurve =
          theCurve.Trim(aLower, anUpper, Precision::PConfusion());
        if (aLocalCurve.IsNull())
        {
          continue;
        }

        const occ::handle<NCollection_HArray1<double>> aDeflectionParameters =
          Extrema_CurveTool::DeflCurvIntervals(*aLocalCurve);
        if (aDeflectionParameters.IsNull())
        {
          continue;
        }
        for (double aParameter : aDeflectionParameters->Array1())
        {
          insertParameter(aParameters, std::clamp(aParameter, aLower, anUpper));
        }
      }

      math_Vector aResult(1, static_cast<int>(aParameters.Size()));
      for (size_t anIndex = 0; anIndex < aParameters.Size(); ++anIndex)
      {
        aResult(static_cast<int>(anIndex + 1)) = aParameters.Value(anIndex);
      }
      return aResult;
    }
    catch (const Standard_Failure&)
    {
      return BuildUniformParams(theDomain.Min,
                                theDomain.Max,
                                ExtremaPC::THE_OTHER_CURVE_NB_SAMPLES);
    }
  }

  //! Returns true when the domain endpoints represent the same curve point.
  static bool IsClosedDomain(const Adaptor3d_Curve&     theCurve,
                             const ExtremaPC::Domain1D& theDomain)
  {
    if (!theDomain.IsFinite() || theDomain.Min == theDomain.Max)
    {
      return false;
    }
    try
    {
      return theCurve.Value(theDomain.Min).Distance(theCurve.Value(theDomain.Max))
             <= Precision::Confusion();
    }
    catch (const Standard_Failure&)
    {
      return false;
    }
  }

private:
  //! Root and the non-uniform cell used to isolate it.
  struct RootCandidate
  {
    double Parameter;
    double LeftBound;
    double RightBound;
  };

  //! Validates the cached partition and its coverage of the requested domain.
  bool validateParams(const ExtremaPC::Domain1D& theDomain) const
  {
    if (myParams.Length() < 2)
    {
      return false;
    }
    if (std::abs(myParams.First() - theDomain.Min) > ExtremaPC::THE_PARAM_TOLERANCE
        || std::abs(myParams.Last() - theDomain.Max) > ExtremaPC::THE_PARAM_TOLERANCE)
    {
      return false;
    }
    for (int anIndex = 1; anIndex < myParams.Length(); ++anIndex)
    {
      if (!std::isfinite(myParams[anIndex - 1]) || !std::isfinite(myParams[anIndex])
          || myParams[anIndex] <= myParams[anIndex - 1])
      {
        return false;
      }
    }
    return true;
  }

  //! Detects a constant curve from the complete cached partition.
  bool isConstantCurve(const Adaptor3d_Curve& theCurve) const
  {
    try
    {
      const gp_Pnt aReference = theCurve.Value(myParams.First());
      for (int aParamIndex = 0; aParamIndex + 1 < myParams.Length(); ++aParamIndex)
      {
        const double aLower = myParams[aParamIndex];
        const double anUpper = myParams[aParamIndex + 1];
        if (theCurve.Value(anUpper).Distance(aReference) > gp::Resolution())
        {
          return false;
        }
        for (int aProbeIndex = 1; aProbeIndex <= 2; ++aProbeIndex)
        {
          const double aParameter =
            (aLower * (3 - aProbeIndex) + anUpper * aProbeIndex) / 3.0;
          gp_Pnt aPoint;
          gp_Vec aD1;
          theCurve.D1(aParameter, aPoint, aD1);
          if (aD1.Magnitude() > gp::Resolution()
              || aPoint.Distance(aReference) > gp::Resolution())
          {
            return false;
          }
        }
      }
      return true;
    }
    catch (const Standard_Failure&)
    {
      return false;
    }
  }

  //! Detects an interval whose distance to the query point is identically constant.
  static bool isConstantDistance(const Adaptor3d_Curve& theCurve,
                                 const gp_Pnt&          thePoint,
                                 double                 theLower,
                                 double                 theUpper)
  {
    constexpr int THE_NB_PROBES = 7;
    try
    {
      const double aReference = thePoint.SquareDistance(theCurve.Value(theLower));
      for (int aProbe = 1; aProbe <= THE_NB_PROBES; ++aProbe)
      {
        const double aParameter =
          (theLower * (THE_NB_PROBES - aProbe) + theUpper * aProbe) / THE_NB_PROBES;
        const double aSquareDistance = thePoint.SquareDistance(theCurve.Value(aParameter));
        if (std::abs(aSquareDistance - aReference) > Precision::SquareConfusion())
        {
          return false;
        }
      }
      return true;
    }
    catch (const Standard_Failure&)
    {
      return false;
    }
  }

  //! Inserts a parameter in ascending order unless it is already present.
  static void insertParameter(NCollection_LinearVector<double>& theParameters, double theParameter)
  {
    for (size_t anIndex = 0; anIndex < theParameters.Size(); ++anIndex)
    {
      if (std::abs(theParameters.Value(anIndex) - theParameter)
          <= ExtremaPC::THE_PARAM_TOLERANCE)
      {
        return;
      }
      if (theParameter < theParameters.Value(anIndex))
      {
        theParameters.Append(theParameter);
        for (size_t aMove = theParameters.Size() - 1; aMove > anIndex; --aMove)
        {
          theParameters.ChangeValue(aMove) = theParameters.Value(aMove - 1);
        }
        theParameters.ChangeValue(anIndex) = theParameter;
        return;
      }
    }
    theParameters.Append(theParameter);
  }

  //! Combines cached sampling parameters with C2 continuity boundaries.
  void buildPartition(const Adaptor3d_Curve&             theCurve,
                      const ExtremaPC::Domain1D&         theDomain,
                      NCollection_LinearVector<double>& theContinuityBounds,
                      NCollection_LinearVector<double>& thePartition) const
  {
    for (int aParamIndex = 0; aParamIndex < myParams.Length(); ++aParamIndex)
    {
      insertParameter(thePartition, myParams[aParamIndex]);
    }

    try
    {
      const int aNbIntervals = theCurve.NbIntervals(GeomAbs_C2);
      NCollection_Array1<double> anIntervals(1, aNbIntervals + 1);
      theCurve.Intervals(anIntervals, GeomAbs_C2);
      for (int anIndex = anIntervals.Lower(); anIndex <= anIntervals.Upper(); ++anIndex)
      {
        const double aParameter =
          std::clamp(anIntervals.Value(anIndex), theDomain.Min, theDomain.Max);
        insertParameter(thePartition, aParameter);
        if (aParameter > theDomain.Min + ExtremaPC::THE_PARAM_TOLERANCE
            && aParameter < theDomain.Max - ExtremaPC::THE_PARAM_TOLERANCE)
        {
          insertParameter(theContinuityBounds, aParameter);
        }
      }
    }
    catch (const Standard_Failure&)
    {
      // Cached sampling remains usable for adaptors without interval APIs.
    }
  }

  //! Appends a sorted root, merging the cells of an equivalent boundary root.
  static void appendRoot(NCollection_LinearVector<RootCandidate>& theRoots,
                         const RootCandidate&                      theRoot)
  {
    for (size_t anIndex = 0; anIndex < theRoots.Size(); ++anIndex)
    {
      RootCandidate& anExisting = theRoots.ChangeValue(anIndex);
      if (std::abs(anExisting.Parameter - theRoot.Parameter)
          <= ExtremaPC::THE_PARAM_TOLERANCE)
      {
        anExisting.LeftBound  = std::min(anExisting.LeftBound, theRoot.LeftBound);
        anExisting.RightBound = std::max(anExisting.RightBound, theRoot.RightBound);
        return;
      }
      if (theRoot.Parameter < anExisting.Parameter)
      {
        theRoots.Append(theRoot);
        for (size_t aMove = theRoots.Size() - 1; aMove > anIndex; --aMove)
        {
          theRoots.ChangeValue(aMove) = theRoots.Value(aMove - 1);
        }
        theRoots.ChangeValue(anIndex) = theRoot;
        return;
      }
    }
    theRoots.Append(theRoot);
  }

  //! Returns the stationarity sign at a probe, treating small values as unresolved.
  static int stationaritySign(ExtremaPC_DistanceFunction& theFunction,
                              double                      theParameter,
                              double                      theTol)
  {
    double aValue = 0.0;
    try
    {
      const double aSignTolerance = std::min(theTol, ExtremaPC::THE_DEFAULT_TOLERANCE);
      if (!theFunction.Value(theParameter, aValue) || std::abs(aValue) <= aSignTolerance)
      {
        return 0;
      }
      return aValue < 0.0 ? -1 : 1;
    }
    catch (const Standard_Failure&)
    {
      return 0;
    }
  }

  //! Adds one classified extremum if it matches the requested mode and is not duplicated.
  void addExtremum(const Adaptor3d_Curve& theCurve,
                   const gp_Pnt&          theP,
                   double                 theParameter,
                   int                    theLeftSign,
                   int                    theRightSign,
                   ExtremaPC::SearchMode  theMode) const
  {
    const bool isMinimum = theLeftSign < 0 && theRightSign > 0;
    const bool isMaximum = theLeftSign > 0 && theRightSign < 0;
    if ((!isMinimum && !isMaximum)
        || (theMode == ExtremaPC::SearchMode::Min && !isMinimum)
        || (theMode == ExtremaPC::SearchMode::Max && !isMaximum))
    {
      return;
    }

    for (int anIndex = 0; anIndex < myResult.Extrema.Length(); ++anIndex)
    {
      if (std::abs(myResult.Extrema.Value(anIndex).Parameter - theParameter)
          <= ExtremaPC::THE_PARAM_TOLERANCE)
      {
        return;
      }
    }

    const gp_Pnt aCurvePoint = theCurve.Value(theParameter);
    ExtremaPC::ExtremumResult anExtremum;
    anExtremum.Parameter      = theParameter;
    anExtremum.Point          = aCurvePoint;
    anExtremum.SquareDistance = theP.SquareDistance(aCurvePoint);
    anExtremum.IsMinimum      = isMinimum;
    anExtremum.IsMaximum      = isMaximum;
    myResult.Extrema.Append(anExtremum);
  }

  //! Classifies smooth stationary roots within their isolation cells.
  void classifyRoots(const Adaptor3d_Curve&                  theCurve,
                     const gp_Pnt&                           theP,
                     const ExtremaPC::Domain1D&              theDomain,
                     ExtremaPC_DistanceFunction&             theFunction,
                     const NCollection_LinearVector<double>& theContinuityBounds,
                     const NCollection_LinearVector<RootCandidate>& theRoots,
                     double                                  theTol,
                     ExtremaPC::SearchMode                   theMode) const
  {
    for (size_t aRootIndex = 0; aRootIndex < theRoots.Size(); ++aRootIndex)
    {
      const RootCandidate& aRoot = theRoots.Value(aRootIndex);
      bool                 isJunction = false;
      for (size_t aJunctionIndex = 0; aJunctionIndex < theContinuityBounds.Size();
           ++aJunctionIndex)
      {
        if (std::abs(aRoot.Parameter - theContinuityBounds.Value(aJunctionIndex))
            <= ExtremaPC::THE_PARAM_TOLERANCE)
        {
          isJunction = true;
          break;
        }
      }
      if (isJunction)
      {
        continue;
      }

      const bool isAtFirst = aRoot.Parameter <= theDomain.Min + ExtremaPC::THE_PARAM_TOLERANCE;
      const bool isAtLast  = aRoot.Parameter >= theDomain.Max - ExtremaPC::THE_PARAM_TOLERANCE;
      if (isAtFirst || isAtLast)
      {
        continue;
      }

      double aLeftBound  = aRoot.LeftBound;
      double aRightBound = aRoot.RightBound;
      if (aRootIndex > 0)
      {
        const RootCandidate& aPrevious = theRoots.Value(aRootIndex - 1);
        aLeftBound = std::max(aLeftBound, (aPrevious.Parameter + aRoot.Parameter) * 0.5);
      }
      if (aRootIndex + 1 < theRoots.Size())
      {
        const RootCandidate& aNext = theRoots.Value(aRootIndex + 1);
        aRightBound = std::min(aRightBound, (aRoot.Parameter + aNext.Parameter) * 0.5);
      }
      const int aLeftSign =
        stationaritySign(theFunction, (aLeftBound + aRoot.Parameter) * 0.5, theTol);
      const int aRightSign =
        stationaritySign(theFunction, (aRoot.Parameter + aRightBound) * 0.5, theTol);
      addExtremum(theCurve, theP, aRoot.Parameter, aLeftSign, aRightSign, theMode);
    }
  }

  //! Classifies C2 continuity boundaries and the periodic seam from one-sided values.
  void classifyJunctions(const Adaptor3d_Curve&                  theCurve,
                         const gp_Pnt&                           theP,
                         const ExtremaPC::Domain1D&              theDomain,
                         ExtremaPC_DistanceFunction&             theFunction,
                         const NCollection_LinearVector<double>& theContinuityBounds,
                         const NCollection_LinearVector<double>& thePartition,
                         const NCollection_LinearVector<RootCandidate>& theRoots,
                         bool                                    theIsClosed,
                         double                                  theTol,
                         ExtremaPC::SearchMode                   theMode) const
  {
    for (size_t aJunctionIndex = 0; aJunctionIndex < theContinuityBounds.Size(); ++aJunctionIndex)
    {
      const double aJunction = theContinuityBounds.Value(aJunctionIndex);
      double       aLeftBound  = theDomain.Min;
      double       aRightBound = theDomain.Max;
      for (size_t aParamIndex = 1; aParamIndex < thePartition.Size(); ++aParamIndex)
      {
        if (thePartition.Value(aParamIndex) >= aJunction)
        {
          aLeftBound  = thePartition.Value(aParamIndex - 1);
          aRightBound = thePartition.Value(aParamIndex);
          if (std::abs(aRightBound - aJunction) <= ExtremaPC::THE_PARAM_TOLERANCE
              && aParamIndex + 1 < thePartition.Size())
          {
            aRightBound = thePartition.Value(aParamIndex + 1);
          }
          break;
        }
      }
      for (size_t aRootIndex = 0; aRootIndex < theRoots.Size(); ++aRootIndex)
      {
        const double aRoot = theRoots.Value(aRootIndex).Parameter;
        if (aRoot > aLeftBound + ExtremaPC::THE_PARAM_TOLERANCE
            && aRoot < aJunction - ExtremaPC::THE_PARAM_TOLERANCE)
        {
          aLeftBound = aRoot;
        }
        else if (aRoot > aJunction + ExtremaPC::THE_PARAM_TOLERANCE
                 && aRoot < aRightBound - ExtremaPC::THE_PARAM_TOLERANCE)
        {
          aRightBound = aRoot;
          break;
        }
      }

      const int aLeftSign =
        stationaritySign(theFunction, (aLeftBound + aJunction) * 0.5, theTol);
      const int aRightSign =
        stationaritySign(theFunction, (aJunction + aRightBound) * 0.5, theTol);
      addExtremum(theCurve, theP, aJunction, aLeftSign, aRightSign, theMode);
    }

    if (theIsClosed)
    {
      double aLeftBound  = myParams[myParams.Length() - 2];
      double aRightBound = myParams[1];
      for (size_t aRootIndex = 0; aRootIndex < theRoots.Size(); ++aRootIndex)
      {
        const double aRoot = theRoots.Value(aRootIndex).Parameter;
        if (aRoot > aLeftBound + ExtremaPC::THE_PARAM_TOLERANCE
            && aRoot < theDomain.Max - ExtremaPC::THE_PARAM_TOLERANCE)
        {
          aLeftBound = aRoot;
        }
        if (aRoot > theDomain.Min + ExtremaPC::THE_PARAM_TOLERANCE
            && aRoot < aRightBound - ExtremaPC::THE_PARAM_TOLERANCE)
        {
          aRightBound = aRoot;
        }
      }
      const int aLeftSign =
        stationaritySign(theFunction, (aLeftBound + theDomain.Max) * 0.5, theTol);
      const int aRightSign =
        stationaritySign(theFunction, (theDomain.Min + aRightBound) * 0.5, theTol);
      addExtremum(theCurve, theP, theDomain.Min, aLeftSign, aRightSign, theMode);
    }
  }

private:
  NCollection_Array1<double> myParams; //!< Cached parameter partition
  mutable ExtremaPC::Result  myResult; //!< Reusable result
};

#endif // _ExtremaPC_GridEvaluator_HeaderFile
