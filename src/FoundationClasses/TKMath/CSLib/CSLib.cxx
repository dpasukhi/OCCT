// Created on: 1991-09-09
// Created by: Michel Chauvat
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

#include <CSLib.hxx>
#include <CSLib_NormalPolyDef.hxx>
#include <gp.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <math_FunctionRoots.hxx>
#include <PLib.hxx>
#include <Precision.hxx>
#include <TColgp_Array2OfVec.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array2OfReal.hxx>

//=================================================================================================

void CSLib::Normal(const gp_Vec&           D1U,
                   const gp_Vec&           D1V,
                   const Standard_Real     SinTol,
                   CSLib_DerivativeStatus& theStatus,
                   gp_Dir&                 Normal)
{

  // Function: Calculation of the normal from tangents by u and by v.

  Standard_Real D1UMag  = D1U.SquareMagnitude();
  Standard_Real D1VMag  = D1V.SquareMagnitude();
  gp_Vec        D1UvD1V = D1U.Crossed(D1V);

  if (D1UMag <= gp::Resolution() && D1VMag <= gp::Resolution())
  {
    theStatus = CSLib_D1IsNull;
  }
  else if (D1UMag <= gp::Resolution())
    theStatus = CSLib_D1uIsNull;
  else if (D1VMag <= gp::Resolution())
    theStatus = CSLib_D1vIsNull;
  //  else if ((D1VMag / D1UMag) <= RealEpsilon())   theStatus = CSLib_D1vD1uRatioIsNull;
  //  else if ((D1UMag / D1VMag) <= RealEpsilon())   theStatus = CSLib_D1uD1vRatioIsNull;
  else
  {
    Standard_Real Sin2 = D1UvD1V.SquareMagnitude() / (D1UMag * D1VMag);

    if (Sin2 < (SinTol * SinTol))
    {
      theStatus = CSLib_D1uIsParallelD1v;
    }
    else
    {
      Normal    = gp_Dir(D1UvD1V);
      theStatus = CSLib_Done;
    }
  }
}

//=================================================================================================

void CSLib::Normal(const gp_Vec&       D1U,
                   const gp_Vec&       D1V,
                   const gp_Vec&       D2U,
                   const gp_Vec&       D2V,
                   const gp_Vec&       DUV,
                   const Standard_Real SinTol,
                   Standard_Boolean&   Done,
                   CSLib_NormalStatus& theStatus,
                   gp_Dir&             Normal)
{

  //  Calculation of an approximate normale in case of a null normal.
  //  Use limited development of the normal of order 1:
  //     N(u0+du,v0+dv) = N0 + dN/du(u0,v0) * du + dN/dv(u0,v0) * dv + epsilon
  //  -> N ~ dN/du + dN/dv.

  gp_Vec D1Nu = D2U.Crossed(D1V);
  D1Nu.Add(D1U.Crossed(DUV));

  gp_Vec D1Nv = DUV.Crossed(D1V);
  D1Nv.Add(D1U.Crossed(D2V));

  Standard_Real LD1Nu = D1Nu.SquareMagnitude();
  Standard_Real LD1Nv = D1Nv.SquareMagnitude();

  if (LD1Nu <= RealEpsilon() && LD1Nv <= RealEpsilon())
  {
    theStatus = CSLib_D1NIsNull;
    Done      = Standard_False;
  }
  else if (LD1Nu < RealEpsilon())
  {
    theStatus = CSLib_D1NuIsNull;
    Done      = Standard_True;
    Normal    = gp_Dir(D1Nv);
  }
  else if (LD1Nv < RealEpsilon())
  {
    theStatus = CSLib_D1NvIsNull;
    Done      = Standard_True;
    Normal    = gp_Dir(D1Nu);
  }
  else if ((LD1Nv / LD1Nu) <= RealEpsilon())
  {
    theStatus = CSLib_D1NvNuRatioIsNull;
    Done      = Standard_False;
  }
  else if ((LD1Nu / LD1Nv) <= RealEpsilon())
  {
    theStatus = CSLib_D1NuNvRatioIsNull;
    Done      = Standard_False;
  }
  else
  {
    gp_Vec        D1NCross = D1Nu.Crossed(D1Nv);
    Standard_Real Sin2     = D1NCross.SquareMagnitude() / (LD1Nu * LD1Nv);

    if (Sin2 < (SinTol * SinTol))
    {
      theStatus = CSLib_D1NuIsParallelD1Nv;
      Done      = Standard_True;
      Normal    = gp_Dir(D1Nu);
    }
    else
    {
      theStatus = CSLib_InfinityOfSolutions;
      Done      = Standard_False;
    }
  }
}

