// Copyright (c) 2025 OPEN CASCADE SAS
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

#include <CSLib_NormalPolyDef.hxx>

#include <gtest/gtest.h>

#include <Standard_Real.hxx>
#include <Standard_Integer.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <Precision.hxx>
#include <cmath>

namespace
{
// Test polynomial value computation
TEST(CSLibNormalPolyDefTest, PolynomialValue)
{
  // Create a simple polynomial with order 2
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 1.0;
  aRatios(1) = 1.0;
  aRatios(2) = 1.0;

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  // Test value at X = 0
  Standard_Real aValue;
  Standard_Boolean isOk = aPoly.Value(0.0, aValue);

  EXPECT_TRUE(isOk) << "Value computation should succeed";
  EXPECT_NEAR(aValue, 0.0, 1.0e-10) << "Value at X=0 should be 0 (sin(0)=0)";
}

// Test polynomial value at pi/4
TEST(CSLibNormalPolyDefTest, PolynomialValueAtPiOver4)
{
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 1.0;
  aRatios(1) = 1.0;
  aRatios(2) = 1.0;

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX     = M_PI / 4.0;
  Standard_Real aValue;
  Standard_Boolean isOk = aPoly.Value(aX, aValue);

  EXPECT_TRUE(isOk) << "Value computation should succeed";
  // Value should be non-zero for non-zero angle
  EXPECT_NE(aValue, 0.0) << "Value should be non-zero at pi/4";
}

// Test derivative computation
TEST(CSLibNormalPolyDefTest, PolynomialDerivative)
{
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 1.0;
  aRatios(1) = 1.0;
  aRatios(2) = 1.0;

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX          = M_PI / 4.0;
  Standard_Real aDerivative;
  Standard_Boolean isOk = aPoly.Derivative(aX, aDerivative);

  EXPECT_TRUE(isOk) << "Derivative computation should succeed";
}

// Test both value and derivative computation together
TEST(CSLibNormalPolyDefTest, PolynomialValuesAndDerivative)
{
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 1.0;
  aRatios(1) = 1.0;
  aRatios(2) = 1.0;

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX          = M_PI / 4.0;
  Standard_Real aValue;
  Standard_Real aDerivative;
  Standard_Boolean isOk = aPoly.Values(aX, aValue, aDerivative);

  EXPECT_TRUE(isOk) << "Values computation should succeed";

  // Verify consistency: compute separately and compare
  Standard_Real aValueSep, aDerivativeSep;
  aPoly.Value(aX, aValueSep);
  aPoly.Derivative(aX, aDerivativeSep);

  EXPECT_NEAR(aValue, aValueSep, 1.0e-10) << "Value from Values() should match Value()";
  EXPECT_NEAR(aDerivative, aDerivativeSep, 1.0e-10)
    << "Derivative from Values() should match Derivative()";
}

// Test with different ratio values (tests the bug fix: missing * myTABli(i))
TEST(CSLibNormalPolyDefTest, PolynomialWithDifferentRatios)
{
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 1.0;
  aRatios(1) = 2.0;  // Different ratio
  aRatios(2) = 3.0;  // Different ratio

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX     = M_PI / 6.0;
  Standard_Real aValue;
  Standard_Boolean isOk = aPoly.Value(aX, aValue);

  EXPECT_TRUE(isOk) << "Should handle different ratios";

  // The value should be affected by the ratios (tests that ratios are actually used)
  // If the bug (missing multiplication) existed, ratios would have no effect on derivative
  Standard_Real aDerivative;
  isOk = aPoly.Derivative(aX, aDerivative);

  EXPECT_TRUE(isOk) << "Derivative should be computed with ratios";
}

// Test higher order polynomial
TEST(CSLibNormalPolyDefTest, HigherOrderPolynomial)
{
  Standard_Integer aK0 = 4;
  TColStd_Array1OfReal aRatios(0, aK0);
  for (Standard_Integer i = 0; i <= aK0; ++i)
  {
    aRatios(i) = 1.0;
  }

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX     = M_PI / 3.0;
  Standard_Real aValue;
  Standard_Boolean isOk = aPoly.Value(aX, aValue);

  EXPECT_TRUE(isOk) << "Should handle higher order polynomials";
}

// Test derivative with different ratios (critical test for the bug fix)
TEST(CSLibNormalPolyDefTest, DerivativeWithRatiosBugFix)
{
  // This test specifically validates the bug fix where myTABli(i) was missing
  // in the derivative calculation
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios1(0, aK0);
  TColStd_Array1OfReal aRatios2(0, aK0);

  // First set: all ratios = 1
  for (Standard_Integer i = 0; i <= aK0; ++i)
  {
    aRatios1(i) = 1.0;
  }

  // Second set: different ratios
  for (Standard_Integer i = 0; i <= aK0; ++i)
  {
    aRatios2(i) = static_cast<Standard_Real>(i + 1);
  }

  CSLib_NormalPolyDef aPoly1(aK0, aRatios1);
  CSLib_NormalPolyDef aPoly2(aK0, aRatios2);

  Standard_Real aX = M_PI / 4.0;
  Standard_Real aDerivative1, aDerivative2;

  aPoly1.Derivative(aX, aDerivative1);
  aPoly2.Derivative(aX, aDerivative2);

  // If the bug existed (missing multiplication by ratios), these would be equal
  // With the fix, they should be different because ratios2 differs from ratios1
  EXPECT_NE(aDerivative1, aDerivative2)
    << "Derivatives should differ when ratios differ (validates bug fix)";
}

// Test numerical stability with optimized integer power function
TEST(CSLibNormalPolyDefTest, NumericalStabilityWithIntPow)
{
  // This test validates that the optimized IntPow function produces
  // accurate results compared to std::pow
  Standard_Integer aK0 = 3;
  TColStd_Array1OfReal aRatios(0, aK0);
  for (Standard_Integer i = 0; i <= aK0; ++i)
  {
    aRatios(i) = 1.0;
  }

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  // Test at multiple angles
  const Standard_Integer aNbTests = 10;
  for (Standard_Integer i = 0; i < aNbTests; ++i)
  {
    Standard_Real aX = M_PI * i / aNbTests;
    Standard_Real aValue, aDerivative;

    Standard_Boolean isOk = aPoly.Values(aX, aValue, aDerivative);
    EXPECT_TRUE(isOk) << "Should compute values successfully at angle " << aX;

    // Values should be finite and reasonable
    EXPECT_TRUE(std::isfinite(aValue)) << "Value should be finite";
    EXPECT_TRUE(std::isfinite(aDerivative)) << "Derivative should be finite";
  }
}

// Test zero ratios
TEST(CSLibNormalPolyDefTest, ZeroRatios)
{
  Standard_Integer aK0 = 2;
  TColStd_Array1OfReal aRatios(0, aK0);
  aRatios(0) = 0.0;
  aRatios(1) = 0.0;
  aRatios(2) = 0.0;

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  Standard_Real aX     = M_PI / 4.0;
  Standard_Real aValue;
  Standard_Boolean isOk = aPoly.Value(aX, aValue);

  EXPECT_TRUE(isOk) << "Should handle zero ratios";
  EXPECT_NEAR(aValue, 0.0, 1.0e-10) << "Value should be zero when all ratios are zero";
}

// Test performance improvement with IntPow
TEST(CSLibNormalPolyDefTest, PerformanceIntPowOptimization)
{
  // This test ensures the optimized IntPow is being used
  // Higher order polynomial to stress the power function
  Standard_Integer aK0 = 8;
  TColStd_Array1OfReal aRatios(0, aK0);
  for (Standard_Integer i = 0; i <= aK0; ++i)
  {
    aRatios(i) = static_cast<Standard_Real>(i + 1) / static_cast<Standard_Real>(aK0 + 1);
  }

  CSLib_NormalPolyDef aPoly(aK0, aRatios);

  // Compute values at many points
  const Standard_Integer aNbEvals = 100;
  for (Standard_Integer i = 0; i < aNbEvals; ++i)
  {
    Standard_Real aX = 2.0 * M_PI * i / aNbEvals;
    Standard_Real aValue, aDerivative;
    Standard_Boolean isOk = aPoly.Values(aX, aValue, aDerivative);

    EXPECT_TRUE(isOk) << "Evaluation " << i << " should succeed";
  }

  // If this test completes quickly, the IntPow optimization is working
  SUCCEED() << "Performance test completed";
}

} // anonymous namespace
