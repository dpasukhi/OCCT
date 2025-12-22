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

#ifndef _Geom2dAdaptor_CurveCore_HeaderFile
#define _Geom2dAdaptor_CurveCore_HeaderFile

#include <BSplCLib_Cache.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_OffsetCurve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Hypr2d.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Parab2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf2d.hxx>
#include <gp_Vec2d.hxx>
#include <Precision.hxx>
#include <Standard_ConstructionError.hxx>
#include <Standard_NullObject.hxx>
#include <TColStd_Array1OfReal.hxx>

#include <cmath>
#include <memory>
#include <optional>
#include <variant>

//! Value-type core implementation for 2D curve adaptor evaluation.
//!
//! This class provides all 2D curve evaluation functionality without virtual dispatch,
//! supporting optional coordinate transformation that can be combined with any modifier.
//! It is designed for stack allocation and value semantics, serving as the main
//! implementation body for Geom2dAdaptor_Curve.
//!
//! The evaluation pipeline consists of three stages:
//! 1. Parameter modification (pre-evaluation): ParameterModifier transforms input parameter
//! 2. Curve evaluation: EvaluationVariant determines how the curve is evaluated
//! 3. Result modification (post-evaluation): gp_Trsf2d and PostProcessor transform outputs
//!
//! The class supports multiple evaluation types through EvaluationVariant:
//! - OffsetData: For offset curves
//! - PiecewiseData: For composite curves
//! - BezierData: For cached Bezier evaluation
//! - BSplineData: For cached B-spline evaluation
//!
//! Transformation (gp_Trsf2d) is stored separately and applied AFTER evaluation,
//! allowing combination of any evaluation type with transformation.
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class Geom2dAdaptor_CurveCore
{
public:
  //! Internal structure for 2D offset curve evaluation data.
  struct OffsetData
  {
    std::unique_ptr<Geom2dAdaptor_CurveCore> BasisCore;     //!< Core for basis curve
    double                                   Offset = 0.0;  //!< Offset distance
  };

  //! Internal structure for piecewise (composite) 2D curve evaluation data.
  struct PiecewiseData
  {
    std::vector<Geom2dAdaptor_CurveCore> Curves;  //!< Array of curve segments
    std::vector<double>                  Knots;   //!< Junction parameters
  };

  //! Internal structure for 2D Bezier curve cache data.
  struct BezierData
  {
    mutable Handle(BSplCLib_Cache) Cache;  //!< Cached data for evaluation
  };

  //! Internal structure for 2D BSpline curve cache data.
  struct BSplineData
  {
    Handle(Geom2d_BSplineCurve)    Curve;  //!< BSpline curve to prevent downcasts
    mutable Handle(BSplCLib_Cache) Cache;  //!< Cached data for evaluation
  };

  //! Variant type for 2D curve-specific evaluation data.
  //! Holds cache data (BSpline/Bezier) or alternative curve representations (Offset, Piecewise).
  using EvaluationVariant = std::variant<std::monostate,
                                         OffsetData,
                                         PiecewiseData,
                                         BezierData,
                                         BSplineData>;

  // === Parameter Modifiers (pre-evaluation) ===

  //! Linear parameter transformation: result = Scale * input + Offset.
  //! Used for reparametrization (e.g., BRepAdaptor trimmed curves).
  struct LinearParameterModifier
  {
    double Scale  = 1.0;  //!< Scale factor for parameter
    double Offset = 0.0;  //!< Offset added after scaling
  };

  //! Periodic parameter normalization.
  //! Brings parameter into [FirstParam, FirstParam + Period) range.
  struct PeriodicParameterModifier
  {
    double Period     = 0.0;  //!< Period value
    double FirstParam = 0.0;  //!< First parameter of periodic range
  };

  //! Variant for pre-evaluation parameter transformation.
  using ParameterModifier = std::variant<std::monostate,
                                         LinearParameterModifier,
                                         PeriodicParameterModifier>;

  // === Post-Processors (post-evaluation) ===

  //! Derivative scaling for chain rule application.
  //! Scales derivatives based on parameter transformation.
  struct DerivativeScaleModifier
  {
    double Scale = 1.0;  //!< Derivative scale factor (applied as Scale^N for Nth derivative)
  };

  //! Variant for post-evaluation result modification.
  using PostProcessor = std::variant<std::monostate,
                                     DerivativeScaleModifier>;

public:
  //! Default constructor. Creates an empty core with no curve loaded.
  Standard_EXPORT Geom2dAdaptor_CurveCore();

  //! Constructor with curve. Optional transformation is not set.
  //! @param[in] theCurve the 2D geometry curve to adapt
  Standard_EXPORT explicit Geom2dAdaptor_CurveCore(const Handle(Geom2d_Curve)& theCurve);

  //! Constructor with curve and parameter bounds.
  //! @param[in] theCurve the 2D geometry curve to adapt
  //! @param[in] theUFirst first parameter bound
  //! @param[in] theULast last parameter bound
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  Standard_EXPORT Geom2dAdaptor_CurveCore(const Handle(Geom2d_Curve)& theCurve,
                                          double                      theUFirst,
                                          double                      theULast);

  //! Copy constructor. Performs deep copy of modifier data.
  Standard_EXPORT Geom2dAdaptor_CurveCore(const Geom2dAdaptor_CurveCore& theOther);

  //! Move constructor.
  Standard_EXPORT Geom2dAdaptor_CurveCore(Geom2dAdaptor_CurveCore&& theOther) noexcept;

  //! Copy assignment. Performs deep copy of modifier data.
  Standard_EXPORT Geom2dAdaptor_CurveCore& operator=(const Geom2dAdaptor_CurveCore& theOther);

  //! Move assignment.
  Standard_EXPORT Geom2dAdaptor_CurveCore& operator=(Geom2dAdaptor_CurveCore&& theOther) noexcept;

  //! Destructor. Defined in .cxx to allow incomplete types in unique_ptr.
  Standard_EXPORT ~Geom2dAdaptor_CurveCore();

  // === Initialization ===

  //! Load a curve. Clears any existing modifier and transformation.
  //! @param[in] theCurve the 2D geometry curve to load
  //! @throw Standard_NullObject if theCurve is null
  void Load(const Handle(Geom2d_Curve)& theCurve)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject("Geom2dAdaptor_CurveCore::Load - null curve");
    }
    load(theCurve, theCurve->FirstParameter(), theCurve->LastParameter());
  }

  //! Load a curve with parameter bounds. Clears any existing modifier and transformation.
  //! @param[in] theCurve the 2D geometry curve to load
  //! @param[in] theUFirst first parameter bound
  //! @param[in] theULast last parameter bound
  //! @throw Standard_NullObject if theCurve is null
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  void Load(const Handle(Geom2d_Curve)& theCurve, double theUFirst, double theULast)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject("Geom2dAdaptor_CurveCore::Load - null curve");
    }
    if (theUFirst > theULast + Precision::Confusion())
    {
      throw Standard_ConstructionError("Geom2dAdaptor_CurveCore::Load - invalid parameter range");
    }
    load(theCurve, theUFirst, theULast);
  }

  //! Reset to empty state. Clears curve, modifier, and transformation.
  Standard_EXPORT void Reset();

  // === Transformation ===

  //! Set the 2D transformation to apply to all output geometry.
  //! Transformation is applied AFTER any modifier evaluation.
  //! @param[in] theTrsf 2D transformation to apply
  void SetTransformation(const gp_Trsf2d& theTrsf) { myTrsf = theTrsf; }

  //! Clear the transformation (output will be in curve's coordinate system).
  void ClearTransformation() { myTrsf.reset(); }

  //! Check if transformation is set.
  //! @return true if transformation is active
  bool HasTransformation() const { return myTrsf.has_value(); }

  //! Get the 2D transformation.
  //! @return the current transformation
  //! @throw Standard_NoSuchObject if no transformation is set
  Standard_EXPORT const gp_Trsf2d& Transformation() const;

  // === Parameter Modifier ===

  //! Set linear parameter modifier: result = Scale * input + Offset.
  //! @param[in] theScale scale factor for parameter
  //! @param[in] theOffset offset added after scaling
  void SetLinearParameterModifier(double theScale, double theOffset)
  {
    myParamModifier = LinearParameterModifier{theScale, theOffset};
  }

  //! Set periodic parameter modifier.
  //! @param[in] thePeriod period value
  //! @param[in] theFirstParam first parameter of periodic range
  void SetPeriodicParameterModifier(double thePeriod, double theFirstParam)
  {
    myParamModifier = PeriodicParameterModifier{thePeriod, theFirstParam};
  }

  //! Clear the parameter modifier.
  void ClearParameterModifier() { myParamModifier = std::monostate{}; }

  //! Check if parameter modifier is set.
  //! @return true if parameter modifier is active
  bool HasParameterModifier() const { return !std::holds_alternative<std::monostate>(myParamModifier); }

  //! Get the parameter modifier variant.
  //! @return reference to parameter modifier variant
  const ParameterModifier& GetParameterModifier() const { return myParamModifier; }

  // === Post-Processor ===

  //! Set derivative scale modifier.
  //! @param[in] theScale scale factor for derivatives (applied as Scale^N for Nth derivative)
  void SetDerivativeScaleModifier(double theScale)
  {
    myPostProcessor = DerivativeScaleModifier{theScale};
  }

  //! Clear the post-processor.
  void ClearPostProcessor() { myPostProcessor = std::monostate{}; }

  //! Check if post-processor is set.
  //! @return true if post-processor is active
  bool HasPostProcessor() const { return !std::holds_alternative<std::monostate>(myPostProcessor); }

  //! Get the post-processor variant.
  //! @return reference to post-processor variant
  const PostProcessor& GetPostProcessor() const { return myPostProcessor; }

  // === Curve access ===

  //! Returns the underlying 2D curve (may be null if using modifier-only mode).
  const Handle(Geom2d_Curve)& Curve() const { return myCurve; }

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

  //! Computes the 2D point at parameter U.
  //! @param[in] theU parameter value
  //! @return the 2D point (transformed if transformation is set)
  Standard_EXPORT gp_Pnt2d Value(double theU) const;

  //! Computes the 2D point at parameter U.
  //! @param[in] theU parameter value
  //! @param[out] theP computed 2D point (transformed if transformation is set)
  Standard_EXPORT void D0(double theU, gp_Pnt2d& theP) const;

  //! Computes point and first derivative.
  //! @param[in] theU parameter value
  //! @param[out] theP 2D point (transformed)
  //! @param[out] theV first derivative (transformed)
  Standard_EXPORT void D1(double theU, gp_Pnt2d& theP, gp_Vec2d& theV) const;

  //! Computes point and first two derivatives.
  //! @param[in] theU parameter value
  //! @param[out] theP 2D point (transformed)
  //! @param[out] theV1 first derivative (transformed)
  //! @param[out] theV2 second derivative (transformed)
  Standard_EXPORT void D2(double theU, gp_Pnt2d& theP, gp_Vec2d& theV1, gp_Vec2d& theV2) const;

  //! Computes point and first three derivatives.
  //! @param[in] theU parameter value
  //! @param[out] theP 2D point (transformed)
  //! @param[out] theV1 first derivative (transformed)
  //! @param[out] theV2 second derivative (transformed)
  //! @param[out] theV3 third derivative (transformed)
  Standard_EXPORT void D3(double    theU,
                          gp_Pnt2d& theP,
                          gp_Vec2d& theV1,
                          gp_Vec2d& theV2,
                          gp_Vec2d& theV3) const;

  //! Computes the Nth derivative.
  //! @param[in] theU parameter value
  //! @param[in] theN derivative order (>=1)
  //! @return the Nth derivative vector (transformed)
  Standard_EXPORT gp_Vec2d DN(double theU, int theN) const;

  //! Returns the parametric resolution corresponding to real space resolution R2d.
  //! @param[in] theR2d real space resolution
  Standard_EXPORT double Resolution(double theR2d) const;

  // === Curve primitives (with transformation applied) ===

  //! Returns the 2D line (GetType() must be GeomAbs_Line).
  Standard_EXPORT gp_Lin2d Line() const;

  //! Returns the 2D circle (GetType() must be GeomAbs_Circle).
  Standard_EXPORT gp_Circ2d Circle() const;

  //! Returns the 2D ellipse (GetType() must be GeomAbs_Ellipse).
  Standard_EXPORT gp_Elips2d Ellipse() const;

  //! Returns the 2D hyperbola (GetType() must be GeomAbs_Hyperbola).
  Standard_EXPORT gp_Hypr2d Hyperbola() const;

  //! Returns the 2D parabola (GetType() must be GeomAbs_Parabola).
  Standard_EXPORT gp_Parab2d Parabola() const;

  // === Spline properties ===

  //! Returns the degree (for BezierCurve or BSplineCurve).
  Standard_EXPORT int Degree() const;

  //! Returns true if the curve is rational (for BezierCurve or BSplineCurve).
  Standard_EXPORT bool IsRational() const;

  //! Returns the number of poles (for BezierCurve or BSplineCurve).
  Standard_EXPORT int NbPoles() const;

  //! Returns the number of knots (for BSplineCurve).
  Standard_EXPORT int NbKnots() const;

  //! Returns the 2D Bezier curve (GetType() must be GeomAbs_BezierCurve).
  Standard_EXPORT Handle(Geom2d_BezierCurve) Bezier() const;

  //! Returns the 2D BSpline curve (GetType() must be GeomAbs_BSplineCurve).
  Standard_EXPORT Handle(Geom2d_BSplineCurve) BSpline() const;

  //! Returns the 2D offset curve (GetType() must be GeomAbs_OffsetCurve).
  Standard_EXPORT Handle(Geom2d_OffsetCurve) OffsetCurve() const;

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
  Standard_EXPORT void load(const Handle(Geom2d_Curve)& theCurve,
                            double                      theUFirst,
                            double                      theULast);

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

  //! Apply 2D transformation to point (if set).
  void applyTransform(gp_Pnt2d& theP) const
  {
    if (myTrsf.has_value())
    {
      theP.Transform(*myTrsf);
    }
  }

  //! Apply 2D transformation to vector (if set).
  void applyTransform(gp_Vec2d& theV) const
  {
    if (myTrsf.has_value())
    {
      theV.Transform(*myTrsf);
    }
  }

  //! Apply parameter modifier (if set).
  //! @param[in] theU input parameter
  //! @return transformed parameter
  double applyParamModifier(double theU) const
  {
    if (const auto* aLinear = std::get_if<LinearParameterModifier>(&myParamModifier))
    {
      return aLinear->Scale * theU + aLinear->Offset;
    }
    else if (const auto* aPeriodic = std::get_if<PeriodicParameterModifier>(&myParamModifier))
    {
      double aU = theU - aPeriodic->FirstParam;
      if (aPeriodic->Period > 0.0)
      {
        aU = aU - aPeriodic->Period * std::floor(aU / aPeriodic->Period);
      }
      return aU + aPeriodic->FirstParam;
    }
    return theU;
  }

  //! Apply post-processor to derivative vector (if set).
  //! @param[in,out] theV derivative vector to modify
  //! @param[in] theOrder derivative order (1, 2, 3, ...)
  void applyPostProcessor(gp_Vec2d& theV, int theOrder) const
  {
    if (const auto* aDerivScale = std::get_if<DerivativeScaleModifier>(&myPostProcessor))
    {
      double aScale = aDerivScale->Scale;
      for (int i = 1; i < theOrder; ++i)
      {
        aScale *= aDerivScale->Scale;
      }
      theV *= aScale;
    }
  }

private:
  Handle(Geom2d_Curve)       myCurve;          //!< The underlying 2D geometry curve
  GeomAbs_CurveType          myTypeCurve;      //!< Curve type for fast dispatch
  double                     myFirst;          //!< First parameter bound
  double                     myLast;           //!< Last parameter bound
  EvaluationVariant          myEvalData;       //!< Curve-specific evaluation data (cache or alternative representation)
  std::optional<gp_Trsf2d>   myTrsf;           //!< Optional 2D transformation modifier
  ParameterModifier          myParamModifier;  //!< Parameter modification (pre-evaluation)
  PostProcessor              myPostProcessor;  //!< Result modification (post-transformation)
};

#endif // _Geom2dAdaptor_CurveCore_HeaderFile
