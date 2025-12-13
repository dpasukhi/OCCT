// Created on: 1993-02-19
// Created by: Remi LEQUETTE
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

#include <BRepAdaptor_Curve.hxx>

#include <BRep_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_Surface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAdaptor_CurveModifier.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Lin.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NullObject.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

IMPLEMENT_STANDARD_RTTIEXT(BRepAdaptor_Curve, Adaptor3d_Curve)

//=================================================================================================

BRepAdaptor_Curve::BRepAdaptor_Curve() {}

//=================================================================================================

BRepAdaptor_Curve::BRepAdaptor_Curve(const BRepAdaptor_Curve& theOther)
    : Adaptor3d_Curve(),
      myTrsf(theOther.myTrsf),
      myCurve(theOther.myCurve),
      myEdge(theOther.myEdge)
{
}

//=================================================================================================

BRepAdaptor_Curve::BRepAdaptor_Curve(BRepAdaptor_Curve&& theOther) noexcept
    : Adaptor3d_Curve(),
      myTrsf(theOther.myTrsf),
      myCurve(std::move(theOther.myCurve)),
      myEdge(std::move(theOther.myEdge))
{
  theOther.myTrsf = gp_Trsf();
}

//=================================================================================================

BRepAdaptor_Curve& BRepAdaptor_Curve::operator=(const BRepAdaptor_Curve& theOther)
{
  if (this != &theOther)
  {
    myTrsf  = theOther.myTrsf;
    myCurve = theOther.myCurve;
    myEdge  = theOther.myEdge;
  }
  return *this;
}

//=================================================================================================

BRepAdaptor_Curve& BRepAdaptor_Curve::operator=(BRepAdaptor_Curve&& theOther) noexcept
{
  if (this != &theOther)
  {
    myTrsf  = theOther.myTrsf;
    myCurve = std::move(theOther.myCurve);
    myEdge  = std::move(theOther.myEdge);

    theOther.myTrsf = gp_Trsf();
  }
  return *this;
}

//=================================================================================================

BRepAdaptor_Curve BRepAdaptor_Curve::Copy() const
{
  return BRepAdaptor_Curve(*this);
}

//=================================================================================================

BRepAdaptor_Curve::BRepAdaptor_Curve(const TopoDS_Edge& E)
{
  Initialize(E);
}

//=================================================================================================

BRepAdaptor_Curve::BRepAdaptor_Curve(const TopoDS_Edge& E, const TopoDS_Face& F)
{
  Initialize(E, F);
}

//=================================================================================================

Handle(Adaptor3d_Curve) BRepAdaptor_Curve::ShallowCopy() const
{
  Handle(BRepAdaptor_Curve) aCopy = new BRepAdaptor_Curve();

  aCopy->myTrsf = myTrsf;

  const Handle(Adaptor3d_Curve) aCurve     = myCurve.ShallowCopy();
  const GeomAdaptor_Curve&      aGeomCurve = *(Handle(GeomAdaptor_Curve)::DownCast(aCurve));
  aCopy->myCurve                           = aGeomCurve;

  aCopy->myEdge = myEdge;

  return aCopy;
}

//=================================================================================================

void BRepAdaptor_Curve::Reset()
{
  myCurve.Reset();
  myEdge.Nullify();
  myTrsf = gp_Trsf();
}

//=================================================================================================

void BRepAdaptor_Curve::Initialize(const TopoDS_Edge& E)
{
  myEdge = E;
  Standard_Real pf, pl;

  TopLoc_Location    L;
  Handle(Geom_Curve) C = BRep_Tool::Curve(E, L, pf, pl);

  if (!C.IsNull())
  {
    // 3D curve case
    myCurve.Load(C, pf, pl);
    myTrsf = L.Transformation();

    // Set transformation modifier on myCurve for automatic coordinate transformation
    if (myTrsf.Form() != gp_Identity)
    {
      myCurve.SetTransformation(myTrsf);
    }
    else
    {
      myCurve.ClearModifier();
    }
  }
  else
  {
    // Curve-on-surface case
    Handle(Geom2d_Curve) PC;
    Handle(Geom_Surface) S;
    BRep_Tool::CurveOnSurface(E, PC, S, L, pf, pl);
    if (!PC.IsNull())
    {
      myTrsf = L.Transformation();

      // Create surface adaptor and apply transformation if needed
      auto aSurfAdaptor = std::make_unique<GeomAdaptor_Surface>(S);
      if (myTrsf.Form() != gp_Identity)
      {
        aSurfAdaptor->SetTransformation(myTrsf);
      }

      // Create PCurve adaptor
      auto aPCurveAdaptor = std::make_unique<Geom2dAdaptor_Curve>(PC, pf, pl);

      // Set up curve-on-surface modifier
      myCurve.SetCurveOnSurface(std::move(aPCurveAdaptor), std::move(aSurfAdaptor));
    }
    else
    {
      throw Standard_NullObject("BRepAdaptor_Curve::No geometry");
    }
  }
}

