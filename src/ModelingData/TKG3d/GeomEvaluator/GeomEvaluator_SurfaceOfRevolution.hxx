// Created on: 2015-09-21
// Copyright (c) 2015 OPEN CASCADE SAS
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

#ifndef _GeomEvaluator_SurfaceOfRevolution_HeaderFile
#define _GeomEvaluator_SurfaceOfRevolution_HeaderFile

#include <Adaptor3d_Curve.hxx>
#include <GeomEvaluator_Surface.hxx>
#include <Geom_Curve.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax1.hxx>

//! Allows to calculate values and derivatives for surfaces of revolution
class GeomEvaluator_SurfaceOfRevolution : public GeomEvaluator_Surface
{
public:
  //! Initialize evaluator by revolved curve, the axis of revolution and the location
  Standard_EXPORT GeomEvaluator_SurfaceOfRevolution(const Handle(Geom_Curve)& theBase,
                                                    const gp_Dir&             theRevolDir,
                                                    const gp_Pnt&             theRevolLoc);
  //! Initialize evaluator by adaptor of the revolved curve, the axis of revolution and the location
  Standard_EXPORT GeomEvaluator_SurfaceOfRevolution(const Handle(Adaptor3d_Curve)& theBase,
                                                    const gp_Dir&                  theRevolDir,
                                                    const gp_Pnt&                  theRevolLoc);

  //! Change direction of the axis of revolution
  void SetDirection(const gp_Dir& theDirection) { myRotAxis.SetDirection(theDirection); }

  //! Change location of the axis of revolution
  void SetLocation(const gp_Pnt& theLocation) { myRotAxis.SetLocation(theLocation); }

  //! Change the axis of revolution
  void SetAxis(const gp_Ax1& theAxis) { myRotAxis = theAxis; }

  //! Value of surface
  //! @return Point value if calculation succeeds, std::nullopt otherwise
  Standard_EXPORT std::optional<gp_Pnt> D0(const Standard_Real theU,
                                            const Standard_Real theV) const Standard_OVERRIDE;
  //! Value and first derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  Standard_EXPORT std::optional<D1Result> D1(const Standard_Real theU,
                                              const Standard_Real theV) const Standard_OVERRIDE;
  //! Value, first and second derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  Standard_EXPORT std::optional<D2Result> D2(const Standard_Real theU,
                                              const Standard_Real theV) const Standard_OVERRIDE;
  //! Value, first, second and third derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  Standard_EXPORT std::optional<D3Result> D3(const Standard_Real theU,
                                              const Standard_Real theV) const Standard_OVERRIDE;
  //! Calculates N-th derivatives of surface, where N = theDerU + theDerV.
  //!
  //! Raises if N < 1 or theDerU < 0 or theDerV < 0
  //! @return Derivative vector if calculation succeeds, std::nullopt otherwise
  Standard_EXPORT std::optional<gp_Vec> DN(const Standard_Real    theU,
                                            const Standard_Real    theV,
                                            const Standard_Integer theDerU,
                                            const Standard_Integer theDerV) const Standard_OVERRIDE;

  Standard_EXPORT Handle(GeomEvaluator_Surface) ShallowCopy() const Standard_OVERRIDE;

  DEFINE_STANDARD_RTTIEXT(GeomEvaluator_SurfaceOfRevolution, GeomEvaluator_Surface)

private:
  Handle(Geom_Curve)      myBaseCurve;
  Handle(Adaptor3d_Curve) myBaseAdaptor;
  gp_Ax1                  myRotAxis;
};

DEFINE_STANDARD_HANDLE(GeomEvaluator_SurfaceOfRevolution, GeomEvaluator_Surface)

#endif // _GeomEvaluator_SurfaceOfRevolution_HeaderFile
