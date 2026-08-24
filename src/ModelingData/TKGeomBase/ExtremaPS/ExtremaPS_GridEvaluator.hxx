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

#ifndef _ExtremaPS_GridEvaluator_HeaderFile
#define _ExtremaPS_GridEvaluator_HeaderFile

#include <ExtremaPS.hxx>
#include <GeomGridEval.hxx>
#include <math_Vector.hxx>
#include <NCollection_Array1.hxx>
#include <NCollection_Array2.hxx>
#include <NCollection_LinearVector.hxx>

#include <optional>
#include <utility>

class Adaptor3d_Surface;

//! Shared grid and Newton evaluator for numerical point-surface extrema.
//! The grid is built once for a bound surface and reused by subsequent point queries.
class ExtremaPS_GridEvaluator
{
private:
  //! Compact surface sample used by the query-time grid scan.
  struct GridSample
  {
    double PointX;
    double PointY;
    double PointZ;
    double DUX;
    double DUY;
    double DUZ;
    double DVX;
    double DVY;
    double DVZ;
  };

  //! Candidate cell for Newton refinement.
  struct Candidate
  {
    size_t IdxU;    //!< U grid index
    size_t IdxV;    //!< V grid index
    double StartU;  //!< Starting U for Newton
    double StartV;  //!< Starting V for Newton
    double EstDist; //!< Estimated squared distance
    double GradMag; //!< Gradient magnitude (smaller = closer to extremum)
  };

public:
  //! Default constructor.
  ExtremaPS_GridEvaluator() = default;

  //! Stores an evaluated D1 grid and its parameter values.
  //! Replaces geometry-dependent data from any previous initialization.
  //! @param[in] theD1Grid sampled surface points and first derivatives
  //! @param[in] theUParams ordered U parameters matching the grid rows
  //! @param[in] theVParams ordered V parameters matching the grid columns
  Standard_EXPORT void SetGrid(const NCollection_Array2<GeomGridEval::SurfD1>& theD1Grid,
                               const math_Vector&                              theUParams,
                               const math_Vector&                              theVParams);

  //! Returns mutable result storage for evaluator-specific post-processing.
  //! @return reusable result storage owned by this evaluator
  Standard_EXPORT ExtremaPS::Result& ChangeResult() const;

  //! Performs extrema computation using cached grid (interior only).
  //!
  //! @param[in] theSurface surface adaptor evaluated during refinement
  //! @param[in] theP query point
  //! @param[in] theDomain parameter domain matching the cached grid
  //! @param[in] theTol tolerance for refinement and duplicate removal
  //! @param[in] theMode requested extrema type
  //! @return evaluator-owned result with interior extrema, valid until the next computation
  [[nodiscard]] Standard_EXPORT const ExtremaPS::Result& Perform(
    const Adaptor3d_Surface&    theSurface,
    const gp_Pnt&               theP,
    const ExtremaPS::Domain2D&  theDomain,
    const double                theTol,
    const ExtremaPS::SearchMode theMode) const;

  //! Builds uniform parameter grids.
  //! @param[in] theMin first parameter
  //! @param[in] theMax last parameter
  //! @param[in] theNbSamples number of parameters, at least two
  //! @return ordered parameter vector including both bounds
  Standard_EXPORT static math_Vector BuildUniformParams(const double theMin,
                                                        const double theMax,
                                                        const size_t theNbSamples);

private:
  static constexpr double        THE_GRADIENT_TOL_FACTOR      = 100.0;
  static constexpr double        THE_COHERENCE_THRESHOLD_SQ   = 100.0;
  static constexpr double        THE_TRAJECTORY_MIN_RATIO     = 0.5;
  static constexpr double        THE_TRAJECTORY_MAX_RATIO     = 2.0;
  static constexpr double        THE_TRAJECTORY_MIN_COS       = 0.7;
  static constexpr double        THE_TRAJECTORY_QUADRATIC_COS = 0.9;
  static constexpr double        THE_AFFINE_COORD_TOL_FACTOR  = 16.0;
  static constexpr size_t        THE_CACHE_SIZE               = 5;
  static constexpr unsigned char THE_NEGATIVE_SIGN_MASK       = 1;
  static constexpr unsigned char THE_POSITIVE_SIGN_MASK       = 2;
  static constexpr unsigned char THE_BOTH_SIGNS_MASK =
    THE_NEGATIVE_SIGN_MASK | THE_POSITIVE_SIGN_MASK;

  //! Entry for sorting candidates.
  struct SortEntry
  {
    size_t Idx;
    double Dist;
    double GradMag;
  };

  //! Query-dependent distance, gradient magnitude, and gradient sign masks.
  struct GridPointData
  {
    double        GradSq; //!< Squared gradient magnitude
    double        Dist;   //!< Squared distance to query point
    unsigned char SignU;  //!< Bit 0 for negative, bit 1 for positive U gradient
    unsigned char SignV;  //!< Bit 0 for negative, bit 1 for positive V gradient
  };

