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

#include <ExtremaPC.hxx>
#include <MathRoot_Multiple.hxx>
#include <math_Vector.hxx>
#include <NCollection_LinearVector.hxx>

#include <optional>

class Adaptor3d_Curve;
class ExtremaPC_DistanceFunction;

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

  //! Stores the numerical search partition and root-sampling density.
  //! @param[in] theParams ordered parameter values
  //! @param[in] theNbRootIntervals requested root-isolation intervals per partition interval
  Standard_EXPORT void SetParams(const math_Vector& theParams,
                                 const size_t theNbRootIntervals = MathRoot::THE_MIN_NB_INTERVALS);

  //! Returns mutable reference to the result for post-processing.
  //! @return reusable result storage owned by this evaluator
  Standard_EXPORT ExtremaPC::Result& ChangeResult() const;

  //! Performs extrema computation using the cached parameter partition.
  //! Only stationary extrema inside the supplied domain are returned.
  //! @param[in] theCurve curve adaptor evaluated during root refinement
  //! @param[in] theP query point
  //! @param[in] theDomain parameter domain matching the cached partition
  //! @param[in] theTol tolerance for root refinement and duplicate removal
  //! @param[in] theMode requested extrema type
  //! @return evaluator-owned result, valid until the next computation
  [[nodiscard]] Standard_EXPORT const ExtremaPC::Result& Perform(
    const Adaptor3d_Curve&      theCurve,
    const gp_Pnt&               theP,
    const ExtremaPC::Domain1D&  theDomain,
    const double                theTol,
    const ExtremaPC::SearchMode theMode) const;

  //! Builds a uniform parameter partition.
  //! @param[in] theUMin first parameter
  //! @param[in] theUMax last parameter
  //! @param[in] theNbSamples number of partition parameters, at least two
  //! @return ordered parameter vector including both bounds
  Standard_EXPORT static math_Vector BuildUniformParams(const double theUMin,
                                                        const double theUMax,
                                                        const size_t theNbSamples);

  //! Builds a deflection-aware partition within each C2-smooth curve interval.
  //! @param[in] theCurve curve adaptor to sample
  //! @param[in] theDomain parameter domain to partition
  //! @return ordered parameters suitable for repeated extrema queries
  Standard_EXPORT static math_Vector BuildCurveAwareParams(const Adaptor3d_Curve&     theCurve,
                                                           const ExtremaPC::Domain1D& theDomain);

  //! Returns true when the domain endpoints represent the same curve point.
  //! @param[in] theCurve curve adaptor to evaluate
  //! @param[in] theDomain parameter domain to test
  //! @return true when the domain forms a geometrically closed seam
  Standard_EXPORT static bool IsClosedDomain(const Adaptor3d_Curve&     theCurve,
                                             const ExtremaPC::Domain1D& theDomain);

private:
  //! Root and the non-uniform cell used to isolate it.
  struct RootCandidate
  {
    double Parameter;
    double LeftBound;
    double RightBound;
  };

  bool validateParams(const ExtremaPC::Domain1D& theDomain) const;

  static std::optional<double> constantSquareDistance(const Adaptor3d_Curve&     theCurve,
                                                      const gp_Pnt&              thePoint,
                                                      const ExtremaPC::Domain1D& theDomain);

  static void insertParameter(NCollection_LinearVector<double>& theParameters,
                              const double                      theParameter);

  void buildPartition(const Adaptor3d_Curve&            theCurve,
                      const ExtremaPC::Domain1D&        theDomain,
                      NCollection_LinearVector<double>& theContinuityBounds,
                      NCollection_LinearVector<double>& thePartition) const;

  static void appendRoot(NCollection_LinearVector<RootCandidate>& theRoots,
                         const RootCandidate&                     theRoot);

  static int stationaritySign(ExtremaPC_DistanceFunction& theFunction,
                              const double                theParameter,
                              const double                theTol);

  void addExtremum(const Adaptor3d_Curve&      theCurve,
                   const gp_Pnt&               theP,
                   const double                theParameter,
                   const int                   theLeftSign,
                   const int                   theRightSign,
                   const ExtremaPC::SearchMode theMode) const;

  void classifyRoots(const Adaptor3d_Curve&                         theCurve,
                     const gp_Pnt&                                  theP,
                     const ExtremaPC::Domain1D&                     theDomain,
                     ExtremaPC_DistanceFunction&                    theFunction,
                     const NCollection_LinearVector<double>&        theContinuityBounds,
                     const NCollection_LinearVector<RootCandidate>& theRoots,
                     const double                                   theTol,
                     const ExtremaPC::SearchMode                    theMode) const;

  void classifyJunctions(const Adaptor3d_Curve&                         theCurve,
                         const gp_Pnt&                                  theP,
                         const ExtremaPC::Domain1D&                     theDomain,
                         ExtremaPC_DistanceFunction&                    theFunction,
                         const NCollection_LinearVector<double>&        theContinuityBounds,
                         const NCollection_LinearVector<double>&        thePartition,
                         const NCollection_LinearVector<RootCandidate>& theRoots,
                         const bool                                     theIsClosed,
                         const double                                   theTol,
                         const ExtremaPC::SearchMode                    theMode) const;

private:
  NCollection_LinearVector<double> myParams;                 //!< Cached parameter partition
  size_t myNbRootIntervals = MathRoot::THE_MIN_NB_INTERVALS; //!< Root-isolation intervals per cell
  mutable NCollection_LinearVector<double>        myContinuityBounds; //!< Reusable C2 bounds
  mutable NCollection_LinearVector<double>        myPartition;        //!< Reusable merged partition
  mutable NCollection_LinearVector<RootCandidate> myRoots;            //!< Reusable root candidates
  mutable ExtremaPC::Result                       myResult;           //!< Reusable result
};

#endif // _ExtremaPC_GridEvaluator_HeaderFile
