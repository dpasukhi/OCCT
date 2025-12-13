// Created on: 1993-04-29
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

// 20/02/97 : PMN -> Positionement local sur BSpline (PRO6902)
// 10/07/97 : PMN -> Pas de calcul de resolution dans Nb(Intervals)(PRO9248)
// 20/10/97 : RBV -> traitement des offset curves

#define No_Standard_RangeError
#define No_Standard_OutOfRange

#include <GeomAdaptor_Curve.hxx>

#include <BSplCLib.hxx>
#include <BSplCLib_Cache.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomAbs_Shape.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomEvaluator_OffsetCurve.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Hypr.hxx>
#include <gp_Lin.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <Standard_DomainError.hxx>
#include <Standard_NoSuchObject.hxx>
#include <Standard_NotImplemented.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>

static const double PosTol = Precision::PConfusion() / 2;

IMPLEMENT_STANDARD_RTTIEXT(GeomAdaptor_Curve, Adaptor3d_Curve)

//==================================================================================================

GeomAdaptor_Curve::GeomAdaptor_Curve(const GeomAdaptor_Curve& theOther)
    : Adaptor3d_Curve(),
      myCurve(theOther.myCurve),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myBSplineCurve(theOther.myBSplineCurve)
{
  if (!theOther.myNestedEvaluator.IsNull())
  {
    myNestedEvaluator = theOther.myNestedEvaluator->ShallowCopy();
  }

  // Deep copy the modifier
  std::visit(
    [this](const auto& mod)
    {
      using T = std::decay_t<decltype(mod)>;
      if constexpr (std::is_same_v<T, std::monostate>)
      {
        myModifier = std::monostate{};
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_TrsfModifier>)
      {
        myModifier = mod; // TrsfModifier is copyable
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_CurveOnSurfaceModifier>)
      {
        myModifier = mod.Copy();
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_IsoCurveModifier>)
      {
        myModifier = mod.Copy();
      }
    },
    theOther.myModifier);
}

//==================================================================================================

GeomAdaptor_Curve::GeomAdaptor_Curve(GeomAdaptor_Curve&& theOther) noexcept
    : myCurve(std::move(theOther.myCurve)),
      myTypeCurve(theOther.myTypeCurve),
      myFirst(theOther.myFirst),
      myLast(theOther.myLast),
      myModifier(std::move(theOther.myModifier)),
      myBSplineCurve(std::move(theOther.myBSplineCurve)),
      myCurveCache(std::move(theOther.myCurveCache)),
      myNestedEvaluator(std::move(theOther.myNestedEvaluator))
{
  theOther.myTypeCurve = GeomAbs_OtherCurve;
  theOther.myFirst     = 0.0;
  theOther.myLast      = 0.0;
}

//==================================================================================================

