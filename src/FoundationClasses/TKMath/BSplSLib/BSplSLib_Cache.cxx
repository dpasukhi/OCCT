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

#include <BSplSLib_Cache.hxx>
#include <BSplSLib.hxx>

#include <NCollection_LocalArray.hxx>

#include <gp_Pnt.hxx>
#include <NCollection_Array2.hxx>
#include <NCollection_HArray2.hxx>

#include <utility>

IMPLEMENT_STANDARD_RTTIEXT(BSplSLib_Cache, Standard_Transient)

namespace
{

//! Converts handle of array of double into the pointer to double
double* ConvertArray(const occ::handle<NCollection_HArray2<double>>& theHArray)
{
  const NCollection_Array2<double>& anArray = theHArray->Array2();
  return (double*)&(anArray(anArray.LowerRow(), anArray.LowerCol()));
}

//=================================================================================================

//! Computes local UV parameters for D0 evaluation (no derivative scaling needed).
//! BSplSLib uses different convention for span parameters than BSplCLib
//! (Start is in the middle of the span and length is half-span).
std::pair<double, double> toLocalParamsD0(double                      theU,
                                          double                      theV,
                                          const BSplCLib_CacheParams& theParamsU,
                                          const BSplCLib_CacheParams& theParamsV)
{
  const double aNewU        = theParamsU.PeriodicNormalization(theU);
  const double aNewV        = theParamsV.PeriodicNormalization(theV);
  const double aSpanLengthU = 0.5 * theParamsU.SpanLength;
  const double aSpanStartU  = theParamsU.SpanStart + aSpanLengthU;
  const double aSpanLengthV = 0.5 * theParamsV.SpanLength;
  const double aSpanStartV  = theParamsV.SpanStart + aSpanLengthV;
  return {(aNewU - aSpanStartU) / aSpanLengthU, (aNewV - aSpanStartV) / aSpanLengthV};
}

//! Computes local UV parameters and inverse span lengths for derivative evaluation.
//! The same inverse values are used for both parameter transformation and derivative scaling
//! to maintain numerical consistency with the original implementation.
std::pair<double, double> toLocalParams(double                      theU,
                                        double                      theV,
                                        const BSplCLib_CacheParams& theParamsU,
                                        const BSplCLib_CacheParams& theParamsV,
                                        double&                     theInvU,
                                        double&                     theInvV)
{
  const double aNewU        = theParamsU.PeriodicNormalization(theU);
  const double aNewV        = theParamsV.PeriodicNormalization(theV);
  const double aSpanLengthU = 0.5 * theParamsU.SpanLength;
  const double aSpanStartU  = theParamsU.SpanStart + aSpanLengthU;
  const double aSpanLengthV = 0.5 * theParamsV.SpanLength;
  const double aSpanStartV  = theParamsV.SpanStart + aSpanLengthV;
  // Compute inverses once and reuse for both parameter transformation and derivative scaling
  // to maintain numerical consistency with the original implementation
  theInvU = 1.0 / aSpanLengthU;
  theInvV = 1.0 / aSpanLengthV;
  return {(aNewU - aSpanStartU) * theInvU, (aNewV - aSpanStartV) * theInvV};
}

//=================================================================================================

//! Evaluates the polynomials and their derivatives.
//! @param[in] thePolesWeights handle to the array of poles and weights
//! @param[in] theParamsU parameters for U direction
//! @param[in] theParamsV parameters for V direction
//! @param[in] theIsRational flag indicating if the surface is rational
//! @param[in] theLocalU local U parameter
//! @param[in] theLocalV local V parameter
//! @param[in] theUDerivMax maximum U derivative
//! @param[in] theVDerivMax maximum V derivative
//! @param[out] theResultArray array to store the results
void EvaluatePolynomials(const occ::handle<NCollection_HArray2<double>>& thePolesWeights,
                         const BSplCLib_CacheParams&                     theParamsU,
                         const BSplCLib_CacheParams&                     theParamsV,
                         const bool                                      theIsRational,
                         double                                          theLocalU,
                         double                                          theLocalV,
                         int                                             theUDerivMax,
                         int                                             theVDerivMax,
                         double*                                         theResultArray)
{
  double* const aPolesArray = ConvertArray(thePolesWeights);
  const int     aDimension  = theIsRational ? 4 : 3;
  const int     aCacheCols  = thePolesWeights->RowLength();

  const bool isMaxU = (theParamsU.Degree > theParamsV.Degree);
  const auto [aMinParam, aMaxParam] =
    isMaxU ? std::make_pair(theLocalV, theLocalU) : std::make_pair(theLocalU, theLocalV);

  // Determine derivatives to calculate along each direction
  const int aMaxDeriv = isMaxU ? theUDerivMax : theVDerivMax;
  const int aMinDeriv = isMaxU ? theVDerivMax : theUDerivMax;

  // Stride between rows in the result array (corresponds to one step in MaxParam direction)
  // For full grid (required by RationalDerivative), stride is (aMinDeriv + 1) points.
  const int aRowStride = (aMinDeriv + 1) * aDimension;

  // clang-format off
  // Transient coefficients array size:
  // (aMaxDeriv + 1) * CacheCols  for the first evaluation (along max degree parameter)
  // (aMinDeriv + 1) * Dimension for the second evaluation (along min degree parameter)
  NCollection_LocalArray<double> aTransientCoeffs(std::max((aMaxDeriv + 1) * aCacheCols, (aMinDeriv + 1) * aDimension));
  // clang-format on

  // Calculate intermediate values and derivatives of bivariate polynomial along variable with
  // maximal degree
  PLib::EvalPolynomial(aMaxParam,
                       aMaxDeriv,
                       isMaxU ? theParamsU.Degree : theParamsV.Degree,
                       aCacheCols,
                       aPolesArray[0],
                       aTransientCoeffs[0]);

  // Block 0: Evaluate derivatives along variable with minimal degree for D0_max
  // Produces (0,0), (0,1)...(0, aMinDeriv)
  // Writes to offset 0
  PLib::EvalPolynomial(aMinParam,
                       aMinDeriv,
                       isMaxU ? theParamsV.Degree : theParamsU.Degree,
                       aDimension,
                       aTransientCoeffs[0],
                       theResultArray[0]);

  if (aMaxDeriv > 0)
  {
    // Block 1: Evaluate derivatives along variable with minimal degree for D1_max
    // Writes to offset aRowStride (start of second row)
    // If Rational, we need full row (up to aMinDeriv).
    // If Not Rational, we can optimize: we strictly need (1,0) and (1,1).
    // D1Local calls with (1,1) -> aMinDeriv=1. We need up to 1.
    // D2Local calls with (2,2) -> aMinDeriv=2. We need up to 1 for mixed D2.
    // So usually min(aMinDeriv, 1) is sufficient for non-rational.
    const int aDeriv = theIsRational ? aMinDeriv : std::min(aMinDeriv, 1);

    PLib::EvalPolynomial(aMinParam,
                         aDeriv,
                         isMaxU ? theParamsV.Degree : theParamsU.Degree,
                         aDimension,
                         aTransientCoeffs[aCacheCols],
                         theResultArray[aRowStride]);

    if (aMaxDeriv > 1)
    {
      // Block 2: Evaluate derivatives along variable with minimal degree for D2_max
      // Writes to offset 2 * aRowStride (start of third row)
      // If Rational, full row.
      // If Not Rational, we only need (2,0) for standard D2.
      const int aDeriv2 = theIsRational ? aMinDeriv : 0;

      if (aDeriv2 == 0)
      {
        PLib::NoDerivativeEvalPolynomial(aMinParam,
                                         isMaxU ? theParamsV.Degree : theParamsU.Degree,
                                         aDimension,
                                         (isMaxU ? theParamsV.Degree : theParamsU.Degree)
                                           * aDimension,
                                         aTransientCoeffs[aCacheCols << 1],
                                         theResultArray[aRowStride << 1]);
      }
      else
      {
        PLib::EvalPolynomial(aMinParam,
                             aDeriv2,
                             isMaxU ? theParamsV.Degree : theParamsU.Degree,
                             aDimension,
                             aTransientCoeffs[aCacheCols << 1],
                             theResultArray[aRowStride << 1]);
      }
    }
  }

  if (theIsRational)
  {
    // RationalDerivative is NOT safe for in-place operation because it reads 4-component data
    // and writes 3-component data to potentially overlapping memory locations.
    // We need a separate temporary storage for the output.
    const int                      aResultSize = (theUDerivMax + 1) * (theVDerivMax + 1) * 3;
    NCollection_LocalArray<double> aTempStorage(aResultSize);

    if (isMaxU)
    {
      BSplSLib::RationalDerivative(theUDerivMax,
                                   theVDerivMax,
                                   theUDerivMax,
                                   theVDerivMax,
                                   theResultArray[0],
                                   aTempStorage[0]);
    }
    else
    {
      // If V is max degree, our result array is V-major (transposed relative to what
      // RationalDerivative expects) We swap U/V arguments to trick RationalDerivative into
      // processing it correctly.
      BSplSLib::RationalDerivative(theVDerivMax,
                                   theUDerivMax,
                                   theVDerivMax,
                                   theUDerivMax,
                                   theResultArray[0],
                                   aTempStorage[0]);
    }

    // Copy results back to the output array
    for (int i = 0; i < aResultSize; ++i)
    {
      theResultArray[i] = aTempStorage[i];
    }
  }
}
} // namespace

