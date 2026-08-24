// Copyright (c) 2014 OPEN CASCADE SAS
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

#ifndef _BSplSLib_Cache_Headerfile
#define _BSplSLib_Cache_Headerfile

#include <NCollection_Array2.hxx>
#include <NCollection_HArray2.hxx>

#include <BSplCLib_CacheParams.hxx>

//! \brief A cache class for Bezier and B-spline surfaces.
//!
//! Defines all data, that can be cached on a span of the surface.
//! The data should be recalculated in going from span to span.
class BSplSLib_Cache : public Standard_Transient
{
public:
  //! Constructor for caching of the span for the surface
  //! \param theDegreeU    degree along the first parameter (U) of the surface
  //! \param thePeriodicU  identify the surface is periodical along U axis
  //! \param theFlatKnotsU knots of the surface (with repetition) along U axis
  //! \param theDegreeV    degree along the second parameter (V) of the surface
  //! \param thePeriodicV  identify the surface is periodical along V axis
  //! \param theFlatKnotsV knots of the surface (with repetition) along V axis
  //! \param theWeights    array of weights of corresponding poles
  Standard_EXPORT BSplSLib_Cache(const int&                        theDegreeU,
                                 const bool&                       thePeriodicU,
                                 const NCollection_Array1<double>& theFlatKnotsU,
                                 const int&                        theDegreeV,
                                 const bool&                       thePeriodicV,
                                 const NCollection_Array1<double>& theFlatKnotsV,
                                 const NCollection_Array2<double>* theWeights = nullptr);

  //! Verifies validity of the cache using parameters of the point
  //! \param theParameterU  first parameter of the point placed in the span
  //! \param theParameterV  second parameter of the point placed in the span
  Standard_EXPORT bool IsCacheValid(double theParameterU, double theParameterV) const;

  //! Recomputes the cache data. Does not verify validity of the cache
  //! \param theParameterU  the parametric value on the U axis to identify the span
  //! \param theParameterV  the parametric value on the V axis to identify the span
  //! \param theDegreeU     degree along U axis
  //! \param thePeriodicU   identify whether the surface is periodic along U axis
  //! \param theFlatKnotsU  flat knots of the surface along U axis
  //! \param theDegreeV     degree along V axis
  //! \param thePeriodicV   identify whether the surface is periodic along V axis
  //! \param theFlatKnotsV  flat knots of the surface along V axis
  //! \param thePoles       array of poles of the surface
  //! \param theWeights     array of weights of corresponding poles
  Standard_EXPORT void BuildCache(const double&                     theParameterU,
                                  const double&                     theParameterV,
                                  const NCollection_Array1<double>& theFlatKnotsU,
                                  const NCollection_Array1<double>& theFlatKnotsV,
                                  const NCollection_Array2<gp_Pnt>& thePoles,
                                  const NCollection_Array2<double>* theWeights = nullptr);

  //! Recomputes cache data for already located flat-knot spans.
  //! @param[in] theSpanU flat-knot span index in U
  //! @param[in] theSpanV flat-knot span index in V
  //! @param[in] theFlatKnotsU flat knots in U
  //! @param[in] theFlatKnotsV flat knots in V
  //! @param[in] thePoles surface poles
  //! @param[in] theWeights optional surface weights
  Standard_EXPORT void BuildCacheForSpan(const int                         theSpanU,
                                         const int                         theSpanV,
                                         const NCollection_Array1<double>& theFlatKnotsU,
                                         const NCollection_Array1<double>& theFlatKnotsV,
                                         const NCollection_Array2<gp_Pnt>& thePoles,
                                         const NCollection_Array2<double>* theWeights = nullptr);

  //! Calculates the point on the surface for specified parameters
  //! \param[in]  theU      first parameter for calculation of the value
  //! \param[in]  theV      second parameter for calculation of the value
  //! \param[out] thePoint  the result of calculation (the point on the surface)
  Standard_EXPORT void D0(const double& theU, const double& theV, gp_Pnt& thePoint) const;

