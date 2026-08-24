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

#include <GeomGridEval_BezierSurface.hxx>

#include <BSplSLib.hxx>
#include <gp_Pnt.hxx>
#include <NCollection_Array2.hxx>
#include <NCollection_LocalArray.hxx>

namespace
{
//! Create and build cache for Bezier surface evaluation.
//! Bezier surfaces are single-span, so cache is built once at parameter (0.5, 0.5).
//! @param theGeom the Bezier surface geometry
//! @return the built cache
occ::handle<BSplSLib_Cache> buildBezierCache(const occ::handle<Geom_BezierSurface>& theGeom)
{
  const NCollection_Array1<double>& aUKnotSequence = theGeom->UKnotSequence();
  const NCollection_Array1<double>& aVKnotSequence = theGeom->VKnotSequence();
  const NCollection_Array2<gp_Pnt>& aPoles         = theGeom->Poles();
  const NCollection_Array2<double>* aWeights       = theGeom->Weights();

  occ::handle<BSplSLib_Cache> aCache = new BSplSLib_Cache(theGeom->UDegree(),
                                                          false,
                                                          aUKnotSequence,
                                                          theGeom->VDegree(),
                                                          false,
                                                          aVKnotSequence,
                                                          aWeights);
  aCache->BuildCache(0.5, 0.5, aUKnotSequence, aVKnotSequence, aPoles, aWeights);
  return aCache;
}

void prepareLocalParams(const NCollection_Array1<double>& theParams,
                        NCollection_LocalArray<double>&   theLocalParams)
{
  for (size_t anIndex = 0; anIndex < theParams.Size(); ++anIndex)
  {
    theLocalParams[anIndex] = 2.0 * theParams.At(anIndex) - 1.0;
  }
}
} // namespace

//=================================================================================================