//=================================================================================================

BSplSLib_Cache::BSplSLib_Cache(const int&                        theDegreeU,
                               const bool&                       thePeriodicU,
                               const NCollection_Array1<double>& theFlatKnotsU,
                               const int&                        theDegreeV,
                               const bool&                       thePeriodicV,
                               const NCollection_Array1<double>& theFlatKnotsV,
                               const NCollection_Array2<double>* theWeights)
    : myIsRational(theWeights != nullptr),
      myParamsU(theDegreeU, thePeriodicU, theFlatKnotsU),
      myParamsV(theDegreeV, thePeriodicV, theFlatKnotsV)
{
  int aMinDegree   = std::min(theDegreeU, theDegreeV);
  int aMaxDegree   = std::max(theDegreeU, theDegreeV);
  int aPWColNumber = (myIsRational ? 4 : 3);
  myPolesWeights =
    new NCollection_HArray2<double>(1, aMaxDegree + 1, 1, aPWColNumber * (aMinDegree + 1));
}

//=================================================================================================

bool BSplSLib_Cache::IsCacheValid(double theParameterU, double theParameterV) const
{
  return myParamsU.IsCacheValid(theParameterU) && myParamsV.IsCacheValid(theParameterV);
}

//=================================================================================================

void BSplSLib_Cache::BuildCache(const double&                     theParameterU,
                                const double&                     theParameterV,
                                const NCollection_Array1<double>& theFlatKnotsU,
                                const NCollection_Array1<double>& theFlatKnotsV,
                                const NCollection_Array2<gp_Pnt>& thePoles,
                                const NCollection_Array2<double>* theWeights)
{
  // Normalize the parameters for periodical B-splines
  double aNewParamU = myParamsU.PeriodicNormalization(theParameterU);
  double aNewParamV = myParamsV.PeriodicNormalization(theParameterV);

  myParamsU.LocateParameter(aNewParamU, theFlatKnotsU);
  myParamsV.LocateParameter(aNewParamV, theFlatKnotsV);

  buildCache(theFlatKnotsU, theFlatKnotsV, thePoles, theWeights);
}

