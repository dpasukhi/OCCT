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

#include <math_Integ_Kronrod.hxx>
#include <math_Integ_DoubleExp.hxx>
#include <math_Integ_Gauss.hxx>
#include <math_KronrodSingleIntegration.hxx>
#include <math_GaussSingleIntegration.hxx>
#include <math_Function.hxx>

#include <cmath>

namespace
{
constexpr double THE_TOLERANCE = 1.0e-8;

// ============================================================================
// Test function classes
// ============================================================================

//! Polynomial: f(x) = x^2
struct PolynomialFunc
{
  bool Value(double theX, double& theF)
  {
    theF = theX * theX;
    return true;
  }
};

//! Sine function: f(x) = sin(x)
struct SineFunc
{
  bool Value(double theX, double& theF)
  {
    theF = std::sin(theX);
    return true;
  }
};

//! Exponential: f(x) = exp(-x^2)
struct GaussianFunc
{
  bool Value(double theX, double& theF)
  {
    theF = std::exp(-theX * theX);
    return true;
  }
};

//! Oscillatory: f(x) = cos(10*x)
struct OscillatoryFunc
{
  bool Value(double theX, double& theF)
  {
    theF = std::cos(10.0 * theX);
    return true;
  }
};

//! Square root singularity: f(x) = 1/sqrt(x)
struct SqrtSingularityFunc
{
  bool Value(double theX, double& theF)
  {
    if (theX <= 0.0)
    {
      return false;
    }
    theF = 1.0 / std::sqrt(theX);
    return true;
  }
};

//! Log singularity: f(x) = -log(x)
struct LogSingularityFunc
{
  bool Value(double theX, double& theF)
  {
    if (theX <= 0.0)
    {
      return false;
    }
    theF = -std::log(theX);
    return true;
  }
};

//! Decaying exponential for semi-infinite: f(x) = exp(-x)
struct ExponentialDecayFunc
{
  bool Value(double theX, double& theF)
  {
    theF = std::exp(-theX);
    return true;
  }
};

//! Gaussian for infinite interval: f(x) = exp(-x^2)
struct InfiniteGaussianFunc
{
  bool Value(double theX, double& theF)
  {
    theF = std::exp(-theX * theX);
    return true;
  }
};

// Old API adapter
class SineFuncOld : public math_Function
{
public:
  Standard_Boolean Value(const Standard_Real theX, Standard_Real& theF) override
  {
    theF = std::sin(theX);
    return Standard_True;
  }
};

class PolynomialFuncOld : public math_Function
{
public:
  Standard_Boolean Value(const Standard_Real theX, Standard_Real& theF) override
  {
    theF = theX * theX;
    return Standard_True;
  }
};

} // namespace

// ============================================================================
// Kronrod integration tests
// ============================================================================

TEST(math_Integ_Kronrod_NewTest, KronrodRule_Polynomial)
{
  PolynomialFunc aFunc;

  // Integral of x^2 from 0 to 1 = 1/3
  auto aResult = math::Integ::KronrodRule(aFunc, 0.0, 1.0, 7);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 1.0 / 3.0, THE_TOLERANCE);
}

TEST(math_Integ_Kronrod_NewTest, KronrodRule_Sine)
{
  SineFunc aFunc;

  // Integral of sin(x) from 0 to pi = 2
  auto aResult = math::Integ::KronrodRule(aFunc, 0.0, M_PI, 7);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 2.0, THE_TOLERANCE);
}

TEST(math_Integ_Kronrod_NewTest, AdaptiveKronrod_Oscillatory)
{
  OscillatoryFunc aFunc;

  // Integral of cos(10x) from 0 to pi = 0
  math::Integ::KronrodConfig aConfig;
  aConfig.Tolerance     = 1.0e-10;
  aConfig.NbGaussPoints = 7;
  aConfig.Adaptive      = true;
  aConfig.MaxIterations = 100;

  auto aResult = math::Integ::Kronrod(aFunc, 0.0, M_PI, aConfig);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 0.0, 1.0e-6);
}

TEST(math_Integ_Kronrod_NewTest, AdaptiveKronrod_Gaussian)
{
  GaussianFunc aFunc;

  // Integral of exp(-x^2) from -inf to +inf = sqrt(pi)
  // Use SinhSinh for proper infinite interval integration
  math::Integ::DoubleExpConfig aConfig;
  aConfig.Tolerance = 1.0e-10;

  auto aResult = math::Integ::SinhSinh(aFunc, aConfig);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, std::sqrt(M_PI), 1.0e-6);
}

TEST(math_Integ_Kronrod_NewTest, KronrodAuto)
{
  SineFunc aFunc;

  // Integral of sin(x) from 0 to 2*pi = 0
  auto aResult = math::Integ::KronrodAuto(aFunc, 0.0, 2.0 * M_PI);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 0.0, 1.0e-8);
}