NCollection_Array2<gp_Pnt> GeomGridEval_BezierSurface::EvaluateGrid(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return NCollection_Array2<gp_Pnt>();
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_LocalArray<double> aLocalU(aNbU);
  NCollection_LocalArray<double> aLocalV(aNbV);
  prepareLocalParams(theUParams, aLocalU);
  prepareLocalParams(theVParams, aLocalV);

  const occ::handle<BSplSLib_Cache> aCache = buildBezierCache(myGeom);
  NCollection_Array2<gp_Pnt>        aResult(1, static_cast<int>(aNbU), 1, static_cast<int>(aNbV));
  aCache->D0GridLocal(aLocalU, aNbU, aLocalV, aNbV, aResult.Data(), aNbV);
  return aResult;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD1> GeomGridEval_BezierSurface::EvaluateGridD1(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return {};
  }
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_LocalArray<double> aLocalU(aNbU);
  NCollection_LocalArray<double> aLocalV(aNbV);
  prepareLocalParams(theUParams, aLocalU);
  prepareLocalParams(theVParams, aLocalV);
  const occ::handle<BSplSLib_Cache> aCache = buildBezierCache(myGeom);
  NCollection_Array2<GeomGridEval::SurfD1> aResult(1,
                                                  static_cast<int>(aNbU),
                                                  1,
                                                  static_cast<int>(aNbV));
  for (size_t aUIndex = 0; aUIndex < aNbU; ++aUIndex)
  {
    for (size_t aVIndex = 0; aVIndex < aNbV; ++aVIndex)
    {
      GeomGridEval::SurfD1& aValue = aResult.ChangeAt(aUIndex * aNbV + aVIndex);
      aCache->D1Local(
        aLocalU[aUIndex], aLocalV[aVIndex], aValue.Point, aValue.D1U, aValue.D1V);
    }
  }
  return aResult;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD1Coords>
  GeomGridEval_BezierSurface::EvaluateGridD1Coords(
    const NCollection_Array1<double>& theUParams,
    const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return {};
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_LocalArray<double> aLocalU(aNbU);
  NCollection_LocalArray<double> aLocalV(aNbV);
  prepareLocalParams(theUParams, aLocalU);
  prepareLocalParams(theVParams, aLocalV);

  const occ::handle<BSplSLib_Cache> aCache = buildBezierCache(myGeom);
  NCollection_Array2<GeomGridEval::SurfD1Coords> aGrid(aNbU, aNbV);
  aCache->D1GridLocal(aLocalU,
                      aNbU,
                      aLocalV,
                      aNbV,
                      reinterpret_cast<double*>(aGrid.Data()),
                      aNbV * 9);
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD2> GeomGridEval_BezierSurface::EvaluateGridD2(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return {};
  }
  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_LocalArray<double> aLocalU(aNbU);
  NCollection_LocalArray<double> aLocalV(aNbV);
  prepareLocalParams(theUParams, aLocalU);
  prepareLocalParams(theVParams, aLocalV);
  const occ::handle<BSplSLib_Cache> aCache = buildBezierCache(myGeom);
  NCollection_Array2<GeomGridEval::SurfD2> aResult(1,
                                                  static_cast<int>(aNbU),
                                                  1,
                                                  static_cast<int>(aNbV));
  for (size_t aUIndex = 0; aUIndex < aNbU; ++aUIndex)
  {
    for (size_t aVIndex = 0; aVIndex < aNbV; ++aVIndex)
    {
      GeomGridEval::SurfD2& aValue = aResult.ChangeAt(aUIndex * aNbV + aVIndex);
      aCache->D2Local(aLocalU[aUIndex],
                      aLocalV[aVIndex],
                      aValue.Point,
                      aValue.D1U,
                      aValue.D1V,
                      aValue.D2U,
                      aValue.D2V,
                      aValue.D2UV);
    }
  }
  return aResult;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD2Coords>
  GeomGridEval_BezierSurface::EvaluateGridD2Coords(
    const NCollection_Array1<double>& theUParams,
    const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return {};
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_LocalArray<double> aLocalU(aNbU);
  NCollection_LocalArray<double> aLocalV(aNbV);
  prepareLocalParams(theUParams, aLocalU);
  prepareLocalParams(theVParams, aLocalV);

  const occ::handle<BSplSLib_Cache> aCache = buildBezierCache(myGeom);
  NCollection_Array2<GeomGridEval::SurfD2Coords> aGrid(aNbU, aNbV);
  aCache->D2GridLocal(aLocalU,
                      aNbU,
                      aLocalV,
                      aNbV,
                      reinterpret_cast<double*>(aGrid.Data()),
                      aNbV * 18);
  return aGrid;
}

//=================================================================================================

NCollection_Array2<GeomGridEval::SurfD3> GeomGridEval_BezierSurface::EvaluateGridD3(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty())
  {
    return NCollection_Array2<GeomGridEval::SurfD3>();
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();
  NCollection_Array2<GeomGridEval::SurfD3> aResult(1,
                                                   static_cast<int>(aNbU),
                                                   1,
                                                   static_cast<int>(aNbV));

  // Get degrees, flat knots, poles, and weights from geometry
  const int                         aUDegree       = myGeom->UDegree();
  const int                         aVDegree       = myGeom->VDegree();
  const NCollection_Array1<double>& aUKnotSequence = myGeom->UKnotSequence();
  const NCollection_Array1<double>& aVKnotSequence = myGeom->VKnotSequence();
  const NCollection_Array2<gp_Pnt>& aPoles         = myGeom->Poles();
  const NCollection_Array2<double>* aWeights       = myGeom->Weights();
  const bool                        isRational     = (aWeights != nullptr);

  // D3 evaluation using BSplSLib::D3 directly
  // Bezier surface is single span (span index = 0), non-periodic
  for (size_t aUIndex = 0; aUIndex < aNbU; ++aUIndex)
  {
    const double aU = theUParams.At(aUIndex);
    for (size_t aVIndex = 0; aVIndex < aNbV; ++aVIndex)
    {
      gp_Pnt aPoint;
      gp_Vec aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV;

      BSplSLib::D3(aU,
                   theVParams.At(aVIndex),
                   0, // U span index (single span for Bezier)
                   0, // V span index (single span for Bezier)
                   aPoles,
                   aWeights,
                   aUKnotSequence,
                   aVKnotSequence,
                   nullptr, // U multiplicities (nullptr for flat knots)
                   nullptr, // V multiplicities (nullptr for flat knots)
                   aUDegree,
                   aVDegree,
                   isRational,
                   isRational,
                   false, // not U periodic
                   false, // not V periodic
                   aPoint,
                   aD1U,
                   aD1V,
                   aD2U,
                   aD2V,
                   aD2UV,
                   aD3U,
                   aD3V,
                   aD3UUV,
                   aD3UVV);

      aResult.ChangeAt(aUIndex * aNbV + aVIndex) =
        {aPoint, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV};
    }
  }

  return aResult;
}

//=================================================================================================

NCollection_Array2<gp_Vec> GeomGridEval_BezierSurface::EvaluateGridDN(
  const NCollection_Array1<double>& theUParams,
  const NCollection_Array1<double>& theVParams,
  int                               theNU,
  int                               theNV) const
{
  if (myGeom.IsNull() || theUParams.IsEmpty() || theVParams.IsEmpty() || theNU < 0 || theNV < 0
      || (theNU + theNV) < 1)
  {
    return NCollection_Array2<gp_Vec>();
  }

  const size_t aNbU = theUParams.Size();
  const size_t aNbV = theVParams.Size();

  NCollection_Array2<gp_Vec> aResult(1,
                                     static_cast<int>(aNbU),
                                     1,
                                     static_cast<int>(aNbV));

  // For Bezier surfaces, derivatives become zero when order exceeds degree in that direction
  const int aUDegree = myGeom->UDegree();
  const int aVDegree = myGeom->VDegree();

  if (theNU > aUDegree || theNV > aVDegree)
  {
    // All derivatives are zero
    const gp_Vec aZeroVec(0.0, 0.0, 0.0);
    for (size_t anIndex = 0; anIndex < aResult.Size(); ++anIndex)
    {
      aResult.ChangeAt(anIndex) = aZeroVec;
    }
    return aResult;
  }

  // Get poles, weights, and flat knots from geometry
  const NCollection_Array2<gp_Pnt>& aPoles         = myGeom->Poles();
  const NCollection_Array2<double>* aWeights       = myGeom->Weights();
  const bool                        isRational     = (aWeights != nullptr);
  const NCollection_Array1<double>& aUKnotSequence = myGeom->UKnotSequence();
  const NCollection_Array1<double>& aVKnotSequence = myGeom->VKnotSequence();

  // Bezier has a single span (index 0 with flat knots), non-periodic
  for (size_t aUIndex = 0; aUIndex < aNbU; ++aUIndex)
  {
    const double aU = theUParams.At(aUIndex);
    for (size_t aVIndex = 0; aVIndex < aNbV; ++aVIndex)
    {
      gp_Vec aDN;
      BSplSLib::DN(aU,
                   theVParams.At(aVIndex),
                   theNU,
                   theNV,
                   0, // U span index (single span for Bezier with flat knots)
                   0, // V span index (single span for Bezier with flat knots)
                   aPoles,
                   aWeights,
                   aUKnotSequence,
                   aVKnotSequence,
                   nullptr, // no U multiplicities with flat knots
                   nullptr, // no V multiplicities with flat knots
                   aUDegree,
                   aVDegree,
                   isRational,
                   isRational,
                   false, // not U-periodic
                   false, // not V-periodic
                   aDN);
      aResult.ChangeAt(aUIndex * aNbV + aVIndex) = aDN;
    }
  }

  return aResult;
}