//=================================================================================================

void BSplSLib_Cache::BuildCacheForSpan(const int                         theSpanU,
                                       const int                         theSpanV,
                                       const NCollection_Array1<double>& theFlatKnotsU,
                                       const NCollection_Array1<double>& theFlatKnotsV,
                                       const NCollection_Array2<gp_Pnt>& thePoles,
                                       const NCollection_Array2<double>* theWeights)
{
  myParamsU.SpanIndex  = theSpanU;
  myParamsU.SpanStart  = theFlatKnotsU.Value(theSpanU);
  myParamsU.SpanLength = theFlatKnotsU.Value(theSpanU + 1) - myParamsU.SpanStart;
  myParamsV.SpanIndex  = theSpanV;
  myParamsV.SpanStart  = theFlatKnotsV.Value(theSpanV);
  myParamsV.SpanLength = theFlatKnotsV.Value(theSpanV + 1) - myParamsV.SpanStart;

  buildCache(theFlatKnotsU, theFlatKnotsV, thePoles, theWeights);
}

//=================================================================================================

void BSplSLib_Cache::buildCache(const NCollection_Array1<double>& theFlatKnotsU,
                                const NCollection_Array1<double>& theFlatKnotsV,
                                const NCollection_Array2<gp_Pnt>& thePoles,
                                const NCollection_Array2<double>* theWeights)
{

  // BSplSLib uses different convention for span parameters than BSplCLib
  // (Start is in the middle of the span and length is half-span),
  // thus we need to amend them here
  double aSpanLengthU = 0.5 * myParamsU.SpanLength;
  double aSpanStartU  = myParamsU.SpanStart + aSpanLengthU;
  double aSpanLengthV = 0.5 * myParamsV.SpanLength;
  double aSpanStartV  = myParamsV.SpanStart + aSpanLengthV;

  // Calculate new cache data
  BSplSLib::BuildCache(aSpanStartU,
                       aSpanStartV,
                       aSpanLengthU,
                       aSpanLengthV,
                       myParamsU.IsPeriodic,
                       myParamsV.IsPeriodic,
                       myParamsU.Degree,
                       myParamsV.Degree,
                       myParamsU.SpanIndex,
                       myParamsV.SpanIndex,
                       theFlatKnotsU,
                       theFlatKnotsV,
                       thePoles,
                       theWeights,
                       myPolesWeights->ChangeArray2());
}

//=================================================================================================

void BSplSLib_Cache::D0(const double& theU, const double& theV, gp_Pnt& thePoint) const
{
  const auto [aLocalU, aLocalV] = toLocalParamsD0(theU, theV, myParamsU, myParamsV);
  D0Local(aLocalU, aLocalV, thePoint);
}

//=================================================================================================