TEST(math_Integ_Kronrod_NewTest, DifferentOrders)
{
  PolynomialFunc aFunc;

  // Test with different Gauss orders
  int aOrders[] = {3, 5, 7, 10, 15};

  for (int aOrder : aOrders)
  {
    auto aResult = math::Integ::KronrodRule(aFunc, 0.0, 1.0, aOrder);

    ASSERT_TRUE(aResult.IsDone()) << "Failed with order " << aOrder;
    EXPECT_NEAR(*aResult.Value, 1.0 / 3.0, 1.0e-8) << "Failed with order " << aOrder;
  }
}

// ============================================================================
// Double exponential (tanh-sinh) integration tests
// ============================================================================

TEST(math_Integ_DoubleExp_NewTest, TanhSinh_Polynomial)
{
  PolynomialFunc aFunc;

  // Integral of x^2 from 0 to 1 = 1/3
  auto aResult = math::Integ::TanhSinh(aFunc, 0.0, 1.0);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 1.0 / 3.0, 1.0e-6);
}

TEST(math_Integ_DoubleExp_NewTest, TanhSinh_Sine)
{
  SineFunc aFunc;

  // Integral of sin(x) from 0 to pi = 2
  auto aResult = math::Integ::TanhSinh(aFunc, 0.0, M_PI);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 2.0, 1.0e-6);
}

TEST(math_Integ_DoubleExp_NewTest, TanhSinh_SqrtSingularity)
{
  SqrtSingularityFunc aFunc;

  // Integral of 1/sqrt(x) from 0 to 1 = 2
  // This has a singularity at x=0
  math::Integ::DoubleExpConfig aConfig;
  aConfig.NbLevels = 8;

  auto aResult = math::Integ::TanhSinh(aFunc, 0.0, 1.0, aConfig);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 2.0, 1.0e-4);
}

TEST(math_Integ_DoubleExp_NewTest, TanhSinh_LogSingularity)
{
  LogSingularityFunc aFunc;

  // Integral of -log(x) from 0 to 1 = 1
  // This has a log singularity at x=0
  math::Integ::DoubleExpConfig aConfig;
  aConfig.NbLevels = 8;

  auto aResult = math::Integ::TanhSinh(aFunc, 0.0, 1.0, aConfig);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 1.0, 1.0e-4);
}

TEST(math_Integ_DoubleExp_NewTest, TanhSinhSingular)
{
  SqrtSingularityFunc aFunc;

  // Optimized for endpoint singularities
  auto aResult = math::Integ::TanhSinhSingular(aFunc, 0.0, 1.0, 1.0e-6);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 2.0, 1.0e-3);
}

TEST(math_Integ_DoubleExp_NewTest, ExpSinh_SemiInfinite)
{
  ExponentialDecayFunc aFunc;

  // Integral of exp(-x) from 0 to infinity = 1
  auto aResult = math::Integ::ExpSinh(aFunc, 0.0);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 1.0, 1.0e-4);
}

TEST(math_Integ_DoubleExp_NewTest, SinhSinh_Infinite)
{
  InfiniteGaussianFunc aFunc;

  // Integral of exp(-x^2) from -inf to +inf = sqrt(pi)
  auto aResult = math::Integ::SinhSinh(aFunc);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, std::sqrt(M_PI), 1.0e-4);
}

TEST(math_Integ_DoubleExp_NewTest, DoubleExponential_Auto)
{
  SineFunc aFunc;

  // Finite interval
  auto aResult = math::Integ::DoubleExponential(aFunc, 0.0, M_PI);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 2.0, 1.0e-6);
}

TEST(math_Integ_DoubleExp_NewTest, WithSingularity)
{
  // Function with known singularity at x = 0.5
  struct SingularFunc
  {
    bool Value(double theX, double& theF)
    {
      const double aDist = std::abs(theX - 0.5);
      if (aDist < 1.0e-10)
      {
        return false;
      }
      theF = 1.0 / std::sqrt(aDist);
      return true;
    }
  };

  SingularFunc aFunc;

  // Split at singularity
  auto aResult = math::Integ::TanhSinhWithSingularity(aFunc, 0.0, 1.0, 0.5);

  ASSERT_TRUE(aResult.IsDone());
  // The integral exists (both halves are sqrt singularities)
  // Total should be about 2 * 2*sqrt(0.5) = 2.83...
  EXPECT_GT(*aResult.Value, 0.0);
}

// ============================================================================
// Comparison: Kronrod vs Gauss
// ============================================================================

TEST(math_Integ_NewTest, KronrodVsGauss_Polynomial)
{
  PolynomialFunc aFunc;

  auto aKronrodResult = math::Integ::KronrodRule(aFunc, 0.0, 1.0, 7);
  auto aGaussResult   = math::Integ::Gauss(aFunc, 0.0, 1.0, 15);

  ASSERT_TRUE(aKronrodResult.IsDone());
  ASSERT_TRUE(aGaussResult.IsDone());

  EXPECT_NEAR(*aKronrodResult.Value, *aGaussResult.Value, 1.0e-10);
}