  //! Coefficients for evaluating one grid sample along a query line.
  struct AffineSample
  {
    double FuOrigin;      //!< U gradient at the query-line origin
    double FvOrigin;      //!< V gradient at the query-line origin
    double FuDirection;   //!< U gradient change per line parameter
    double FvDirection;   //!< V gradient change per line parameter
    double DistOrigin;    //!< Squared distance at the query-line origin
    double DistDirection; //!< Linear squared-distance coefficient
  };

  //! Grid location and value of a sampled distance extremum.
  struct GridDistanceExtremum
  {
    size_t I;
    size_t J;
    double SquareDistance;
  };

    struct CachedSolution
  {
    gp_Pnt QueryPoint;
    double U = 0.0;
    double V = 0.0;
  };

  static GridPointData evaluateGridPoint(const GridSample& theSample,
                                         const double      thePx,
                                         const double      thePy,
                                         const double      thePz);

  //! Evaluates precomputed coefficients for a point on the active query line.
  GridPointData evaluateAffineGridPoint(const AffineSample& theSample,
                                        const double        theParameter) const;

  //! Recognizes a coherent query line and prepares its fixed-origin parameter.
  //! Coefficients are built only after three distinct collinear queries.
  bool prepareAffineEvaluation(const gp_Pnt& thePoint, double& theParameter) const;

  //! Builds query-line coefficients from the immutable surface grid.
  void buildAffineGrid(const gp_Pnt& theOrigin, const gp_Vec& theDirection) const;

  //! Scans 2D grid to find candidate cells for extrema.
  //! Uses pre-computed grid point data to avoid redundant calculations.
  //! @note theTol is intentionally unused - gradient tolerance uses Precision::Confusion() for
  //! consistency.
  void scanGrid(const gp_Pnt&                 theP,
                [[maybe_unused]] const double theTol,
                const ExtremaPS::SearchMode   theMode) const;

  std::optional<std::pair<double, double>> cachedStart(const gp_Pnt&              thePoint,
                                                       const ExtremaPS::Domain2D& theDomain) const;

  //! Adds cached solution as an additional candidate for Newton refinement.
  void addCachedAsCandidate(const gp_Pnt& theP, const ExtremaPS::Domain2D& theDomain) const;

  //! Refines candidates using 2D Newton's method with grid fallback.
  void refineCandidates(const Adaptor3d_Surface&    theSurface,
                        const gp_Pnt&               theP,
                        const ExtremaPS::Domain2D&  theDomain,
                        const double                theTol,
                        const ExtremaPS::SearchMode theMode) const;

  //! Recovers a sampled minimum only after verifying stationarity and curvature.
  void addVerifiedGridMinimum(const Adaptor3d_Surface&    theSurface,
                              const gp_Pnt&               theP,
                              const ExtremaPS::Domain2D&  theDomain,
                              const double                theTol,
                              const ExtremaPS::SearchMode theMode) const;

  //! Adds a solution to the ring buffer cache.
  void addToCache(const gp_Pnt& theP, const double theU, const double theV) const;

  //! Updates the solution cache with the best result.
  void updateSolutionCache(const gp_Pnt& theP, const ExtremaPS::SearchMode theMode) const;

private:
  NCollection_Array2<GridSample> myGrid; //!< D1 grid built for the bound surface

  mutable ExtremaPS::Result                   myResult;     //!< Reusable result
  mutable NCollection_LinearVector<Candidate> myCandidates; //!< Candidates from grid scan
  mutable NCollection_LinearVector<std::pair<double, double>>
                                              myFoundRoots;    //!< Found roots for dedup
  mutable NCollection_LinearVector<SortEntry> mySortedEntries; //!< Sorted candidate indices
  mutable NCollection_Array1<GridPointData>   myRowData[2];    //!< Two scan rows
  mutable NCollection_Array1<AffineSample>    myAffineGrid;    //!< Query-line coefficients
  NCollection_Array1<double>                  myUParams;       //!< Ordered U parameters
  NCollection_Array1<double>                  myVParams;       //!< Ordered V parameters

  mutable gp_Pnt myPreviousQuery;              //!< Query retained to establish a line
  mutable gp_Pnt myAffineOrigin;               //!< Fixed origin of the active query line
  mutable gp_Vec myAffineDirection;            //!< Direction of the active query line
  mutable double myAffineDirectionSq  = 0.0;   //!< Squared query-line direction
  mutable int    myAffineAxis         = 0;     //!< Direction component used to recover parameter
  mutable bool   myHasPreviousQuery   = false; //!< Whether a query is available for line detection
  mutable bool   myHasAffineDirection = false; //!< Whether a candidate query line is available
  mutable bool   myHasAffineGrid      = false; //!< Whether coefficients match the candidate line

  mutable std::optional<GridDistanceExtremum> myGridMinimum;
  mutable std::optional<GridDistanceExtremum> myGridMaximum;

  mutable CachedSolution myCachedSolutions[THE_CACHE_SIZE]; //!< Ring buffer of cached solutions
  mutable size_t         myCacheCount = 0;                  //!< Number of valid entries
  mutable size_t         myCacheIndex = 0;                  //!< Next write index (circular)
};

#endif // _ExtremaPS_GridEvaluator_HeaderFile