  //! Calculates the point on the surface and its first derivative
  //! \param[in]  theU         first parameter of calculation of the value
  //! \param[in]  theV         second parameter of calculation of the value
  //! \param[out] thePoint     the result of calculation (the point on the surface)
  //! \param[out] theTangentU  tangent vector along U axis in the calculated point
  //! \param[out] theTangentV  tangent vector along V axis in the calculated point
  Standard_EXPORT void D1(const double& theU,
                          const double& theV,
                          gp_Pnt&       thePoint,
                          gp_Vec&       theTangentU,
                          gp_Vec&       theTangentV) const;

  //! Calculates the point on the surface and derivatives till second order
  //! \param[in]  theU            first parameter of calculation of the value
  //! \param[in]  theV            second parameter of calculation of the value
  //! \param[out] thePoint        the result of calculation (the point on the surface)
  //! \param[out] theTangentU     tangent vector along U axis in the calculated point
  //! \param[out] theTangentV     tangent vector along V axis in the calculated point
  //! \param[out] theCurvatureU   curvature vector (2nd derivative on U) along U axis
  //! \param[out] theCurvatureV   curvature vector (2nd derivative on V) along V axis
  //! \param[out] theCurvatureUV  2nd mixed derivative on U anv V
  Standard_EXPORT void D2(const double& theU,
                          const double& theV,
                          gp_Pnt&       thePoint,
                          gp_Vec&       theTangentU,
                          gp_Vec&       theTangentV,
                          gp_Vec&       theCurvatureU,
                          gp_Vec&       theCurvatureV,
                          gp_Vec&       theCurvatureUV) const;

  //! Calculates the point using pre-computed local parameters in [-1, 1] range.
  //! This bypasses periodic normalization and local parameter calculation.
  //! @param[in]  theLocalU pre-computed local U parameter: (U - SpanMid) / SpanHalfLen
  //! @param[in]  theLocalV pre-computed local V parameter: (V - SpanMid) / SpanHalfLen
  //! @param[out] thePoint  the result of calculation (the point on the surface)
  Standard_EXPORT void D0Local(double theLocalU, double theLocalV, gp_Pnt& thePoint) const;

  //! Evaluates points on a Cartesian product of pre-computed local parameters.
  //! Consecutive V samples are adjacent; the row stride permits direct evaluation into a
  //! sub-grid of a larger array.
  //! @param[in] theLocalU local U parameters in [-1, 1]
  //! @param[in] theNbU number of U parameters
  //! @param[in] theLocalV local V parameters in [-1, 1]
  //! @param[in] theNbV number of V parameters
  //! @param[out] theResults first point of the destination block
  //! @param[in] theRowStride distance in points between consecutive U rows
  Standard_EXPORT void D0GridLocal(const double* theLocalU,
                                   const size_t  theNbU,
                                   const double* theLocalV,
                                   const size_t  theNbV,
                                   gp_Pnt*       theResults,
                                   const size_t  theRowStride) const;

  //! Calculates the point and first derivatives using pre-computed local parameters in [-1, 1]
  //! range. This bypasses periodic normalization and local parameter calculation.
  //! @param[in]  theLocalU   pre-computed local U parameter: (U - SpanMid) / SpanHalfLen
  //! @param[in]  theLocalV   pre-computed local V parameter: (V - SpanMid) / SpanHalfLen
  //! @param[out] thePoint    the result of calculation (the point on the surface)
  //! @param[out] theTangentU tangent vector along U axis in the calculated point
  //! @param[out] theTangentV tangent vector along V axis in the calculated point
  Standard_EXPORT void D1Local(double  theLocalU,
                               double  theLocalV,
                               gp_Pnt& thePoint,
                               gp_Vec& theTangentU,
                               gp_Vec& theTangentV) const;

