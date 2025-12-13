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

#ifndef GeomAdaptor_TrsfModifier_HeaderFile
#define GeomAdaptor_TrsfModifier_HeaderFile

#include <gp_Trsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>
#include <gp_Vec2d.hxx>
#include <gp_Lin.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Circ.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Elips.hxx>
#include <gp_Elips2d.hxx>
#include <gp_Hypr.hxx>
#include <gp_Hypr2d.hxx>
#include <gp_Parab.hxx>
#include <gp_Parab2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Cone.hxx>
#include <gp_Sphere.hxx>
#include <gp_Torus.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Trsf2d.hxx>

//! Modifier that applies gp_Trsf transformation to curve/surface evaluation results.
//! This class is used by BRepAdaptor to handle TopoDS location transformations.
//!
//! The modifier is lightweight, copyable and movable. It stores only the transformation
//! and provides methods to transform geometric primitives (points, vectors, curves, surfaces).
class GeomAdaptor_TrsfModifier
{
public:
  //! Default constructor - creates identity transformation.
  GeomAdaptor_TrsfModifier() : myTrsf() {}

  //! Constructor from transformation.
  //! @param theTrsf the transformation to apply
  explicit GeomAdaptor_TrsfModifier(const gp_Trsf& theTrsf) : myTrsf(theTrsf) {}

  //! Copy constructor.
  GeomAdaptor_TrsfModifier(const GeomAdaptor_TrsfModifier& theOther) = default;

  //! Move constructor.
  GeomAdaptor_TrsfModifier(GeomAdaptor_TrsfModifier&& theOther) noexcept = default;

  //! Copy assignment.
  GeomAdaptor_TrsfModifier& operator=(const GeomAdaptor_TrsfModifier& theOther) = default;

  //! Move assignment.
  GeomAdaptor_TrsfModifier& operator=(GeomAdaptor_TrsfModifier&& theOther) noexcept = default;

  //! Destructor.
  ~GeomAdaptor_TrsfModifier() = default;

  //! Returns the stored transformation.
  const gp_Trsf& Transformation() const { return myTrsf; }

  //! Returns the stored transformation (non-const).
  gp_Trsf& ChangeTransformation() { return myTrsf; }

  //! Sets the transformation.
  //! @param theTrsf the transformation to set
  void SetTransformation(const gp_Trsf& theTrsf) { myTrsf = theTrsf; }

  //! Returns true if the transformation is identity.
  bool IsIdentity() const { return myTrsf.Form() == gp_Identity; }

  //--- Point Transformation ---

  //! Transforms a 3D point in place.
  //! @param theP the point to transform
  void Transform(gp_Pnt& theP) const { theP.Transform(myTrsf); }

  //! Returns a transformed copy of a 3D point.
  //! @param theP the point to transform
  //! @return transformed point
  [[nodiscard]] gp_Pnt Transformed(const gp_Pnt& theP) const { return theP.Transformed(myTrsf); }

  //--- Vector Transformation ---

  //! Transforms a 3D vector in place.
  //! @param theV the vector to transform
  void Transform(gp_Vec& theV) const { theV.Transform(myTrsf); }

  //! Returns a transformed copy of a 3D vector.
  //! @param theV the vector to transform
  //! @return transformed vector
  [[nodiscard]] gp_Vec Transformed(const gp_Vec& theV) const { return theV.Transformed(myTrsf); }

  //--- Combined Point + Vector Transformation ---

  //! Transforms point and one derivative vector (D1 case).
  //! @param theP  point to transform
  //! @param theV  first derivative vector to transform
  void Transform(gp_Pnt& theP, gp_Vec& theV) const
  {
    theP.Transform(myTrsf);
    theV.Transform(myTrsf);
  }