void BSplSLib_Cache::D0Local(double theLocalU, double theLocalV, gp_Pnt& thePoint) const
{
  double* aPolesArray = ConvertArray(myPolesWeights);
  double  aPoint[4]   = {};

  const int  aDimension               = myIsRational ? 4 : 3;
  const int  aCacheCols               = myPolesWeights->RowLength();
  const bool isMaxU                   = (myParamsU.Degree > myParamsV.Degree);
  const auto [aMinDegree, aMaxDegree] = std::minmax(myParamsU.Degree, myParamsV.Degree);
  const auto [aMinParam, aMaxParam] =
    isMaxU ? std::make_pair(theLocalV, theLocalU) : std::make_pair(theLocalU, theLocalV);

  // Array for intermediate results
  NCollection_LocalArray<double> aTransientCoeffs(aCacheCols);

  // Calculate intermediate value of cached polynomial along variable with maximal degree
  PLib::NoDerivativeEvalPolynomial(aMaxParam,
                                   aMaxDegree,
                                   aCacheCols,
                                   aMaxDegree * aCacheCols,
                                   aPolesArray[0],
                                   aTransientCoeffs[0]);

  // Calculate total value along variable with minimal degree
  PLib::NoDerivativeEvalPolynomial(aMinParam,
                                   aMinDegree,
                                   aDimension,
                                   aDimension * aMinDegree,
                                   aTransientCoeffs[0],
                                   aPoint[0]);

  thePoint.SetCoord(aPoint[0], aPoint[1], aPoint[2]);
  if (myIsRational)
  {
    thePoint.ChangeCoord().Divide(aPoint[3]);
  }
}

//=================================================================================================

void BSplSLib_Cache::D0GridLocal(const double* theLocalU,
                                 const size_t  theNbU,
                                 const double* theLocalV,
                                 const size_t  theNbV,
                                 gp_Pnt*       theResults,
                                 const size_t  theRowStride) const
{
  if (theNbU == 0 || theNbV == 0)
  {
    return;
  }

  const double* const aPolesArray = ConvertArray(myPolesWeights);
  const int           aDimension  = myIsRational ? 4 : 3;
  const int           aCacheCols  = myPolesWeights->RowLength();
  const bool          isMaxU      = myParamsU.Degree > myParamsV.Degree;
  const int           aMaxDegree  = isMaxU ? myParamsU.Degree : myParamsV.Degree;
  const int           aMinDegree  = isMaxU ? myParamsV.Degree : myParamsU.Degree;
  const double*       aMaxParams  = isMaxU ? theLocalU : theLocalV;
  const size_t        aNbMax      = isMaxU ? theNbU : theNbV;

  NCollection_LocalArray<double> anIntermediate(aNbMax * static_cast<size_t>(aCacheCols));
  for (size_t aMaxIndex = 0; aMaxIndex < aNbMax; ++aMaxIndex)
  {
    PLib::NoDerivativeEvalPolynomial(aMaxParams[aMaxIndex],
                                     aMaxDegree,
                                     aCacheCols,
                                     aMaxDegree * aCacheCols,
                                     aPolesArray[0],
                                     anIntermediate[aMaxIndex * static_cast<size_t>(aCacheCols)]);
  }

  for (size_t aUIndex = 0; aUIndex < theNbU; ++aUIndex)
  {
    for (size_t aVIndex = 0; aVIndex < theNbV; ++aVIndex)
    {
      const size_t  aMaxIndex = isMaxU ? aUIndex : aVIndex;
      const double  aMinParam = isMaxU ? theLocalV[aVIndex] : theLocalU[aUIndex];
      const double* aCoeffs =
        &anIntermediate[aMaxIndex * static_cast<size_t>(aCacheCols)];
      double aPoint[4];
      PLib::NoDerivativeEvalPolynomial(aMinParam,
                                       aMinDegree,
                                       aDimension,
                                       aMinDegree * aDimension,
                                       aCoeffs[0],
                                       aPoint[0]);
      gp_Pnt& aResult = theResults[aUIndex * theRowStride + aVIndex];
      if (myIsRational)
      {
        const double anInvWeight = 1.0 / aPoint[3];
        aResult.SetCoord(aPoint[0] * anInvWeight,
                         aPoint[1] * anInvWeight,
                         aPoint[2] * anInvWeight);
      }
      else
      {
        aResult.SetCoord(aPoint[0], aPoint[1], aPoint[2]);
      }
    }
  }
}

//=================================================================================================