GeomAdaptor_Curve& GeomAdaptor_Curve::operator=(GeomAdaptor_Curve&& theOther) noexcept
{
  if (this != &theOther)
  {
    myCurve           = std::move(theOther.myCurve);
    myTypeCurve       = theOther.myTypeCurve;
    myFirst           = theOther.myFirst;
    myLast            = theOther.myLast;
    myModifier        = std::move(theOther.myModifier);
    myBSplineCurve    = std::move(theOther.myBSplineCurve);
    myCurveCache      = std::move(theOther.myCurveCache);
    myNestedEvaluator = std::move(theOther.myNestedEvaluator);

    theOther.myTypeCurve = GeomAbs_OtherCurve;
    theOther.myFirst     = 0.0;
    theOther.myLast      = 0.0;
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_Curve& GeomAdaptor_Curve::operator=(const GeomAdaptor_Curve& theOther)
{
  if (this != &theOther)
  {
    myCurve        = theOther.myCurve;
    myTypeCurve    = theOther.myTypeCurve;
    myFirst        = theOther.myFirst;
    myLast         = theOther.myLast;
    myBSplineCurve = theOther.myBSplineCurve;
    myCurveCache.Nullify();

    if (!theOther.myNestedEvaluator.IsNull())
    {
      myNestedEvaluator = theOther.myNestedEvaluator->ShallowCopy();
    }
    else
    {
      myNestedEvaluator.Nullify();
    }

    // Deep copy the modifier
    std::visit(
      [this](const auto& mod)
      {
        using T = std::decay_t<decltype(mod)>;
        if constexpr (std::is_same_v<T, std::monostate>)
        {
          myModifier = std::monostate{};
        }
        else if constexpr (std::is_same_v<T, GeomAdaptor_TrsfModifier>)
        {
          myModifier = mod; // TrsfModifier is copyable
        }
        else if constexpr (std::is_same_v<T, GeomAdaptor_CurveOnSurfaceModifier>)
        {
          myModifier = mod.Copy();
        }
        else if constexpr (std::is_same_v<T, GeomAdaptor_IsoCurveModifier>)
        {
          myModifier = mod.Copy();
        }
      },
      theOther.myModifier);
  }
  return *this;
}

//==================================================================================================

GeomAdaptor_Curve GeomAdaptor_Curve::Copy() const
{
  GeomAdaptor_Curve aCopy;
  aCopy.myCurve        = myCurve;
  aCopy.myTypeCurve    = myTypeCurve;
  aCopy.myFirst        = myFirst;
  aCopy.myLast         = myLast;
  aCopy.myBSplineCurve = myBSplineCurve;

  if (!myNestedEvaluator.IsNull())
  {
    aCopy.myNestedEvaluator = myNestedEvaluator->ShallowCopy();
  }

  // Deep copy the modifier
  std::visit(
    [&aCopy](const auto& mod)
    {
      using T = std::decay_t<decltype(mod)>;
      if constexpr (std::is_same_v<T, std::monostate>)
      {
        aCopy.myModifier = std::monostate{};
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_TrsfModifier>)
      {
        aCopy.myModifier = mod; // TrsfModifier is copyable
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_CurveOnSurfaceModifier>)
      {
        aCopy.myModifier = mod.Copy();
      }
      else if constexpr (std::is_same_v<T, GeomAdaptor_IsoCurveModifier>)
      {
        aCopy.myModifier = mod.Copy();
      }
    },
    myModifier);

  return aCopy;
}

//==================================================================================================

GeomAdaptor_Curve::~GeomAdaptor_Curve() = default;

//==================================================================================================

Handle(Adaptor3d_Curve) GeomAdaptor_Curve::ShallowCopy() const
{
  return new GeomAdaptor_Curve(*this);
}

//==================================================================================================

void GeomAdaptor_Curve::Reset()
{
  myTypeCurve = GeomAbs_OtherCurve;
  myCurve.Nullify();
  myNestedEvaluator.Nullify();
  myBSplineCurve.Nullify();
  myCurveCache.Nullify();
  myFirst    = 0.0;
  myLast     = 0.0;
  myModifier = std::monostate{};
}

//==================================================================================================

void GeomAdaptor_Curve::SetCurveOnSurface(std::unique_ptr<Geom2dAdaptor_Curve> thePCurve,
                                          std::unique_ptr<GeomAdaptor_Surface> theSurface)
{
  GeomAdaptor_CurveOnSurfaceModifier aModifier(std::move(thePCurve), std::move(theSurface));

  // Update parameter bounds from the PCurve
  myFirst = aModifier.FirstParameter();
  myLast  = aModifier.LastParameter();

  // Update curve type based on the modifier's detected type
  myTypeCurve = aModifier.GetType();

  myModifier = std::move(aModifier);
}

//==================================================================================================

void GeomAdaptor_Curve::SetIsoCurve(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                    GeomAbs_IsoType                      theIsoType,
                                    double                               theParam)
{
  GeomAdaptor_IsoCurveModifier aModifier(std::move(theSurface), theIsoType, theParam);

  // Update parameter bounds from the modifier
  myFirst = aModifier.FirstParameter();
  myLast  = aModifier.LastParameter();

  // Update curve type based on the modifier's detected type
  myTypeCurve = aModifier.GetType();

  myModifier = std::move(aModifier);
}

//==================================================================================================

void GeomAdaptor_Curve::SetIsoCurve(std::unique_ptr<GeomAdaptor_Surface> theSurface,
                                    GeomAbs_IsoType                      theIsoType,
                                    double                               theParam,
                                    double                               theFirst,
                                    double                               theLast)
{
  GeomAdaptor_IsoCurveModifier aModifier(std::move(theSurface), theIsoType, theParam, theFirst, theLast);

  // Update parameter bounds from the modifier
  myFirst = aModifier.FirstParameter();
  myLast  = aModifier.LastParameter();

  // Update curve type based on the modifier's detected type
  myTypeCurve = aModifier.GetType();

  myModifier = std::move(aModifier);
}

//==================================================================================================
// LocalContinuity: Computes the Continuity of a BSplineCurve
//           between the parameters U1 and U2
//           The continuity is C(d-m)
//             with   d = degree,
//                    m = max multiplicity of the Knots between U1 and U2
//==================================================================================================

GeomAbs_Shape GeomAdaptor_Curve::LocalContinuity(double U1, double U2) const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_BSplineCurve, " ");
  int                            Nb     = myBSplineCurve->NbKnots();
  int                            Index1 = 0;
  int                            Index2 = 0;
  double                         newFirst, newLast;
  const TColStd_Array1OfReal&    TK = myBSplineCurve->Knots();
  const TColStd_Array1OfInteger& TM = myBSplineCurve->Multiplicities();
  BSplCLib::LocateParameter(myBSplineCurve->Degree(),
                            TK,
                            TM,
                            U1,
                            myBSplineCurve->IsPeriodic(),
                            1,
                            Nb,
                            Index1,
                            newFirst);
  BSplCLib::LocateParameter(myBSplineCurve->Degree(),
                            TK,
                            TM,
                            U2,
                            myBSplineCurve->IsPeriodic(),
                            1,
                            Nb,
                            Index2,
                            newLast);
  if (std::abs(newFirst - TK(Index1 + 1)) < Precision::PConfusion())
  {
    if (Index1 < Nb)
      Index1++;
  }
  if (std::abs(newLast - TK(Index2)) < Precision::PConfusion())
    Index2--;
  int MultMax;
  // attention aux courbes peridiques.
  if ((myBSplineCurve->IsPeriodic()) && (Index1 == Nb))
    Index1 = 1;

  if ((Index2 - Index1 <= 0) && (!myBSplineCurve->IsPeriodic()))
  {
    MultMax = 100; // CN entre 2 Noeuds consecutifs
  }
  else
  {
    MultMax = TM(Index1 + 1);
    for (int i = Index1 + 1; i <= Index2; i++)
    {
      if (TM(i) > MultMax)
        MultMax = TM(i);
    }
    MultMax = myBSplineCurve->Degree() - MultMax;
  }
  if (MultMax <= 0)
  {
    return GeomAbs_C0;
  }
  else if (MultMax == 1)
  {
    return GeomAbs_C1;
  }
  else if (MultMax == 2)
  {
    return GeomAbs_C2;
  }
  else if (MultMax == 3)
  {
    return GeomAbs_C3;
  }
  else
  {
    return GeomAbs_CN;
  }
}