  //! Transforms point and two derivative vectors (D2 case).
  //! @param theP   point to transform
  //! @param theV1  first derivative vector to transform
  //! @param theV2  second derivative vector to transform
  void Transform(gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const
  {
    theP.Transform(myTrsf);
    theV1.Transform(myTrsf);
    theV2.Transform(myTrsf);
  }

  //! Transforms point and three derivative vectors (D3 case).
  //! @param theP   point to transform
  //! @param theV1  first derivative vector to transform
  //! @param theV2  second derivative vector to transform
  //! @param theV3  third derivative vector to transform
  void Transform(gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2, gp_Vec& theV3) const
  {
    theP.Transform(myTrsf);
    theV1.Transform(myTrsf);
    theV2.Transform(myTrsf);
    theV3.Transform(myTrsf);
  }

  //--- Surface D1/D2/D3 Transformation (with U and V derivatives) ---

  //! Transforms surface D1 results (point + D1U + D1V).
  //! @param theP    point to transform
  //! @param theD1U  partial derivative in U direction
  //! @param theD1V  partial derivative in V direction
  void TransformD1(gp_Pnt& theP, gp_Vec& theD1U, gp_Vec& theD1V) const
  {
    theP.Transform(myTrsf);
    theD1U.Transform(myTrsf);
    theD1V.Transform(myTrsf);
  }

  //! Transforms surface D2 results.
  //! @param theP    point to transform
  //! @param theD1U  first derivative in U direction
  //! @param theD1V  first derivative in V direction
  //! @param theD2U  second derivative in U direction
  //! @param theD2V  second derivative in V direction
  //! @param theD2UV mixed second derivative
  void TransformD2(gp_Pnt& theP,
                   gp_Vec& theD1U,
                   gp_Vec& theD1V,
                   gp_Vec& theD2U,
                   gp_Vec& theD2V,
                   gp_Vec& theD2UV) const
  {
    theP.Transform(myTrsf);
    theD1U.Transform(myTrsf);
    theD1V.Transform(myTrsf);
    theD2U.Transform(myTrsf);
    theD2V.Transform(myTrsf);
    theD2UV.Transform(myTrsf);
  }

  //! Transforms surface D3 results.
  //! @param theP     point to transform
  //! @param theD1U   first derivative in U direction
  //! @param theD1V   first derivative in V direction
  //! @param theD2U   second derivative in U direction
  //! @param theD2V   second derivative in V direction
  //! @param theD2UV  mixed second derivative
  //! @param theD3U   third derivative in U direction
  //! @param theD3V   third derivative in V direction
  //! @param theD3UUV mixed third derivative (UUV)
  //! @param theD3UVV mixed third derivative (UVV)
  void TransformD3(gp_Pnt& theP,
                   gp_Vec& theD1U,
                   gp_Vec& theD1V,
                   gp_Vec& theD2U,
                   gp_Vec& theD2V,
                   gp_Vec& theD2UV,
                   gp_Vec& theD3U,
                   gp_Vec& theD3V,
                   gp_Vec& theD3UUV,
                   gp_Vec& theD3UVV) const
  {
    theP.Transform(myTrsf);
    theD1U.Transform(myTrsf);
    theD1V.Transform(myTrsf);
    theD2U.Transform(myTrsf);
    theD2V.Transform(myTrsf);
    theD2UV.Transform(myTrsf);
    theD3U.Transform(myTrsf);
    theD3V.Transform(myTrsf);
    theD3UUV.Transform(myTrsf);
    theD3UVV.Transform(myTrsf);
  }

  //--- Curve Primitive Transformation ---

  //! Returns a transformed copy of a line.
  //! @param theLin the line to transform
  //! @return transformed line
  [[nodiscard]] gp_Lin Transformed(const gp_Lin& theLin) const { return theLin.Transformed(myTrsf); }

  //! Returns a transformed copy of a circle.
  //! @param theCirc the circle to transform
  //! @return transformed circle
  [[nodiscard]] gp_Circ Transformed(const gp_Circ& theCirc) const
  {
    return theCirc.Transformed(myTrsf);
  }

  //! Returns a transformed copy of an ellipse.
  //! @param theElips the ellipse to transform
  //! @return transformed ellipse
  [[nodiscard]] gp_Elips Transformed(const gp_Elips& theElips) const
  {
    return theElips.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a hyperbola.
  //! @param theHypr the hyperbola to transform
  //! @return transformed hyperbola
  [[nodiscard]] gp_Hypr Transformed(const gp_Hypr& theHypr) const
  {
    return theHypr.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a parabola.
  //! @param theParab the parabola to transform
  //! @return transformed parabola
  [[nodiscard]] gp_Parab Transformed(const gp_Parab& theParab) const
  {
    return theParab.Transformed(myTrsf);
  }

  //--- Surface Primitive Transformation ---

  //! Returns a transformed copy of a plane.
  //! @param thePln the plane to transform
  //! @return transformed plane
  [[nodiscard]] gp_Pln Transformed(const gp_Pln& thePln) const { return thePln.Transformed(myTrsf); }

  //! Returns a transformed copy of a cylinder.
  //! @param theCyl the cylinder to transform
  //! @return transformed cylinder
  [[nodiscard]] gp_Cylinder Transformed(const gp_Cylinder& theCyl) const
  {
    return theCyl.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a cone.
  //! @param theCone the cone to transform
  //! @return transformed cone
  [[nodiscard]] gp_Cone Transformed(const gp_Cone& theCone) const
  {
    return theCone.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a sphere.
  //! @param theSphere the sphere to transform
  //! @return transformed sphere
  [[nodiscard]] gp_Sphere Transformed(const gp_Sphere& theSphere) const
  {
    return theSphere.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a torus.
  //! @param theTorus the torus to transform
  //! @return transformed torus
  [[nodiscard]] gp_Torus Transformed(const gp_Torus& theTorus) const
  {
    return theTorus.Transformed(myTrsf);
  }

  //--- Axis/Direction Transformation ---

  //! Returns a transformed copy of an axis.
  //! @param theAx1 the axis to transform
  //! @return transformed axis
  [[nodiscard]] gp_Ax1 Transformed(const gp_Ax1& theAx1) const { return theAx1.Transformed(myTrsf); }

  //! Returns a transformed copy of a direction.
  //! @param theDir the direction to transform
  //! @return transformed direction
  [[nodiscard]] gp_Dir Transformed(const gp_Dir& theDir) const { return theDir.Transformed(myTrsf); }

private:
  gp_Trsf myTrsf; //!< The stored transformation
};

//! 2D version of the transformation modifier.
//! Applies gp_Trsf2d transformation to 2D curve evaluation results.
class GeomAdaptor_Trsf2dModifier
{
public:
  //! Default constructor - creates identity transformation.
  GeomAdaptor_Trsf2dModifier() : myTrsf() {}

  //! Constructor from 2D transformation.
  //! @param theTrsf the 2D transformation to apply
  explicit GeomAdaptor_Trsf2dModifier(const gp_Trsf2d& theTrsf) : myTrsf(theTrsf) {}

  //! Copy constructor.
  GeomAdaptor_Trsf2dModifier(const GeomAdaptor_Trsf2dModifier& theOther) = default;

  //! Move constructor.
  GeomAdaptor_Trsf2dModifier(GeomAdaptor_Trsf2dModifier&& theOther) noexcept = default;

  //! Copy assignment.
  GeomAdaptor_Trsf2dModifier& operator=(const GeomAdaptor_Trsf2dModifier& theOther) = default;

  //! Move assignment.
  GeomAdaptor_Trsf2dModifier& operator=(GeomAdaptor_Trsf2dModifier&& theOther) noexcept = default;

  //! Destructor.
  ~GeomAdaptor_Trsf2dModifier() = default;

  //! Returns the stored 2D transformation.
  const gp_Trsf2d& Transformation() const { return myTrsf; }

  //! Returns the stored 2D transformation (non-const).
  gp_Trsf2d& ChangeTransformation() { return myTrsf; }

  //! Sets the 2D transformation.
  //! @param theTrsf the 2D transformation to set
  void SetTransformation(const gp_Trsf2d& theTrsf) { myTrsf = theTrsf; }

  //! Returns true if the 2D transformation is identity.
  bool IsIdentity() const { return myTrsf.Form() == gp_Identity; }

  //--- Point Transformation ---

  //! Transforms a 2D point in place.
  //! @param theP the 2D point to transform
  void Transform(gp_Pnt2d& theP) const { theP.Transform(myTrsf); }

  //! Returns a transformed copy of a 2D point.
  //! @param theP the 2D point to transform
  //! @return transformed 2D point
  [[nodiscard]] gp_Pnt2d Transformed(const gp_Pnt2d& theP) const { return theP.Transformed(myTrsf); }

  //--- Vector Transformation ---

  //! Transforms a 2D vector in place.
  //! @param theV the 2D vector to transform
  void Transform(gp_Vec2d& theV) const { theV.Transform(myTrsf); }

  //! Returns a transformed copy of a 2D vector.
  //! @param theV the 2D vector to transform
  //! @return transformed 2D vector
  [[nodiscard]] gp_Vec2d Transformed(const gp_Vec2d& theV) const { return theV.Transformed(myTrsf); }

  //--- Combined Point + Vector Transformation ---

  //! Transforms 2D point and one derivative vector (D1 case).
  //! @param theP  2D point to transform
  //! @param theV  2D first derivative vector to transform
  void Transform(gp_Pnt2d& theP, gp_Vec2d& theV) const
  {
    theP.Transform(myTrsf);
    theV.Transform(myTrsf);
  }

  //! Transforms 2D point and two derivative vectors (D2 case).
  //! @param theP   2D point to transform
  //! @param theV1  2D first derivative vector to transform
  //! @param theV2  2D second derivative vector to transform
  void Transform(gp_Pnt2d& theP, gp_Vec2d& theV1, gp_Vec2d& theV2) const
  {
    theP.Transform(myTrsf);
    theV1.Transform(myTrsf);
    theV2.Transform(myTrsf);
  }

  //! Transforms 2D point and three derivative vectors (D3 case).
  //! @param theP   2D point to transform
  //! @param theV1  2D first derivative vector to transform
  //! @param theV2  2D second derivative vector to transform
  //! @param theV3  2D third derivative vector to transform
  void Transform(gp_Pnt2d& theP, gp_Vec2d& theV1, gp_Vec2d& theV2, gp_Vec2d& theV3) const
  {
    theP.Transform(myTrsf);
    theV1.Transform(myTrsf);
    theV2.Transform(myTrsf);
    theV3.Transform(myTrsf);
  }

  //--- 2D Curve Primitive Transformation ---

  //! Returns a transformed copy of a 2D line.
  //! @param theLin the 2D line to transform
  //! @return transformed 2D line
  [[nodiscard]] gp_Lin2d Transformed(const gp_Lin2d& theLin) const
  {
    return theLin.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a 2D circle.
  //! @param theCirc the 2D circle to transform
  //! @return transformed 2D circle
  [[nodiscard]] gp_Circ2d Transformed(const gp_Circ2d& theCirc) const
  {
    return theCirc.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a 2D ellipse.
  //! @param theElips the 2D ellipse to transform
  //! @return transformed 2D ellipse
  [[nodiscard]] gp_Elips2d Transformed(const gp_Elips2d& theElips) const
  {
    return theElips.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a 2D hyperbola.
  //! @param theHypr the 2D hyperbola to transform
  //! @return transformed 2D hyperbola
  [[nodiscard]] gp_Hypr2d Transformed(const gp_Hypr2d& theHypr) const
  {
    return theHypr.Transformed(myTrsf);
  }

  //! Returns a transformed copy of a 2D parabola.
  //! @param theParab the 2D parabola to transform
  //! @return transformed 2D parabola
  [[nodiscard]] gp_Parab2d Transformed(const gp_Parab2d& theParab) const
  {
    return theParab.Transformed(myTrsf);
  }

private:
  gp_Trsf2d myTrsf; //!< The stored 2D transformation
};

#endif // GeomAdaptor_TrsfModifier_HeaderFile
