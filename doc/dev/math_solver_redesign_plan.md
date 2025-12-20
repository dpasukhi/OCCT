# Math Solver Redesign Plan

## Overview

This document outlines the redesign of the `src/FoundationClasses/TKMath/math` package solvers using modern C++17 features, template-based APIs, and namespace organization.

### Goals

- **Modernize API**: Use C++17 features (`std::optional`, `std::array`, `constexpr`)
- **Simplify Usage**: Pure functions instead of classes with state
- **Improve Flexibility**: Templates for custom function types
- **Non-Virtual Functors**: Provide ready-to-use functor classes without virtual dispatch
- **Reduce Compile Time**: Granular includes per algorithm
- **Avoid Code Duplication**: Shared internal utilities
- **Follow OCCT Conventions**: Naming (theParam, aVar, myField), native C++ types

### Non-Goals

- Redesign `math_Vector`, `math_Matrix`, and other containers (they remain as-is)
- Change existing algorithm logic (preserve numerical behavior)

---

## File Structure

```
src/FoundationClasses/TKMath/math/
├── math_Types.hxx                          # Result types, status enum
├── math_Config.hxx                         # Configuration structs
│
├── math_Roots.hxx                          # Root finding umbrella
├── math_Roots_Newton.hxx                   # Newton-Raphson, NewtonBounded
├── math_Roots_Brent.hxx                    # Brent's method
├── math_Roots_Bisection.hxx                # Bisection, BisectionNewton
├── math_Roots_Secant.hxx                   # Secant method
│
├── math_Poly.hxx                           # Polynomial roots umbrella
├── math_Poly_Quadratic.hxx                 # Linear (ax+b=0), Quadratic
├── math_Poly_Cubic.hxx                     # Cubic equations
├── math_Poly_Quartic.hxx                   # Quartic equations
│
├── math_Min.hxx                            # Minimization umbrella
├── math_Min_Brent.hxx                      # 1D Brent, Golden section
├── math_Min_Powell.hxx                     # N-D Powell (gradient-free)
├── math_Min_BFGS.hxx                       # N-D BFGS (gradient)
├── math_Min_FRPR.hxx                       # N-D Fletcher-Reeves-Polak-Ribiere
├── math_Min_Newton.hxx                     # N-D Newton (Hessian)
├── math_Min_PSO.hxx                        # Particle Swarm Optimization
├── math_Min_GlobOpt.hxx                    # Global optimization (DE, MultiStart, PSOHybrid)
│
├── math_Sys.hxx                            # Systems umbrella
├── math_Sys_Newton.hxx                     # Newton for nonlinear systems
├── math_Sys_LevenbergMarquardt.hxx         # Levenberg-Marquardt
│
├── math_Lin.hxx                            # Linear algebra umbrella
├── math_Lin_Gauss.hxx                      # Gaussian elimination, LU
├── math_Lin_SVD.hxx                        # SVD decomposition
├── math_Lin_Householder.hxx                # Householder QR
├── math_Lin_Jacobi.hxx                     # Jacobi eigenvalues
├── math_Lin_LeastSquares.hxx               # Least squares solver
│
├── math_Integ.hxx                          # Integration umbrella
├── math_Integ_Gauss.hxx                    # Gauss-Legendre quadrature
├── math_Integ_Kronrod.hxx                  # Gauss-Kronrod adaptive
├── math_Integ_DoubleExp.hxx                # TanhSinh, ExpSinh, SinhSinh
│
├── math_Functor.hxx                        # Non-virtual functor base (NEW)
├── math_Functor_Scalar.hxx                 # Scalar function functors (NEW)
├── math_Functor_Vector.hxx                 # Vector function functors (NEW)
│
└── math_Internal*.hxx                      # Internal utilities
```

---

## Implementation Status

### Completed (Phase 1-8)

| Category | Files | Status |
|----------|-------|--------|
| **Types & Config** | `math_Types.hxx`, `math_Config.hxx` | ✅ Done |
| **Root Finding** | `math_Roots_*.hxx` (Newton, Brent, Bisection, Secant) | ✅ Done |
| **Polynomials** | `math_Poly_*.hxx` (Linear, Quadratic, Cubic, Quartic) | ✅ Done |
| **1D Minimization** | `math_Min_Brent.hxx` (Brent, Golden) | ✅ Done |
| **N-D Minimization** | `math_Min_*.hxx` (Powell, BFGS, FRPR, Newton, PSO, GlobOpt) | ✅ Done |
| **Integration** | `math_Integ_*.hxx` (Gauss, Kronrod, DoubleExp) | ✅ Done |
| **Linear Algebra** | `math_Lin_*.hxx` (Gauss, SVD, Householder, Jacobi, LeastSquares) | ✅ Done |
| **Nonlinear Systems** | `math_Sys_*.hxx` (Newton, LevenbergMarquardt) | ✅ Done |
| **Internal Utilities** | `math_Internal*.hxx` | ✅ Done |