//=================================================================================================

void CSLib::Normal(const gp_Vec&       D1U,
                   const gp_Vec&       D1V,
                   const Standard_Real MagTol,
                   CSLib_NormalStatus& theStatus,
                   gp_Dir&             Normal)
{
  // Function: Calculate the normal from tangents by u and by v.

  // Optimize by comparing squared magnitudes first to avoid sqrt() calls
  const Standard_Real MagTol2 = MagTol * MagTol;
  const Standard_Real D1UMag2 = D1U.SquareMagnitude();
  const Standard_Real D1VMag2 = D1V.SquareMagnitude();

  // Check if tangent vectors are too small
  if (D1UMag2 <= MagTol2 || D1VMag2 <= MagTol2)
  {
    theStatus = CSLib_Singular;
    return;
  }

  gp_Vec              D1UvD1V = D1U.Crossed(D1V);
  const Standard_Real NMag2   = D1UvD1V.SquareMagnitude();

  if (NMag2 <= MagTol2)
  {
    theStatus = CSLib_Singular;
  }
  else
  {
    // Normalize tangent vectors D1U and D1V for numerical stability
    // This approach is more stable than normalizing the cross product directly
    gp_Dir aD1U(D1U);
    gp_Dir aD1V(D1V);
    Normal    = gp_Dir(aD1U.Crossed(aD1V));
    theStatus = CSLib_Defined;
  }
}

//=================================================================================================

