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

#ifndef _math_GaussKronrodWeights_HeaderFile
#define _math_GaussKronrodWeights_HeaderFile

//! @file math_GaussKronrodWeights.hxx
//! Wrapper for accessing Gauss-Kronrod quadrature weights.
//! This header isolates the old class math from the new namespace math.
//! Implementation is in math_GaussKronrodWeights.cxx to avoid conflicts.

#include <Standard_Macro.hxx>
#include <math_Vector.hxx>

//! Get Gauss-Kronrod points and weights.
//! @param theNbKronrod number of Kronrod points (should be 2n+1)
//! @param thePoints output vector for points
//! @param theWeights output vector for weights
//! @return true if successful
Standard_EXPORT bool GetKronrodPointsAndWeights(int          theNbKronrod,
                                                math_Vector& thePoints,
                                                math_Vector& theWeights);

//! Get ordered Gauss points and weights.
//! @param theNbGauss number of Gauss points
//! @param thePoints output vector for points
//! @param theWeights output vector for weights
//! @return true if successful
Standard_EXPORT bool GetOrderedGaussPointsAndWeights(int          theNbGauss,
                                                     math_Vector& thePoints,
                                                     math_Vector& theWeights);

#endif // _math_GaussKronrodWeights_HeaderFile