  //! Evaluates a Cartesian product of local parameters using shared tensor contractions.
  //! Results are written as nine doubles per sample: P, D1U, D1V. Consecutive V samples are
  //! adjacent; the row stride permits direct evaluation into a sub-grid of a larger array.
  //! @param[in] theLocalU local U parameters in [-1, 1]
  //! @param[in] theNbU number of U parameters
  //! @param[in] theLocalV local V parameters in [-1, 1]
  //! @param[in] theNbV number of V parameters
  //! @param[out] theResults first sample of the destination block
  //! @param[in] theRowStride distance in doubles between consecutive U rows
  Standard_EXPORT void D1GridLocal(const double* theLocalU,
                                   const size_t  theNbU,
                                   const double* theLocalV,
                                   const size_t  theNbV,
                                   double*       theResults,
                                   const size_t  theRowStride) const;

  //! Evaluates points and derivatives through second order on a Cartesian product of local
  //! parameters using shared tensor contractions. Results contain 18 doubles per sample in the
  //! order P, D1U, D1V, D2U, D2V, D2UV.
  //! @param[in] theLocalU local U parameters in [-1, 1]
  //! @param[in] theNbU number of U parameters
  //! @param[in] theLocalV local V parameters in [-1, 1]
  //! @param[in] theNbV number of V parameters
  //! @param[out] theResults first sample of the destination block
  //! @param[in] theRowStride distance in doubles between consecutive U rows
  Standard_EXPORT void D2GridLocal(const double* theLocalU,
                                   const size_t  theNbU,
                                   const double* theLocalV,
                                   const size_t  theNbV,
                                   double*       theResults,
                                   const size_t  theRowStride) const;

  //! Calculates the point and derivatives till second order using pre-computed local parameters.
  //! This bypasses periodic normalization and local parameter calculation.
  //! @param[in]  theLocalU      pre-computed local U parameter: (U - SpanMid) / SpanHalfLen
  //! @param[in]  theLocalV      pre-computed local V parameter: (V - SpanMid) / SpanHalfLen
  //! @param[out] thePoint       the result of calculation (the point on the surface)
  //! @param[out] theTangentU    tangent vector along U axis in the calculated point
  //! @param[out] theTangentV    tangent vector along V axis in the calculated point
  //! @param[out] theCurvatureU  curvature vector (2nd derivative on U) along U axis
  //! @param[out] theCurvatureV  curvature vector (2nd derivative on V) along V axis
  //! @param[out] theCurvatureUV 2nd mixed derivative on U and V
  Standard_EXPORT void D2Local(double  theLocalU,
                               double  theLocalV,
                               gp_Pnt& thePoint,
                               gp_Vec& theTangentU,
                               gp_Vec& theTangentV,
                               gp_Vec& theCurvatureU,
                               gp_Vec& theCurvatureV,
                               gp_Vec& theCurvatureUV) const;

  DEFINE_STANDARD_RTTIEXT(BSplSLib_Cache, Standard_Transient)

private:
  void buildCache(const NCollection_Array1<double>& theFlatKnotsU,
                  const NCollection_Array1<double>& theFlatKnotsV,
                  const NCollection_Array2<gp_Pnt>& thePoles,
                  const NCollection_Array2<double>* theWeights);

  // copying is prohibited
  BSplSLib_Cache(const BSplSLib_Cache&) = delete;
  void operator=(const BSplSLib_Cache&) = delete;

private:
  // clang-format off
  bool myIsRational;                //!< identifies the rationality of Bezier/B-spline surface
  BSplCLib_CacheParams myParamsU, myParamsV;    //!< cache parameters by U and V directions
  occ::handle<NCollection_HArray2<double>> myPolesWeights; //!< array of poles and weights of calculated cache
                                                // the array has following structure:
                                                //       x11 y11 z11 [w11] x12 y12 z12 [w12] ...
                                                //       x21 y21 z21 [w21] x22 y22 z22 [w22] etc
                                                // for non-rational surfaces there is no weight;
                                                // size of array: (max(myDegree)+1) * A*(min(myDegree)+1), where A = 4 or 3
  // clang-format on
};

#endif