void BSplSLib_Cache::D1Local(double  theLocalU,
                             double  theLocalV,
                             gp_Pnt& thePoint,
                             gp_Vec& theTangentU,
                             gp_Vec& theTangentV) const
{
  double aPntDeriv[16] = {}; // Result storage for D1, zero-initialized
  EvaluatePolynomials(myPolesWeights,
                      myParamsU,
                      myParamsV,
                      myIsRational,
                      theLocalU,
                      theLocalV,
                      1,
                      1,
                      aPntDeriv);

  // After RationalDerivative (for rational surfaces), the output dimension is 3 (not 4)
  // because weights have been processed out
  const int aDimension = 3;

  thePoint.SetCoord(aPntDeriv[0], aPntDeriv[1], aPntDeriv[2]);

  // Tangents are stored after the point coordinates
  // Order depends on which parameter has higher degree
  // If U degree > V degree: layout is [P, DV, DU, ...]
  // If V degree >= U degree: layout is [P, DU, DV, ...]
  if (myParamsU.Degree > myParamsV.Degree)
  {
    theTangentV.SetCoord(aPntDeriv[aDimension],
                         aPntDeriv[aDimension + 1],
                         aPntDeriv[aDimension + 2]);
    theTangentU.SetCoord(aPntDeriv[aDimension << 1],
                         aPntDeriv[(aDimension << 1) + 1],
                         aPntDeriv[(aDimension << 1) + 2]);
  }
  else
  {
    theTangentU.SetCoord(aPntDeriv[aDimension],
                         aPntDeriv[aDimension + 1],
                         aPntDeriv[aDimension + 2]);
    theTangentV.SetCoord(aPntDeriv[aDimension << 1],
                         aPntDeriv[(aDimension << 1) + 1],
                         aPntDeriv[(aDimension << 1) + 2]);
  }

  // Use direct division for better numerical stability with very small span lengths
  const double aSpanLengthU = 0.5 * myParamsU.SpanLength;
  const double aSpanLengthV = 0.5 * myParamsV.SpanLength;
  theTangentU.Divide(aSpanLengthU);
  theTangentV.Divide(aSpanLengthV);
}

//=================================================================================================

void BSplSLib_Cache::D1GridLocal(const double* theLocalU,
                                 const size_t  theNbU,
                                 const double* theLocalV,
                                 const size_t  theNbV,
                                 double*       theResults,
                                 const size_t  theRowStride) const
{
  if (theNbU == 0 || theNbV == 0)
  {
    return;
  }

  const double* const aPolesArray = ConvertArray(myPolesWeights);
  const int           aDimension  = myIsRational ? 4 : 3;
  const int           aCacheCols  = myPolesWeights->RowLength();
  const bool          isMaxU      = myParamsU.Degree > myParamsV.Degree;
  const int           aMaxDegree  = isMaxU ? myParamsU.Degree : myParamsV.Degree;
  const int           aMinDegree  = isMaxU ? myParamsV.Degree : myParamsU.Degree;
  const double*       aMaxParams  = isMaxU ? theLocalU : theLocalV;
  const size_t        aNbMax      = isMaxU ? theNbU : theNbV;

  NCollection_LocalArray<double> anIntermediate(aNbMax * static_cast<size_t>(2 * aCacheCols));
  for (size_t aMaxIndex = 0; aMaxIndex < aNbMax; ++aMaxIndex)
  {
    PLib::EvalPolynomial(aMaxParams[aMaxIndex],
                         1,
                         aMaxDegree,
                         aCacheCols,
                         aPolesArray[0],
                         anIntermediate[aMaxIndex * static_cast<size_t>(2 * aCacheCols)]);
  }

  const double anInvSpanU = 2.0 / myParamsU.SpanLength;
  const double anInvSpanV = 2.0 / myParamsV.SpanLength;
  for (size_t aUIndex = 0; aUIndex < theNbU; ++aUIndex)
  {
    for (size_t aVIndex = 0; aVIndex < theNbV; ++aVIndex)
    {
      const size_t  aMaxIndex = isMaxU ? aUIndex : aVIndex;
      const double  aMinParam = isMaxU ? theLocalV[aVIndex] : theLocalU[aUIndex];
      const double* aCoeffs =
        &anIntermediate[aMaxIndex * static_cast<size_t>(2 * aCacheCols)];
      double aMinResult[8];
      double aMaxResult[4];
      PLib::EvalPolynomial(
        aMinParam, 1, aMinDegree, aDimension, aCoeffs[0], aMinResult[0]);
      PLib::NoDerivativeEvalPolynomial(aMinParam,
                                       aMinDegree,
                                       aDimension,
                                       aMinDegree * aDimension,
                                       aCoeffs[aCacheCols],
                                       aMaxResult[0]);

      const double* aDerivU = isMaxU ? aMaxResult : &aMinResult[aDimension];
      const double* aDerivV = isMaxU ? &aMinResult[aDimension] : aMaxResult;
      double* const aResult = &theResults[aUIndex * theRowStride + aVIndex * 9];
      if (myIsRational)
      {
        const double anInvWeight = 1.0 / aMinResult[3];
        for (int aCoord = 0; aCoord < 3; ++aCoord)
        {
          aResult[aCoord] = aMinResult[aCoord] * anInvWeight;
          aResult[3 + aCoord] =
            (aDerivU[aCoord] - aResult[aCoord] * aDerivU[3]) * anInvWeight * anInvSpanU;
          aResult[6 + aCoord] =
            (aDerivV[aCoord] - aResult[aCoord] * aDerivV[3]) * anInvWeight * anInvSpanV;
        }
      }
      else
      {
        for (int aCoord = 0; aCoord < 3; ++aCoord)
        {
          aResult[aCoord]     = aMinResult[aCoord];
          aResult[3 + aCoord] = aDerivU[aCoord] * anInvSpanU;
          aResult[6 + aCoord] = aDerivV[aCoord] * anInvSpanV;
        }
      }
    }
  }
}

//=================================================================================================