//==================================================================================================

void GeomAdaptor_Curve::load(const Handle(Geom_Curve) & C, double UFirst, double ULast)
{
  myFirst = UFirst;
  myLast  = ULast;
  myCurveCache.Nullify();

  if (myCurve != C)
  {
    myCurve = C;
    myNestedEvaluator.Nullify();
    myBSplineCurve.Nullify();

    const Handle(Standard_Type)& TheType = C->DynamicType();
    if (TheType == STANDARD_TYPE(Geom_TrimmedCurve))
    {
      Load(Handle(Geom_TrimmedCurve)::DownCast(C)->BasisCurve(), UFirst, ULast);
    }
    else if (TheType == STANDARD_TYPE(Geom_Circle))
    {
      myTypeCurve = GeomAbs_Circle;
    }
    else if (TheType == STANDARD_TYPE(Geom_Line))
    {
      myTypeCurve = GeomAbs_Line;
    }
    else if (TheType == STANDARD_TYPE(Geom_Ellipse))
    {
      myTypeCurve = GeomAbs_Ellipse;
    }
    else if (TheType == STANDARD_TYPE(Geom_Parabola))
    {
      myTypeCurve = GeomAbs_Parabola;
    }
    else if (TheType == STANDARD_TYPE(Geom_Hyperbola))
    {
      myTypeCurve = GeomAbs_Hyperbola;
    }
    else if (TheType == STANDARD_TYPE(Geom_BezierCurve))
    {
      myTypeCurve = GeomAbs_BezierCurve;
    }
    else if (TheType == STANDARD_TYPE(Geom_BSplineCurve))
    {
      myTypeCurve    = GeomAbs_BSplineCurve;
      myBSplineCurve = Handle(Geom_BSplineCurve)::DownCast(myCurve);
    }
    else if (TheType == STANDARD_TYPE(Geom_OffsetCurve))
    {
      myTypeCurve                            = GeomAbs_OffsetCurve;
      Handle(Geom_OffsetCurve) anOffsetCurve = Handle(Geom_OffsetCurve)::DownCast(myCurve);
      // Create nested adaptor for base curve
      Handle(Geom_Curve) aBaseCurve = anOffsetCurve->BasisCurve();
      // Note: Using a temporary GeomAdaptor_Curve on heap for backward compatibility
      // with GeomEvaluator_OffsetCurve which expects Handle(Adaptor3d_Curve)
      // This will be cleaned up when we remove Adaptor3d_Curve completely
      GeomAdaptor_Curve* aBaseAdaptor = new GeomAdaptor_Curve();
      aBaseAdaptor->Load(aBaseCurve);
      myNestedEvaluator =
        new GeomEvaluator_OffsetCurve(aBaseAdaptor, anOffsetCurve->Offset(), anOffsetCurve->Direction());
    }
    else
    {
      myTypeCurve = GeomAbs_OtherCurve;
    }
  }
}

