// Copyright (c) 2024 OPEN CASCADE SAS
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

#ifndef _GeomAdaptor_SurfaceCore_HeaderFile
#define _GeomAdaptor_SurfaceCore_HeaderFile

#include <BSplSLib_Cache.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Surface.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Ax1.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_XYZ.hxx>
#include <Precision.hxx>
#include <Standard_ConstructionError.hxx>
#include <Standard_NullObject.hxx>
#include <TColStd_Array1OfReal.hxx>

#include <memory>
#include <optional>
#include <variant>

class GeomAdaptor_CurveCore;

//! Value-type core implementation for 3D surface adaptor evaluation.
//!
//! This class provides all surface evaluation functionality without virtual dispatch,
//! supporting optional coordinate transformation that can be combined with any modifier.
//! It is designed for stack allocation and value semantics, serving as the main
//! implementation body for GeomAdaptor_Surface.
//!
//! The class supports multiple modifier types through a variant:
//! - OffsetData: For offset surfaces
//! - ExtrusionData: For surfaces of linear extrusion
//! - RevolutionData: For surfaces of revolution
//! - BezierData: For cached Bezier evaluation
//! - BSplineData: For cached B-spline evaluation
//!
//! Transformation (gp_Trsf) is stored separately and applied AFTER the modifier,
//! allowing combination of any modifier with transformation.
//!
//! Polynomial coefficients of BSpline surfaces used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class GeomAdaptor_SurfaceCore
{
public:
  //! Internal structure for offset surface evaluation data.
  struct OffsetData
  {
    std::unique_ptr<GeomAdaptor_SurfaceCore> BasisCore;       //!< Core for basis surface
    std::unique_ptr<GeomAdaptor_SurfaceCore> EquivalentCore;  //!< Core for equivalent (canonical) surface
    Handle(Geom_OffsetSurface)               OffsetSurface;   //!< Offset surface for osculating queries
    double                                   Offset = 0.0;    //!< Offset distance
  };

  //! Internal structure for surface of linear extrusion evaluation data.
  struct ExtrusionData
  {
    std::unique_ptr<GeomAdaptor_CurveCore> BasisCurve;  //!< Basis curve for extrusion
    gp_XYZ                                 Direction;   //!< Extrusion direction (as XYZ for fast access)
  };

  //! Internal structure for surface of revolution evaluation data.
  struct RevolutionData
  {
    std::unique_ptr<GeomAdaptor_CurveCore> BasisCurve;  //!< Basis curve for revolution
    gp_Ax1                                 Axis;        //!< Revolution axis
  };

  //! Internal structure for Bezier surface cache data.
  struct BezierData
  {
    mutable Handle(BSplSLib_Cache) Cache;  //!< Cached data for evaluation
  };

  //! Internal structure for BSpline surface cache data.
  struct BSplineData
  {
    Handle(Geom_BSplineSurface)    Surface;  //!< BSpline surface to prevent downcasts
    mutable Handle(BSplSLib_Cache) Cache;    //!< Cached data for evaluation
  };

  //! Variant type for surface-specific evaluation data.
  //! Holds cache data (BSpline/Bezier) or alternative surface representations
  //! (Offset, Extrusion, Revolution).
  using EvaluationVariant = std::variant<std::monostate,
                                         OffsetData,
                                         ExtrusionData,
                                         RevolutionData,
                                         BezierData,
                                         BSplineData>;

public:
  //! Default constructor. Creates an empty core with no surface loaded.
  Standard_EXPORT GeomAdaptor_SurfaceCore();

  //! Constructor with surface. Optional transformation is not set.
  //! @param[in] theSurface the geometry surface to adapt
  Standard_EXPORT explicit GeomAdaptor_SurfaceCore(const Handle(Geom_Surface)& theSurface);

  //! Constructor with surface and parameter bounds.
  //! @param[in] theSurface the geometry surface to adapt
  //! @param[in] theUFirst first U parameter bound
  //! @param[in] theULast last U parameter bound
  //! @param[in] theVFirst first V parameter bound
  //! @param[in] theVLast last V parameter bound
  //! @param[in] theTolU tolerance in U direction (default: 0.0)
  //! @param[in] theTolV tolerance in V direction (default: 0.0)
  Standard_EXPORT GeomAdaptor_SurfaceCore(const Handle(Geom_Surface)& theSurface,
                                          double                      theUFirst,
                                          double                      theULast,
                                          double                      theVFirst,
                                          double                      theVLast,
                                          double                      theTolU = 0.0,
                                          double                      theTolV = 0.0);

  //! Copy constructor. Performs deep copy of modifier data.
  Standard_EXPORT GeomAdaptor_SurfaceCore(const GeomAdaptor_SurfaceCore& theOther);

  //! Move constructor.
  Standard_EXPORT GeomAdaptor_SurfaceCore(GeomAdaptor_SurfaceCore&& theOther) noexcept;

  //! Copy assignment. Performs deep copy of modifier data.
  Standard_EXPORT GeomAdaptor_SurfaceCore& operator=(const GeomAdaptor_SurfaceCore& theOther);

  //! Move assignment.
  Standard_EXPORT GeomAdaptor_SurfaceCore& operator=(GeomAdaptor_SurfaceCore&& theOther) noexcept;

  //! Destructor. Defined in .cxx to allow incomplete types in unique_ptr.
  Standard_EXPORT ~GeomAdaptor_SurfaceCore();

  // === Initialization ===

  //! Load a surface. Clears any existing modifier and transformation.
  //! @param[in] theSurface the geometry surface to load
  //! @throw Standard_NullObject if theSurface is null
  void Load(const Handle(Geom_Surface)& theSurface)
  {
    if (theSurface.IsNull())
    {
      throw Standard_NullObject("GeomAdaptor_SurfaceCore::Load - null surface");
    }
    double aU1, aU2, aV1, aV2;
    theSurface->Bounds(aU1, aU2, aV1, aV2);
    load(theSurface, aU1, aU2, aV1, aV2, 0.0, 0.0);
  }

  //! Load a surface with parameter bounds. Clears any existing modifier and transformation.
  //! @param[in] theSurface the geometry surface to load
  //! @param[in] theUFirst first U parameter bound
  //! @param[in] theULast last U parameter bound
  //! @param[in] theVFirst first V parameter bound
  //! @param[in] theVLast last V parameter bound
  //! @param[in] theTolU tolerance in U direction (default: 0.0)
  //! @param[in] theTolV tolerance in V direction (default: 0.0)
  //! @throw Standard_NullObject if theSurface is null
  void Load(const Handle(Geom_Surface)& theSurface,
            double                      theUFirst,
            double                      theULast,
            double                      theVFirst,
            double                      theVLast,
            double                      theTolU = 0.0,
            double                      theTolV = 0.0)
  {
    if (theSurface.IsNull())
    {
      throw Standard_NullObject("GeomAdaptor_SurfaceCore::Load - null surface");
    }
    load(theSurface, theUFirst, theULast, theVFirst, theVLast, theTolU, theTolV);
  }

  //! Reset to empty state. Clears surface, modifier, and transformation.
  Standard_EXPORT void Reset();

  // === Transformation ===

  //! Set the transformation to apply to all output geometry.
  //! Transformation is applied AFTER any modifier evaluation.
  //! @param[in] theTrsf transformation to apply
  void SetTransformation(const gp_Trsf& theTrsf) { myTrsf = theTrsf; }

  //! Clear the transformation (output will be in surface's coordinate system).
  void ClearTransformation() { myTrsf.reset(); }

  //! Check if transformation is set.
  //! @return true if transformation is active
  bool HasTransformation() const { return myTrsf.has_value(); }

  //! Get the transformation.
  //! @return the current transformation
  //! @throw Standard_NoSuchObject if no transformation is set
  Standard_EXPORT const gp_Trsf& Transformation() const;

  // === Surface access ===

  //! Returns the underlying surface (may be null if using modifier-only mode).
  const Handle(Geom_Surface)& Surface() const { return mySurface; }

  //! Returns the first U parameter.
  double FirstUParameter() const { return myUFirst; }

  //! Returns the last U parameter.
  double LastUParameter() const { return myULast; }

  //! Returns the first V parameter.
  double FirstVParameter() const { return myVFirst; }

  //! Returns the last V parameter.
  double LastVParameter() const { return myVLast; }

  //! Returns the parameter bounds of the surface.
  //! @param[out] theU1 first U parameter
  //! @param[out] theU2 last U parameter
  //! @param[out] theV1 first V parameter
  //! @param[out] theV2 last V parameter
  void Bounds(double& theU1, double& theU2, double& theV1, double& theV2) const
  {
    theU1 = myUFirst;
    theU2 = myULast;
    theV1 = myVFirst;
    theV2 = myVLast;
  }

  //! Returns the surface type.
  GeomAbs_SurfaceType GetType() const { return mySurfaceType; }

  // === Continuity ===

  //! Returns the continuity in U direction.
  Standard_EXPORT GeomAbs_Shape UContinuity() const;

  //! Returns the continuity in V direction.
  Standard_EXPORT GeomAbs_Shape VContinuity() const;

  //! Returns the number of U intervals for the given continuity.
  //! @param[in] theS the required continuity
  Standard_EXPORT int NbUIntervals(GeomAbs_Shape theS) const;

  //! Returns the number of V intervals for the given continuity.
  //! @param[in] theS the required continuity
  Standard_EXPORT int NbVIntervals(GeomAbs_Shape theS) const;

  //! Stores the U interval bounds for the given continuity.
  //! @param[out] theT array to receive interval bounds (size must be >= NbUIntervals(theS) + 1)
  //! @param[in] theS the required continuity
  Standard_EXPORT void UIntervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const;

  //! Stores the V interval bounds for the given continuity.
  //! @param[out] theT array to receive interval bounds (size must be >= NbVIntervals(theS) + 1)
  //! @param[in] theS the required continuity
  Standard_EXPORT void VIntervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const;

  // === Evaluation methods ===
  // All evaluation methods apply transformation if set.

  //! Computes the point at parameters (U, V).
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @return the point (transformed if transformation is set)
  Standard_EXPORT gp_Pnt Value(double theU, double theV) const;

  //! Computes the point at parameters (U, V).
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[out] theP computed point (transformed if transformation is set)
  Standard_EXPORT void D0(double theU, double theV, gp_Pnt& theP) const;

  //! Computes point and first derivatives.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theD1U first derivative in U (transformed)
  //! @param[out] theD1V first derivative in V (transformed)
  Standard_EXPORT void D1(double   theU,
                          double   theV,
                          gp_Pnt&  theP,
                          gp_Vec&  theD1U,
                          gp_Vec&  theD1V) const;

  //! Computes point and first two derivatives.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theD1U first derivative in U (transformed)
  //! @param[out] theD1V first derivative in V (transformed)
  //! @param[out] theD2U second derivative in U (transformed)
  //! @param[out] theD2V second derivative in V (transformed)
  //! @param[out] theD2UV mixed second derivative (transformed)
  Standard_EXPORT void D2(double   theU,
                          double   theV,
                          gp_Pnt&  theP,
                          gp_Vec&  theD1U,
                          gp_Vec&  theD1V,
                          gp_Vec&  theD2U,
                          gp_Vec&  theD2V,
                          gp_Vec&  theD2UV) const;

  //! Computes point and first three derivatives.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theD1U first derivative in U (transformed)
  //! @param[out] theD1V first derivative in V (transformed)
  //! @param[out] theD2U second derivative in U (transformed)
  //! @param[out] theD2V second derivative in V (transformed)
  //! @param[out] theD2UV mixed second derivative (transformed)
  //! @param[out] theD3U third derivative in U (transformed)
  //! @param[out] theD3V third derivative in V (transformed)
  //! @param[out] theD3UUV mixed third derivative UUV (transformed)
  //! @param[out] theD3UVV mixed third derivative UVV (transformed)
  Standard_EXPORT void D3(double   theU,
                          double   theV,
                          gp_Pnt&  theP,
                          gp_Vec&  theD1U,
                          gp_Vec&  theD1V,
                          gp_Vec&  theD2U,
                          gp_Vec&  theD2V,
                          gp_Vec&  theD2UV,
                          gp_Vec&  theD3U,
                          gp_Vec&  theD3V,
                          gp_Vec&  theD3UUV,
                          gp_Vec&  theD3UVV) const;

  //! Computes the derivative of order (Nu, Nv).
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theNu derivative order in U (>=0)
  //! @param[in] theNv derivative order in V (>=0)
  //! @return the derivative vector (transformed)
  Standard_EXPORT gp_Vec DN(double theU, double theV, int theNu, int theNv) const;

  //! Returns the parametric U resolution corresponding to real space resolution R3d.
  //! @param[in] theR3d real space resolution
  Standard_EXPORT double UResolution(double theR3d) const;

  //! Returns the parametric V resolution corresponding to real space resolution R3d.
  //! @param[in] theR3d real space resolution
  Standard_EXPORT double VResolution(double theR3d) const;

  // === Surface primitives (with transformation applied) ===

  //! Returns the plane (GetType() must be GeomAbs_Plane).
  Standard_EXPORT gp_Pln Plane() const;

  //! Returns the cylinder (GetType() must be GeomAbs_Cylinder).
  Standard_EXPORT gp_Cylinder Cylinder() const;

  //! Returns the cone (GetType() must be GeomAbs_Cone).
  Standard_EXPORT gp_Cone Cone() const;

  //! Returns the sphere (GetType() must be GeomAbs_Sphere).
  Standard_EXPORT gp_Sphere Sphere() const;

  //! Returns the torus (GetType() must be GeomAbs_Torus).
  Standard_EXPORT gp_Torus Torus() const;

  // === Spline properties ===

  //! Returns the U degree (for BezierSurface or BSplineSurface).
  Standard_EXPORT int UDegree() const;

  //! Returns the V degree (for BezierSurface or BSplineSurface).
  Standard_EXPORT int VDegree() const;

  //! Returns the number of U poles (for BezierSurface or BSplineSurface).
  Standard_EXPORT int NbUPoles() const;

  //! Returns the number of V poles (for BezierSurface or BSplineSurface).
  Standard_EXPORT int NbVPoles() const;

  //! Returns the number of U knots (for BSplineSurface).
  Standard_EXPORT int NbUKnots() const;

  //! Returns the number of V knots (for BSplineSurface).
  Standard_EXPORT int NbVKnots() const;

  //! Returns true if the surface is U-rational.
  Standard_EXPORT bool IsURational() const;

  //! Returns true if the surface is V-rational.
  Standard_EXPORT bool IsVRational() const;

  //! Returns the Bezier surface (GetType() must be GeomAbs_BezierSurface).
  Standard_EXPORT Handle(Geom_BezierSurface) Bezier() const;

  //! Returns the BSpline surface (GetType() must be GeomAbs_BSplineSurface).
  Standard_EXPORT Handle(Geom_BSplineSurface) BSpline() const;

  // === Closure and periodicity ===

  //! Returns true if the surface is U-closed.
  Standard_EXPORT bool IsUClosed() const;

  //! Returns true if the surface is V-closed.
  Standard_EXPORT bool IsVClosed() const;

  //! Returns true if the surface is U-periodic.
  Standard_EXPORT bool IsUPeriodic() const;

  //! Returns true if the surface is V-periodic.
  Standard_EXPORT bool IsVPeriodic() const;

  //! Returns the U period (for U-periodic surfaces).
  Standard_EXPORT double UPeriod() const;

  //! Returns the V period (for V-periodic surfaces).
  Standard_EXPORT double VPeriod() const;

  // === Revolution/Extrusion properties ===

  //! Returns the axis of revolution (GetType() must be GeomAbs_SurfaceOfRevolution).
  Standard_EXPORT gp_Ax1 AxeOfRevolution() const;

  //! Returns the extrusion direction (GetType() must be GeomAbs_SurfaceOfExtrusion).
  Standard_EXPORT gp_Dir Direction() const;

  // === Offset properties ===

  //! Returns the offset value (GetType() must be GeomAbs_OffsetSurface).
  Standard_EXPORT double OffsetValue() const;

  // === Evaluation data access ===

  //! Returns the evaluation data variant.
  const EvaluationVariant& EvaluationData() const { return myEvalData; }

  //! Returns the evaluation data variant for modification.
  EvaluationVariant& ChangeEvaluationData() { return myEvalData; }

private:
  //! Internal load implementation.
  Standard_EXPORT void load(const Handle(Geom_Surface)& theSurface,
                            double                      theUFirst,
                            double                      theULast,
                            double                      theVFirst,
                            double                      theVLast,
                            double                      theTolU,
                            double                      theTolV);

  //! Rebuild spline evaluation cache.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  void rebuildCache(double theU, double theV) const;

  //! Apply transformation to point (if set).
  void applyTransform(gp_Pnt& theP) const
  {
    if (myTrsf.has_value())
    {
      theP.Transform(*myTrsf);
    }
  }

  //! Apply transformation to vector (if set).
  void applyTransform(gp_Vec& theV) const
  {
    if (myTrsf.has_value())
    {
      theV.Transform(*myTrsf);
    }
  }

private:
  Handle(Geom_Surface)     mySurface;      //!< The underlying geometry surface
  GeomAbs_SurfaceType      mySurfaceType;  //!< Surface type for fast dispatch
  double                   myUFirst;       //!< First U parameter bound
  double                   myULast;        //!< Last U parameter bound
  double                   myVFirst;       //!< First V parameter bound
  double                   myVLast;        //!< Last V parameter bound
  double                   myTolU;         //!< U tolerance for boundary detection
  double                   myTolV;         //!< V tolerance for boundary detection
  EvaluationVariant        myEvalData;     //!< Surface-specific evaluation data (cache or alternative representation)
  std::optional<gp_Trsf>   myTrsf;         //!< Optional transformation modifier
};

#endif // _GeomAdaptor_SurfaceCore_HeaderFile