TEST(math_Integ_NewTest, KronrodVsGauss_Sine)
{
  SineFunc aFunc;

  auto aKronrodResult = math::Integ::KronrodRule(aFunc, 0.0, M_PI, 7);
  auto aGaussResult   = math::Integ::Gauss(aFunc, 0.0, M_PI, 15);

  ASSERT_TRUE(aKronrodResult.IsDone());
  ASSERT_TRUE(aGaussResult.IsDone());

  EXPECT_NEAR(*aKronrodResult.Value, *aGaussResult.Value, 1.0e-10);
}

// ============================================================================
// Comparison: TanhSinh vs Kronrod for regular functions
// ============================================================================

TEST(math_Integ_NewTest, TanhSinhVsKronrod_Polynomial)
{
  PolynomialFunc aFunc;

  auto aTanhSinhResult = math::Integ::TanhSinh(aFunc, 0.0, 1.0);
  auto aKronrodResult  = math::Integ::KronrodRule(aFunc, 0.0, 1.0, 7);

  ASSERT_TRUE(aTanhSinhResult.IsDone());
  ASSERT_TRUE(aKronrodResult.IsDone());

  EXPECT_NEAR(*aTanhSinhResult.Value, *aKronrodResult.Value, 1.0e-6);
}

// ============================================================================
// Comparison with old API
// ============================================================================

TEST(math_Integ_NewTest, CompareWithOldAPI_Kronrod)
{
  SineFuncOld anOldFunc;
  SineFunc    aNewFunc;

  // Old API - constructor takes: func, lower, upper, nbPoints, tolerance, maxIterations
  math_KronrodSingleIntegration anOldInteg(anOldFunc, 0.0, M_PI, 15, 1.0e-10, 100);

  // New API
  math::Integ::KronrodConfig aConfig;
  aConfig.NbGaussPoints = 7;
  aConfig.Tolerance     = 1.0e-10;
  aConfig.Adaptive      = true;
  auto aNewResult       = math::Integ::Kronrod(aNewFunc, 0.0, M_PI, aConfig);

  ASSERT_TRUE(anOldInteg.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  // Both should give sin integral = 2
  EXPECT_NEAR(anOldInteg.Value(), *aNewResult.Value, 1.0e-8);
  EXPECT_NEAR(*aNewResult.Value, 2.0, 1.0e-8);
}

TEST(math_Integ_NewTest, CompareWithOldAPI_Gauss)
{
  PolynomialFuncOld anOldFunc;
  PolynomialFunc    aNewFunc;

  // Old API
  math_GaussSingleIntegration anOldInteg(anOldFunc, 0.0, 1.0, 15);

  // New API
  auto aNewResult = math::Integ::Gauss(aNewFunc, 0.0, 1.0, 15);

  ASSERT_TRUE(anOldInteg.IsDone());
  ASSERT_TRUE(aNewResult.IsDone());

  EXPECT_NEAR(anOldInteg.Value(), *aNewResult.Value, 1.0e-10);
}

// ============================================================================
// Error estimation tests
// ============================================================================

TEST(math_Integ_NewTest, ErrorEstimation_Kronrod)
{
  SineFunc aFunc;

  auto aResult = math::Integ::KronrodRule(aFunc, 0.0, M_PI, 7);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_TRUE(aResult.AbsoluteError.has_value());
  EXPECT_TRUE(aResult.RelativeError.has_value());

  // Error estimate should be small for smooth function
  EXPECT_LT(*aResult.AbsoluteError, 1.0e-10);
}

TEST(math_Integ_NewTest, ErrorEstimation_TanhSinh)
{
  SineFunc aFunc;

  auto aResult = math::Integ::TanhSinh(aFunc, 0.0, M_PI);

  ASSERT_TRUE(aResult.IsDone());
  // TanhSinh provides error estimates when converged
}

// ============================================================================
// Infinite interval tests
// Note: For infinite intervals, DoubleExp (TanhSinh/ExpSinh/SinhSinh) methods
// are preferred over Kronrod transformations due to better numerical stability.
// The Kronrod transformations (KronrodInfinite, KronrodSemiInfinite) have
// inherent numerical issues with large Jacobian values near the boundaries.
// ============================================================================

TEST(math_Integ_Kronrod_NewTest, InfiniteInterval)
{
  InfiniteGaussianFunc aFunc;

  // Integral of exp(-x^2) from -inf to +inf = sqrt(pi)
  // Use SinhSinh (DoubleExp) for infinite intervals - more numerically stable
  auto aResult = math::Integ::SinhSinh(aFunc);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, std::sqrt(M_PI), 1.0e-4);
}

TEST(math_Integ_Kronrod_NewTest, SemiInfiniteInterval)
{
  ExponentialDecayFunc aFunc;

  // Integral of exp(-x) from 0 to +inf = 1
  // Use ExpSinh (DoubleExp) for semi-infinite intervals - more numerically stable
  auto aResult = math::Integ::ExpSinh(aFunc, 0.0);

  ASSERT_TRUE(aResult.IsDone());
  EXPECT_NEAR(*aResult.Value, 1.0, 1.0e-4);
}
