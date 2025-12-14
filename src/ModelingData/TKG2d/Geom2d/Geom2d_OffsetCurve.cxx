// Created on: 1991-06-25
// Created by: JCV
// Copyright (c) 1991-1999 Matra Datavision
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

//  modified by Edward AGAPOV (eap) Jan 28 2002 --- DN(), occ143(BUC60654)

#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Geometry.hxx>
#include <Geom2d_OffsetCurve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2d_UndefinedDerivative.hxx>
#include <gp.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf2d.hxx>
#include <gp_Vec2d.hxx>
#include <gp_XY.hxx>
#include <Precision.hxx>
#include <Standard_ConstructionError.hxx>
#include <Standard_NotImplemented.hxx>
#include <Standard_NullValue.hxx>
#include <Standard_RangeError.hxx>
#include <Standard_Type.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom2d_OffsetCurve, Geom2d_Curve)

static const Standard_Real MyAngularToleranceForG1 = Precision::Angular();

//=================================================================================================

Handle(Geom2d_Geometry) Geom2d_OffsetCurve::Copy() const
{
  return new Geom2d_OffsetCurve(*this);
}

//=======================================================================
// function : Geom2d_OffsetCurve
// purpose  : Basis curve cannot be an Offset curve or trimmed from
//            offset curve.
//=======================================================================

Geom2d_OffsetCurve::Geom2d_OffsetCurve(const Handle(Geom2d_Curve)& theCurve,
                                       const Standard_Real         theOffset,
                                       const Standard_Boolean      isTheNotCheckC0)
    : offsetValue(theOffset)
{
  SetBasisCurve(theCurve, isTheNotCheckC0);
}

//=================================================================================================

Geom2d_OffsetCurve::Geom2d_OffsetCurve(const Geom2d_OffsetCurve& theOther)
    : offsetValue(theOther.offsetValue),
      myBasisCurveContinuity(theOther.myBasisCurveContinuity)
{
  // Deep copy basis curve without validation
  basisCurve = Handle(Geom2d_Curve)::DownCast(theOther.basisCurve->Copy());
}

//=================================================================================================

void Geom2d_OffsetCurve::Reverse()
{
  basisCurve->Reverse();
  offsetValue = -offsetValue;
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::ReversedParameter(const Standard_Real U) const
{
  return basisCurve->ReversedParameter(U);
}

//=================================================================================================

void Geom2d_OffsetCurve::SetBasisCurve(const Handle(Geom2d_Curve)& C,
                                       const Standard_Boolean      isNotCheckC0)
{
  const Standard_Real  aUf = C->FirstParameter(), aUl = C->LastParameter();
  Handle(Geom2d_Curve) aCheckingCurve = C;
  Standard_Boolean     isTrimmed      = Standard_False;

  while (aCheckingCurve->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve))
         || aCheckingCurve->IsKind(STANDARD_TYPE(Geom2d_OffsetCurve)))
  {
    if (aCheckingCurve->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve)))
    {
      Handle(Geom2d_TrimmedCurve) aTrimC = Handle(Geom2d_TrimmedCurve)::DownCast(aCheckingCurve);
      aCheckingCurve                     = aTrimC->BasisCurve();
      isTrimmed                          = Standard_True;
    }

    if (aCheckingCurve->IsKind(STANDARD_TYPE(Geom2d_OffsetCurve)))
    {
      Handle(Geom2d_OffsetCurve) aOC = Handle(Geom2d_OffsetCurve)::DownCast(aCheckingCurve);
      aCheckingCurve                 = aOC->BasisCurve();
      offsetValue += aOC->Offset();
    }
  }

  myBasisCurveContinuity = aCheckingCurve->Continuity();

  Standard_Boolean isC0 = !isNotCheckC0 && (myBasisCurveContinuity == GeomAbs_C0);

  // Basis curve must be at least C1
  if (isC0 && aCheckingCurve->IsKind(STANDARD_TYPE(Geom2d_BSplineCurve)))
  {
    Handle(Geom2d_BSplineCurve) aBC = Handle(Geom2d_BSplineCurve)::DownCast(aCheckingCurve);
    if (aBC->IsG1(aUf, aUl, MyAngularToleranceForG1))
    {
      // Checking if basis curve has more smooth (C1, G2 and above) is not done.
      // It can be done in case of need.
      myBasisCurveContinuity = GeomAbs_G1;
      isC0                   = Standard_False;
    }

    // Raise exception if still C0
    if (isC0)
      throw Standard_ConstructionError("Offset on C0 curve");
  }
  //
  if (isTrimmed)
  {
    basisCurve = new Geom2d_TrimmedCurve(aCheckingCurve, aUf, aUl);
  }
  else
  {
    basisCurve = aCheckingCurve;
  }
}