### Test Status

- **Total GTests**: 2649 - All Pass (100%)
- **Math-specific Tests**: 844 - All Pass (100%)

---

## Ongoing Work (Phase 9)

### Phase 9: Migrate Remaining Old Algorithms

Old classes that need modern API wrappers:

| Old Class | New Location | Description | Status |
|-----------|--------------|-------------|--------|
| `math_BissecNewton` | `math_Roots_Bisection.hxx` | Hybrid bisection-Newton | ✅ Done |
| `math_FunctionRoots` | `math_Roots_Multiple.hxx` | Multiple roots finder | ✅ Done |
| `math_FunctionAllRoots` | `math_Roots_All.hxx` | All roots with intervals | ⏳ In Progress |
| `math_TrigonometricFunctionRoots` | `math_Roots_Trig.hxx` | Trigonometric equations | ⏳ In Progress |
| `math_GaussMultipleIntegration` | `math_Integ_Multiple.hxx` | N-D Gauss integration | ⏳ In Progress |
| `math_GaussSetIntegration` | `math_Integ_Set.hxx` | Vector function integration | ⏳ In Progress |
| `math_Crout` | `math_Lin_Crout.hxx` | Symmetric matrix LDL^T | ⏳ In Progress |
| `math_EigenValuesSearcher` | `math_Lin_EigenSearch.hxx` | Tridiagonal eigenvalues | ⏳ In Progress |
| `math_Uzawa` | `math_Opt_Uzawa.hxx` | Constrained optimization | ⏳ In Progress |

#### Algorithm Details

**math_Roots_All.hxx** - Sample-based root finding:
- Uses function sampling to find null intervals
- Refines interval bounds with root finding
- Returns both isolated roots and null intervals

**math_Roots_Trig.hxx** - Trigonometric equation solver:
- Solves: a·cos²(x) + 2b·cos(x)·sin(x) + c·cos(x) + d·sin(x) + e = 0
- Supports degree 2, 3, 4 equations
- Returns roots in specified interval [InfBound, SupBound]

**math_Integ_Multiple.hxx** - Multi-dimensional Gauss integration:
- N-dimensional tensor product quadrature
- Configurable order per dimension
- Maximum 61 points per dimension

**math_Integ_Set.hxx** - Vector function integration:
- Integrates vector-valued functions F: R → R^n
- Returns vector of integrals
- Note: M>1 input dimensions not implemented in legacy

**math_Lin_Crout.hxx** - Symmetric matrix decomposition:
- A = L·D·L^T where L is lower triangular, D is diagonal
- Faster than Gauss for symmetric matrices
- Only lower triangle needed as input

**math_Lin_EigenSearch.hxx** - Tridiagonal eigenvalue solver:
- QR algorithm with implicit shifts
- Input: diagonal and subdiagonal elements
- Output: eigenvalues and orthonormal eigenvectors

**math_Opt_Uzawa.hxx** - Constrained least squares:
- Solves C·X = S with min ||X - X₀||
- Supports equality and inequality constraints
- Uses dual variables for constraint handling

---

## Completed (Phase 10)

### Phase 10: Non-Virtual Functors ✅

Created ready-to-use functor classes that work with the new template API without virtual dispatch overhead.

**Implemented Files:**
- `math_Functor.hxx` - Umbrella header
- `math_Functor_Scalar.hxx` - Scalar function functors
- `math_Functor_Vector.hxx` - Vector function functors

**Scalar Functors (math::Functor namespace):**
- `ScalarLambda<Lambda>` - Lambda wrapper for value-only functions
- `ScalarLambdaWithDerivative<Lambda>` - Lambda wrapper with derivative
- `Polynomial` - Polynomial functor with Horner's method
- `Rational` - Rational function P(x)/Q(x)
- `Composite<F,G>` - Function composition f(g(x))
- `Sum<F,G>`, `Difference<F,G>` - Arithmetic combinations
- `Product<F,G>`, `Quotient<F,G>` - Multiplication/division
- `Scaled<F>`, `Shifted<F>`, `Negated<F>` - Transformations
- `Constant`, `Linear` - Simple functions
- `Sine`, `Cosine`, `Exponential`, `Power`, `Gaussian` - Common functions