//    --
//    --     Global methods - Apply to the whole curve.
//    --

//==================================================================================================

GeomAbs_Shape GeomAdaptor_Curve::Continuity() const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
    return LocalContinuity(myFirst, myLast);

  if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    const GeomAbs_Shape S = Handle(Geom_OffsetCurve)::DownCast(myCurve)->GetBasisCurveContinuity();
    switch (S)
    {
      case GeomAbs_CN:
        return GeomAbs_CN;
      case GeomAbs_C3:
        return GeomAbs_C2;
      case GeomAbs_C2:
        return GeomAbs_C1;
      case GeomAbs_C1:
        return GeomAbs_C0;
      case GeomAbs_G1:
        return GeomAbs_G1;
      case GeomAbs_G2:
        return GeomAbs_G2;
      default:
        throw Standard_NoSuchObject("GeomAdaptor_Curve::Continuity");
    }
  }
  else if (myTypeCurve == GeomAbs_OtherCurve)
  {
    throw Standard_NoSuchObject("GeomAdaptor_Curve::Contunuity");
  }

  return GeomAbs_CN;
}

//==================================================================================================

int GeomAdaptor_Curve::NbIntervals(GeomAbs_Shape S) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    if ((!myBSplineCurve->IsPeriodic() && S <= Continuity()) || S == GeomAbs_C0)
    {
      return 1;
    }

    int aDegree = myBSplineCurve->Degree();
    int aCont;

    switch (S)
    {
      case GeomAbs_C1:
        aCont = 1;
        break;
      case GeomAbs_C2:
        aCont = 2;
        break;
      case GeomAbs_C3:
        aCont = 3;
        break;
      case GeomAbs_CN:
        aCont = aDegree;
        break;
      default:
        throw Standard_DomainError("GeomAdaptor_Curve::NbIntervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    return BSplCLib::Intervals(myBSplineCurve->Knots(),
                               myBSplineCurve->Multiplicities(),
                               aDegree,
                               myBSplineCurve->IsPeriodic(),
                               aCont,
                               myFirst,
                               myLast,
                               anEps,
                               nullptr);
  }

  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    int           myNbIntervals = 1;
    GeomAbs_Shape BaseS         = GeomAbs_C0;
    switch (S)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("GeomAdaptor_Curve::NbIntervals");
        break;
      case GeomAbs_C0:
        BaseS = GeomAbs_C1;
        break;
      case GeomAbs_C1:
        BaseS = GeomAbs_C2;
        break;
      case GeomAbs_C2:
        BaseS = GeomAbs_C3;
        break;
      default:
        BaseS = GeomAbs_CN;
    }
    GeomAdaptor_Curve C(Handle(Geom_OffsetCurve)::DownCast(myCurve)->BasisCurve(), myFirst, myLast);
    // akm 05/04/02 (OCC278)  If our curve is trimmed we must recalculate
    //                    the number of intervals obtained from the basis to
    //              vvv   reflect parameter bounds
    int iNbBasisInt = C.NbIntervals(BaseS), iInt;
    if (iNbBasisInt > 1)
    {
      TColStd_Array1OfReal rdfInter(1, 1 + iNbBasisInt);
      C.Intervals(rdfInter, BaseS);
      for (iInt = 1; iInt <= iNbBasisInt; iInt++)
        if (rdfInter(iInt) > myFirst && rdfInter(iInt) < myLast)
          myNbIntervals++;
    }
    // akm 05/04/02 ^^^
    return myNbIntervals;
  }

  else
  {
    return 1;
  }
}

//==================================================================================================