//=================================================================================================

void Geom2d_OffsetCurve::SetOffsetValue(const Standard_Real D)
{
  offsetValue = D;
}

//=================================================================================================

Handle(Geom2d_Curve) Geom2d_OffsetCurve::BasisCurve() const
{
  return basisCurve;
}

//=================================================================================================

GeomAbs_Shape Geom2d_OffsetCurve::Continuity() const
{
  GeomAbs_Shape OffsetShape = GeomAbs_C0;
  switch (myBasisCurveContinuity)
  {
    case GeomAbs_C0:
      OffsetShape = GeomAbs_C0;
      break;
    case GeomAbs_C1:
      OffsetShape = GeomAbs_C0;
      break;
    case GeomAbs_C2:
      OffsetShape = GeomAbs_C1;
      break;
    case GeomAbs_C3:
      OffsetShape = GeomAbs_C2;
      break;
    case GeomAbs_CN:
      OffsetShape = GeomAbs_CN;
      break;
    case GeomAbs_G1:
      OffsetShape = GeomAbs_G1;
      break;
    case GeomAbs_G2:
      OffsetShape = GeomAbs_G2;
      break;
  }

  return OffsetShape;
}

//=================================================================================================

void Geom2d_OffsetCurve::D0(const Standard_Real theU, gp_Pnt2d& theP) const
{
  gp_Vec2d aD1;
  basisCurve->D1(theU, theP, aD1);
  calculateD0(theP, aD1, offsetValue);
}

//=================================================================================================

void Geom2d_OffsetCurve::D1(const Standard_Real theU, gp_Pnt2d& theP, gp_Vec2d& theV1) const
{
  gp_Vec2d aD2;
  basisCurve->D2(theU, theP, theV1, aD2);
  calculateD1(theP, theV1, aD2, offsetValue);
}

//=================================================================================================

void Geom2d_OffsetCurve::D2(const Standard_Real theU,
                            gp_Pnt2d&           theP,
                            gp_Vec2d&           theV1,
                            gp_Vec2d&           theV2) const
{
  gp_Vec2d aD3;
  basisCurve->D3(theU, theP, theV1, theV2, aD3);

  bool isDirectionChange = false;
  if (theV1.SquareMagnitude() <= gp::Resolution())
  {
    gp_Vec2d aDummyD4;
    isDirectionChange = adjustDerivative(3, theU, theV1, theV2, aD3, aDummyD4);
  }

  calculateD2(theP, theV1, theV2, aD3, isDirectionChange, offsetValue);
}

//=================================================================================================

void Geom2d_OffsetCurve::D3(const Standard_Real theU,
                            gp_Pnt2d&           theP,
                            gp_Vec2d&           theV1,
                            gp_Vec2d&           theV2,
                            gp_Vec2d&           theV3) const
{
  basisCurve->D3(theU, theP, theV1, theV2, theV3);
  gp_Vec2d aD4 = basisCurve->DN(theU, 4);

  bool isDirectionChange = false;
  if (theV1.SquareMagnitude() <= gp::Resolution())
    isDirectionChange = adjustDerivative(4, theU, theV1, theV2, theV3, aD4);

  calculateD3(theP, theV1, theV2, theV3, aD4, isDirectionChange, offsetValue);
}