void BSplSLib_Cache::D2GridLocal(const double* theLocalU,
                                 const size_t  theNbU,
                                 const double* theLocalV,
                                 const size_t  theNbV,
                                 double*       theResults,
                                 const size_t  theRowStride) const
{
  if (theNbU == 0 || theNbV == 0)
  {
    return;
  }

  const double* const aPolesArray = ConvertArray(myPolesWeights);
  const int           aDimension  = myIsRational ? 4 : 3;
  const int           aCacheCols  = myPolesWeights->RowLength();
  const bool          isMaxU      = myParamsU.Degree > myParamsV.Degree;
  const int           aMaxDegree  = isMaxU ? myParamsU.Degree : myParamsV.Degree;
  const int           aMinDegree  = isMaxU ? myParamsV.Degree : myParamsU.Degree;
  const double*       aMaxParams  = isMaxU ? theLocalU : theLocalV;
  const size_t        aNbMax      = isMaxU ? theNbU : theNbV;

  NCollection_LocalArray<double> anIntermediate(aNbMax * static_cast<size_t>(3 * aCacheCols));
  for (size_t aMaxIndex = 0; aMaxIndex < aNbMax; ++aMaxIndex)
  {
    PLib::EvalPolynomial(aMaxParams[aMaxIndex],
                         2,
                         aMaxDegree,
                         aCacheCols,
                         aPolesArray[0],
                         anIntermediate[aMaxIndex * static_cast<size_t>(3 * aCacheCols)]);
  }

  const double anInvSpanU   = 2.0 / myParamsU.SpanLength;
  const double anInvSpanV   = 2.0 / myParamsV.SpanLength;
  const double anInvSpanUSq = anInvSpanU * anInvSpanU;
  const double anInvSpanVSq = anInvSpanV * anInvSpanV;
  const double anInvSpanUV  = anInvSpanU * anInvSpanV;
  for (size_t aUIndex = 0; aUIndex < theNbU; ++aUIndex)
  {
    for (size_t aVIndex = 0; aVIndex < theNbV; ++aVIndex)
    {
      const size_t  aMaxIndex = isMaxU ? aUIndex : aVIndex;
      const double  aMinParam = isMaxU ? theLocalV[aVIndex] : theLocalU[aUIndex];
      const double* aCoeffs =
        &anIntermediate[aMaxIndex * static_cast<size_t>(3 * aCacheCols)];
      double aMin0[12];
      double aMin1[8];
      double aMax2[4];
      PLib::EvalPolynomial(aMinParam, 2, aMinDegree, aDimension, aCoeffs[0], aMin0[0]);
      PLib::EvalPolynomial(
        aMinParam, 1, aMinDegree, aDimension, aCoeffs[aCacheCols], aMin1[0]);
      PLib::NoDerivativeEvalPolynomial(aMinParam,
                                       aMinDegree,
                                       aDimension,
                                       aMinDegree * aDimension,
                                       aCoeffs[2 * aCacheCols],
                                       aMax2[0]);

      const double* aD1U  = isMaxU ? aMin1 : &aMin0[aDimension];
      const double* aD1V  = isMaxU ? &aMin0[aDimension] : aMin1;
      const double* aD2U  = isMaxU ? aMax2 : &aMin0[2 * aDimension];
      const double* aD2V  = isMaxU ? &aMin0[2 * aDimension] : aMax2;
      const double* aD2UV = &aMin1[aDimension];
      double* const aResult = &theResults[aUIndex * theRowStride + aVIndex * 18];
      if (myIsRational)
      {
        const double anInvWeight = 1.0 / aMin0[3];
        for (int aCoord = 0; aCoord < 3; ++aCoord)
        {
          aResult[aCoord] = aMin0[aCoord] * anInvWeight;
          aResult[3 + aCoord] =
            (aD1U[aCoord] - aResult[aCoord] * aD1U[3]) * anInvWeight;
          aResult[6 + aCoord] =
            (aD1V[aCoord] - aResult[aCoord] * aD1V[3]) * anInvWeight;
          aResult[9 + aCoord] =
            (aD2U[aCoord] - 2.0 * aResult[3 + aCoord] * aD1U[3]
             - aResult[aCoord] * aD2U[3])
            * anInvWeight;
          aResult[12 + aCoord] =
            (aD2V[aCoord] - 2.0 * aResult[6 + aCoord] * aD1V[3]
             - aResult[aCoord] * aD2V[3])
            * anInvWeight;
          aResult[15 + aCoord] =
            (aD2UV[aCoord] - aResult[3 + aCoord] * aD1V[3]
             - aResult[6 + aCoord] * aD1U[3] - aResult[aCoord] * aD2UV[3])
            * anInvWeight;
        }
      }
      else
      {
        for (int aCoord = 0; aCoord < 3; ++aCoord)
        {
          aResult[aCoord]      = aMin0[aCoord];
          aResult[3 + aCoord]  = aD1U[aCoord];
          aResult[6 + aCoord]  = aD1V[aCoord];
          aResult[9 + aCoord]  = aD2U[aCoord];
          aResult[12 + aCoord] = aD2V[aCoord];
          aResult[15 + aCoord] = aD2UV[aCoord];
        }
      }

      for (int aCoord = 0; aCoord < 3; ++aCoord)
      {
        aResult[3 + aCoord] *= anInvSpanU;
        aResult[6 + aCoord] *= anInvSpanV;
        aResult[9 + aCoord] *= anInvSpanUSq;
        aResult[12 + aCoord] *= anInvSpanVSq;
        aResult[15 + aCoord] *= anInvSpanUV;
      }
    }
  }
}

