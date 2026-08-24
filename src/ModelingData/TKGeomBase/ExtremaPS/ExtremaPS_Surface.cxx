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

#include <ExtremaPS_Surface.hxx>

#include <algorithm>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_ToroidalSurface.hxx>

namespace
{
const ExtremaPS::Result& notDoneResult()
{
  static const ExtremaPS::Result aResult;
  return aResult;
}
} // namespace

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const GeomAdaptor_Surface& theSurface)
    : myEvaluator(std::monostate{})
{
  ExtremaPS::Domain2D aDomain(theSurface.FirstUParameter(),
                              theSurface.LastUParameter(),
                              theSurface.FirstVParameter(),
                              theSurface.LastVParameter());
  initializeEvaluator(theSurface, aDomain);
}

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const GeomAdaptor_Surface& theSurface,
                                     const ExtremaPS::Domain2D& theDomain)
    : myEvaluator(std::monostate{})
{
  initializeEvaluator(theSurface, theDomain);
}

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const GeomAdaptor_TransformedSurface& theSurface)
    : myEvaluator(std::monostate{})
{
  ExtremaPS::Domain2D aDomain(theSurface.FirstUParameter(),
                              theSurface.LastUParameter(),
                              theSurface.FirstVParameter(),
                              theSurface.LastVParameter());
  initializeEvaluator(theSurface, aDomain);
}

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const GeomAdaptor_TransformedSurface& theSurface,
                                     const ExtremaPS::Domain2D&            theDomain)
    : myEvaluator(std::monostate{})
{
  initializeEvaluator(theSurface, theDomain);
}

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const occ::handle<Geom_Surface>& theSurface)
    : myEvaluator(std::monostate{})
{
  if (theSurface.IsNull())
  {
    return;
  }

  // Check for rectangular trimmed surface - if so, use bounds
  occ::handle<Geom_RectangularTrimmedSurface> aTrimmed =
    occ::down_cast<Geom_RectangularTrimmedSurface>(theSurface);
  if (!aTrimmed.IsNull())
  {
    double aU1, aU2, aV1, aV2;
    aTrimmed->Bounds(aU1, aU2, aV1, aV2);
    ExtremaPS::Domain2D aDomain(aU1, aU2, aV1, aV2);
    initFromGeomSurface(aTrimmed->BasisSurface(), aDomain);
    return;
  }

  // Initialize based on surface type - without setting domain (unbounded)
  initFromGeomSurface(theSurface, std::nullopt);
}

//==================================================================================================

ExtremaPS_Surface::ExtremaPS_Surface(const occ::handle<Geom_Surface>& theSurface,
                                     const ExtremaPS::Domain2D&       theDomain)
    : myEvaluator(std::monostate{})
{
  if (theSurface.IsNull())
  {
    return;
  }

  // Get base surface and effective bounds
  occ::handle<Geom_Surface> aBaseSurface = theSurface;
  double                    aEffectiveU1 = theDomain.UMin;
  double                    aEffectiveU2 = theDomain.UMax;
  double                    aEffectiveV1 = theDomain.VMin;
  double                    aEffectiveV2 = theDomain.VMax;

  // For trimmed surface, intersect input bounds with trimmed bounds
  occ::handle<Geom_RectangularTrimmedSurface> aTrimmed =
    occ::down_cast<Geom_RectangularTrimmedSurface>(theSurface);
  if (!aTrimmed.IsNull())
  {
    aBaseSurface = aTrimmed->BasisSurface();
    double aU1, aU2, aV1, aV2;
    aTrimmed->Bounds(aU1, aU2, aV1, aV2);
    // Intersect bounds: take max of mins, min of maxs
    aEffectiveU1 = std::max(theDomain.UMin, aU1);
    aEffectiveU2 = std::min(theDomain.UMax, aU2);
    aEffectiveV1 = std::max(theDomain.VMin, aV1);
    aEffectiveV2 = std::min(theDomain.VMax, aV2);
  }

  // Use common helper for surface type detection and evaluator creation
  ExtremaPS::Domain2D aDomain(aEffectiveU1, aEffectiveU2, aEffectiveV1, aEffectiveV2);
  initFromGeomSurface(aBaseSurface, aDomain);
}

//==================================================================================================

