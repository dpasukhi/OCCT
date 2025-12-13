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

#include <GeomAdaptor_CurveOnSurfaceModifier.hxx>

#include <Geom2dAdaptor_Curve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <ElCLib.hxx>
#include <ElSLib.hxx>
#include <gp_Ax22d.hxx>
#include <gp_Circ2d.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <Precision.hxx>
#include <Standard_NotImplemented.hxx>

namespace
{
  //! Converts a 2D point on a plane to 3D.
  static gp_Pnt to3d(const gp_Pln& thePl, const gp_Pnt2d& theP)
  {
    return ElSLib::Value(theP.X(), theP.Y(), thePl);
  }

  //! Converts a 2D vector on a plane to 3D.
  static gp_Vec to3d(const gp_Pln& thePl, const gp_Vec2d& theV)
  {
    gp_Vec aVx = thePl.XAxis().Direction();
    gp_Vec aVy = thePl.YAxis().Direction();
    aVx.Multiply(theV.X());
    aVy.Multiply(theV.Y());
    aVx.Add(aVy);
    return aVx;
  }

  //! Converts a 2D coordinate system on a plane to 3D.
  static gp_Ax2 to3d(const gp_Pln& thePl, const gp_Ax22d& theA)
  {
    gp_Pnt aP  = to3d(thePl, theA.Location());
    gp_Vec aVX = to3d(thePl, theA.XAxis().Direction());
    gp_Vec aVY = to3d(thePl, theA.YAxis().Direction());
    return gp_Ax2(aP, aVX.Crossed(aVY), aVX);
  }

  //! Converts a 2D circle on a plane to 3D.
  static gp_Circ to3d(const gp_Pln& thePl, const gp_Circ2d& theC)
  {
    return gp_Circ(to3d(thePl, theC.Axis()), theC.Radius());
  }