//=================================================================================================

void BSplSLib_Cache::D2Local(double  theLocalU,
                             double  theLocalV,
                             gp_Pnt& thePoint,
                             gp_Vec& theTangentU,
                             gp_Vec& theTangentV,
                             gp_Vec& theCurvatureU,
                             gp_Vec& theCurvatureV,
                             gp_Vec& theCurvatureUV) const
{
  double aPntDeriv[36] = {}; // Result storage for D2, zero-initialized
  EvaluatePolynomials(myPolesWeights,
                      myParamsU,
                      myParamsV,
                      myIsRational,
                      theLocalU,
                      theLocalV,
                      2,
                      2,
                      aPntDeriv);

  // After RationalDerivative (for rational surfaces), the output dimension is 3 (not 4)
  // because weights have been processed out
  const int aDimension = 3;
  const int aShift     = aDimension; // Shift for first derivatives
  const int aShift2    = aDimension << 1;
  const int aShift3    = aShift2 + aDimension;
  const int aShift4    = aShift3 + aDimension;
  const int aShift6    = 6 * aDimension;

  thePoint.SetCoord(aPntDeriv[0], aPntDeriv[1], aPntDeriv[2]);

  // Derivatives are stored consecutively
  // If Max=U (U degree > V degree):
  // [0]=P, [Dim]=DV, [2Dim]=DVV
  // [3Dim]=DU, [4Dim]=DUV
  // [6Dim]=DUU
  if (myParamsU.Degree > myParamsV.Degree)
  {
    theTangentV.SetCoord(aPntDeriv[aShift], aPntDeriv[aShift + 1], aPntDeriv[aShift + 2]);
    theCurvatureV.SetCoord(aPntDeriv[aShift2], aPntDeriv[aShift2 + 1], aPntDeriv[aShift2 + 2]);
    theTangentU.SetCoord(aPntDeriv[aShift3], aPntDeriv[aShift3 + 1], aPntDeriv[aShift3 + 2]);
    theCurvatureUV.SetCoord(aPntDeriv[aShift4], aPntDeriv[aShift4 + 1], aPntDeriv[aShift4 + 2]);
    theCurvatureU.SetCoord(aPntDeriv[aShift6], aPntDeriv[aShift6 + 1], aPntDeriv[aShift6 + 2]);
  }
  else
  {
    // If Max=V (V degree >= U degree):
    // [0]=P, [Dim]=DU, [2Dim]=DUU
    // [3Dim]=DV, [4Dim]=DUV (DVU is symmetric)
    // [6Dim]=DVV
    theTangentU.SetCoord(aPntDeriv[aShift], aPntDeriv[aShift + 1], aPntDeriv[aShift + 2]);
    theCurvatureU.SetCoord(aPntDeriv[aShift2], aPntDeriv[aShift2 + 1], aPntDeriv[aShift2 + 2]);
    theTangentV.SetCoord(aPntDeriv[aShift3], aPntDeriv[aShift3 + 1], aPntDeriv[aShift3 + 2]);
    theCurvatureUV.SetCoord(aPntDeriv[aShift4], aPntDeriv[aShift4 + 1], aPntDeriv[aShift4 + 2]);
    theCurvatureV.SetCoord(aPntDeriv[aShift6], aPntDeriv[aShift6 + 1], aPntDeriv[aShift6 + 2]);
  }

  // Use direct division for better numerical stability with very small span lengths
  const double aSpanLengthU  = 0.5 * myParamsU.SpanLength;
  const double aSpanLengthV  = 0.5 * myParamsV.SpanLength;
  const double aSpanLengthU2 = aSpanLengthU * aSpanLengthU;
  const double aSpanLengthV2 = aSpanLengthV * aSpanLengthV;
  theTangentU.Divide(aSpanLengthU);
  theTangentV.Divide(aSpanLengthV);
  theCurvatureU.Divide(aSpanLengthU2);
  theCurvatureV.Divide(aSpanLengthV2);
  theCurvatureUV.Divide(aSpanLengthU * aSpanLengthV);
}

//=================================================================================================