//=================================================================================================

gp_Vec2d Geom2d_OffsetCurve::DN(const Standard_Real U, const Standard_Integer N) const
{
  Standard_RangeError_Raise_if(N < 1, "Exception: Geom2d_OffsetCurve::DN(). N<1.");

  gp_Vec2d VN, VBidon;
  gp_Pnt2d PBidon;
  switch (N)
  {
    case 1:
      D1(U, PBidon, VN);
      break;
    case 2:
      D2(U, PBidon, VBidon, VN);
      break;
    case 3:
      D3(U, PBidon, VBidon, VBidon, VN);
      break;
    default:
      throw Standard_NotImplemented("Exception: Derivative order is greater than 3. "
                                    "Cannot compute of derivative.");
  }

  return VN;
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::FirstParameter() const
{
  return basisCurve->FirstParameter();
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::LastParameter() const
{
  return basisCurve->LastParameter();
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::Offset() const
{
  return offsetValue;
}

//=================================================================================================

Standard_Boolean Geom2d_OffsetCurve::IsClosed() const
{
  gp_Pnt2d PF, PL;
  D0(FirstParameter(), PF);
  D0(LastParameter(), PL);
  return (PF.Distance(PL) <= gp::Resolution());
}

//=================================================================================================

Standard_Boolean Geom2d_OffsetCurve::IsCN(const Standard_Integer N) const
{
  Standard_RangeError_Raise_if(N < 0, " ");
  return basisCurve->IsCN(N + 1);
}

//=================================================================================================

Standard_Boolean Geom2d_OffsetCurve::IsPeriodic() const
{
  return basisCurve->IsPeriodic();
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::Period() const
{
  return basisCurve->Period();
}

//=================================================================================================

void Geom2d_OffsetCurve::Transform(const gp_Trsf2d& T)
{
  basisCurve->Transform(T);
  offsetValue *= std::abs(T.ScaleFactor());
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::TransformedParameter(const Standard_Real U,
                                                       const gp_Trsf2d&    T) const
{
  return basisCurve->TransformedParameter(U, T);
}

//=================================================================================================

Standard_Real Geom2d_OffsetCurve::ParametricTransformation(const gp_Trsf2d& T) const
{
  return basisCurve->ParametricTransformation(T);
}

//=================================================================================================

GeomAbs_Shape Geom2d_OffsetCurve::GetBasisCurveContinuity() const
{
  return myBasisCurveContinuity;
}

//=================================================================================================

void Geom2d_OffsetCurve::DumpJson(Standard_OStream& theOStream, Standard_Integer theDepth) const
{
  OCCT_DUMP_TRANSIENT_CLASS_BEGIN(theOStream)

  OCCT_DUMP_BASE_CLASS(theOStream, theDepth, Geom2d_Curve)

  OCCT_DUMP_FIELD_VALUES_DUMPED(theOStream, theDepth, basisCurve.get())

  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, offsetValue)
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myBasisCurveContinuity)
}

//=================================================================================================

void Geom2d_OffsetCurve::calculateD0(gp_Pnt2d&       theValue,
                                     const gp_Vec2d& theD1,
                                     double          theOffset)
{
  if (theD1.SquareMagnitude() <= gp::Resolution())
    throw Standard_NullValue("Geom2d_OffsetCurve: Undefined normal vector "
                             "because tangent vector has zero-magnitude!");

  gp_Dir2d aNormal(theD1.Y(), -theD1.X());
  theValue.ChangeCoord().Add(aNormal.XY() * theOffset);
}

//=================================================================================================

void Geom2d_OffsetCurve::calculateD1(gp_Pnt2d&       theValue,
                                     gp_Vec2d&       theD1,
                                     const gp_Vec2d& theD2,
                                     double          theOffset)
{
  // P(u) = p(u) + Offset * Ndir / R
  // with R = || p' ^ Z|| and Ndir = P' ^ Z
  // P'(u) = p'(u) + (Offset / R**2) * (DNdir/DU * R -  Ndir * (DR/R))

  gp_XY         Ndir(theD1.Y(), -theD1.X());
  gp_XY         DNdir(theD2.Y(), -theD2.X());
  Standard_Real R2 = Ndir.SquareModulus();
  Standard_Real R  = std::sqrt(R2);
  Standard_Real R3 = R * R2;
  Standard_Real Dr = Ndir.Dot(DNdir);
  if (R3 <= gp::Resolution())
  {
    if (R2 <= gp::Resolution())
      throw Standard_NullValue("Geom2d_OffsetCurve: Null derivative");
    // We try another computation but the stability is not very good.
    DNdir.Multiply(R);
    DNdir.Subtract(Ndir.Multiplied(Dr / R));
    DNdir.Multiply(theOffset / R2);
  }
  else
  {
    // Same computation as IICURV in EUCLID-IS because the stability is better
    DNdir.Multiply(theOffset / R);
    DNdir.Subtract(Ndir.Multiplied(theOffset * Dr / R3));
  }

  Ndir.Multiply(theOffset / R);
  // P(u)
  theValue.ChangeCoord().Add(Ndir);
  // P'(u)
  theD1.Add(gp_Vec2d(DNdir));
}

//=================================================================================================

void Geom2d_OffsetCurve::calculateD2(gp_Pnt2d&       theValue,
                                     gp_Vec2d&       theD1,
                                     gp_Vec2d&       theD2,
                                     const gp_Vec2d& theD3,
                                     bool            theIsDirChange,
                                     double          theOffset)
{
  // P(u) = p(u) + Offset * Ndir / R
  // with R = || p' ^ Z|| and Ndir = P' ^ Z
  // P'(u) = p'(u) + (Offset / R**2) * (DNdir/DU * R -  Ndir * (DR/R))
  // P"(u) = p"(u) + (Offset / R) * (D2Ndir/DU - DNdir * (2.0 * Dr/ R**2) +
  //         Ndir * ( (3.0 * Dr**2 / R**4) - (D2r / R**2)))

  gp_XY         Ndir(theD1.Y(), -theD1.X());
  gp_XY         DNdir(theD2.Y(), -theD2.X());
  gp_XY         D2Ndir(theD3.Y(), -theD3.X());
  Standard_Real R2  = Ndir.SquareModulus();
  Standard_Real R   = std::sqrt(R2);
  Standard_Real R3  = R2 * R;
  Standard_Real R4  = R2 * R2;
  Standard_Real R5  = R3 * R2;
  Standard_Real Dr  = Ndir.Dot(DNdir);
  Standard_Real D2r = Ndir.Dot(D2Ndir) + DNdir.Dot(DNdir);
  if (R5 <= gp::Resolution())
  {
    if (R4 <= gp::Resolution())
      throw Standard_NullValue("Geom2d_OffsetCurve: Null derivative");
    // We try another computation but the stability is not very good dixit ISG.
    //  V2 = P" (U) :
    D2Ndir.Subtract(DNdir.Multiplied(2.0 * Dr / R2));
    D2Ndir.Add(Ndir.Multiplied(((3.0 * Dr * Dr) / R4) - (D2r / R2)));
    D2Ndir.Multiply(theOffset / R);

    // V1 = P' (U) :
    DNdir.Multiply(R);
    DNdir.Subtract(Ndir.Multiplied(Dr / R));
    DNdir.Multiply(theOffset / R2);
  }
  else
  {
    // Same computation as IICURV in EUCLID-IS because the stability is better.
    // V2 = P" (U) :
    D2Ndir.Multiply(theOffset / R);
    D2Ndir.Subtract(DNdir.Multiplied(2.0 * theOffset * Dr / R3));
    D2Ndir.Add(Ndir.Multiplied(theOffset * (((3.0 * Dr * Dr) / R5) - (D2r / R3))));

    // V1 = P' (U)
    DNdir.Multiply(theOffset / R);
    DNdir.Subtract(Ndir.Multiplied(theOffset * Dr / R3));
  }

  Ndir.Multiply(theOffset / R);
  // P(u)
  theValue.ChangeCoord().Add(Ndir);
  // P'(u) :
  theD1.Add(gp_Vec2d(DNdir));
  // P"(u) :
  if (theIsDirChange)
    theD2.Reverse();
  theD2.Add(gp_Vec2d(D2Ndir));
}

//=================================================================================================

void Geom2d_OffsetCurve::calculateD3(gp_Pnt2d&       theValue,
                                     gp_Vec2d&       theD1,
                                     gp_Vec2d&       theD2,
                                     gp_Vec2d&       theD3,
                                     const gp_Vec2d& theD4,
                                     bool            theIsDirChange,
                                     double          theOffset)
{
  // P(u) = p(u) + Offset * Ndir / R
  // with R = || p' ^ Z|| and Ndir = P' ^ Z
  // P'(u)  = p'(u) + (Offset / R**2) * (DNdir/DU * R -  Ndir * (DR/R))
  // P"(u)  = p"(u) + (Offset / R) * (D2Ndir/DU - DNdir * (2.0 * Dr/ R**2) +
  //          Ndir * ( (3.0 * Dr**2 / R**4) - (D2r / R**2)))
  // P"'(u) = p"'(u) + (Offset / R) * (D3Ndir - (3.0 * Dr/R**2 ) * D2Ndir -
  //          (3.0 * D2r / R2) * DNdir) + (3.0 * Dr * Dr / R4) * DNdir -
  //          (D3r/R2) * Ndir + (6.0 * Dr * Dr / R4) * Ndir +
  //          (6.0 * Dr * D2r / R4) * Ndir - (15.0 * Dr* Dr* Dr /R6) * Ndir

  gp_XY         Ndir(theD1.Y(), -theD1.X());
  gp_XY         DNdir(theD2.Y(), -theD2.X());
  gp_XY         D2Ndir(theD3.Y(), -theD3.X());
  gp_XY         D3Ndir(theD4.Y(), -theD4.X());
  Standard_Real R2  = Ndir.SquareModulus();
  Standard_Real R   = std::sqrt(R2);
  Standard_Real R3  = R2 * R;
  Standard_Real R4  = R2 * R2;
  Standard_Real R5  = R3 * R2;
  Standard_Real R6  = R3 * R3;
  Standard_Real R7  = R5 * R2;
  Standard_Real Dr  = Ndir.Dot(DNdir);
  Standard_Real D2r = Ndir.Dot(D2Ndir) + DNdir.Dot(DNdir);
  Standard_Real D3r = Ndir.Dot(D3Ndir) + 3.0 * DNdir.Dot(D2Ndir);

  if (R7 <= gp::Resolution())
  {
    if (R6 <= gp::Resolution())
      throw Standard_NullValue("Geom2d_OffsetCurve: Null derivative");
    // We try another computation but the stability is not very good dixit ISG.
    //  V3 = P"' (U) :
    D3Ndir.Subtract(D2Ndir.Multiplied(3.0 * theOffset * Dr / R2));
    D3Ndir.Subtract((DNdir.Multiplied((3.0 * theOffset) * ((D2r / R2) + (Dr * Dr) / R4))));
    D3Ndir.Add(Ndir.Multiplied(
      (theOffset * (6.0 * Dr * Dr / R4 + 6.0 * Dr * D2r / R4 - 15.0 * Dr * Dr * Dr / R6 - D3r))));
    D3Ndir.Multiply(theOffset / R);
    // V2 = P" (U) :
    R4 = R2 * R2;
    D2Ndir.Subtract(DNdir.Multiplied(2.0 * Dr / R2));
    D2Ndir.Subtract(Ndir.Multiplied(((3.0 * Dr * Dr) / R4) - (D2r / R2)));
    D2Ndir.Multiply(theOffset / R);
    // V1 = P' (U) :
    DNdir.Multiply(R);
    DNdir.Subtract(Ndir.Multiplied(Dr / R));
    DNdir.Multiply(theOffset / R2);
  }
  else
  {
    // Same computation as IICURV in EUCLID-IS because the stability is better.
    // V3 = P"' (U) :
    D3Ndir.Multiply(theOffset / R);
    D3Ndir.Subtract(D2Ndir.Multiplied(3.0 * theOffset * Dr / R3));
    D3Ndir.Subtract(DNdir.Multiplied(((3.0 * theOffset) * ((D2r / R3) + (Dr * Dr) / R5))));
    D3Ndir.Add(Ndir.Multiplied(
      (theOffset * (6.0 * Dr * Dr / R5 + 6.0 * Dr * D2r / R5 - 15.0 * Dr * Dr * Dr / R7 - D3r))));
    // V2 = P" (U) :
    D2Ndir.Multiply(theOffset / R);
    D2Ndir.Subtract(DNdir.Multiplied(2.0 * theOffset * Dr / R3));
    D2Ndir.Subtract(Ndir.Multiplied(theOffset * (((3.0 * Dr * Dr) / R5) - (D2r / R3))));
    // V1 = P' (U) :
    DNdir.Multiply(theOffset / R);
    DNdir.Subtract(Ndir.Multiplied(theOffset * Dr / R3));
  }

  Ndir.Multiply(theOffset / R);
  // P(u)
  theValue.ChangeCoord().Add(Ndir);
  // P'(u) :
  theD1.Add(gp_Vec2d(DNdir));
  // P"(u)
  theD2.Add(gp_Vec2d(D2Ndir));
  // P"'(u)
  if (theIsDirChange)
    theD3.Reverse();
  theD3.Add(gp_Vec2d(D2Ndir));
}

//=================================================================================================

bool Geom2d_OffsetCurve::adjustDerivative(int       theMaxDerivative,
                                          double    theU,
                                          gp_Vec2d& theD1,
                                          gp_Vec2d& theD2,
                                          gp_Vec2d& theD3,
                                          gp_Vec2d& theD4) const
{
  static const Standard_Real    aTol           = gp::Resolution();
  static const Standard_Real    aMinStep       = 1e-7;
  static const Standard_Integer aMaxDerivOrder = 3;

  bool          isDirectionChange = false;
  Standard_Real anUinfium         = basisCurve->FirstParameter();
  Standard_Real anUsupremum       = basisCurve->LastParameter();

  static const Standard_Real DivisionFactor = 1.e-3;
  Standard_Real              du;
  if ((anUsupremum >= RealLast()) || (anUinfium <= RealFirst()))
    du = 0.0;
  else
    du = anUsupremum - anUinfium;

  const Standard_Real aDelta = std::max(du * DivisionFactor, aMinStep);

  // Derivative is approximated by Taylor-series
  Standard_Integer anIndex = 1; // Derivative order
  gp_Vec2d         V;

  do
  {
    V = basisCurve->DN(theU, ++anIndex);
  } while ((V.SquareMagnitude() <= aTol) && anIndex < aMaxDerivOrder);

  Standard_Real u;

  if (theU - anUinfium < aDelta)
    u = theU + aDelta;
  else
    u = theU - aDelta;

  gp_Pnt2d P1, P2;
  basisCurve->D0(std::min(theU, u), P1);
  basisCurve->D0(std::max(theU, u), P2);

  gp_Vec2d V1(P1, P2);
  isDirectionChange       = V.Dot(V1) < 0.0;
  Standard_Real aSign     = isDirectionChange ? -1.0 : 1.0;

  theD1                   = V * aSign;
  gp_Vec2d* aDeriv[3]     = {&theD2, &theD3, &theD4};
  for (Standard_Integer i = 1; i < theMaxDerivative; i++)
    *(aDeriv[i - 1]) = basisCurve->DN(theU, anIndex + i) * aSign;

  return isDirectionChange;
}
