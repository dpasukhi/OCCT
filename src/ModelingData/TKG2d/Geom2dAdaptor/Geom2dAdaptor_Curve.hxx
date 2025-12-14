// Created on: 1993-06-03
// Created by: Bruno DUMORTIER
// Copyright (c) 1993-1999 Matra Datavision
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

#ifndef _Geom2dAdaptor_Curve_HeaderFile
#define _Geom2dAdaptor_Curve_HeaderFile

#include <BSplCLib_Cache.hxx>
#include <Geom2d_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_Shape.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Pnt2d.hxx>
#include <Precision.hxx>
#include <Standard_NullObject.hxx>
#include <Standard_Transient.hxx>
#include <TColStd_Array1OfReal.hxx>

#include <gp_Circ2d.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Hypr2d.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Parab2d.hxx>

class gp_Vec2d;
class gp_Dir2d;
class Geom2d_BezierCurve;
class Geom2d_BSplineCurve;

DEFINE_STANDARD_HANDLE(Geom2dAdaptor_Curve, Standard_Transient)

//! An interface between the services provided by any
//! curve from the package Geom2d and those required
//! of the curve by algorithms which use it.
//!
//! This adaptor also supports:
//! - Direct Line2d representation (SetLine method) for UV domain boundaries
//! - Offset curve modifier (SetOffset method) for algorithmic offset curves
//!
//! Polynomial coefficients of BSpline curves used for their evaluation are
//! cached for better performance. Therefore these evaluations are not
//! thread-safe and parallel evaluations need to be prevented.
class Geom2dAdaptor_Curve : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(Geom2dAdaptor_Curve, Standard_Transient)
public:
  Standard_EXPORT Geom2dAdaptor_Curve();

  //! Copy constructor.
  Standard_EXPORT Geom2dAdaptor_Curve(const Geom2dAdaptor_Curve& theOther);

  //! Move constructor.
  Standard_EXPORT Geom2dAdaptor_Curve(Geom2dAdaptor_Curve&& theOther) noexcept;

  Standard_EXPORT Geom2dAdaptor_Curve(const Handle(Geom2d_Curve)& C);

  //! Standard_ConstructionError is raised if Ufirst>Ulast
  Standard_EXPORT Geom2dAdaptor_Curve(const Handle(Geom2d_Curve)& C,
                                      const Standard_Real         UFirst,
                                      const Standard_Real         ULast);

  //! Copy assignment operator.
  Standard_EXPORT Geom2dAdaptor_Curve& operator=(const Geom2dAdaptor_Curve& theOther);

  //! Move assignment operator.
  Standard_EXPORT Geom2dAdaptor_Curve& operator=(Geom2dAdaptor_Curve&& theOther) noexcept;

  //! Creates an explicit deep copy of the adaptor.
  [[nodiscard]] Standard_EXPORT Geom2dAdaptor_Curve Copy() const;

  //! Shallow copy of adaptor
  Standard_EXPORT Handle(Geom2dAdaptor_Curve) ShallowCopy() const;

  //! Reset currently loaded curve (undone Load()).
  Standard_EXPORT void Reset();

  void Load(const Handle(Geom2d_Curve)& theCurve)
  {
    if (theCurve.IsNull())
    {
      throw Standard_NullObject();
    }
    load(theCurve, theCurve->FirstParameter(), theCurve->LastParameter());
  }

  //! Standard_ConstructionError is raised if theUFirst > theULast + Precision::PConfusion()
  void Load(const Handle(Geom2d_Curve)& theCurve,
            const Standard_Real         theUFirst,
            const Standard_Real         theULast)
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

  //! Sets up the adaptor to represent a 2D line directly (without Geom2d_Line).
  //! This is useful for UV parameter space boundaries.
  //! @param theOrigin Line origin point
  //! @param theDirection Line direction
  //! @param theFirst First parameter value
  //! @param theLast Last parameter value
  Standard_EXPORT void SetLine(const gp_Pnt2d& theOrigin,
                               const gp_Dir2d& theDirection,
                               double          theFirst,
                               double          theLast);

  //! Sets up the adaptor to represent a 2D line from gp_Lin2d.
  //! @param theLine The 2D line
  //! @param theFirst First parameter value
  //! @param theLast Last parameter value
  Standard_EXPORT void SetLine(const gp_Lin2d& theLine, double theFirst, double theLast);

  //! Sets an offset modifier on the current curve.
  //! The adaptor will evaluate the base curve and apply the offset.
  //! @param theBaseCurve The base curve adaptor (will be copied)
  //! @param theOffset The offset distance (positive = left of curve direction)
  Standard_EXPORT void SetOffset(const Handle(Geom2dAdaptor_Curve)& theBaseCurve,
                                 double                             theOffset);

  //! Sets an offset modifier with parameter limits.
  //! @param theBaseCurve The base curve adaptor (will be copied)
  //! @param theOffset The offset distance
  //! @param theFirst First parameter value
  //! @param theLast Last parameter value
  Standard_EXPORT void SetOffset(const Handle(Geom2dAdaptor_Curve)& theBaseCurve,
                                 double                             theOffset,
                                 double                             theFirst,
                                 double                             theLast);

  //! Returns true if this adaptor represents a direct Line2d (not via Geom2d_Line).
  bool IsLine2d() const { return myIsLine2d; }

  //! Returns true if this adaptor has an offset modifier applied.
  bool HasOffset() const { return myHasOffset; }

  //! Returns the offset value (0.0 if no offset).
  double Offset() const { return myOffset; }

  //! Returns the base curve when offset is applied.
  //! Returns null handle if no offset is set.
  const Handle(Geom2dAdaptor_Curve)& BaseCurve() const { return myBaseCurve; }

  const Handle(Geom2d_Curve)& Curve() const { return myCurve; }

  Standard_Real FirstParameter() const { return myFirst; }

  Standard_Real LastParameter() const { return myLast; }

  Standard_EXPORT GeomAbs_Shape Continuity() const;

  //! If necessary, breaks the curve in intervals of
  //! continuity <S>. And returns the number of
  //! intervals.
  Standard_EXPORT Standard_Integer NbIntervals(const GeomAbs_Shape S) const;

  //! Stores in <T> the parameters bounding the intervals
  //! of continuity <S>.
  //!
  //! The array must provide enough room to accommodate
  //! for the parameters. i.e. T.Length() > NbIntervals()
  Standard_EXPORT void Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const;

  //! Returns a curve equivalent of <me> between
  //! parameters <First> and <Last>. <Tol> is used to
  //! test for 3d points confusion.
  //! If <First> >= <Last>
  Standard_EXPORT Handle(Geom2dAdaptor_Curve) Trim(const Standard_Real First,
                                                   const Standard_Real Last,
                                                   const Standard_Real Tol) const;

  //! Returns a trimmed curve by value.
  //! @param theFirst the first parameter
  //! @param theLast the last parameter
  //! @param theTol tolerance (unused)
  //! @return trimmed curve adaptor by value
  [[nodiscard]] Standard_EXPORT Geom2dAdaptor_Curve TrimByValue(double theFirst,
                                                                double theLast,
                                                                double theTol) const;

  Standard_EXPORT Standard_Boolean IsClosed() const;

  Standard_EXPORT Standard_Boolean IsPeriodic() const;

  Standard_EXPORT Standard_Real Period() const;

  //! Computes the point of parameter U on the curve
  Standard_EXPORT gp_Pnt2d Value(const Standard_Real U) const;

  //! Computes the point of parameter U.
  Standard_EXPORT void D0(const Standard_Real U, gp_Pnt2d& P) const;

  //! Computes the point of parameter U on the curve with its
  //! first derivative.
  //! Raised if the continuity of the current interval
  //! is not C1.
  Standard_EXPORT void D1(const Standard_Real U, gp_Pnt2d& P, gp_Vec2d& V) const;

  //! Returns the point P of parameter U, the first and second
  //! derivatives V1 and V2.
  //! Raised if the continuity of the current interval
  //! is not C2.
  Standard_EXPORT void D2(const Standard_Real U,
                          gp_Pnt2d&           P,
                          gp_Vec2d&           V1,
                          gp_Vec2d&           V2) const;

  //! Returns the point P of parameter U, the first, the second
  //! and the third derivative.
  //! Raised if the continuity of the current interval
  //! is not C3.
  Standard_EXPORT void D3(const Standard_Real U,
                          gp_Pnt2d&           P,
                          gp_Vec2d&           V1,
                          gp_Vec2d&           V2,
                          gp_Vec2d&           V3) const;

  //! The returned vector gives the value of the derivative for the
  //! order of derivation N.
  //! Raised if the continuity of the current interval
  //! is not CN.
  //! Raised if N < 1.
  Standard_EXPORT gp_Vec2d DN(const Standard_Real U, const Standard_Integer N) const;

  //! returns the parametric resolution
  Standard_EXPORT Standard_Real Resolution(const Standard_Real Ruv) const;

  GeomAbs_CurveType GetType() const
  {
    if (myIsLine2d)
      return GeomAbs_Line;
    if (myHasOffset && myOffset != 0.0)
      return getOffsetType();
    return myTypeCurve;
  }

  Standard_EXPORT gp_Lin2d Line() const;

  Standard_EXPORT gp_Circ2d Circle() const;

  Standard_EXPORT gp_Elips2d Ellipse() const;

  Standard_EXPORT gp_Hypr2d Hyperbola() const;

  Standard_EXPORT gp_Parab2d Parabola() const;

  Standard_EXPORT Standard_Integer Degree() const;

  Standard_EXPORT Standard_Boolean IsRational() const;

  Standard_EXPORT Standard_Integer NbPoles() const;

  Standard_EXPORT Standard_Integer NbKnots() const;

  Standard_EXPORT Standard_Integer NbSamples() const;

  Standard_EXPORT Handle(Geom2d_BezierCurve) Bezier() const;

  Standard_EXPORT Handle(Geom2d_BSplineCurve) BSpline() const;