  //! Computes third derivative using linear form.
  static gp_Vec setLinearForm(const gp_Vec2d& theDW,
                              const gp_Vec2d& theD2W,
                              const gp_Vec2d& theD3W,
                              const gp_Vec&   theD1U,
                              const gp_Vec&   theD1V,
                              const gp_Vec&   theD2U,
                              const gp_Vec&   theD2V,
                              const gp_Vec&   theD2UV,
                              const gp_Vec&   theD3U,
                              const gp_Vec&   theD3V,
                              const gp_Vec&   theD3UUV,
                              const gp_Vec&   theD3UVV)
  {
    gp_Vec aV31, aV32, aV33, aV34, aV3;
    aV31.SetLinearForm(theDW.X(), theD1U, theD2W.X() * theDW.X(), theD2U, theD2W.X() * theDW.Y(), theD2UV);
    aV31.SetLinearForm(theD3W.Y(), theD1V, theD2W.Y() * theDW.X(), theD2UV, theD2W.Y() * theDW.Y(), theD2V, aV31);
    aV32.SetLinearForm(theDW.X() * theDW.X() * theDW.Y(), theD3UUV, theDW.X() * theDW.Y() * theDW.Y(), theD3UVV);
    aV32.SetLinearForm(theD2W.X() * theDW.Y() + theDW.X() * theD2W.Y(),
                       theD2UV,
                       theDW.X() * theDW.Y() * theDW.Y(),
                       theD3UVV,
                       aV32);
    aV33.SetLinearForm(2 * theD2W.X() * theDW.X(),
                       theD2U,
                       theDW.X() * theDW.X() * theDW.X(),
                       theD3U,
                       theDW.X() * theDW.X() * theDW.Y(),
                       theD3UUV);
    aV34.SetLinearForm(2 * theD2W.Y() * theDW.Y(),
                       theD2V,
                       theDW.Y() * theDW.Y() * theDW.X(),
                       theD3UVV,
                       theDW.Y() * theDW.Y() * theDW.Y(),
                       theD3V);
    aV3.SetLinearForm(1, aV31, 2, aV32, 1, aV33, aV34);
    return aV3;
  }
} // namespace

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier::GeomAdaptor_CurveOnSurfaceModifier()
    : myType(GeomAbs_OtherCurve)
{
}

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier::GeomAdaptor_CurveOnSurfaceModifier(
  std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
  std::unique_ptr<GeomAdaptor_Surface> theSurface)
    : myPCurve(std::move(thePCurve)),
      mySurface(std::move(theSurface)),
      myType(GeomAbs_OtherCurve)
{
  if (myPCurve && mySurface)
  {
    evalKPart();
  }
}

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier::GeomAdaptor_CurveOnSurfaceModifier(
  GeomAdaptor_CurveOnSurfaceModifier&& theOther) noexcept
    : myPCurve(std::move(theOther.myPCurve)),
      mySurface(std::move(theOther.mySurface)),
      myType(theOther.myType),
      myLin(theOther.myLin),
      myCirc(theOther.myCirc)
{
  theOther.myType = GeomAbs_OtherCurve;
}

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier& GeomAdaptor_CurveOnSurfaceModifier::operator=(
  GeomAdaptor_CurveOnSurfaceModifier&& theOther) noexcept
{
  if (this != &theOther)
  {
    myPCurve        = std::move(theOther.myPCurve);
    mySurface       = std::move(theOther.mySurface);
    myType          = theOther.myType;
    myLin           = theOther.myLin;
    myCirc          = theOther.myCirc;
    theOther.myType = GeomAbs_OtherCurve;
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier GeomAdaptor_CurveOnSurfaceModifier::Copy() const
{
  GeomAdaptor_CurveOnSurfaceModifier aCopy;
  if (myPCurve)
  {
    // TODO: Change to myPCurve->Copy() when Geom2dAdaptor_Curve is rewritten
    aCopy.myPCurve = std::make_unique<Geom2dAdaptor_Curve>(*myPCurve);
  }
  if (mySurface)
  {
    // TODO: Change to mySurface->Copy() when GeomAdaptor_Surface is rewritten
    aCopy.mySurface = std::make_unique<GeomAdaptor_Surface>(*mySurface);
  }
  aCopy.myType  = myType;
  aCopy.myLin   = myLin;
  aCopy.myCirc  = myCirc;
  return aCopy;
}

//==================================================================================================

GeomAdaptor_CurveOnSurfaceModifier::~GeomAdaptor_CurveOnSurfaceModifier() = default;

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::Load(std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
                                              std::unique_ptr<GeomAdaptor_Surface> theSurface)
{
  myPCurve  = std::move(thePCurve);
  mySurface = std::move(theSurface);
  myType    = GeomAbs_OtherCurve;

  if (myPCurve && mySurface)
  {
    evalKPart();
  }
}

//==================================================================================================

double GeomAdaptor_CurveOnSurfaceModifier::FirstParameter() const
{
  return myPCurve ? myPCurve->FirstParameter() : 0.0;
}

//==================================================================================================

double GeomAdaptor_CurveOnSurfaceModifier::LastParameter() const
{
  return myPCurve ? myPCurve->LastParameter() : 1.0;
}

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::D0(double theU, gp_Pnt& theP) const
{
  if (myType == GeomAbs_Line)
  {
    theP = ElCLib::Value(theU, myLin);
  }
  else if (myType == GeomAbs_Circle)
  {
    theP = ElCLib::Value(theU, myCirc);
  }
  else
  {
    gp_Pnt2d aPuv;
    myPCurve->D0(theU, aPuv);
    mySurface->D0(aPuv.X(), aPuv.Y(), theP);
  }
}

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::D1(double theU, gp_Pnt& theP, gp_Vec& theV) const
{
  if (myType == GeomAbs_Line)
  {
    ElCLib::D1(theU, myLin, theP, theV);
  }
  else if (myType == GeomAbs_Circle)
  {
    ElCLib::D1(theU, myCirc, theP, theV);
  }
  else
  {
    gp_Pnt2d aPuv;
    gp_Vec2d aDuv;
    gp_Vec   aD1U, aD1V;

    myPCurve->D1(theU, aPuv, aDuv);
    mySurface->D1(aPuv.X(), aPuv.Y(), theP, aD1U, aD1V);

    // Chain rule: V = dU * D1U + dV * D1V
    theV.SetLinearForm(aDuv.X(), aD1U, aDuv.Y(), aD1V);
  }
}

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::D2(double   theU,
                                            gp_Pnt&  theP,
                                            gp_Vec&  theV1,
                                            gp_Vec&  theV2) const
{
  if (myType == GeomAbs_Line)
  {
    ElCLib::D1(theU, myLin, theP, theV1);
    theV2.SetCoord(0., 0., 0.);
  }
  else if (myType == GeomAbs_Circle)
  {
    ElCLib::D2(theU, myCirc, theP, theV1, theV2);
  }
  else
  {
    gp_Pnt2d aUV;
    gp_Vec2d aDW, aD2W;
    gp_Vec   aD1U, aD1V, aD2U, aD2V, aD2UV;

    myPCurve->D2(theU, aUV, aDW, aD2W);
    mySurface->D2(aUV.X(), aUV.Y(), theP, aD1U, aD1V, aD2U, aD2V, aD2UV);

    // First derivative: V1 = dU * D1U + dV * D1V
    theV1.SetLinearForm(aDW.X(), aD1U, aDW.Y(), aD1V);

    // Second derivative using chain rule
    theV2.SetLinearForm(aD2W.X(), aD1U, aD2W.Y(), aD1V, 2. * aDW.X() * aDW.Y(), aD2UV);
    theV2.SetLinearForm(aDW.X() * aDW.X(), aD2U, aDW.Y() * aDW.Y(), aD2V, theV2);
  }
}

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::D3(double   theU,
                                            gp_Pnt&  theP,
                                            gp_Vec&  theV1,
                                            gp_Vec&  theV2,
                                            gp_Vec&  theV3) const
{
  if (myType == GeomAbs_Line)
  {
    ElCLib::D1(theU, myLin, theP, theV1);
    theV2.SetCoord(0., 0., 0.);
    theV3.SetCoord(0., 0., 0.);
  }
  else if (myType == GeomAbs_Circle)
  {
    ElCLib::D3(theU, myCirc, theP, theV1, theV2, theV3);
  }
  else
  {
    gp_Pnt2d aUV;
    gp_Vec2d aDW, aD2W, aD3W;
    gp_Vec   aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV;

    myPCurve->D3(theU, aUV, aDW, aD2W, aD3W);
    mySurface->D3(aUV.X(), aUV.Y(), theP, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV);

    // First derivative
    theV1.SetLinearForm(aDW.X(), aD1U, aDW.Y(), aD1V);

    // Second derivative
    theV2.SetLinearForm(aD2W.X(), aD1U, aD2W.Y(), aD1V, 2. * aDW.X() * aDW.Y(), aD2UV);
    theV2.SetLinearForm(aDW.X() * aDW.X(), aD2U, aDW.Y() * aDW.Y(), aD2V, theV2);

    // Third derivative
    theV3 = setLinearForm(aDW, aD2W, aD3W, aD1U, aD1V, aD2U, aD2V, aD2UV, aD3U, aD3V, aD3UUV, aD3UVV);
  }
}

//==================================================================================================

gp_Vec GeomAdaptor_CurveOnSurfaceModifier::DN(double theU, int theN) const
{
  gp_Pnt aP;
  gp_Vec aV1, aV2, aV;
  switch (theN)
  {
    case 1: D1(theU, aP, aV); break;
    case 2: D2(theU, aP, aV1, aV); break;
    case 3: D3(theU, aP, aV1, aV2, aV); break;
    default: throw Standard_NotImplemented("GeomAdaptor_CurveOnSurfaceModifier:DN");
  }
  return aV;
}

//==================================================================================================

void GeomAdaptor_CurveOnSurfaceModifier::evalKPart()
{
  myType = GeomAbs_OtherCurve;

  GeomAbs_SurfaceType aSType = mySurface->GetType();
  GeomAbs_CurveType   aCType = myPCurve->GetType();

  if (aSType == GeomAbs_Plane)
  {
    // On a plane, the 2D curve type is preserved in 3D
    myType = aCType;
    if (myType == GeomAbs_Circle)
    {
      myCirc = to3d(mySurface->Plane(), myPCurve->Circle());
    }
    else if (myType == GeomAbs_Line)
    {
      // Compute the 3D line from D1 at parameter 0
      gp_Pnt   aP;
      gp_Vec   aV;
      gp_Pnt2d aPuv;
      gp_Vec2d aDuv;
      myPCurve->D1(0., aPuv, aDuv);
      gp_Vec aD1U, aD1V;
      mySurface->D1(aPuv.X(), aPuv.Y(), aP, aD1U, aD1V);
      aV.SetLinearForm(aDuv.X(), aD1U, aDuv.Y(), aD1V);
      myLin = gp_Lin(aP, aV);
    }
  }
  else if (aCType == GeomAbs_Line)
  {
    // 2D line on quadric surfaces can produce circles or lines
    gp_Dir2d aD = myPCurve->Line().Direction();

    if (aD.IsParallel(gp::DX2d(), Precision::Angular()))
    {
      // Iso V curve
      gp_Pnt2d aP = myPCurve->Line().Location();
      if (aSType == GeomAbs_Cylinder)
      {
        myType          = GeomAbs_Circle;
        gp_Cylinder aCyl = mySurface->Cylinder();
        gp_Ax3      anAxis = aCyl.Position();
        myCirc          = ElSLib::CylinderVIso(anAxis, aCyl.Radius(), aP.Y());
        gp_Dir aDRev    = anAxis.XDirection().Crossed(anAxis.YDirection());
        gp_Ax1 anAxeRev(anAxis.Location(), aDRev);
        myCirc.Rotate(anAxeRev, aP.X());
        if (aD.IsOpposite(gp::DX2d(), Precision::Angular()))
        {
          gp_Ax2 anAx = myCirc.Position();
          anAx.SetDirection(anAx.Direction().Reversed());
          myCirc.SetPosition(anAx);
        }
      }
      else if (aSType == GeomAbs_Cone)
      {
        myType        = GeomAbs_Circle;
        gp_Cone aCone = mySurface->Cone();
        gp_Ax3  anAxis = aCone.Position();
        myCirc        = ElSLib::ConeVIso(anAxis, aCone.RefRadius(), aCone.SemiAngle(), aP.Y());
        gp_Dir aDRev  = anAxis.XDirection().Crossed(anAxis.YDirection());
        gp_Ax1 anAxeRev(anAxis.Location(), aDRev);
        myCirc.Rotate(anAxeRev, aP.X());
        if (aD.IsOpposite(gp::DX2d(), Precision::Angular()))
        {
          gp_Ax2 anAx = myCirc.Position();
          anAx.SetDirection(anAx.Direction().Reversed());
          myCirc.SetPosition(anAx);
        }
      }
      else if (aSType == GeomAbs_Sphere)
      {
        if (std::abs(std::abs(aP.Y()) - M_PI / 2.) >= Precision::PConfusion())
        {
          myType         = GeomAbs_Circle;
          gp_Sphere aSph = mySurface->Sphere();
          gp_Ax3    anAxis = aSph.Position();
          myCirc         = ElSLib::SphereVIso(anAxis, aSph.Radius(), aP.Y());
          gp_Dir aDRev   = anAxis.XDirection().Crossed(anAxis.YDirection());
          gp_Ax1 anAxeRev(anAxis.Location(), aDRev);
          myCirc.Rotate(anAxeRev, aP.X());
          if (aD.IsOpposite(gp::DX2d(), Precision::Angular()))
          {
            gp_Ax2 anAx = myCirc.Position();
            anAx.SetDirection(anAx.Direction().Reversed());
            myCirc.SetPosition(anAx);
          }
        }
      }
      else if (aSType == GeomAbs_Torus)
      {
        myType         = GeomAbs_Circle;
        gp_Torus aTore = mySurface->Torus();
        gp_Ax3   anAxis = aTore.Position();
        myCirc         = ElSLib::TorusVIso(anAxis, aTore.MajorRadius(), aTore.MinorRadius(), aP.Y());
        gp_Dir aDRev   = anAxis.XDirection().Crossed(anAxis.YDirection());
        gp_Ax1 anAxeRev(anAxis.Location(), aDRev);
        myCirc.Rotate(anAxeRev, aP.X());
        if (aD.IsOpposite(gp::DX2d(), Precision::Angular()))
        {
          gp_Ax2 anAx = myCirc.Position();
          anAx.SetDirection(anAx.Direction().Reversed());
          myCirc.SetPosition(anAx);
        }
      }
    }
    else if (aD.IsParallel(gp::DY2d(), Precision::Angular()))
    {
      // Iso U curve
      gp_Pnt2d aP = myPCurve->Line().Location();
      if (aSType == GeomAbs_Cylinder)
      {
        myType          = GeomAbs_Line;
        gp_Cylinder aCyl = mySurface->Cylinder();
        myLin           = ElSLib::CylinderUIso(aCyl.Position(), aCyl.Radius(), aP.X());
        gp_Vec aTr(myLin.Direction());
        aTr.Multiply(aP.Y());
        myLin.Translate(aTr);
        if (aD.IsOpposite(gp::DY2d(), Precision::Angular()))
        {
          myLin.Reverse();
        }
      }
      else if (aSType == GeomAbs_Cone)
      {
        myType        = GeomAbs_Line;
        gp_Cone aCone = mySurface->Cone();
        myLin = ElSLib::ConeUIso(aCone.Position(), aCone.RefRadius(), aCone.SemiAngle(), aP.X());
        gp_Vec aTr(myLin.Direction());
        aTr.Multiply(aP.Y());
        myLin.Translate(aTr);
        if (aD.IsOpposite(gp::DY2d(), Precision::Angular()))
        {
          myLin.Reverse();
        }
      }
      else if (aSType == GeomAbs_Sphere)
      {
        myType         = GeomAbs_Circle;
        gp_Sphere aSph = mySurface->Sphere();
        gp_Ax3    anAxis = aSph.Position();
        // Compute iso 0 then rotate
        myCirc = ElSLib::SphereUIso(anAxis, aSph.Radius(), 0.);

        gp_Dir aDRev = anAxis.XDirection().Crossed(anAxis.Direction());
        gp_Ax1 anAxeRev(anAxis.Location(), aDRev);
        myCirc.Rotate(anAxeRev, aP.Y());

        aDRev   = anAxis.XDirection().Crossed(anAxis.YDirection());
        anAxeRev = gp_Ax1(anAxis.Location(), aDRev);
        myCirc.Rotate(anAxeRev, aP.X());

        if (aD.IsOpposite(gp::DY2d(), Precision::Angular()))
        {
          gp_Ax2 anAx = myCirc.Position();
          anAx.SetDirection(anAx.Direction().Reversed());
          myCirc.SetPosition(anAx);
        }
      }
      else if (aSType == GeomAbs_Torus)
      {
        myType         = GeomAbs_Circle;
        gp_Torus aTore = mySurface->Torus();
        gp_Ax3   anAxis = aTore.Position();
        myCirc         = ElSLib::TorusUIso(anAxis, aTore.MajorRadius(), aTore.MinorRadius(), aP.X());
        myCirc.Rotate(myCirc.Axis(), aP.Y());

        if (aD.IsOpposite(gp::DY2d(), Precision::Angular()))
        {
          gp_Ax2 anAx = myCirc.Position();
          anAx.SetDirection(anAx.Direction().Reversed());
          myCirc.SetPosition(anAx);
        }
      }
    }
  }
}