void GeomAdaptor_Curve::Intervals(TColStd_Array1OfReal& T, GeomAbs_Shape S) const
{
  if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    if ((!myBSplineCurve->IsPeriodic() && S <= Continuity()) || S == GeomAbs_C0)
    {
      T(T.Lower())     = myFirst;
      T(T.Lower() + 1) = myLast;
      return;
    }

    int aDegree = myBSplineCurve->Degree();
    int aCont;

    switch (S)
    {
      case GeomAbs_C1:
        aCont = 1;
        break;
      case GeomAbs_C2:
        aCont = 2;
        break;
      case GeomAbs_C3:
        aCont = 3;
        break;
      case GeomAbs_CN:
        aCont = aDegree;
        break;
      default:
        throw Standard_DomainError("GeomAdaptor_Curve::Intervals()");
    }

    double anEps = std::min(Resolution(Precision::Confusion()), Precision::PConfusion());

    BSplCLib::Intervals(myBSplineCurve->Knots(),
                        myBSplineCurve->Multiplicities(),
                        aDegree,
                        myBSplineCurve->IsPeriodic(),
                        aCont,
                        myFirst,
                        myLast,
                        anEps,
                        &T);
  }

  else if (myTypeCurve == GeomAbs_OffsetCurve)
  {
    int           myNbIntervals = 1;
    GeomAbs_Shape BaseS         = GeomAbs_C0;
    switch (S)
    {
      case GeomAbs_G1:
      case GeomAbs_G2:
        throw Standard_DomainError("GeomAdaptor_Curve::NbIntervals");
        break;
      case GeomAbs_C0:
        BaseS = GeomAbs_C1;
        break;
      case GeomAbs_C1:
        BaseS = GeomAbs_C2;
        break;
      case GeomAbs_C2:
        BaseS = GeomAbs_C3;
        break;
      default:
        BaseS = GeomAbs_CN;
    }
    GeomAdaptor_Curve C(Handle(Geom_OffsetCurve)::DownCast(myCurve)->BasisCurve(), myFirst, myLast);
    // akm 05/04/02 (OCC278)  If our curve is trimmed we must recalculate
    //                    the array of intervals obtained from the basis to
    //              vvv   reflect parameter bounds
    int iNbBasisInt = C.NbIntervals(BaseS), iInt;
    if (iNbBasisInt > 1)
    {
      TColStd_Array1OfReal rdfInter(1, 1 + iNbBasisInt);
      C.Intervals(rdfInter, BaseS);
      for (iInt = 1; iInt <= iNbBasisInt; iInt++)
        if (rdfInter(iInt) > myFirst && rdfInter(iInt) < myLast)
          T(++myNbIntervals) = rdfInter(iInt);
    }
    // old - myNbIntervals = C.NbIntervals(BaseS);
    // old - C.Intervals(T, BaseS);
    // akm 05/04/02 ^^^
    T(T.Lower())                 = myFirst;
    T(T.Lower() + myNbIntervals) = myLast;
  }

  else
  {
    T(T.Lower())     = myFirst;
    T(T.Lower() + 1) = myLast;
  }
}

//==================================================================================================

Handle(Adaptor3d_Curve) GeomAdaptor_Curve::Trim(const Standard_Real First,
                                                 const Standard_Real Last,
                                                 const Standard_Real Tol) const
{
  return new GeomAdaptor_Curve(TrimByValue(First, Last, Tol));
}

//==================================================================================================

GeomAdaptor_Curve GeomAdaptor_Curve::TrimByValue(double First, double Last, double /*Tol*/) const
{
  GeomAdaptor_Curve aResult = Copy();
  aResult.myFirst           = First;
  aResult.myLast            = Last;
  return aResult;
}

//==================================================================================================

bool GeomAdaptor_Curve::IsClosed() const
{
  if (!Precision::IsPositiveInfinite(myLast) && !Precision::IsNegativeInfinite(myFirst))
  {
    const gp_Pnt Pd = Value(myFirst);
    const gp_Pnt Pf = Value(myLast);
    return (Pd.Distance(Pf) <= Precision::Confusion());
  }
  return false;
}

//==================================================================================================

bool GeomAdaptor_Curve::IsPeriodic() const
{
  if (myCurve.IsNull())
    return false;
  return myCurve->IsPeriodic();
}

//==================================================================================================

double GeomAdaptor_Curve::Period() const
{
  return myCurve->LastParameter() - myCurve->FirstParameter();
}

//==================================================================================================

