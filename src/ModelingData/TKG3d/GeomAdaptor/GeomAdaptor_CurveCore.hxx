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

#ifndef _GeomAdaptor_CurveCore_HeaderFile
#define _GeomAdaptor_CurveCore_HeaderFile

#include <BSplCLib_Cache.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_OffsetCurve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_IsoType.hxx>
#include <GeomAbs_Shape.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Lin.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <Standard_NullObject.hxx>
#include <Standard_ConstructionError.hxx>
#include <TColStd_Array1OfReal.hxx>

#include <memory>
#include <optional>
#include <variant>

class GeomAdaptor_SurfaceCore;
class Geom2dAdaptor_CurveCore;

//! Value-type core implementation for 3D curve adaptor evaluation.
//!
//! This class provides all curve evaluation functionality without virtual dispatch,
//! supporting optional coordinate transformation that can be combined with any modifier.
//! It is designed for stack allocation and value semantics, serving as the main
//! implementation body for GeomAdaptor_Curve.
//!
//! The class supports multiple modifier types through a variant:
//! - OffsetData: For offset curves
//! - CurveOnSurfaceData: For curves defined by 2D curve on 3D surface
//! - IsoCurveData: For iso-parametric curves on surfaces
//! - PiecewiseData: For composite curves
//! - BezierData: For cached Bezier evaluation
//! - BSplineData: For cached B-spline evaluation
//!
//! Transformation (gp_Trsf) is stored separately and applied AFTER the modifier,
//! allowing combination of any modifier with transformation.
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class GeomAdaptor_CurveCore
{
public:
  //! Internal structure for offset curve evaluation data.
  struct OffsetData
  {
    std::unique_ptr<GeomAdaptor_CurveCore> BasisCore;     //!< Core for basis curve
    double                                 Offset = 0.0;  //!< Offset distance
    gp_Dir                                 Direction;     //!< Offset direction
  };

  //! Internal structure for curve-on-surface evaluation data.
  //! Evaluates 2D curve point, then evaluates 3D surface at that (U,V) location.
  struct CurveOnSurfaceData
  {
    std::unique_ptr<Geom2dAdaptor_CurveCore> Curve2d;  //!< 2D parametric curve
    std::unique_ptr<GeomAdaptor_SurfaceCore> Surface;  //!< 3D surface
  };

  //! Internal structure for iso-parametric curve evaluation data.
  //! Fixes one surface parameter, varies the other.
  struct IsoCurveData
  {
    std::unique_ptr<GeomAdaptor_SurfaceCore> Surface;      //!< Base surface
    GeomAbs_IsoType                          IsoType;      //!< IsoU or IsoV
    double                                   Parameter;    //!< Fixed parameter value
  };

  //! Internal structure for piecewise (composite) curve evaluation data.
  struct PiecewiseData
  {
    std::vector<GeomAdaptor_CurveCore> Curves;  //!< Array of curve segments
    std::vector<double>                Knots;   //!< Junction parameters
  };

  //! Internal structure for Bezier curve cache data.
  struct BezierData
  {
    mutable Handle(BSplCLib_Cache) Cache;  //!< Cached data for evaluation
  };

  //! Internal structure for BSpline curve cache data.
  struct BSplineData
  {
    Handle(Geom_BSplineCurve)      Curve;  //!< BSpline curve to prevent downcasts
    mutable Handle(BSplCLib_Cache) Cache;  //!< Cached data for evaluation
  };

  //! Variant type for curve-specific evaluation data.
  //! Holds cache data (BSpline/Bezier) or alternative curve representations
  //! (Offset, CurveOnSurface, IsoCurve, Piecewise).
  using EvaluationVariant = std::variant<std::monostate,
                                         OffsetData,
                                         CurveOnSurfaceData,
                                         IsoCurveData,
                                         PiecewiseData,
                                         BezierData,
                                         BSplineData>;

public:
  //! Default constructor. Creates an empty core with no curve loaded.
  Standard_EXPORT GeomAdaptor_CurveCore();

  //! Constructor with curve. Optional transformation is not set.
  //! @param[in] theCurve the geometry curve to adapt
  Standard_EXPORT explicit GeomAdaptor_CurveCore(const Handle(Geom_Curve)& theCurve);

  //! Constructor with curve and parameter bounds.
  //! @param[in] theCurve the geometry curve to adapt
  //! @param[in] theUFirst first parameter bound
  //! @param[in] theULast last parameter bound
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  Standard_EXPORT GeomAdaptor_CurveCore(const Handle(Geom_Curve)& theCurve,
                                        double                    theUFirst,
                                        double                    theULast);

  //! Copy constructor. Performs deep copy of modifier data.
  Standard_EXPORT GeomAdaptor_CurveCore(const GeomAdaptor_CurveCore& theOther);

  //! Move constructor.
  Standard_EXPORT GeomAdaptor_CurveCore(GeomAdaptor_CurveCore&& theOther) noexcept;

  //! Copy assignment. Performs deep copy of modifier data.
  Standard_EXPORT GeomAdaptor_CurveCore& operator=(const GeomAdaptor_CurveCore& theOther);

  //! Move assignment.
  Standard_EXPORT GeomAdaptor_CurveCore& operator=(GeomAdaptor_CurveCore&& theOther) noexcept;

  //! Destructor. Defined in .cxx to allow incomplete types in unique_ptr.
  Standard_EXPORT ~GeomAdaptor_CurveCore();

  // === Initialization ===

  //! Load a curve. Clears any existing modifier and transformation.
  //! @param[in] theCurve the geometry curve to load
  //! @throw Standard_NullObject if theCurve is null
  void Load(const Handle(Geom_Curve)& theCurve)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject("GeomAdaptor_CurveCore::Load - null curve");
    }
    load(theCurve, theCurve->FirstParameter(), theCurve->LastParameter());
  }

  //! Load a curve with parameter bounds. Clears any existing modifier and transformation.
  //! @param[in] theCurve the geometry curve to load
  //! @param[in] theUFirst first parameter bound
  //! @param[in] theULast last parameter bound
  //! @throw Standard_NullObject if theCurve is null
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  void Load(const Handle(Geom_Curve)& theCurve, double theUFirst, double theULast)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject("GeomAdaptor_CurveCore::Load - null curve");
    }
    if (theUFirst > theULast + Precision::Confusion())
    {
      throw Standard_ConstructionError("GeomAdaptor_CurveCore::Load - invalid parameter range");
    }
    load(theCurve, theUFirst, theULast);
  }

  //! Reset to empty state. Clears curve, modifier, and transformation.
  Standard_EXPORT void Reset();

  // === Transformation ===

  //! Set the transformation to apply to all output geometry.
  //! Transformation is applied AFTER any modifier evaluation.
  //! @param[in] theTrsf transformation to apply
  void SetTransformation(const gp_Trsf& theTrsf) { myTrsf = theTrsf; }

  //! Clear the transformation (output will be in curve's coordinate system).
  void ClearTransformation() { myTrsf.reset(); }

  //! Check if transformation is set.
  //! @return true if transformation is active
  bool HasTransformation() const { return myTrsf.has_value(); }

  //! Get the transformation.
  //! @return the current transformation
  //! @throw Standard_NoSuchObject if no transformation is set
  Standard_EXPORT const gp_Trsf& Transformation() const;

  // === Curve access ===

  //! Returns the underlying curve (may be null if using modifier-only mode).
  const Handle(Geom_Curve)& Curve() const { return myCurve; }

  //! Returns the first parameter.
  double FirstParameter() const { return myFirst; }

  //! Returns the last parameter.
  double LastParameter() const { return myLast; }

  //! Returns the curve type.
  GeomAbs_CurveType GetType() const { return myTypeCurve; }

  // === Continuity ===

  //! Returns the continuity of the curve.
  Standard_EXPORT GeomAbs_Shape Continuity() const;

  //! Returns the number of intervals for the given continuity.
  //! @param[in] theS the required continuity
  Standard_EXPORT int NbIntervals(GeomAbs_Shape theS) const;

  //! Stores the interval bounds for the given continuity.
  //! @param[out] theT array to receive interval bounds (size must be >= NbIntervals(theS) + 1)
  //! @param[in] theS the required continuity
  Standard_EXPORT void Intervals(TColStd_Array1OfReal& theT, GeomAbs_Shape theS) const;

  // === Evaluation methods ===
  // All evaluation methods apply transformation if set.

  //! Computes the point at parameter U.
  //! @param[in] theU parameter value
  //! @return the point (transformed if transformation is set)
  Standard_EXPORT gp_Pnt Value(double theU) const;

  //! Computes the point at parameter U.
  //! @param[in] theU parameter value
  //! @param[out] theP computed point (transformed if transformation is set)
  Standard_EXPORT void D0(double theU, gp_Pnt& theP) const;

  //! Computes point and first derivative.
  //! @param[in] theU parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theV first derivative (transformed)
  Standard_EXPORT void D1(double theU, gp_Pnt& theP, gp_Vec& theV) const;

  //! Computes point and first two derivatives.
  //! @param[in] theU parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theV1 first derivative (transformed)
  //! @param[out] theV2 second derivative (transformed)
  Standard_EXPORT void D2(double theU, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const;

  //! Computes point and first three derivatives.
  //! @param[in] theU parameter value
  //! @param[out] theP point (transformed)
  //! @param[out] theV1 first derivative (transformed)
  //! @param[out] theV2 second derivative (transformed)
  //! @param[out] theV3 third derivative (transformed)
  Standard_EXPORT void D3(double   theU,
                          gp_Pnt&  theP,
                          gp_Vec&  theV1,
                          gp_Vec&  theV2,
                          gp_Vec&  theV3) const;

  //! Computes the Nth derivative.
  //! @param[in] theU parameter value
  //! @param[in] theN derivative order (>=1)
  //! @return the Nth derivative vector (transformed)
  Standard_EXPORT gp_Vec DN(double theU, int theN) const;

  //! Returns the parametric resolution corresponding to real space resolution R3d.
  //! @param[in] theR3d real space resolution
  Standard_EXPORT double Resolution(double theR3d) const;

  // === Curve primitives (with transformation applied) ===

  //! Returns the line (GetType() must be GeomAbs_Line).
  Standard_EXPORT gp_Lin Line() const;

  //! Returns the circle (GetType() must be GeomAbs_Circle).
  Standard_EXPORT gp_Circ Circle() const;

  //! Returns the ellipse (GetType() must be GeomAbs_Ellipse).
  Standard_EXPORT gp_Elips Ellipse() const;

  //! Returns the hyperbola (GetType() must be GeomAbs_Hyperbola).
  Standard_EXPORT gp_Hypr Hyperbola() const;

  //! Returns the parabola (GetType() must be GeomAbs_Parabola).
  Standard_EXPORT gp_Parab Parabola() const;

  // === Spline properties ===

  //! Returns the degree (for BezierCurve or BSplineCurve).
  Standard_EXPORT int Degree() const;

  //! Returns true if the curve is rational (for BezierCurve or BSplineCurve).
  Standard_EXPORT bool IsRational() const;

  //! Returns the number of poles (for BezierCurve or BSplineCurve).
  Standard_EXPORT int NbPoles() const;

  //! Returns the number of knots (for BSplineCurve).
  Standard_EXPORT int NbKnots() const;

  //! Returns the Bezier curve (GetType() must be GeomAbs_BezierCurve).
  Standard_EXPORT Handle(Geom_BezierCurve) Bezier() const;

  //! Returns the BSpline curve (GetType() must be GeomAbs_BSplineCurve).
  Standard_EXPORT Handle(Geom_BSplineCurve) BSpline() const;

  //! Returns the offset curve (GetType() must be GeomAbs_OffsetCurve).
  Standard_EXPORT Handle(Geom_OffsetCurve) OffsetCurve() const;

  // === Other properties ===

  //! Returns true if the curve is closed.
  Standard_EXPORT bool IsClosed() const;

  //! Returns true if the curve is periodic.
  Standard_EXPORT bool IsPeriodic() const;

  //! Returns the period (for periodic curves).
  Standard_EXPORT double Period() const;

  // === Evaluation data access ===

  //! Returns the evaluation data variant.
  const EvaluationVariant& EvaluationData() const { return myEvalData; }

  //! Returns the evaluation data variant for modification.
  EvaluationVariant& ChangeEvaluationData() { return myEvalData; }

private:
  //! Internal load implementation.
  Standard_EXPORT void load(const Handle(Geom_Curve)& theCurve,
                            double                    theUFirst,
                            double                    theULast);

  //! Computes the continuity of a BSplineCurve between U1 and U2.
  Standard_EXPORT GeomAbs_Shape localContinuity(double theU1, double theU2) const;

  //! Rebuild spline evaluation cache.
  //! @param[in] theParameter parameter value to identify caching span
  void rebuildCache(double theParameter) const;

  //! Check if parameter is at boundary for BSpline.
  //! @param[in] theU parameter value
  //! @param[out] theSpanStart start of boundary span
  //! @param[out] theSpanFinish end of boundary span
  //! @return true if at boundary
  bool isBoundary(double theU, int& theSpanStart, int& theSpanFinish) const;

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
  Handle(Geom_Curve)     myCurve;      //!< The underlying geometry curve
  GeomAbs_CurveType      myTypeCurve;  //!< Curve type for fast dispatch
  double                 myFirst;      //!< First parameter bound
  double                 myLast;       //!< Last parameter bound
  EvaluationVariant      myEvalData;   //!< Curve-specific evaluation data (cache or alternative representation)
  std::optional<gp_Trsf> myTrsf;       //!< Optional transformation modifier
};

#endif // _GeomAdaptor_CurveCore_HeaderFile
