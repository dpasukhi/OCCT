// Created on: 2005-12-21
// Created by: Julia GERASIMOVA
// Copyright (c) 2005-2014 OPEN CASCADE SAS
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

#include <math_Array1OfValueAndWeight.hxx>
#include <math_ComputeKronrodPointsAndWeights.hxx>
#include <math_EigenValuesSearcher.hxx>
#include <Standard_ErrorHandler.hxx>

#include <algorithm>

math_ComputeKronrodPointsAndWeights::math_ComputeKronrodPointsAndWeights(
  const Standard_Integer Number)
    : myPoints(1, 2 * Number + 1), myWeights(1, 2 * Number + 1), myIsDone(Standard_False)
{
  try
  {
    Standard_Integer i, j;
    Standard_Integer a2NP1 = 2 * Number + 1;

    TColStd_Array1OfReal aDiag(1, a2NP1);
    TColStd_Array1OfReal aSubDiag(1, a2NP1);

    // Initialize symmetric tridiagonal matrix.
    Standard_Integer n         = Number;
    Standard_Integer aKronrodN = 2 * Number + 1;
    Standard_Integer a3KN2p1   = Min(3 * (Number + 1) / 2 + 1, aKronrodN);
    for (i = 1; i <= a3KN2p1; i++)
    {
      aDiag(i) = 0.;

      if (i == 1)
        aSubDiag(i) = 0.;
      else
      {
        Standard_Integer sqrIm1 = (i - 1) * (i - 1);
        aSubDiag(i)             = sqrIm1 / (4. * sqrIm1 - 1);
      }
    }

    for (i = a3KN2p1 + 1; i <= aKronrodN; i++)
    {
      aDiag(i)    = 0.;
      aSubDiag(i) = 0.;
    }

    // Algorithm calculates weights and points symmetrically and uses -1 index
    // by design. Using math_Vector with base index -1 for exception safety.
    Standard_Integer aNd2 = Number / 2;
    math_Vector      s(-1, aNd2);
    math_Vector      t(-1, aNd2);

    for (i = -1; i <= aNd2; i++)
    {
      s(i) = 0.;
      t(i) = 0.;
    }

    // Generation of Jacobi-Kronrod matrix.
    // Using base index 0 to support offset pointer access pattern (a = aa + 1).
    math_Vector aa(0, a2NP1);
    math_Vector bb(0, a2NP1);
    for (i = 1; i <= a2NP1; i++)
    {
      aa(i) = aDiag(i);
      bb(i) = aSubDiag(i);
    }
    math_Vector*     ptrtmp;
    Standard_Real    u;
    Standard_Integer m;
    Standard_Integer k;
    Standard_Integer l;

    // Create offset views: a points to aa[1..], b points to bb[1..]
    // Using pointer arithmetic for compatibility with original algorithm.
    Standard_Real* a = &aa(1);
    Standard_Real* b = &bb(1);

    // Eastward phase.
    t(0) = b[Number + 1];

    for (m = 0; m <= n - 2; m++)
    {
      u = 0;

      for (k = (m + 1) / 2; k >= 0; k--)
      {
        l = m - k;
        u += (a[k + n + 1] - a[l]) * t(k) + b[k + n + 1] * s(k - 1) - b[l] * s(k);
        s(k) = u;
      }

      std::swap(s, t);
    }

    for (j = aNd2; j >= 0; j--)
      s(j) = s(j - 1);

    // Southward phase.
    for (m = n - 1; m <= 2 * n - 3; m++)
    {
      u = 0;

      for (k = m + 1 - n; k <= (m - 1) / 2; k++)
      {
        l = m - k;
        j = n - 1 - l;
        u += -(a[k + n + 1] - a[l]) * t(j) - b[k + n + 1] * s(j) + b[l] * s(j + 1);
        s(j) = u;
      }

      if (m % 2 == 0)
      {
        k            = m / 2;
        a[k + n + 1] = a[k] + (s(j) - b[k + n + 1] * s(j + 1)) / t(j + 1);
      }
      else
      {
        k            = (m + 1) / 2;
        b[k + n + 1] = s(j) / s(j + 1);
      }

      std::swap(s, t);
    }

    // Termination phase.
    a[2 * Number] = a[n - 1] - b[2 * Number] * s(0) / t(0);

    // Copy results back to aDiag and aSubDiag
    for (i = 1; i <= a2NP1; i++)
    {
      aDiag(i)    = aa(i);
      aSubDiag(i) = bb(i);
    }

    for (i = 1; i <= a2NP1; i++)
      aSubDiag(i) = Sqrt(aSubDiag(i));

    // Compute eigen values.
    math_EigenValuesSearcher EVsearch(aDiag, aSubDiag);

    if (EVsearch.IsDone())
    {
      math_Array1OfValueAndWeight VWarray(1, a2NP1);
      for (i = 1; i <= a2NP1; i++)
      {
        math_Vector   anEigenVector = EVsearch.EigenVector(i);
        Standard_Real aWeight       = anEigenVector(1);
        aWeight                     = 2. * aWeight * aWeight;
        math_ValueAndWeight EVW(EVsearch.EigenValue(i), aWeight);
        VWarray(i) = EVW;
      }

      std::sort(VWarray.begin(), VWarray.end());

      for (i = 1; i <= a2NP1; i++)
      {
        myPoints(i)  = VWarray(i).Value();
        myWeights(i) = VWarray(i).Weight();
      }
      myIsDone = Standard_True;
    }
  }
  catch (Standard_Failure const&)
  {
  }
}

Standard_Boolean math_ComputeKronrodPointsAndWeights::IsDone() const
{
  return myIsDone;
}

math_Vector math_ComputeKronrodPointsAndWeights::Points() const
{
  return myPoints;
}

math_Vector math_ComputeKronrodPointsAndWeights::Weights() const
{
  return myWeights;
}