void CSLib::Normal(const Standard_Integer    MaxOrder,
                   const TColgp_Array2OfVec& DerNUV,
                   const Standard_Real       SinTol,
                   const Standard_Real       U,
                   const Standard_Real       V,
                   const Standard_Real       Umin,
                   const Standard_Real       Umax,
                   const Standard_Real       Vmin,
                   const Standard_Real       Vmax,
                   CSLib_NormalStatus&       theStatus,
                   gp_Dir&                   Normal,
                   Standard_Integer&         OrderU,
                   Standard_Integer&         OrderV)
{
  // This function handles the computation of normals at singular points where
  // the standard cross product D1U ^ D1V is null or undefined. It uses higher
  // order derivatives to find the first non-null derivative direction.
  //
  // Algorithm:
  // 1. Search for the first order k0 where at least one derivative is non-null
  // 2. Check if all derivatives at order k0 are parallel and point in same direction
  // 3. If conditions are met, determine the normal orientation using polynomial analysis

  Standard_Integer i = 0, Order = -1;
  Standard_Boolean Trouve = Standard_False;
  Standard_Real    Norme;
  gp_Vec           D;

  // Find the first order k0 such that at least one derivative N=dS/du ^ dS/dv
  // is non-null. All derivatives of lower orders (0 to k0-1) must be null.
  while (!Trouve && Order < MaxOrder)
  {
    Order++;
    i = Order;
    while ((i >= 0) && (!Trouve))
    {
      Standard_Integer j = Order - i;
      D                  = DerNUV(i, j);
      Norme              = D.Magnitude();
      Trouve             = (Trouve || (Norme >= SinTol));
      i--;
    }
  }
  OrderU = i + 1;
  OrderV = Order - OrderU;
  // Vko first non null derivative of N : reference
  if (Trouve)
  {
    if (Order == 0)
    {
      theStatus = CSLib_Defined;
      Normal    = D.Normalized();
    }
    else
    {
      gp_Vec Vk0;
      Vk0 = DerNUV(OrderU, OrderV);
      TColStd_Array1OfReal Ratio(0, Order);
      // Calculate lambda i
      i                        = 0;
      Standard_Boolean definie = Standard_False;
      while (i <= Order && !definie)
      {
        if (DerNUV(i, Order - i).Magnitude() <= SinTol)
          Ratio(i) = 0;
        else
        {
          if (DerNUV(i, Order - i).IsParallel(Vk0, 1e-6))
          {
            //            Ratio(i) = DerNUV(i,Order-i).Magnitude() / Vk0.Magnitude();
            //            if(DerNUV(i,Order-i).IsOpposite(Vk0,1e-6)) Ratio(i)=-Ratio(i);
            Standard_Real r = DerNUV(i, Order - i).Magnitude() / Vk0.Magnitude();
            if (DerNUV(i, Order - i).IsOpposite(Vk0, 1e-6))
              r = -r;
            Ratio(i) = r;
          }
          else
          {
            definie = Standard_True;
            //
          }
        }
        i++;
      } // end while
      if (!definie)
      { // All lambda i exist - all derivatives at order k0 are parallel to Vk0
        Standard_Integer SP;
        Standard_Real    inf, sup;
        inf = 0.0 - M_PI;
        sup = 0.0 + M_PI;
        Standard_Boolean FU, LU, FV, LV;

        // Creation of the angular domain [inf, sup] for polynomial root finding
        // The domain depends on whether the point (U,V) is in the interior,
        // on an edge, or at a corner of the parameter domain.
        // - Interior point: [-π, π]
        // - Edge point: restricted to appropriate half-plane
        // - Corner point: restricted to appropriate quadrant
        FU = (std::abs(U - Umin) < Precision::PConfusion());
        LU = (std::abs(U - Umax) < Precision::PConfusion());
        FV = (std::abs(V - Vmin) < Precision::PConfusion());
        LV = (std::abs(V - Vmax) < Precision::PConfusion());
        if (LU)
        {
          inf = M_PI / 2;
          sup = 3 * inf;
          if (LV)
          {
            inf = M_PI;
          }
          if (FV)
          {
            sup = M_PI;
          }
        }
        else if (FU)
        {
          sup = M_PI / 2;
          inf = -sup;
          if (LV)
          {
            sup = 0;
          }
          if (FV)
          {
            inf = 0;
          }
        }
        else if (LV)
        {
          inf = 0.0 - M_PI;
          sup = 0;
        }
        else if (FV)
        {
          inf = 0;
          sup = M_PI;
        }
        Standard_Boolean CS    = 0;
        Standard_Real    Vprec = 0., Vsuiv = 0.;

        // Create the polynomial function whose sign determines normal orientation
        // The polynomial is built from the ratios of derivative magnitudes
        CSLib_NormalPolyDef Poly(Order, Ratio);

        // Find zeros of the polynomial in the angular domain
        // This determines where the normal direction changes sign
        math_FunctionRoots
          FindRoots(Poly, inf, sup, 200, 1e-5, Precision::Confusion(), Precision::Confusion());
        // If there are zeros
        if (FindRoots.IsDone() && FindRoots.NbSolutions() > 0)
        {
          // ranking by increasing order of roots of SAPS in Sol0

          TColStd_Array1OfReal Sol0(0, FindRoots.NbSolutions() + 1);
          Sol0(1)            = FindRoots.Value(1);
          Standard_Integer n = 1;
          while (n <= FindRoots.NbSolutions())
          {
            Standard_Real    ASOL = FindRoots.Value(n);
            Standard_Integer j    = n - 1;
            while ((j >= 1) && (Sol0(j) > ASOL))
            {
              Sol0(j + 1) = Sol0(j);
              j--;
            }
            Sol0(j + 1) = ASOL;
            n++;
          } // end while(n
          // Add limits of the domains
          Sol0(0)                           = inf;
          Sol0(FindRoots.NbSolutions() + 1) = sup;
          // Find change of sign of SAPS in comparison with its
          // values to the left and right of each root
          Standard_Integer ifirst = 0;
          for (i = 0; i <= FindRoots.NbSolutions(); i++)
          {
            if (std::abs(Sol0(i + 1) - Sol0(i)) > Precision::PConfusion())
            {
              Poly.Value((Sol0(i) + Sol0(i + 1)) / 2.0, Vsuiv);
              if (ifirst == 0)
              {
                ifirst = i;
                CS     = Standard_False;
                Vprec  = Vsuiv;
              }
              else
              {
                CS    = (Vprec * Vsuiv) < 0;
                Vprec = Vsuiv;
              }
            }
          }
        }
        else
        {
          // SAPS has no root, so forcedly do not change the sign
          CS = Standard_False;
          Poly.Value(inf, Vsuiv);
        }
        // fin if(MFR.IsDone() && MFR.NbSolutions()>0)

        // Determine the sign of the polynomial (SP):
        // SP = 0  : polynomial changes sign -> infinity of solutions (ambiguous normal)
        // SP = 1  : polynomial always positive -> normal in +Vk0 direction
        // SP = -1 : polynomial always negative -> normal in -Vk0 direction
        if (CS)
          SP = 0; // Sign changes - ambiguous
        else if (Vsuiv > 0)
          SP = 1; // Always positive
        else
          SP = -1; // Always negative

        if (SP == 0)
          theStatus = CSLib_InfinityOfSolutions;
        else
        {
          theStatus = CSLib_Defined;
          Normal    = SP * Vk0.Normalized();
        }
      }
      else
      {
        theStatus = CSLib_Defined;
        Normal    = D.Normalized();
      }
    }
  }
}

//=================================================================================================