//=================================================================================================

void BRepAdaptor_Curve::Initialize(const TopoDS_Edge& E, const TopoDS_Face& F)
{
  myEdge = E;
  TopLoc_Location      L;
  Standard_Real        pf, pl;
  Handle(Geom_Surface) S  = BRep_Tool::Surface(F, L);
  Handle(Geom2d_Curve) PC = BRep_Tool::CurveOnSurface(E, F, pf, pl);

  myTrsf = L.Transformation();

  // Create surface adaptor and apply transformation if needed
  auto aSurfAdaptor = std::make_unique<GeomAdaptor_Surface>(S);
  if (myTrsf.Form() != gp_Identity)
  {
    aSurfAdaptor->SetTransformation(myTrsf);
  }

  // Create PCurve adaptor
  auto aPCurveAdaptor = std::make_unique<Geom2dAdaptor_Curve>(PC, pf, pl);

  // Set up curve-on-surface modifier
  myCurve.SetCurveOnSurface(std::move(aPCurveAdaptor), std::move(aSurfAdaptor));
}

//=================================================================================================

const gp_Trsf& BRepAdaptor_Curve::Trsf() const
{
  return myTrsf;
}

//=================================================================================================

Standard_Boolean BRepAdaptor_Curve::Is3DCurve() const
{
  return !IsCurveOnSurfaceModifier(myCurve.Modifier());
}

//=================================================================================================

Standard_Boolean BRepAdaptor_Curve::IsCurveOnSurface() const
{
  return IsCurveOnSurfaceModifier(myCurve.Modifier());
}

//=================================================================================================

const GeomAdaptor_Curve& BRepAdaptor_Curve::Curve() const
{
  return myCurve;
}

//=================================================================================================

const Geom2dAdaptor_Curve& BRepAdaptor_Curve::GetPCurve() const
{
  const auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myCurve.Modifier());
  if (pCOS == nullptr || pCOS->PCurve() == nullptr)
  {
    throw Standard_NoSuchObject("BRepAdaptor_Curve::GetPCurve - not a curve-on-surface");
  }
  return *pCOS->PCurve();
}

//=================================================================================================

const GeomAdaptor_Surface& BRepAdaptor_Curve::GetSurface() const
{
  const auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myCurve.Modifier());
  if (pCOS == nullptr || pCOS->Surface() == nullptr)
  {
    throw Standard_NoSuchObject("BRepAdaptor_Curve::GetSurface - not a curve-on-surface");
  }
  return *pCOS->Surface();
}

//=================================================================================================

const TopoDS_Edge& BRepAdaptor_Curve::Edge() const
{
  return myEdge;
}

//=================================================================================================

Standard_Real BRepAdaptor_Curve::Tolerance() const
{
  return BRep_Tool::Tolerance(myEdge);
}

//=================================================================================================

Standard_Real BRepAdaptor_Curve::FirstParameter() const
{
  return myCurve.FirstParameter();
}

//=================================================================================================

Standard_Real BRepAdaptor_Curve::LastParameter() const
{
  return myCurve.LastParameter();
}

//=================================================================================================

GeomAbs_Shape BRepAdaptor_Curve::Continuity() const
{
  return myCurve.Continuity();
}

//=================================================================================================

Standard_Integer BRepAdaptor_Curve::NbIntervals(const GeomAbs_Shape S) const
{
  return myCurve.NbIntervals(S);
}

//=================================================================================================

void BRepAdaptor_Curve::Intervals(TColStd_Array1OfReal& T, const GeomAbs_Shape S) const
{
  myCurve.Intervals(T, S);
}

//=================================================================================================

Handle(Adaptor3d_Curve) BRepAdaptor_Curve::Trim(const Standard_Real First,
                                                const Standard_Real Last,
                                                const Standard_Real Tol) const
{
  return new BRepAdaptor_Curve(TrimByValue(First, Last, Tol));
}

//=================================================================================================

