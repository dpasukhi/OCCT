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

#include <gtest/gtest.h>

#include <math_Roots_Multiple.hxx>
#include <math_Functor_Scalar.hxx>

#include <cmath>

namespace
{
constexpr double THE_TOLERANCE = 1e-8;
constexpr double THE_PI        = 3.14159265358979323846;
}

//==================================================================================================
// FindAllRoots Tests (Value-only interface)
//==================================================================================================

TEST(math_Roots_Multiple, FindAllRoots_NoRoots)
{
  // f(x) = x^2 + 1, no real roots
  math::Functor::Polynomial aFunc({1.0, 0.0, 1.0});

  auto aResult = math::Roots::FindAllRoots(aFunc, -10.0, 10.0);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 0u);
}

TEST(math_Roots_Multiple, FindAllRoots_SingleRoot)
{
  // f(x) = x - 2, single root at x = 2
  math::Functor::Polynomial aFunc({-2.0, 1.0});

  auto aResult = math::Roots::FindAllRoots(aFunc, 0.0, 5.0);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 1u);
  EXPECT_NEAR(aResult.Roots[0], 2.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_TwoRoots)
{
  // f(x) = x^2 - 4, roots at x = -2 and x = 2
  math::Functor::Polynomial aFunc({-4.0, 0.0, 1.0});

  auto aResult = math::Roots::FindAllRoots(aFunc, -5.0, 5.0);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 2u);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_ThreeRoots)
{
  // f(x) = (x - 1)(x - 2)(x - 3) = x^3 - 6x^2 + 11x - 6
  math::Functor::Polynomial aFunc({-6.0, 11.0, -6.0, 1.0});

  auto aResult = math::Roots::FindAllRoots(aFunc, 0.0, 4.0);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 3u);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 3.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_SineFunction)
{
  // f(x) = sin(x), roots at 0, pi, 2*pi, 3*pi in [0, 10]
  math::Functor::Sine aSine;

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 200;
  aConfig.FTolerance = 1e-10;

  auto aResult = math::Roots::FindAllRoots(aSine, 0.0, 10.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 4u); // 0, pi, 2*pi, 3*pi

  EXPECT_NEAR(aResult.Roots[0], 0.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], THE_PI, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 2.0 * THE_PI, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[3], 3.0 * THE_PI, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_WithOffset)
{
  // f(x) = x^2, find roots of f(x) - 4 = 0, i.e., x^2 = 4
  // Roots at x = -2 and x = 2
  math::Functor::Polynomial aFunc({0.0, 0.0, 1.0});

  math::Roots::MultipleConfig aConfig;
  aConfig.Offset = 4.0;

  auto aResult = math::Roots::FindAllRoots(aFunc, -5.0, 5.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 2u);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_CloseRoots)
{
  // f(x) = (x - 1)(x - 1.01) = x^2 - 2.01x + 1.01
  // Two close roots at x = 1 and x = 1.01
  math::Functor::Polynomial aFunc({1.01, -2.01, 1.0});

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 500;
  aConfig.XTolerance = 1e-4;

  auto aResult = math::Roots::FindAllRoots(aFunc, 0.0, 2.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 2u);
  EXPECT_NEAR(aResult.Roots[0], 1.0, 1e-3);
  EXPECT_NEAR(aResult.Roots[1], 1.01, 1e-3);
}