void BSplSLib_Cache::D1(const double& theU,
                        const double& theV,
                        gp_Pnt&       thePoint,
                        gp_Vec&       theTangentU,
                        gp_Vec&       theTangentV) const
{
  // Use the same inverse values for both parameter transformation and derivative scaling
  // to maintain numerical consistency with the original implementation
  double anInvU = 0.0, anInvV = 0.0;
  const auto [aLocalU, aLocalV] = toLocalParams(theU, theV, myParamsU, myParamsV, anInvU, anInvV);

  double aPntDeriv[16] = {};
  EvaluatePolynomials(myPolesWeights,
                      myParamsU,
                      myParamsV,
                      myIsRational,
                      aLocalU,
                      aLocalV,
                      1,
                      1,
                      aPntDeriv);

  const int aDimension = 3;
  thePoint.SetCoord(aPntDeriv[0], aPntDeriv[1], aPntDeriv[2]);

  if (myParamsU.Degree > myParamsV.Degree)
  {
    theTangentV.SetCoord(aPntDeriv[aDimension],
                         aPntDeriv[aDimension + 1],
                         aPntDeriv[aDimension + 2]);
    theTangentU.SetCoord(aPntDeriv[aDimension << 1],
                         aPntDeriv[(aDimension << 1) + 1],
                         aPntDeriv[(aDimension << 1) + 2]);
  }
  else
  {
    theTangentU.SetCoord(aPntDeriv[aDimension],
                         aPntDeriv[aDimension + 1],
                         aPntDeriv[aDimension + 2]);
    theTangentV.SetCoord(aPntDeriv[aDimension << 1],
                         aPntDeriv[(aDimension << 1) + 1],
                         aPntDeriv[(aDimension << 1) + 2]);
  }

  // Scale derivatives using the same inverse values used for parameter transformation
  theTangentU.Multiply(anInvU);
  theTangentV.Multiply(anInvV);
}

//=================================================================================================

void BSplSLib_Cache::D2(const double& theU,
                        const double& theV,
                        gp_Pnt&       thePoint,
                        gp_Vec&       theTangentU,
                        gp_Vec&       theTangentV,
                        gp_Vec&       theCurvatureU,
                        gp_Vec&       theCurvatureV,
                        gp_Vec&       theCurvatureUV) const
{
  // Use the same inverse values for both parameter transformation and derivative scaling
  // to maintain numerical consistency with the original implementation
  double anInvU = 0.0, anInvV = 0.0;
  const auto [aLocalU, aLocalV] = toLocalParams(theU, theV, myParamsU, myParamsV, anInvU, anInvV);

  double aPntDeriv[36] = {};
  EvaluatePolynomials(myPolesWeights,
                      myParamsU,
                      myParamsV,
                      myIsRational,
                      aLocalU,
                      aLocalV,
                      2,
                      2,
                      aPntDeriv);

  const int aDimension = 3;
  const int aShift     = aDimension;
  const int aShift2    = aDimension << 1;
  const int aShift3    = aShift2 + aDimension;
  const int aShift4    = aShift3 + aDimension;
  const int aShift6    = 6 * aDimension;

  thePoint.SetCoord(aPntDeriv[0], aPntDeriv[1], aPntDeriv[2]);

  if (myParamsU.Degree > myParamsV.Degree)
  {
    theTangentV.SetCoord(aPntDeriv[aShift], aPntDeriv[aShift + 1], aPntDeriv[aShift + 2]);
    theCurvatureV.SetCoord(aPntDeriv[aShift2], aPntDeriv[aShift2 + 1], aPntDeriv[aShift2 + 2]);
    theTangentU.SetCoord(aPntDeriv[aShift3], aPntDeriv[aShift3 + 1], aPntDeriv[aShift3 + 2]);
    theCurvatureUV.SetCoord(aPntDeriv[aShift4], aPntDeriv[aShift4 + 1], aPntDeriv[aShift4 + 2]);
    theCurvatureU.SetCoord(aPntDeriv[aShift6], aPntDeriv[aShift6 + 1], aPntDeriv[aShift6 + 2]);
  }
  else
  {
    theTangentU.SetCoord(aPntDeriv[aShift], aPntDeriv[aShift + 1], aPntDeriv[aShift + 2]);
    theCurvatureU.SetCoord(aPntDeriv[aShift2], aPntDeriv[aShift2 + 1], aPntDeriv[aShift2 + 2]);
    theTangentV.SetCoord(aPntDeriv[aShift3], aPntDeriv[aShift3 + 1], aPntDeriv[aShift3 + 2]);
    theCurvatureUV.SetCoord(aPntDeriv[aShift4], aPntDeriv[aShift4 + 1], aPntDeriv[aShift4 + 2]);
    theCurvatureV.SetCoord(aPntDeriv[aShift6], aPntDeriv[aShift6 + 1], aPntDeriv[aShift6 + 2]);
  }

  // Scale derivatives using the same inverse values used for parameter transformation
  theTangentU.Multiply(anInvU);
  theTangentV.Multiply(anInvV);
  theCurvatureU.Multiply(anInvU * anInvU);
  theCurvatureV.Multiply(anInvV * anInvV);
  theCurvatureUV.Multiply(anInvU * anInvV);
}
