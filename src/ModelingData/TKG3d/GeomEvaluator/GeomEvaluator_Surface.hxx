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

#ifndef _GeomEvaluator_Surface_HeaderFile
#define _GeomEvaluator_Surface_HeaderFile

#include <Standard_Transient.hxx>
#include <Standard_Type.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <optional>

//! Interface for calculation of values and derivatives for different kinds of surfaces.
//! Works both with adaptors and surfaces.
//! All methods return std::optional to properly handle calculation failures.
class GeomEvaluator_Surface : public Standard_Transient
{
public:
  //! Result structure for D1 evaluation - point and first derivatives
  struct D1Result
  {
    gp_Pnt theValue;
    gp_Vec theD1U;
    gp_Vec theD1V;
  };

  //! Result structure for D2 evaluation - point, first and second derivatives
  struct D2Result
  {
    gp_Pnt theValue;
    gp_Vec theD1U;
    gp_Vec theD1V;
    gp_Vec theD2U;
    gp_Vec theD2V;
    gp_Vec theD2UV;
  };

  //! Result structure for D3 evaluation - point, first, second and third derivatives
  struct D3Result
  {
    gp_Pnt theValue;
    gp_Vec theD1U;
    gp_Vec theD1V;
    gp_Vec theD2U;
    gp_Vec theD2V;
    gp_Vec theD2UV;
    gp_Vec theD3U;
    gp_Vec theD3V;
    gp_Vec theD3UUV;
    gp_Vec theD3UVV;
  };

  GeomEvaluator_Surface() {}

  //! Value of surface
  //! @return Point value if calculation succeeds, std::nullopt otherwise
  virtual std::optional<gp_Pnt> D0(const Standard_Real theU, const Standard_Real theV) const = 0;

  //! Value and first derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  virtual std::optional<D1Result> D1(const Standard_Real theU, const Standard_Real theV) const = 0;

  //! Value, first and second derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  virtual std::optional<D2Result> D2(const Standard_Real theU, const Standard_Real theV) const = 0;

  //! Value, first, second and third derivatives of surface
  //! @return Result structure with point and derivatives if calculation succeeds, std::nullopt otherwise
  virtual std::optional<D3Result> D3(const Standard_Real theU, const Standard_Real theV) const = 0;

  //! Calculates N-th derivatives of surface, where N = theDerU + theDerV.
  //!
  //! Raises if N < 1 or theDerU < 0 or theDerV < 0
  //! @return Derivative vector if calculation succeeds, std::nullopt otherwise
  virtual std::optional<gp_Vec> DN(const Standard_Real    theU,
                                    const Standard_Real    theV,
                                    const Standard_Integer theDerU,
                                    const Standard_Integer theDerV) const = 0;

  virtual Handle(GeomEvaluator_Surface) ShallowCopy() const = 0;

  DEFINE_STANDARD_RTTI_INLINE(GeomEvaluator_Surface, Standard_Transient)
};

DEFINE_STANDARD_HANDLE(GeomEvaluator_Surface, Standard_Transient)

#endif // _GeomEvaluator_Surface_HeaderFile