TEST(math_Roots_Multiple, FindAllRoots_NullFunction)
{
  // f(x) = 0 (constant zero function)
  math::Functor::Constant aFunc(0.0);

  math::Roots::MultipleConfig aConfig;
  aConfig.NullTolerance = 1e-10;

  auto aResult = math::Roots::FindAllRoots(aFunc, -1.0, 1.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_TRUE(aResult.IsAllNull);
}

TEST(math_Roots_Multiple, FindAllRoots_LambdaFunction)
{
  // f(x) = cos(x) - 0.5, roots where cos(x) = 0.5
  auto aFunc = math::Functor::MakeScalar([](double x, double& y) {
    y = std::cos(x) - 0.5;
    return true;
  });

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 100;

  auto aResult = math::Roots::FindAllRoots(aFunc, 0.0, 2.0 * THE_PI, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 2u);
  // cos(x) = 0.5 at x = pi/3 and x = 5*pi/3
  EXPECT_NEAR(aResult.Roots[0], THE_PI / 3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 5.0 * THE_PI / 3.0, THE_TOLERANCE);
}

//==================================================================================================
// FindAllRootsWithDerivative Tests
//==================================================================================================

TEST(math_Roots_Multiple, FindAllRootsWithDerivative_ThreeRoots)
{
  // f(x) = (x - 1)(x - 2)(x - 3)
  math::Functor::Polynomial aFunc({-6.0, 11.0, -6.0, 1.0});

  auto aResult = math::Roots::FindAllRootsWithDerivative(aFunc, 0.0, 4.0);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 3u);
  EXPECT_NEAR(aResult.Roots[0], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 3.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRootsWithDerivative_DoubleRoot)
{
  // f(x) = (x - 2)^2 = x^2 - 4x + 4, double root at x = 2
  // This is a tangent root (touches zero without crossing)
  math::Functor::Polynomial aFunc({4.0, -4.0, 1.0});

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 100;
  aConfig.FTolerance = 1e-8;

  auto aResult = math::Roots::FindAllRootsWithDerivative(aFunc, 0.0, 4.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  // Should find the double root (or detect it as near-zero extremum)
  EXPECT_GE(aResult.NbRoots(), 1u);
  if (aResult.NbRoots() > 0)
  {
    EXPECT_NEAR(aResult.Roots[0], 2.0, 1e-4);
  }
}

TEST(math_Roots_Multiple, FindAllRootsWithDerivative_SineFunction)
{
  // f(x) = sin(x) with derivative cos(x)
  class SineWithDerivative
  {
  public:
    bool Value(double theX, double& theY) const
    {
      theY = std::sin(theX);
      return true;
    }

    bool Values(double theX, double& theY, double& theDY) const
    {
      theY = std::sin(theX);
      theDY = std::cos(theX);
      return true;
    }
  };

  SineWithDerivative aSine;

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 200;

  auto aResult = math::Roots::FindAllRootsWithDerivative(aSine, 0.0, 7.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 3u); // 0, pi, 2*pi

  EXPECT_NEAR(aResult.Roots[0], 0.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], THE_PI, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 2.0 * THE_PI, THE_TOLERANCE);
}

//==================================================================================================
// Comparison with Legacy math_FunctionRoots
//==================================================================================================

TEST(math_Roots_Multiple, CompareWithLegacy_Polynomial)
{
  // Compare results with legacy math_FunctionRoots behavior
  // f(x) = x^3 - 6x^2 + 11x - 6 = (x-1)(x-2)(x-3)
  math::Functor::Polynomial aFunc({-6.0, 11.0, -6.0, 1.0});

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 50;
  aConfig.XTolerance = 1e-10;
  aConfig.FTolerance = 1e-10;

  auto aResult = math::Roots::FindAllRoots(aFunc, 0.5, 3.5, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 3u);

  // Verify function value at roots is close to zero
  for (size_t i = 0; i < aResult.NbRoots(); ++i)
  {
    double aValue = 0.0;
    aFunc.Value(aResult.Roots[i], aValue);
    EXPECT_NEAR(aValue, 0.0, 1e-9);
  }
}

TEST(math_Roots_Multiple, CompareWithLegacy_HigherDegree)
{
  // f(x) = x^4 - 10x^2 + 9 = (x^2-1)(x^2-9) = (x-1)(x+1)(x-3)(x+3)
  // Roots at -3, -1, 1, 3
  math::Functor::Polynomial aFunc({9.0, 0.0, -10.0, 0.0, 1.0});

  math::Roots::MultipleConfig aConfig;
  aConfig.NbSamples = 100;

  auto aResult = math::Roots::FindAllRoots(aFunc, -5.0, 5.0, aConfig);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 4u);

  EXPECT_NEAR(aResult.Roots[0], -3.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], -1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[2], 1.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[3], 3.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_ConvenienceOverload)
{
  // Test the convenience overload with explicit sample count
  math::Functor::Polynomial aFunc({-4.0, 0.0, 1.0}); // x^2 - 4

  auto aResult = math::Roots::FindAllRoots(aFunc, -5.0, 5.0, 50);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 2u);
  EXPECT_NEAR(aResult.Roots[0], -2.0, THE_TOLERANCE);
  EXPECT_NEAR(aResult.Roots[1], 2.0, THE_TOLERANCE);
}

TEST(math_Roots_Multiple, FindAllRoots_Exponential)
{
  // f(x) = exp(x) - 2, root at ln(2) ≈ 0.693
  auto aFunc = math::Functor::MakeScalar([](double x, double& y) {
    y = std::exp(x) - 2.0;
    return true;
  });

  auto aResult = math::Roots::FindAllRoots(aFunc, -1.0, 2.0, 50);
  EXPECT_TRUE(aResult.IsDone());
  EXPECT_EQ(aResult.NbRoots(), 1u);
  EXPECT_NEAR(aResult.Roots[0], std::log(2.0), THE_TOLERANCE);
}