gp_Vec CSLib::DNNUV(const Standard_Integer    Nu,
                    const Standard_Integer    Nv,
                    const TColgp_Array2OfVec& DerSurf)
{
  Standard_Integer i, j;
  gp_Vec           D(0, 0, 0), VG, VD, PV;
  for (i = 0; i <= Nu; i++)
    for (j = 0; j <= Nv; j++)
    {
      VG = DerSurf.Value(i + 1, j);
      VD = DerSurf.Value(Nu - i, Nv + 1 - j);
      PV = VG ^ VD;
      D  = D + PLib::Bin(Nu, i) * PLib::Bin(Nv, j) * PV;
    }
  return D;
}

//=================================================================================================

gp_Vec CSLib::DNNUV(const Standard_Integer    Nu,
                    const Standard_Integer    Nv,
                    const TColgp_Array2OfVec& DerSurf1,
                    const TColgp_Array2OfVec& DerSurf2)
{
  Standard_Integer i, j;
  gp_Vec           D(0, 0, 0), VG, VD, PV;
  for (i = 0; i <= Nu; i++)
    for (j = 0; j <= Nv; j++)
    {
      VG = DerSurf1.Value(i + 1, j);
      VD = DerSurf2.Value(Nu - i, Nv + 1 - j);
      PV = VG ^ VD;
      D  = D + PLib::Bin(Nu, i) * PLib::Bin(Nv, j) * PV;
    }
  return D;
}

//=================================================================================================

