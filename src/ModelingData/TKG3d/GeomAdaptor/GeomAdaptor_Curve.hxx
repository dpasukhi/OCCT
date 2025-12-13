// Created on: 1992-09-01
// Created by: Modelistation
// Copyright (c) 1992-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

#ifndef _GeomAdaptor_Curve_HeaderFile
#define _GeomAdaptor_Curve_HeaderFile

#include <Adaptor3d_Curve.hxx>
#include <BSplCLib_Cache.hxx>
#include <Geom_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomAdaptor_CurveModifier.hxx>
#include <GeomEvaluator_Curve.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Lin.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <Standard_ConstructionError.hxx>
#include <Standard_NullObject.hxx>
#include <TColStd_Array1OfReal.hxx>

class Geom2dAdaptor_Curve;
class GeomAdaptor_Surface;
class Geom_BezierCurve;
class Geom_BSplineCurve;
class Geom_OffsetCurve;

DEFINE_STANDARD_HANDLE(GeomAdaptor_Curve, Adaptor3d_Curve)

//! This class provides an interface between the services provided by any
//! curve from the package Geom and those required of the curve by algorithms which use it.
//! Creation of the loaded curve the curve is C1 by piece.
//!
//! The adaptor supports modifiers that transform evaluation results:
//! - TrsfModifier: Applies gp_Trsf transformation
//! - CurveOnSurfaceModifier: Evaluates 2D parametric curve on 3D surface
//! - IsoCurveModifier: Evaluates isoparametric curve on surface
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class GeomAdaptor_Curve : public Adaptor3d_Curve
{
  DEFINE_STANDARD_RTTIEXT(GeomAdaptor_Curve, Adaptor3d_Curve)
public:
  //! Default constructor. Creates an empty adaptor.
  GeomAdaptor_Curve()
      : myTypeCurve(GeomAbs_OtherCurve),
        myFirst(0.0),
        myLast(0.0)
  {
  }

  //! Constructor from a Geom_Curve.
  //! @param theCurve the curve to adapt
  explicit GeomAdaptor_Curve(const Handle(Geom_Curve) & theCurve) { Load(theCurve); }

  //! Constructor from a Geom_Curve with parameter bounds.
  //! @param theCurve the curve to adapt
  //! @param theUFirst first parameter
  //! @param theULast last parameter
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::PConfusion()
  GeomAdaptor_Curve(const Handle(Geom_Curve) & theCurve, double theUFirst, double theULast)
  {
    Load(theCurve, theUFirst, theULast);
  }

  //! Copy constructor.
  Standard_EXPORT GeomAdaptor_Curve(const GeomAdaptor_Curve& theOther);

  //! Move constructor.
  Standard_EXPORT GeomAdaptor_Curve(GeomAdaptor_Curve&& theOther) noexcept;

  //! Copy assignment operator.
  Standard_EXPORT GeomAdaptor_Curve& operator=(const GeomAdaptor_Curve& theOther);

  //! Move assignment operator.
  Standard_EXPORT GeomAdaptor_Curve& operator=(GeomAdaptor_Curve&& theOther) noexcept;

  //! Creates an explicit deep copy of this adaptor.
  //! @return a new adaptor with copied data
  [[nodiscard]] Standard_EXPORT GeomAdaptor_Curve Copy() const;

  //! Shallow copy (creates a handle-wrapped copy for compatibility).
  //! @return handle to a new adaptor with copied data
  Standard_EXPORT virtual Handle(Adaptor3d_Curve) ShallowCopy() const Standard_OVERRIDE;

  //! Destructor.
  Standard_EXPORT virtual ~GeomAdaptor_Curve();

  //! Reset currently loaded curve (undoes Load()).
  Standard_EXPORT void Reset();

  //! Loads the curve.
  //! @param theCurve the curve to load
  //! @throw Standard_NullObject if theCurve is null
  void Load(const Handle(Geom_Curve) & theCurve)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject();
    }
    load(theCurve, theCurve->FirstParameter(), theCurve->LastParameter());
  }

  //! Loads the curve with parameter bounds.
  //! @param theCurve the curve to load
  //! @param theUFirst first parameter
  //! @param theULast last parameter
  //! @throw Standard_NullObject if theCurve is null
  //! @throw Standard_ConstructionError if theUFirst > theULast + Precision::Confusion()
  void Load(const Handle(Geom_Curve) & theCurve, double theUFirst, double theULast)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject();
    }
    if (theUFirst > theULast + Precision::Confusion())
    {
      throw Standard_ConstructionError();
    }
    load(theCurve, theUFirst, theULast);
  }

  //! Returns the underlying Geom_Curve.
  const Handle(Geom_Curve) & Curve() const { return myCurve; }

  //--- Modifier Management ---

  //! Check if this adaptor has a modifier set.
  [[nodiscard]] bool HasModifier() const { return !IsEmptyModifier(myModifier); }

  //! Returns the modifier variant.
  [[nodiscard]] const GeomAdaptor_CurveModifierVariant& Modifier() const { return myModifier; }

  //! Clear any modifier, making this a direct curve adaptor.
  void ClearModifier() { myModifier = std::monostate{}; }

  //! Set a transformation modifier.
  //! @param theTrsf the transformation to apply to all evaluation results
  void SetTransformation(const gp_Trsf& theTrsf)
  {
    myModifier = GeomAdaptor_TrsfModifier(theTrsf);
  }

  //! Set a curve-on-surface modifier.
  //! The adaptor takes ownership of both the PCurve and Surface adaptors.
  //! @param thePCurve 2D parametric curve adaptor (ownership transferred)
  //! @param theSurface 3D surface adaptor (ownership transferred)
  Standard_EXPORT void SetCurveOnSurface(std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
                                         std::unique_ptr<GeomAdaptor_Surface> theSurface);

  //! Set an iso-curve modifier.
  //! The adaptor takes ownership of the surface adaptor.
  //! @param theSurface surface adaptor (ownership transferred)
  //! @param theIsoType type of iso curve (IsoU or IsoV)
  //! @param theParam value of the fixed parameter
  Standard_EXPORT void SetIsoCurve(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                   GeomAbs_IsoType                      theIsoType,
                                   double                               theParam);

  //! Set an iso-curve modifier with explicit bounds.
  //! @param theSurface surface adaptor (ownership transferred)
  //! @param theIsoType type of iso curve (IsoU or IsoV)
  //! @param theParam value of the fixed parameter
  //! @param theFirst first parameter of the curve
  //! @param theLast last parameter of the curve
  Standard_EXPORT void SetIsoCurve(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                   GeomAbs_IsoType                      theIsoType,
                                   double                               theParam,
                                   double                               theFirst,
                                   double                               theLast);

  //--- Parameter Domain ---

  [[nodiscard]] Standard_Real FirstParameter() const Standard_OVERRIDE { return myFirst; }

  [[nodiscard]] Standard_Real LastParameter() const Standard_OVERRIDE { return myLast; }

  Standard_EXPORT GeomAbs_Shape Continuity() const Standard_OVERRIDE;

  //! Returns the number of intervals for continuity <S>.
  //! May be one if Continuity(me) >= <S>
  Standard_EXPORT Standard_Integer NbIntervals(GeomAbs_Shape S) const Standard_OVERRIDE;

  //! Stores in <T> the parameters bounding the intervals of continuity <S>.
  //! The array must provide enough room to accommodate for the parameters.
  //! i.e. T.Length() > NbIntervals()
  Standard_EXPORT void Intervals(TColStd_Array1OfReal& T, GeomAbs_Shape S) const Standard_OVERRIDE;

  //! Returns a curve equivalent of <me> between parameters <First> and <Last>.
  //! <Tol> is used to test for 3d points confusion.
  //! @return a handle to a new adaptor for the trimmed portion
  Standard_EXPORT virtual Handle(Adaptor3d_Curve) Trim(const Standard_Real First,
                                                       const Standard_Real Last,
                                                       const Standard_Real Tol) const Standard_OVERRIDE;

  //! Returns a trimmed curve by value.
  //! @return a new adaptor for the trimmed portion (by value, movable)
  [[nodiscard]] Standard_EXPORT GeomAdaptor_Curve TrimByValue(double First, double Last, double Tol) const;

  Standard_EXPORT Standard_Boolean IsClosed() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Boolean IsPeriodic() const Standard_OVERRIDE;

  Standard_EXPORT Standard_Real Period() const Standard_OVERRIDE;

  //--- Evaluation Methods ---

  //! Computes the point of parameter U.
  [[nodiscard]] gp_Pnt Value(const Standard_Real U) const Standard_OVERRIDE
  {
    gp_Pnt P;
    D0(U, P);
    return P;
  }

  //! Computes the point of parameter U.
  Standard_EXPORT void D0(const Standard_Real U, gp_Pnt& P) const Standard_OVERRIDE;

  //! Computes the point of parameter U on the curve with its first derivative.
  //! Warning: On the specific case of BSplineCurve:
  //! if the curve is cut in interval of continuity at least C1, the
  //! derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis curve.
  Standard_EXPORT void D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V) const Standard_OVERRIDE;

  //! Returns the point P of parameter U, the first and second derivatives V1 and V2.
  //! Warning: On the specific case of BSplineCurve:
  //! if the curve is cut in interval of continuity at least C2, the
  //! derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis curve.
  Standard_EXPORT void D2(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const Standard_OVERRIDE;

  //! Returns the point P of parameter U, the first, the second and the third derivative.
  //! Warning: On the specific case of BSplineCurve:
  //! if the curve is cut in interval of continuity at least C3, the
  //! derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis curve.
  Standard_EXPORT void D3(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2, gp_Vec& V3) const Standard_OVERRIDE;

  //! The returned vector gives the value of the derivative for the order of derivation N.
  //! Warning: On the specific case of BSplineCurve:
  //! if the curve is cut in interval of continuity CN, the
  //! derivatives are computed on the current interval.
  //! else the derivatives are computed on the basis curve.
  //! @throw Standard_OutOfRange if N < 1.
  [[nodiscard]] Standard_EXPORT gp_Vec DN(const Standard_Real U, const Standard_Integer N) const Standard_OVERRIDE;

  //! Returns the parametric resolution corresponding to the given space resolution R3D.
  Standard_EXPORT Standard_Real Resolution(const Standard_Real R3d) const Standard_OVERRIDE;

  //--- Curve Type ---

  [[nodiscard]] GeomAbs_CurveType GetType() const Standard_OVERRIDE { return myTypeCurve; }

  //--- Geometry Access (only valid when GetType() returns the corresponding type) ---

  Standard_EXPORT gp_Lin Line() const Standard_OVERRIDE;

  Standard_EXPORT gp_Circ Circle() const Standard_OVERRIDE;

  Standard_EXPORT gp_Elips Ellipse() const Standard_OVERRIDE;

  Standard_EXPORT gp_Hypr Hyperbola() const Standard_OVERRIDE;

  Standard_EXPORT gp_Parab Parabola() const Standard_OVERRIDE;

  //! This should NEVER make a copy of the underlying curve to read the relevant information.
  Standard_EXPORT Standard_Integer Degree() const Standard_OVERRIDE;

  //! This should NEVER make a copy of the underlying curve to read the relevant information.
  Standard_EXPORT Standard_Boolean IsRational() const Standard_OVERRIDE;

  //! This should NEVER make a copy of the underlying curve to read the relevant information.
  Standard_EXPORT Standard_Integer NbPoles() const Standard_OVERRIDE;

  //! This should NEVER make a copy of the underlying curve to read the relevant information.
  Standard_EXPORT Standard_Integer NbKnots() const Standard_OVERRIDE;

  //! This will NOT make a copy of the Bezier Curve:
  //! If you want to modify the Curve please make a copy yourself.
  //! Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT Handle(Geom_BezierCurve) Bezier() const Standard_OVERRIDE;

  //! This will NOT make a copy of the BSpline Curve:
  //! If you want to modify the Curve please make a copy yourself.
  //! Also it will NOT trim the surface to myFirst/Last.
  Standard_EXPORT Handle(Geom_BSplineCurve) BSpline() const Standard_OVERRIDE;

  Standard_EXPORT Handle(Geom_OffsetCurve) OffsetCurve() const Standard_OVERRIDE;

  friend class GeomAdaptor_Surface;

private:
  Standard_EXPORT GeomAbs_Shape LocalContinuity(double U1, double U2) const;

  Standard_EXPORT void load(const Handle(Geom_Curve) & C, double UFirst, double ULast);

  //! Check if theU relates to start or finish point of B-spline curve
  //! and return indices of span the point is located.
  bool IsBoundary(double theU, int& theSpanStart, int& theSpanFinish) const;

  //! Rebuilds B-spline cache.
  //! @param theParameter the value on the knot axis which identifies the caching span
  void RebuildCache(double theParameter) const;

  //! Evaluate base curve D0 (without modifier)
  void evaluateD0(double U, gp_Pnt& P) const;

  //! Evaluate base curve D1 (without modifier)
  void evaluateD1(double U, gp_Pnt& P, gp_Vec& V) const;

  //! Evaluate base curve D2 (without modifier)
  void evaluateD2(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const;

  //! Evaluate base curve D3 (without modifier)
  void evaluateD3(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2, gp_Vec& V3) const;

private:
  Handle(Geom_Curve)            myCurve;
  GeomAbs_CurveType             myTypeCurve;
  double                        myFirst;
  double                        myLast;
  GeomAdaptor_CurveModifierVariant myModifier;

  Handle(Geom_BSplineCurve)      myBSplineCurve;    //!< B-spline representation to prevent castings
  mutable Handle(BSplCLib_Cache) myCurveCache;      //!< Cached data for B-spline or Bezier curve
  Handle(GeomEvaluator_Curve)    myNestedEvaluator; //!< Calculates value of offset curve
};

#endif // _GeomAdaptor_Curve_HeaderFile