**Vector Functors:**
- `VectorLambda<Lambda>` - Lambda wrapper for N-D functions
- `VectorLambdaWithGradient<V,G>` - With gradient support
- `QuadraticForm` - x^T A x + b^T x + c
- `Rosenbrock`, `Booth`, `Beale`, `Himmelblau` - Test functions
- `Sphere`, `Rastrigin`, `Ackley` - Optimization benchmarks
- `LinearResidual` - ||Ax - b||^2 for least squares
- `SystemLambda<Lambda>` - Nonlinear system wrapper

**Tests:** 47 new tests, all passing.

#### Usage Examples

```cpp
#include <math_Roots.hxx>
#include <math_Functor_Scalar.hxx>

// Using lambda wrapper
auto aLambda = math::Functor::ScalarLambda([](double x, double& y) {
  y = x * x - 2.0;  // Find sqrt(2)
  return true;
});
auto aResult = math::Roots::Brent(aLambda, 0.0, 2.0);

// Using polynomial functor
math::Functor::Polynomial aPoly({-6.0, 11.0, -6.0, 1.0});  // x^3 - 6x^2 + 11x - 6
auto aRoots = math::Roots::Newton(aPoly, 0.5);

// Using composite functor
math::Functor::Polynomial aInner({0.0, 1.0});  // x
math::Functor::Polynomial aOuter({-1.0, 0.0, 1.0});  // x^2 - 1
auto aComposite = math::Functor::Composite(aOuter, aInner);  // x^2 - 1
```

### Phase 11: Performance Benchmarks

| Task | Priority | Description |
|------|----------|-------------|
| Benchmark new vs old API | Low | Compare performance of template vs virtual dispatch |
| Memory usage analysis | Low | Compare memory footprint |
| Compile time comparison | Low | Measure compile time impact of templates |

---

## Migration Guide

### Old API → New API

```cpp
// OLD: Using virtual base class
class MyFunc : public math_FunctionWithDerivative
{
public:
  bool Value(double X, double& F) override { F = X*X - 2; return true; }
  bool Derivative(double X, double& D) override { D = 2*X; return true; }
};
MyFunc aFunc;
math_NewtonFunctionRoot aSolver(aFunc, 1.0, 1e-10, 1e-10);
double aRoot = aSolver.Root();

// NEW: Using template function
class MyFunc  // No inheritance needed
{
public:
  bool Values(double theX, double& theF, double& theDF)
  {
    theF = theX * theX - 2.0;
    theDF = 2.0 * theX;
    return true;
  }
};
MyFunc aFunc;
auto aResult = math::Roots::Newton(aFunc, 1.0);
if (aResult) double aRoot = *aResult.Root;

// NEW: Using lambda (even simpler)
auto aResult = math::Roots::Brent(
  [](double x, double& y) { y = x*x - 2; return true; },
  0.0, 2.0);
```

---

## Namespace Structure

```cpp
namespace math
{
  // Status and result types
  enum class Status { OK, NotConverged, MaxIterations, ... };
  struct ScalarResult { ... };
  struct VectorResult { ... };
  struct PolyResult { ... };
  struct IntegResult { ... };
  struct LinearResult { ... };

  // Configuration structs
  struct Config { ... };
  struct BoundedConfig : Config { ... };
  struct NDimConfig : Config { ... };
  struct IntegConfig { ... };

  namespace Poly { Linear(), Quadratic(), Cubic(), Quartic() }
  namespace Roots { Newton(), Brent(), Bisection(), Secant() }
  namespace Min { Brent(), Golden(), Powell(), BFGS(), FRPR(), Newton(), PSO(), GlobalMinimum() }
  namespace Integ { Gauss(), Kronrod(), TanhSinh(), ExpSinh(), SinhSinh() }
  namespace Lin { Gauss(), SVD(), Householder(), Jacobi(), LeastSquares() }
  namespace Sys { Newton(), LevenbergMarquardt() }
  namespace Functor { ScalarLambda, Polynomial, Rational, ... }  // NEW
  namespace Internal { ... }  // Not public API
}
```

---

## Summary

| Phase | Status | Description |
|-------|--------|-------------|
| 1-7 | ✅ Complete | Core implementation (Types, Config, all algorithm headers) |
| 8 | ✅ Complete | Bug fixes (MultiStart, TanhSinh, Kronrod, PSO tolerances) |
| 9 | ⏳ In Progress | Migrate remaining old algorithms |
| 10 | ✅ Complete | Non-virtual functors (47 tests) |
| 11 | ⏳ Pending | Performance benchmarks |