gp_Vec CSLib::DNNormal(const Standard_Integer    Nu,
                       const Standard_Integer    Nv,
                       const TColgp_Array2OfVec& DerNUV,
                       const Standard_Integer    Iduref,
                       const Standard_Integer    Idvref)
{
  // Calculate the derivatives of the normalized normal vector from the derivatives
  // of the non-normalized normal vector
  //
  // This function computes d^(Nu+Nv)n/(du^Nu dv^Nv) where n is the unit normal vector.
  // The computation uses the chain rule and Leibniz formula for derivatives of products.
  //
  // Algorithm:
  // 1. Start with the reference normalized normal at (Iduref, Idvref)
  // 2. Build up derivatives iteratively using previously computed lower-order derivatives
  // 3. For each derivative order (p,q), compute:
  //    - Scalar product n·D^(p,q)n (stored in TabScal)
  //    - Magnitude ||D^(p,q)NUV|| (stored in TabNorm)
  //    - Normalized derivative vector D^(p,q)n (stored in DerVecNor)

  const Standard_Integer Kderiv = Nu + Nv;
  TColgp_Array2OfVec     DerVecNor(0, Kderiv, 0, Kderiv); // Normalized derivative vectors
  TColStd_Array2OfReal   TabScal(0, Kderiv, 0, Kderiv);   // Scalar products n·D^(p,q)n
  TColStd_Array2OfReal   TabNorm(0, Kderiv, 0, Kderiv);   // Magnitudes ||D^(p,q)NUV||

  // Initialize with the base normalized normal vector
  gp_Vec DerNor = (DerNUV.Value(Iduref, Idvref)).Normalized();
  DerVecNor.SetValue(0, 0, DerNor);
  Standard_Real Dnorm = DerNUV.Value(Iduref, Idvref) * DerVecNor.Value(0, 0);
  TabNorm.SetValue(0, 0, Dnorm);
  TabScal.SetValue(0, 0, 0.);

  // Iteratively compute derivatives of increasing order
  for (Standard_Integer Mderiv = 1; Mderiv <= Kderiv; Mderiv++)
  {
    for (Standard_Integer Pderiv = 0; Pderiv <= Mderiv; Pderiv++)
    {
      const Standard_Integer Qderiv = Mderiv - Pderiv;
      if (Pderiv > Nu || Qderiv > Nv)
      {
        continue;
      }

      // Compute the scalar product n · D^(p,q)n using Leibniz rule
      // This accounts for all combinations of lower-order derivatives
      Standard_Real Scal = 0.;
      if (Pderiv > Qderiv)
      {
        for (Standard_Integer Jderiv = 1; Jderiv <= Qderiv; Jderiv++)
        {
          Scal = Scal
                 - PLib::Bin(Qderiv, Jderiv)
                     * (DerVecNor.Value(0, Jderiv) * DerVecNor.Value(Pderiv, Qderiv - Jderiv));
        }

        for (Standard_Integer Jderiv = 0; Jderiv < Qderiv; Jderiv++)
        {
          Scal = Scal
                 - PLib::Bin(Qderiv, Jderiv)
                     * (DerVecNor.Value(Pderiv, Jderiv) * DerVecNor.Value(0, Qderiv - Jderiv));
        }

        for (Standard_Integer Ideriv = 1; Ideriv < Pderiv; Ideriv++)
        {
          for (Standard_Integer Jderiv = 0; Jderiv <= Qderiv; Jderiv++)
          {
            Scal = Scal
                   - PLib::Bin(Pderiv, Ideriv) * PLib::Bin(Qderiv, Jderiv)
                       * (DerVecNor.Value(Ideriv, Jderiv)
                          * DerVecNor.Value(Pderiv - Ideriv, Qderiv - Jderiv));
          }
        }
      }
      else
      {
        for (Standard_Integer Ideriv = 1; Ideriv <= Pderiv; Ideriv++)
        {
          Scal = Scal
                 - PLib::Bin(Pderiv, Ideriv) * DerVecNor.Value(Ideriv, 0)
                     * DerVecNor.Value(Pderiv - Ideriv, Qderiv);
        }

        for (Standard_Integer Ideriv = 0; Ideriv < Pderiv; Ideriv++)
        {
          Scal = Scal
                 - PLib::Bin(Pderiv, Ideriv) * DerVecNor.Value(Ideriv, Qderiv)
                     * DerVecNor.Value(Pderiv - Ideriv, 0);
        }

        for (Standard_Integer Ideriv = 0; Ideriv <= Pderiv; Ideriv++)
        {
          for (Standard_Integer Jderiv = 1; Jderiv < Qderiv; Jderiv++)
          {
            Scal = Scal
                   - PLib::Bin(Pderiv, Ideriv) * PLib::Bin(Qderiv, Jderiv)
                       * (DerVecNor.Value(Ideriv, Jderiv)
                          * DerVecNor.Value(Pderiv - Ideriv, Qderiv - Jderiv));
          }
        }
      }
      TabScal.SetValue(Pderiv, Qderiv, Scal / 2.);

      // Compute the derivative (n,p) of NUV Length
      Dnorm = (DerNUV.Value(Pderiv + Iduref, Qderiv + Idvref)) * DerVecNor.Value(0, 0);
      for (Standard_Integer Jderiv = 0; Jderiv < Qderiv; Jderiv++)
      {
        Dnorm = Dnorm
                - PLib::Bin(Qderiv + Idvref, Jderiv + Idvref) * TabNorm.Value(Pderiv, Jderiv)
                    * TabScal.Value(0, Qderiv - Jderiv);
      }

      for (Standard_Integer Ideriv = 0; Ideriv < Pderiv; Ideriv++)
      {
        for (Standard_Integer Jderiv = 0; Jderiv <= Qderiv; Jderiv++)
        {
          Dnorm = Dnorm
                  - PLib::Bin(Pderiv + Iduref, Ideriv + Iduref)
                      * PLib::Bin(Qderiv + Idvref, Jderiv + Idvref) * TabNorm.Value(Ideriv, Jderiv)
                      * TabScal.Value(Pderiv - Ideriv, Qderiv - Jderiv);
        }
      }
      TabNorm.SetValue(Pderiv, Qderiv, Dnorm);

      // Compute derivative (p,q) of n
      DerNor = DerNUV.Value(Pderiv + Iduref, Qderiv + Idvref);
      for (Standard_Integer Jderiv = 1; Jderiv <= Qderiv; Jderiv++)
      {
        DerNor = DerNor
                 - PLib::Bin(Pderiv + Iduref, Iduref) * PLib::Bin(Qderiv + Idvref, Jderiv + Idvref)
                     * TabNorm.Value(0, Jderiv) * DerVecNor.Value(Pderiv, Qderiv - Jderiv);
      }

      for (Standard_Integer Ideriv = 1; Ideriv <= Pderiv; Ideriv++)
      {
        for (Standard_Integer Jderiv = 0; Jderiv <= Qderiv; Jderiv++)
        {
          DerNor = DerNor
                   - PLib::Bin(Pderiv + Iduref, Ideriv + Iduref)
                       * PLib::Bin(Qderiv + Idvref, Jderiv + Idvref) * TabNorm.Value(Ideriv, Jderiv)
                       * DerVecNor.Value(Pderiv - Ideriv, Qderiv - Jderiv);
        }
      }
      DerNor = DerNor / PLib::Bin(Pderiv + Iduref, Iduref) / PLib::Bin(Qderiv + Idvref, Idvref)
               / TabNorm.Value(0, 0);
      DerVecNor.SetValue(Pderiv, Qderiv, DerNor);
    }
  }
  return DerVecNor.Value(Nu, Nv);
}