private:
  Standard_EXPORT GeomAbs_Shape LocalContinuity(const Standard_Real U1,
                                                const Standard_Real U2) const;

  Standard_EXPORT void load(const Handle(Geom2d_Curve)& C,
                            const Standard_Real         UFirst,
                            const Standard_Real         ULast);

  //! Check theU relates to start or finish point of B-spline curve and return indices of span the
  //! point is located
  Standard_Boolean IsBoundary(const Standard_Real theU,
                              Standard_Integer&   theSpanStart,
                              Standard_Integer&   theSpanFinish) const;

  //! Rebuilds B-spline cache
  //! \param theParameter the value on the knot axis which identifies the caching span
  void RebuildCache(const Standard_Real theParameter) const;

  //! Returns the curve type for offset curve based on base curve type
  GeomAbs_CurveType getOffsetType() const;

  //! Evaluates Line2d at parameter U
  void evaluateLine2d(double theU, gp_Pnt2d& theP) const;

  //! Evaluates Line2d D1 at parameter U
  void evaluateLine2dD1(double theU, gp_Pnt2d& theP, gp_Vec2d& theV) const;

  //! Evaluates offset curve at parameter U
  void evaluateOffset(double theU, gp_Pnt2d& theP) const;

  //! Evaluates offset curve D1 at parameter U
  void evaluateOffsetD1(double theU, gp_Pnt2d& theP, gp_Vec2d& theV) const;

  //! Evaluates offset curve D2 at parameter U
  void evaluateOffsetD2(double theU, gp_Pnt2d& theP, gp_Vec2d& theV1, gp_Vec2d& theV2) const;

  //! Evaluates offset curve D3 at parameter U
  void evaluateOffsetD3(double    theU,
                        gp_Pnt2d& theP,
                        gp_Vec2d& theV1,
                        gp_Vec2d& theV2,
                        gp_Vec2d& theV3) const;

protected:
  Handle(Geom2d_Curve) myCurve;
  GeomAbs_CurveType    myTypeCurve;
  Standard_Real        myFirst;
  Standard_Real        myLast;

  Handle(Geom2d_BSplineCurve)    myBSplineCurve; ///< B-spline representation to prevent castings
  mutable Handle(BSplCLib_Cache) myCurveCache;   ///< Cached data for B-spline or Bezier curve

  // Line2d support (for UV domain boundaries without Geom2d_Line)
  bool     myIsLine2d; ///< True if adaptor represents a direct Line2d
  gp_Ax2d  myLineAxis; ///< Line axis when myIsLine2d is true

  // Offset curve support
  bool                        myHasOffset;  ///< True if offset modifier is applied
  double                      myOffset;     ///< Offset value
  Handle(Geom2dAdaptor_Curve) myBaseCurve;  ///< Base curve for offset (when myHasOffset is true)
};

#endif // _Geom2dAdaptor_Curve_HeaderFile