void GeomAdaptor_Curve::RebuildCache(double theParameter) const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
  {
    // Create cache for Bezier
    Handle(Geom_BezierCurve) aBezier = Handle(Geom_BezierCurve)::DownCast(myCurve);
    int                      aDeg    = aBezier->Degree();
    TColStd_Array1OfReal     aFlatKnots(BSplCLib::FlatBezierKnots(aDeg), 1, 2 * (aDeg + 1));
    if (myCurveCache.IsNull())
      myCurveCache = new BSplCLib_Cache(aDeg,
                                        aBezier->IsPeriodic(),
                                        aFlatKnots,
                                        aBezier->Poles(),
                                        aBezier->Weights());
    myCurveCache->BuildCache(theParameter, aFlatKnots, aBezier->Poles(), aBezier->Weights());
  }
  else if (myTypeCurve == GeomAbs_BSplineCurve)
  {
    // Create cache for B-spline
    if (myCurveCache.IsNull())
      myCurveCache = new BSplCLib_Cache(myBSplineCurve->Degree(),
                                        myBSplineCurve->IsPeriodic(),
                                        myBSplineCurve->KnotSequence(),
                                        myBSplineCurve->Poles(),
                                        myBSplineCurve->Weights());
    myCurveCache->BuildCache(theParameter,
                             myBSplineCurve->KnotSequence(),
                             myBSplineCurve->Poles(),
                             myBSplineCurve->Weights());
  }
}

//==================================================================================================

bool GeomAdaptor_Curve::IsBoundary(double theU, int& theSpanStart, int& theSpanFinish) const
{
  if (!myBSplineCurve.IsNull() && (theU == myFirst || theU == myLast))
  {
    if (theU == myFirst)
    {
      myBSplineCurve->LocateU(myFirst, PosTol, theSpanStart, theSpanFinish);
      if (theSpanStart < 1)
        theSpanStart = 1;
      if (theSpanStart >= theSpanFinish)
        theSpanFinish = theSpanStart + 1;
    }
    else if (theU == myLast)
    {
      myBSplineCurve->LocateU(myLast, PosTol, theSpanStart, theSpanFinish);
      if (theSpanFinish > myBSplineCurve->NbKnots())
        theSpanFinish = myBSplineCurve->NbKnots();
      if (theSpanStart >= theSpanFinish)
        theSpanStart = theSpanFinish - 1;
    }
    return true;
  }
  return false;
}

//==================================================================================================

void GeomAdaptor_Curve::evaluateD0(double U, gp_Pnt& P) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
    case GeomAbs_BSplineCurve:
    {
      int aStart = 0, aFinish = 0;
      if (IsBoundary(U, aStart, aFinish))
      {
        myBSplineCurve->LocalD0(U, aStart, aFinish, P);
      }
      else
      {
        // use cached data
        if (myCurveCache.IsNull() || !myCurveCache->IsCacheValid(U))
          RebuildCache(U);
        myCurveCache->D0(U, P);
      }
      break;
    }

    case GeomAbs_OffsetCurve:
      myNestedEvaluator->D0(U, P);
      break;

    default:
      myCurve->D0(U, P);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::evaluateD1(double U, gp_Pnt& P, gp_Vec& V) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
    case GeomAbs_BSplineCurve:
    {
      int aStart = 0, aFinish = 0;
      if (IsBoundary(U, aStart, aFinish))
      {
        myBSplineCurve->LocalD1(U, aStart, aFinish, P, V);
      }
      else
      {
        // use cached data
        if (myCurveCache.IsNull() || !myCurveCache->IsCacheValid(U))
          RebuildCache(U);
        myCurveCache->D1(U, P, V);
      }
      break;
    }

    case GeomAbs_OffsetCurve:
      myNestedEvaluator->D1(U, P, V);
      break;

    default:
      myCurve->D1(U, P, V);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::evaluateD2(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
    case GeomAbs_BSplineCurve:
    {
      int aStart = 0, aFinish = 0;
      if (IsBoundary(U, aStart, aFinish))
      {
        myBSplineCurve->LocalD2(U, aStart, aFinish, P, V1, V2);
      }
      else
      {
        // use cached data
        if (myCurveCache.IsNull() || !myCurveCache->IsCacheValid(U))
          RebuildCache(U);
        myCurveCache->D2(U, P, V1, V2);
      }
      break;
    }

    case GeomAbs_OffsetCurve:
      myNestedEvaluator->D2(U, P, V1, V2);
      break;

    default:
      myCurve->D2(U, P, V1, V2);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::evaluateD3(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2, gp_Vec& V3) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
    case GeomAbs_BSplineCurve:
    {
      int aStart = 0, aFinish = 0;
      if (IsBoundary(U, aStart, aFinish))
      {
        myBSplineCurve->LocalD3(U, aStart, aFinish, P, V1, V2, V3);
      }
      else
      {
        // use cached data
        if (myCurveCache.IsNull() || !myCurveCache->IsCacheValid(U))
          RebuildCache(U);
        myCurveCache->D3(U, P, V1, V2, V3);
      }
      break;
    }

    case GeomAbs_OffsetCurve:
      myNestedEvaluator->D3(U, P, V1, V2, V3);
      break;

    default:
      myCurve->D3(U, P, V1, V2, V3);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::D0(double U, gp_Pnt& P) const
{
  // Check for modifiers first
  if (auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myModifier))
  {
    pCOS->D0(U, P);
    return;
  }

  if (auto* pIso = std::get_if<GeomAdaptor_IsoCurveModifier>(&myModifier))
  {
    pIso->D0(U, P);
    return;
  }

  // Evaluate base curve
  evaluateD0(U, P);

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    pTrsf->Transform(P);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::D1(double U, gp_Pnt& P, gp_Vec& V) const
{
  // Check for modifiers first
  if (auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myModifier))
  {
    pCOS->D1(U, P, V);
    return;
  }

  if (auto* pIso = std::get_if<GeomAdaptor_IsoCurveModifier>(&myModifier))
  {
    pIso->D1(U, P, V);
    return;
  }

  // Evaluate base curve
  evaluateD1(U, P, V);

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    pTrsf->Transform(P, V);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::D2(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2) const
{
  // Check for modifiers first
  if (auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myModifier))
  {
    pCOS->D2(U, P, V1, V2);
    return;
  }

  if (auto* pIso = std::get_if<GeomAdaptor_IsoCurveModifier>(&myModifier))
  {
    pIso->D2(U, P, V1, V2);
    return;
  }

  // Evaluate base curve
  evaluateD2(U, P, V1, V2);

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    pTrsf->Transform(P, V1, V2);
  }
}