BRepAdaptor_Curve BRepAdaptor_Curve::TrimByValue(double theFirst, double theLast, double theTol) const
{
  BRepAdaptor_Curve aResult = Copy();
  aResult.myCurve           = myCurve.TrimByValue(theFirst, theLast, theTol);
  return aResult;
}

//=================================================================================================

Standard_Boolean BRepAdaptor_Curve::IsClosed() const
{
  return myCurve.IsClosed();
}

//=================================================================================================

Standard_Boolean BRepAdaptor_Curve::IsPeriodic() const
{
  return myCurve.IsPeriodic();
}

//=================================================================================================

Standard_Real BRepAdaptor_Curve::Period() const
{
  return myCurve.Period();
}

//=================================================================================================

gp_Pnt BRepAdaptor_Curve::Value(const Standard_Real U) const
{
  return myCurve.Value(U);
}

//=================================================================================================

void BRepAdaptor_Curve::D0(const Standard_Real U, gp_Pnt& P) const
{
  myCurve.D0(U, P);
}

//=================================================================================================

void BRepAdaptor_Curve::D1(const Standard_Real U, gp_Pnt& P, gp_Vec& V) const
{
  myCurve.D1(U, P, V);
}

//=================================================================================================

void BRepAdaptor_Curve::D2(const Standard_Real U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const
{
  myCurve.D2(U, P, V1, V2);
}

//=================================================================================================

void BRepAdaptor_Curve::D3(const Standard_Real U,
                           gp_Pnt&             P,
                           gp_Vec&             V1,
                           gp_Vec&             V2,
                           gp_Vec&             V3) const
{
  myCurve.D3(U, P, V1, V2, V3);
}

//=================================================================================================

gp_Vec BRepAdaptor_Curve::DN(const Standard_Real U, const Standard_Integer N) const
{
  return myCurve.DN(U, N);
}

//=================================================================================================

Standard_Real BRepAdaptor_Curve::Resolution(const Standard_Real R) const
{
  return myCurve.Resolution(R);
}

//=================================================================================================

GeomAbs_CurveType BRepAdaptor_Curve::GetType() const
{
  return myCurve.GetType();
}

//=================================================================================================

gp_Lin BRepAdaptor_Curve::Line() const
{
  return myCurve.Line();
}

//=================================================================================================

gp_Circ BRepAdaptor_Curve::Circle() const
{
  return myCurve.Circle();
}

//=================================================================================================

gp_Elips BRepAdaptor_Curve::Ellipse() const
{
  return myCurve.Ellipse();
}

//=================================================================================================

gp_Hypr BRepAdaptor_Curve::Hyperbola() const
{
  return myCurve.Hyperbola();
}

//=================================================================================================

gp_Parab BRepAdaptor_Curve::Parabola() const
{
  return myCurve.Parabola();
}

//=================================================================================================

Standard_Integer BRepAdaptor_Curve::Degree() const
{
  return myCurve.Degree();
}

//=================================================================================================

Standard_Boolean BRepAdaptor_Curve::IsRational() const
{
  return myCurve.IsRational();
}

//=================================================================================================

Standard_Integer BRepAdaptor_Curve::NbPoles() const
{
  return myCurve.NbPoles();
}

//=================================================================================================

Standard_Integer BRepAdaptor_Curve::NbKnots() const
{
  return myCurve.NbKnots();
}

//=================================================================================================

Handle(Geom_BezierCurve) BRepAdaptor_Curve::Bezier() const
{
  Handle(Geom_BezierCurve) BC = myCurve.Bezier();
  return myTrsf.Form() == gp_Identity ? BC
                                      : Handle(Geom_BezierCurve)::DownCast(BC->Transformed(myTrsf));
}

//=================================================================================================

Handle(Geom_BSplineCurve) BRepAdaptor_Curve::BSpline() const
{
  Handle(Geom_BSplineCurve) BS = myCurve.BSpline();
  return myTrsf.Form() == gp_Identity
           ? BS
           : Handle(Geom_BSplineCurve)::DownCast(BS->Transformed(myTrsf));
}

//=================================================================================================

Handle(Geom_OffsetCurve) BRepAdaptor_Curve::OffsetCurve() const
{
  if (!Is3DCurve() || myCurve.GetType() != GeomAbs_OffsetCurve)
    throw Standard_NoSuchObject("BRepAdaptor_Curve::OffsetCurve");

  Handle(Geom_OffsetCurve) anOffC = myCurve.OffsetCurve();
  return myTrsf.Form() == gp_Identity
           ? anOffC
           : Handle(Geom_OffsetCurve)::DownCast(anOffC->Transformed(myTrsf));
}
