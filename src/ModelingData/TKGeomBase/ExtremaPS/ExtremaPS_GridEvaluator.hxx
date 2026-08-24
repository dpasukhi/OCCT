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
  using GridSample = GeomGridEval::SurfD1Coords;

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
                               const NCollection_Array1<double>&                theUParams,
                               const NCollection_Array1<double>&                theVParams);

  //! Takes ownership of a compact D1 grid without copying its samples.
  //! @param[in] theD1Grid sampled surface points and first derivatives
  //! @param[in] theUParams ordered U parameters defining the grid rows
  //! @param[in] theVParams ordered V parameters defining the grid columns
  Standard_EXPORT void SetGrid(NCollection_Array2<GeomGridEval::SurfD1Coords>&& theD1Grid,
                               NCollection_Array1<double>&&                      theUParams,
                               NCollection_Array1<double>&&                      theVParams);

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
  Standard_EXPORT static NCollection_Array1<double> BuildUniformParams(const double theMin,
                                                                       const double theMax,
                                                                       const size_t theNbSamples);

private:
  static constexpr double        THE_GRADIENT_TOL_FACTOR      = 100.0;
  static constexpr double        THE_AFFINE_COORD_TOL_FACTOR  = 16.0;
  //! Cell span balancing conservative-bound precision against block traversal cost.
  static constexpr size_t        THE_BLOCK_NB_CELLS           = 4;
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
    double        Fu;     //!< U stationarity function
    double        Fv;     //!< V stationarity function
    unsigned char SignU;  //!< Bit 0 for negative, bit 1 for positive U gradient
    unsigned char SignV;  //!< Bit 0 for negative, bit 1 for positive V gradient
  };

  //! Coefficients for evaluating one grid sample along a query line.
  struct AffineSample
  {
    double FuOrigin;      //!< U stationarity value at the line origin
    double FvOrigin;      //!< V stationarity value at the line origin
    double FuDirection;   //!< U stationarity reduction per line parameter
    double FvDirection;   //!< V stationarity reduction per line parameter
    double DistOrigin;    //!< Squared distance at the line origin
    double DistDirection; //!< Linear coefficient of squared distance
  };

  //! Grid location and value of a sampled distance extremum.
  struct GridDistanceExtremum
  {
    size_t I;
    size_t J;
    double SquareDistance;
  };

  //! Conservative geometry bounds for a rectangular group of grid cells.
  struct GridBlock
  {
    size_t FirstU;
    size_t LastU;
    size_t FirstV;
    size_t LastV;
    double FuMin[4];
    double FuMax[4];
    double FvMin[4];
    double FvMax[4];
    double PointMin[3];
    double PointMax[3];
  };

  static GridPointData evaluateGridPoint(const GridSample& theSample,
                                         const double      thePx,
                                         const double      thePy,
                                         const double      thePz);

  //! Evaluates a sample using coefficients prepared during an earlier direct scan.
  GridPointData evaluateAffineGridPoint(const AffineSample& theSample,
                                        const double        theParameter) const;

  //! Detects whether the query belongs to the currently established line.
  bool prepareAffineEvaluation(const gp_Pnt& thePoint, double& theParameter) const;

  //! Allocates query workspace, stores grid parameters, and resets geometry-dependent state.
  void initializeGridState(const NCollection_Array1<double>& theUParams,
                           const NCollection_Array1<double>& theVParams);

  //! Allocates query workspace, takes ownership of grid parameters, and resets state.
  void initializeGridState(NCollection_Array1<double>&& theUParams,
                           NCollection_Array1<double>&& theVParams);

  //! Builds conservative block bounds used to reject groups of grid cells.
  void buildBlockHierarchy() const;

  //! Builds grid coefficients for repeated queries along the established line.
  void buildAffineGrid() const;

  //! Scans 2D grid to find candidate cells for extrema.
  //! Uses pre-computed grid point data to avoid redundant calculations.
  void scanGrid(const gp_Pnt& theP, const ExtremaPS::SearchMode theMode) const;

  //! Scans reusable block bounds and evaluates only blocks that cannot be rejected.
  void scanGridHierarchical(const gp_Pnt&               theP,
                            const double                theAffineParameter,
                            const ExtremaPS::SearchMode theMode) const;

  //! Scans the complete grid without constructing reusable hierarchy state.
  void scanGridDense(const gp_Pnt& theP, const ExtremaPS::SearchMode theMode) const;

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

private:
  NCollection_Array2<GridSample> myGrid; //!< D1 grid built for the bound surface

  mutable ExtremaPS::Result                   myResult;     //!< Reusable result
  mutable NCollection_LinearVector<Candidate> myCandidates; //!< Candidates from grid scan
  mutable NCollection_LinearVector<std::pair<double, double>>
                                              myFoundRoots;    //!< Found roots for dedup
  mutable NCollection_LinearVector<SortEntry> mySortedEntries; //!< Sorted candidate indices
  mutable NCollection_Array1<GridPointData>   myRowData[2];    //!< Two scan rows
  mutable NCollection_Array1<AffineSample>    myAffineGrid;    //!< Coherent-query coefficients
  mutable NCollection_LinearVector<GridBlock> myGridBlocks;    //!< Conservative cell blocks
  NCollection_Array1<double>                  myUParams;       //!< Ordered U parameters
  NCollection_Array1<double>                  myVParams;       //!< Ordered V parameters

  mutable gp_Pnt myPreviousQuery;   //!< Most recent query used for line detection
  mutable gp_Pnt myAffineOrigin;    //!< Origin of the current coherent-query line
  mutable gp_Vec myAffineDirection; //!< Direction from the first to the second line query
  mutable double myAffineDirectionSq  = 0.0;   //!< Squared line-direction magnitude
  mutable int    myAffineAxis         = 0;     //!< Stable coordinate used for line parameterization
  mutable bool   myHasPreviousQuery   = false; //!< Whether line detection has a first query
  mutable bool   myHasAffineDirection = false; //!< Whether the current line is defined

  mutable std::optional<GridDistanceExtremum> myGridMinimum;
  mutable std::optional<GridDistanceExtremum> myGridMaximum;

};

#endif // _ExtremaPS_GridEvaluator_HeaderFile