//==================================================================================================

void GeomAdaptor_Curve::D3(double U, gp_Pnt& P, gp_Vec& V1, gp_Vec& V2, gp_Vec& V3) const
{
  // Check for modifiers first
  if (auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myModifier))
  {
    pCOS->D3(U, P, V1, V2, V3);
    return;
  }

  if (auto* pIso = std::get_if<GeomAdaptor_IsoCurveModifier>(&myModifier))
  {
    pIso->D3(U, P, V1, V2, V3);
    return;
  }

  // Evaluate base curve
  evaluateD3(U, P, V1, V2, V3);

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    pTrsf->Transform(P, V1, V2, V3);
  }
}

//==================================================================================================

gp_Vec GeomAdaptor_Curve::DN(double U, int N) const
{
  // Check for modifiers first
  if (auto* pCOS = std::get_if<GeomAdaptor_CurveOnSurfaceModifier>(&myModifier))
  {
    return pCOS->DN(U, N);
  }

  if (auto* pIso = std::get_if<GeomAdaptor_IsoCurveModifier>(&myModifier))
  {
    return pIso->DN(U, N);
  }

  gp_Vec aResult;

  switch (myTypeCurve)
  {
    case GeomAbs_BezierCurve:
    case GeomAbs_BSplineCurve:
    {
      int aStart = 0, aFinish = 0;
      if (IsBoundary(U, aStart, aFinish))
      {
        aResult = myBSplineCurve->LocalDN(U, aStart, aFinish, N);
      }
      else
        aResult = myCurve->DN(U, N);
      break;
    }

    case GeomAbs_OffsetCurve:
      aResult = myNestedEvaluator->DN(U, N);
      break;

    default:
      aResult = myCurve->DN(U, N);
  }

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    pTrsf->Transform(aResult);
  }

  return aResult;
}

//==================================================================================================

double GeomAdaptor_Curve::Resolution(double R3D) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_Line:
      return R3D;
    case GeomAbs_Circle:
    {
      double R = Handle(Geom_Circle)::DownCast(myCurve)->Circ().Radius();
      if (R > R3D / 2.)
        return 2 * std::asin(R3D / (2 * R));
      else
        return 2 * M_PI;
    }
    case GeomAbs_Ellipse:
    {
      return R3D / Handle(Geom_Ellipse)::DownCast(myCurve)->MajorRadius();
    }
    case GeomAbs_BezierCurve:
    {
      double res;
      Handle(Geom_BezierCurve)::DownCast(myCurve)->Resolution(R3D, res);
      return res;
    }
    case GeomAbs_BSplineCurve:
    {
      double res;
      myBSplineCurve->Resolution(R3D, res);
      return res;
    }
    default:
      return Precision::Parametric(R3D);
  }
}

