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

#ifndef GeomAdaptor_IsoCurveModifier_HeaderFile
#define GeomAdaptor_IsoCurveModifier_HeaderFile

#include <GeomAbs_IsoType.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <memory>

// Forward declarations
class GeomAdaptor_Surface;

//! Modifier that evaluates an isoparametric curve on a surface.
//!
//! An isoparametric curve is a curve on a surface where one of the
//! surface parameters (U or V) is held constant:
//! - IsoU curve: P(T) = Surface(U=const, V=T) - V varies along the curve
//! - IsoV curve: P(T) = Surface(U=T, V=const) - U varies along the curve
//!
//! The derivatives are extracted from the surface's partial derivatives
//! along the varying parameter direction.
//!
//! This class is move-only (owns surface adaptor via unique_ptr).
class GeomAdaptor_IsoCurveModifier
{
public:
  //! Default constructor - creates empty modifier.
  Standard_EXPORT GeomAdaptor_IsoCurveModifier();

  //! Constructor with surface, iso type, and iso parameter value.
  //! Parameter bounds are taken from surface.
  //! @param theSurface  3D surface (ownership taken)
  //! @param theIsoType  type of iso curve (IsoU or IsoV)
  //! @param theParam    value of the fixed parameter
  Standard_EXPORT GeomAdaptor_IsoCurveModifier(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                               GeomAbs_IsoType                      theIsoType,
                                               double                               theParam);

  //! Constructor with surface, iso type, iso parameter, and explicit bounds.
  //! @param theSurface  3D surface (ownership taken)
  //! @param theIsoType  type of iso curve (IsoU or IsoV)
  //! @param theParam    value of the fixed parameter
  //! @param theFirst    first parameter of the curve
  //! @param theLast     last parameter of the curve
  Standard_EXPORT GeomAdaptor_IsoCurveModifier(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                               GeomAbs_IsoType                      theIsoType,
                                               double                               theParam,
                                               double                               theFirst,
                                               double                               theLast);

  //! Move constructor.
  Standard_EXPORT GeomAdaptor_IsoCurveModifier(GeomAdaptor_IsoCurveModifier&& theOther) noexcept;

  //! Move assignment.
  Standard_EXPORT GeomAdaptor_IsoCurveModifier& operator=(
    GeomAdaptor_IsoCurveModifier&& theOther) noexcept;

  //! Copy construction is deleted - use Copy() instead.
  GeomAdaptor_IsoCurveModifier(const GeomAdaptor_IsoCurveModifier&) = delete;
  GeomAdaptor_IsoCurveModifier& operator=(const GeomAdaptor_IsoCurveModifier&) = delete;

  //! Create an explicit deep copy.
  [[nodiscard]] Standard_EXPORT GeomAdaptor_IsoCurveModifier Copy() const;

  //! Destructor.
  Standard_EXPORT ~GeomAdaptor_IsoCurveModifier();

  //! Load surface (resets iso to NoneIso).
  //! @param theSurface  3D surface (ownership taken)
  Standard_EXPORT void Load(std::unique_ptr<GeomAdaptor_Surface> theSurface);

  //! Load iso curve definition. Surface must be loaded first.
  //! @param theIsoType  type of iso curve (IsoU or IsoV)
  //! @param theParam    value of the fixed parameter
  Standard_EXPORT void Load(GeomAbs_IsoType theIsoType, double theParam);

  //! Load iso curve definition with explicit bounds.
  //! @param theIsoType  type of iso curve (IsoU or IsoV)
  //! @param theParam    value of the fixed parameter
  //! @param theFirst    first parameter of the curve
  //! @param theLast     last parameter of the curve
  Standard_EXPORT void Load(GeomAbs_IsoType theIsoType,
                            double          theParam,
                            double          theFirst,
                            double          theLast);

  //! Check if modifier is properly initialized.
  [[nodiscard]] bool IsValid() const
  {
    return mySurface != nullptr && myIsoType != GeomAbs_NoneIso;
  }

  //--- Accessors ---

  //! Returns the surface.
  [[nodiscard]] const GeomAdaptor_Surface* Surface() const { return mySurface.get(); }

  //! Returns the iso type.
  [[nodiscard]] GeomAbs_IsoType IsoType() const { return myIsoType; }

  //! Returns the fixed parameter value.
  [[nodiscard]] double Parameter() const { return myParameter; }

  //! Returns the detected curve type based on surface type and iso direction.
  [[nodiscard]] Standard_EXPORT GeomAbs_CurveType GetType() const;

  //--- Parameter Domain ---

  [[nodiscard]] double FirstParameter() const { return myFirst; }
  [[nodiscard]] double LastParameter() const { return myLast; }

  //--- Evaluation Methods ---

  //! Computes the 3D point at parameter T.
  //! @param theT  curve parameter
  //! @param theP  output 3D point
  Standard_EXPORT void D0(double theT, gp_Pnt& theP) const;

  //! Computes the 3D point and first derivative at parameter T.
  //! @param theT  curve parameter
  //! @param theP  output 3D point
  //! @param theV  output first derivative (tangent vector)
  Standard_EXPORT void D1(double theT, gp_Pnt& theP, gp_Vec& theV) const;

  //! Computes the 3D point, first and second derivatives at parameter T.
  //! @param theT   curve parameter
  //! @param theP   output 3D point
  //! @param theV1  output first derivative
  //! @param theV2  output second derivative
  Standard_EXPORT void D2(double theT, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const;

  //! Computes the 3D point and first three derivatives at parameter T.
  //! @param theT   curve parameter
  //! @param theP   output 3D point
  //! @param theV1  output first derivative
  //! @param theV2  output second derivative
  //! @param theV3  output third derivative
  Standard_EXPORT void D3(double theT, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2, gp_Vec& theV3) const;

  //! Computes the Nth derivative at parameter T.
  //! @param theT  curve parameter
  //! @param theN  derivative order
  //! @return Nth derivative vector
  [[nodiscard]] Standard_EXPORT gp_Vec DN(double theT, int theN) const;

private:
  std::unique_ptr<GeomAdaptor_Surface> mySurface;   //!< The surface
  GeomAbs_IsoType                      myIsoType;   //!< Type of iso (U or V)
  double                               myParameter; //!< Fixed parameter value
  double                               myFirst;     //!< First curve parameter
  double                               myLast;      //!< Last curve parameter
};

#endif // GeomAdaptor_IsoCurveModifier_HeaderFile
