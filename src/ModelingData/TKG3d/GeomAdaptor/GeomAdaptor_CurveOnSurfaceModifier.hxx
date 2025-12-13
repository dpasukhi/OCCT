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

#ifndef GeomAdaptor_CurveOnSurfaceModifier_HeaderFile
#define GeomAdaptor_CurveOnSurfaceModifier_HeaderFile

#include <GeomAbs_CurveType.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <memory>

// Forward declarations
class Geom2dAdaptor_Curve;
class GeomAdaptor_Surface;

//! Modifier that evaluates a 2D parametric curve on a 3D surface.
//!
//! This class implements the curve-on-surface pattern where a 2D curve
//! in the surface's parametric (U,V) space is lifted to 3D by evaluating
//! the surface at the curve's (U,V) coordinates.
//!
//! The D0/D1/D2/D3 methods use the chain rule to compute derivatives:
//! - D1: V = dU * D1U + dV * D1V
//! - D2: V = d2U * D1U + d2V * D1V + dU^2 * D2U + 2*dU*dV * D2UV + dV^2 * D2V
//!
//! Special cases are detected for analytic results:
//! - 2D line on plane -> 3D line
//! - 2D circle on plane -> 3D circle
//! - 2D line parallel to U/V on quadric surfaces -> 3D circle or line
//!
//! This class is move-only (owns nested adaptors via unique_ptr).
class GeomAdaptor_CurveOnSurfaceModifier
{
public:
  //! Default constructor - creates empty modifier.
  Standard_EXPORT GeomAdaptor_CurveOnSurfaceModifier();

  //! Constructor from 2D curve and surface.
  //! @param thePCurve   2D parametric curve in surface parameter space (ownership taken)
  //! @param theSurface  3D surface (ownership taken)
  Standard_EXPORT GeomAdaptor_CurveOnSurfaceModifier(
    std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
    std::unique_ptr<GeomAdaptor_Surface> theSurface);

  //! Move constructor.
  Standard_EXPORT GeomAdaptor_CurveOnSurfaceModifier(
    GeomAdaptor_CurveOnSurfaceModifier&& theOther) noexcept;

  //! Move assignment.
  Standard_EXPORT GeomAdaptor_CurveOnSurfaceModifier& operator=(
    GeomAdaptor_CurveOnSurfaceModifier&& theOther) noexcept;

  //! Copy construction is deleted - use Copy() instead.
  GeomAdaptor_CurveOnSurfaceModifier(const GeomAdaptor_CurveOnSurfaceModifier&) = delete;
  GeomAdaptor_CurveOnSurfaceModifier& operator=(const GeomAdaptor_CurveOnSurfaceModifier&) = delete;

  //! Create an explicit deep copy.
  [[nodiscard]] Standard_EXPORT GeomAdaptor_CurveOnSurfaceModifier Copy() const;

  //! Destructor.
  Standard_EXPORT ~GeomAdaptor_CurveOnSurfaceModifier();

  //! Load new curve and surface.
  //! @param thePCurve   2D parametric curve (ownership taken)
  //! @param theSurface  3D surface (ownership taken)
  Standard_EXPORT void Load(std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
                            std::unique_ptr<GeomAdaptor_Surface> theSurface);

  //! Check if modifier is properly initialized.
  [[nodiscard]] bool IsValid() const { return myPCurve != nullptr && mySurface != nullptr; }

  //--- Accessors ---

  //! Returns the 2D curve.
  [[nodiscard]] const Geom2dAdaptor_Curve* PCurve() const { return myPCurve.get(); }

  //! Returns the surface.
  [[nodiscard]] const GeomAdaptor_Surface* Surface() const { return mySurface.get(); }

  //! Returns the detected analytic curve type.
  [[nodiscard]] GeomAbs_CurveType GetType() const { return myType; }

  //--- Parameter Domain (from PCurve) ---

  [[nodiscard]] Standard_EXPORT double FirstParameter() const;
  [[nodiscard]] Standard_EXPORT double LastParameter() const;

  //--- Evaluation Methods ---

  //! Computes the 3D point at parameter U.
  //! @param theU  curve parameter
  //! @param theP  output 3D point
  Standard_EXPORT void D0(double theU, gp_Pnt& theP) const;

  //! Computes the 3D point and first derivative at parameter U.
  //! @param theU  curve parameter
  //! @param theP  output 3D point
  //! @param theV  output first derivative (tangent vector)
  Standard_EXPORT void D1(double theU, gp_Pnt& theP, gp_Vec& theV) const;

  //! Computes the 3D point, first and second derivatives at parameter U.
  //! @param theU   curve parameter
  //! @param theP   output 3D point
  //! @param theV1  output first derivative
  //! @param theV2  output second derivative
  Standard_EXPORT void D2(double theU, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const;

  //! Computes the 3D point and first three derivatives at parameter U.
  //! @param theU   curve parameter
  //! @param theP   output 3D point
  //! @param theV1  output first derivative
  //! @param theV2  output second derivative
  //! @param theV3  output third derivative
  Standard_EXPORT void D3(double theU, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2, gp_Vec& theV3) const;

  //! Computes the Nth derivative at parameter U.
  //! @param theU  curve parameter
  //! @param theN  derivative order (1, 2, or 3)
  //! @return Nth derivative vector
  [[nodiscard]] Standard_EXPORT gp_Vec DN(double theU, int theN) const;

  //--- Analytic Geometry Access ---

  //! Returns the analytic line if GetType() == GeomAbs_Line.
  //! @return 3D line
  [[nodiscard]] const gp_Lin& Line() const { return myLin; }

  //! Returns the analytic circle if GetType() == GeomAbs_Circle.
  //! @return 3D circle
  [[nodiscard]] const gp_Circ& Circle() const { return myCirc; }

private:
  //! Detect analytic curve type from PCurve and Surface combination.
  void evalKPart();

private:
  std::unique_ptr<Geom2dAdaptor_Curve> myPCurve;  //!< 2D parametric curve
  std::unique_ptr<GeomAdaptor_Surface> mySurface; //!< 3D surface

  // Cached analytic type detection
  GeomAbs_CurveType myType; //!< Detected curve type
  gp_Lin            myLin;  //!< Cached line (if type is Line)
  gp_Circ           myCirc; //!< Cached circle (if type is Circle)
};

#endif // GeomAdaptor_CurveOnSurfaceModifier_HeaderFile