void ExtremaPS_Surface::initFromGeomSurface(const occ::handle<Geom_Surface>&          theSurface,
                                            const std::optional<ExtremaPS::Domain2D>& theDomain)
{
  const occ::handle<Geom_RectangularTrimmedSurface> aTrimmed =
    occ::down_cast<Geom_RectangularTrimmedSurface>(theSurface);
  if (!aTrimmed.IsNull())
  {
    initFromGeomSurface(aTrimmed->BasisSurface(), theDomain);
    return;
  }

  // Try specific surface types for direct initialization
  occ::handle<Geom_Plane> aPlane = occ::down_cast<Geom_Plane>(theSurface);
  if (!aPlane.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_Plane(aPlane->Pln(), theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_Plane(aPlane->Pln());
    }
    return;
  }

  occ::handle<Geom_CylindricalSurface> aCylinder =
    occ::down_cast<Geom_CylindricalSurface>(theSurface);
  if (!aCylinder.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_Cylinder(aCylinder->Cylinder(), theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_Cylinder(aCylinder->Cylinder());
    }
    return;
  }

  occ::handle<Geom_ConicalSurface> aCone = occ::down_cast<Geom_ConicalSurface>(theSurface);
  if (!aCone.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_Cone(aCone->Cone(), theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_Cone(aCone->Cone());
    }
    return;
  }

  occ::handle<Geom_SphericalSurface> aSphere = occ::down_cast<Geom_SphericalSurface>(theSurface);
  if (!aSphere.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_Sphere(aSphere->Sphere(), theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_Sphere(aSphere->Sphere());
    }
    return;
  }

  occ::handle<Geom_ToroidalSurface> aTorus = occ::down_cast<Geom_ToroidalSurface>(theSurface);
  if (!aTorus.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_Torus(aTorus->Torus(), theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_Torus(aTorus->Torus());
    }
    return;
  }

  occ::handle<Geom_BezierSurface> aBezier = occ::down_cast<Geom_BezierSurface>(theSurface);
  if (!aBezier.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_BezierSurface(aBezier, theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_BezierSurface(aBezier);
    }
    return;
  }

  occ::handle<Geom_BSplineSurface> aBSpline = occ::down_cast<Geom_BSplineSurface>(theSurface);
  if (!aBSpline.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_BSplineSurface(aBSpline, theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_BSplineSurface(aBSpline);
    }
    return;
  }

  occ::handle<Geom_OffsetSurface> anOffset = occ::down_cast<Geom_OffsetSurface>(theSurface);
  if (!anOffset.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator = ExtremaPS_OffsetSurface(anOffset, theDomain.value());
    }
    else
    {
      myEvaluator = ExtremaPS_OffsetSurface(anOffset);
    }
    return;
  }

  occ::handle<Geom_SurfaceOfRevolution> aRevolution =
    occ::down_cast<Geom_SurfaceOfRevolution>(theSurface);
  if (!aRevolution.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator.emplace<ExtremaPS_SurfaceOfRevolution>(aRevolution, theDomain.value());
    }
    else
    {
      myEvaluator.emplace<ExtremaPS_SurfaceOfRevolution>(aRevolution);
    }
    return;
  }

  occ::handle<Geom_SurfaceOfLinearExtrusion> anExtrusion =
    occ::down_cast<Geom_SurfaceOfLinearExtrusion>(theSurface);
  if (!anExtrusion.IsNull())
  {
    if (theDomain.has_value())
    {
      myEvaluator.emplace<ExtremaPS_SurfaceOfExtrusion>(anExtrusion, theDomain.value());
    }
    else
    {
      myEvaluator.emplace<ExtremaPS_SurfaceOfExtrusion>(anExtrusion);
    }
    return;
  }

  // For all other surfaces, store adaptor and use OtherSurface evaluator
  if (theDomain.has_value())
  {
    myEvaluator = ExtremaPS_OtherSurface(theSurface, theDomain.value());
  }
  else
  {
    myEvaluator = ExtremaPS_OtherSurface(theSurface);
  }
}

//==================================================================================================

void ExtremaPS_Surface::initializeEvaluator(const GeomAdaptor_Surface& theSurface,
                                            const ExtremaPS::Domain2D& theDomain)
{
  if (!theSurface.Surface().IsNull())
  {
    initFromGeomSurface(theSurface.Surface(), theDomain);
  }
}

//==================================================================================================

void ExtremaPS_Surface::initializeEvaluator(const GeomAdaptor_TransformedSurface& theSurface,
                                            const ExtremaPS::Domain2D&            theDomain)
{
  // Dispatch the transformed geometry so all evaluators operate in the same
  // coordinate system as the query point.
  initializeEvaluator(theSurface.AdaptorSurfaceTransformed(), theDomain);
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_Surface::Perform(const gp_Pnt&               theP,
                                                    const double                theTol,
                                                    const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result* aResultPtr = &notDoneResult();
  std::visit(
    [&](auto& theEval) {
      using T = std::decay_t<decltype(theEval)>;
      if constexpr (!std::is_same_v<T, std::monostate>)
      {
        aResultPtr = &theEval.Perform(theP, theTol, theMode);
      }
    },
    myEvaluator);
  return *aResultPtr;
}

//==================================================================================================

const ExtremaPS::Result& ExtremaPS_Surface::PerformWithBoundary(
  const gp_Pnt&               theP,
  const double                theTol,
  const ExtremaPS::SearchMode theMode) const
{
  const ExtremaPS::Result* aResultPtr = &notDoneResult();
  std::visit(
    [&](auto& theEval) {
      using T = std::decay_t<decltype(theEval)>;
      if constexpr (!std::is_same_v<T, std::monostate>)
      {
        aResultPtr = &theEval.PerformWithBoundary(theP, theTol, theMode);
      }
    },
    myEvaluator);
  return *aResultPtr;
}

//==================================================================================================

bool ExtremaPS_Surface::IsInitialized() const
{
  return !std::holds_alternative<std::monostate>(myEvaluator);
}