//    --
//    --     The following methods must  be called when GetType returned
//    --     the corresponding type.
//    --

//==================================================================================================

gp_Lin GeomAdaptor_Curve::Line() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Line,
                                 "GeomAdaptor_Curve::Line() - curve is not a Line");

  gp_Lin aLine = Handle(Geom_Line)::DownCast(myCurve)->Lin();

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    return pTrsf->Transformed(aLine);
  }

  return aLine;
}

//==================================================================================================

gp_Circ GeomAdaptor_Curve::Circle() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Circle,
                                 "GeomAdaptor_Curve::Circle() - curve is not a Circle");

  gp_Circ aCirc = Handle(Geom_Circle)::DownCast(myCurve)->Circ();

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    return pTrsf->Transformed(aCirc);
  }

  return aCirc;
}

//==================================================================================================

gp_Elips GeomAdaptor_Curve::Ellipse() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Ellipse,
                                 "GeomAdaptor_Curve::Ellipse() - curve is not an Ellipse");

  gp_Elips anElips = Handle(Geom_Ellipse)::DownCast(myCurve)->Elips();

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    return pTrsf->Transformed(anElips);
  }

  return anElips;
}

//==================================================================================================

gp_Hypr GeomAdaptor_Curve::Hyperbola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Hyperbola,
                                 "GeomAdaptor_Curve::Hyperbola() - curve is not a Hyperbola");

  gp_Hypr aHypr = Handle(Geom_Hyperbola)::DownCast(myCurve)->Hypr();

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    return pTrsf->Transformed(aHypr);
  }

  return aHypr;
}

//==================================================================================================

gp_Parab GeomAdaptor_Curve::Parabola() const
{
  Standard_NoSuchObject_Raise_if(myTypeCurve != GeomAbs_Parabola,
                                 "GeomAdaptor_Curve::Parabola() - curve is not a Parabola");

  gp_Parab aParab = Handle(Geom_Parabola)::DownCast(myCurve)->Parab();

  // Apply transformation if present
  if (auto* pTrsf = std::get_if<GeomAdaptor_TrsfModifier>(&myModifier))
  {
    return pTrsf->Transformed(aParab);
  }

  return aParab;
}

//==================================================================================================

int GeomAdaptor_Curve::Degree() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom_BezierCurve)::DownCast(myCurve)->Degree();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return myBSplineCurve->Degree();
  else
    throw Standard_NoSuchObject();
}

//==================================================================================================

bool GeomAdaptor_Curve::IsRational() const
{
  switch (myTypeCurve)
  {
    case GeomAbs_BSplineCurve:
      return myBSplineCurve->IsRational();
    case GeomAbs_BezierCurve:
      return Handle(Geom_BezierCurve)::DownCast(myCurve)->IsRational();
    default:
      return false;
  }
}

//==================================================================================================

int GeomAdaptor_Curve::NbPoles() const
{
  if (myTypeCurve == GeomAbs_BezierCurve)
    return Handle(Geom_BezierCurve)::DownCast(myCurve)->NbPoles();
  else if (myTypeCurve == GeomAbs_BSplineCurve)
    return myBSplineCurve->NbPoles();
  else
    throw Standard_NoSuchObject();
}

//==================================================================================================

int GeomAdaptor_Curve::NbKnots() const
{
  if (myTypeCurve != GeomAbs_BSplineCurve)
    throw Standard_NoSuchObject("GeomAdaptor_Curve::NbKnots");
  return myBSplineCurve->NbKnots();
}

//==================================================================================================

Handle(Geom_BezierCurve) GeomAdaptor_Curve::Bezier() const
{
  if (myTypeCurve != GeomAbs_BezierCurve)
    throw Standard_NoSuchObject("GeomAdaptor_Curve::Bezier");
  return Handle(Geom_BezierCurve)::DownCast(myCurve);
}

//==================================================================================================

Handle(Geom_BSplineCurve) GeomAdaptor_Curve::BSpline() const
{
  if (myTypeCurve != GeomAbs_BSplineCurve)
    throw Standard_NoSuchObject("GeomAdaptor_Curve::BSpline");

  return myBSplineCurve;
}

//==================================================================================================

Handle(Geom_OffsetCurve) GeomAdaptor_Curve::OffsetCurve() const
{
  if (myTypeCurve != GeomAbs_OffsetCurve)
    throw Standard_NoSuchObject("GeomAdaptor_Curve::OffsetCurve");
  return Handle(Geom_OffsetCurve)::DownCast(myCurve);
}
