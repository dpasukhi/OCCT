# OCCT 8.0.0 — High-Impact / Low-Effort Architectural Proposals

**Status:** RFC (Request for Comments)
**Target Release:** OCCT 8.0.0
**Baseline Assumptions:** C++17 minimum, ABI/API breakage permitted
**Focus:** Foundation Classes (TKernel, TKMath) and Modeling Data (TKBRep, TKG3d)

This document proposes 7 specific architectural changes that are **not** on the
current roadmap. Each is selected for maximum value-to-effort ratio, with
emphasis on core data structures and geometry evaluation paths.

> **Already on the roadmap (not covered here):**
> Robust Geometric Predicates, Geom_Ellipsoid/Paraboloid (AP242),
> NCollection STL iterators + `std::pmr`, devirtualized math solvers,
> KD-Tree for point clouds.

---

## Table of Contents

| #  | Proposal | Module | Effort | Impact |
|----|----------|--------|--------|--------|
| 1  | `GeomAdaptor_Curve`: extend `std::variant` dispatch to all elementary types | Modeling Data | Low | Very High |
| 2  | `Geom_BSplineCurve/Surface`: consolidate 5 Handle arrays into single allocation | Modeling Data | Medium | Very High |
| 3  | `BRep_TEdge` curve representations: tagged union replacing virtual `Is*()` dispatch | Modeling Data | Medium | Very High |
| 4  | Scoped Enum (`enum class`) migration | Foundation + Modeling | Low | High |
| 5  | `STANDARD_ALIGNED` → `alignas` + dead compiler guard removal | Foundation Classes | Low | High |
| 6  | CMake Presets + Sanitizer / Fuzz-Testing targets | Build System | Low | High |
| 7  | `std::span` views + structured bindings for gp_* types | Foundation Classes | Low | High |

---

## 1. `GeomAdaptor_Curve`: Eliminate Virtual Dispatch for Elementary Curves

**Effort: Low** | **Impact: Very High**

### Problem

`GeomAdaptor_Curve` already uses `std::variant` to avoid virtual dispatch
for BSpline, Bezier, and Offset curves (`GeomAdaptor_Curve.hxx:66`):

```cpp
using CurveDataVariant = std::variant<std::monostate, OffsetData, BezierData, BSplineData>;
```

But the 5 **elementary analytical curve types** — Line, Circle, Ellipse,
Hyperbola, Parabola — all fall through to **virtual dispatch** in the
`default` case (`GeomAdaptor_Curve.cxx:652-653`):

```cpp
void GeomAdaptor_Curve::D0(const double U, gp_Pnt& P) const
{
  switch (myTypeCurve) {
    case GeomAbs_BezierCurve:  { /* cached eval */ break; }
    case GeomAbs_BSplineCurve: { /* cached eval */ break; }
    case GeomAbs_OffsetCurve:  { /* direct eval */ break; }
    default:
      myCurve->D0(U, P);  // ← VIRTUAL CALL for Line, Circle, etc.
  }
}
```

The virtual call dispatches to trivial implementations. For example,
`Geom_Line::D0` (`Geom_Line.cxx:170-172`) is just:

```cpp
void Geom_Line::D0(const double U, gp_Pnt& P) const {
  P = ElCLib::LineValue(U, pos);  // 3 multiply-adds, ~6 FLOPs
}
```

`ElCLib::LineValue` (`ElCLib.cxx:153-158`) is:
```cpp
return gp_Pnt(U * ZDir.X() + PLoc.X(),
              U * ZDir.Y() + PLoc.Y(),
              U * ZDir.Z() + PLoc.Z());
```

For a ~6-FLOP computation, the virtual dispatch overhead (vtable load +
indirect branch + possible misprediction) is proportionally very large.
The same applies to `Geom_Circle::D0` (sin/cos + a few multiplies) and
the other conics.

### Why It's Very High Impact

- **Lines and circles dominate real geometry.** In most CAD models, 30-60%
  of edges are lines or circular arcs. These are the curves evaluated most
  often during meshing, intersection, and display.
- **The type is already known.** `myTypeCurve` is set at `Load()` time.
  The adaptor already dispatches on it — the `default` fallback simply
  doesn't use this information for elementary types.
- **Same pattern as BSpline/Bezier.** The existing `case GeomAbs_BSplineCurve`
  avoids virtual dispatch by using cached data directly. The same idea
  applies to elementary curves, just without needing a cache (since their
  evaluation is analytical).

### Technical Approach

Add explicit `case` branches for the 5 elementary types, calling `ElCLib`
directly with a `static_cast` (safe because `myTypeCurve` was set from the
actual type in `load()`):

```cpp
void GeomAdaptor_Curve::D0(const double U, gp_Pnt& P) const
{
  switch (myTypeCurve)
  {
    case GeomAbs_Line: {
      const auto* L = static_cast<const Geom_Line*>(myCurve.get());
      P = ElCLib::LineValue(U, L->Position());
      break;
    }
    case GeomAbs_Circle: {
      const auto* C = static_cast<const Geom_Circle*>(myCurve.get());
      P = ElCLib::CircleValue(U, C->Position(), C->Radius());
      break;
    }
    case GeomAbs_Ellipse: {
      const auto* E = static_cast<const Geom_Ellipse*>(myCurve.get());
      P = ElCLib::EllipseValue(U, E->Position(), E->MajorRadius(), E->MinorRadius());
      break;
    }
    case GeomAbs_Hyperbola: {
      const auto* H = static_cast<const Geom_Hyperbola*>(myCurve.get());
      P = ElCLib::HyperbolaValue(U, H->Position(), H->MajorRadius(), H->MinorRadius());
      break;
    }
    case GeomAbs_Parabola: {
      const auto* Pb = static_cast<const Geom_Parabola*>(myCurve.get());
      P = ElCLib::ParabolaValue(U, Pb->Position(), Pb->Focal());
      break;
    }

    case GeomAbs_BezierCurve:  { /* existing cached path */ break; }
    case GeomAbs_BSplineCurve: { /* existing cached path */ break; }
    case GeomAbs_OffsetCurve:  { /* existing direct path */ break; }

    default:
      myCurve->D0(U, P);  // only GeomAbs_OtherCurve reaches here
  }
}
```

The same pattern applies to `D1`, `D2`, `D3`, and `DN` methods, using
`ElCLib::LineD1`, `ElCLib::CircleD1`, etc.

**Scope:** The same optimization applies to `GeomAdaptor_Surface` (planes,
cylinders, cones, spheres, tori fall through to virtual dispatch).
Also to `Geom2dAdaptor_Curve` for the 2D analogues.

### Risks

- **Very low.** The `static_cast` is safe because `myTypeCurve` is
  determined from `Geom_Curve::DynamicType()` in `load()` — it
  matches the actual type. This is the same pattern already used
  throughout `BRep_Tool` (e.g., `BRep_Tool.cxx:78`).
- No data structure changes. No new members. Just expanding the
  existing switch with cases that were already implicitly handled.
- The `default` path remains for `GeomAbs_OtherCurve` (user-defined
  or exotic curves), so extensibility is preserved.

---

## 2. `Geom_BSplineCurve/Surface`: Consolidate 5 Handle Arrays

**Effort: Medium** | **Impact: Very High**

### Problem

`Geom_BSplineCurve` stores its data in 5 separately heap-allocated arrays
(`Geom_BSplineCurve.hxx:830-834`):

```cpp
occ::handle<NCollection_HArray1<gp_Pnt>> poles;       // 3D control points
occ::handle<NCollection_HArray1<double>> weights;      // rational weights
occ::handle<NCollection_HArray1<double>> flatknots;    // expanded knot vector
occ::handle<NCollection_HArray1<double>> knots;        // distinct knots
occ::handle<NCollection_HArray1<int>>    mults;         // knot multiplicities
```

Each `occ::handle<NCollection_HArray1<T>>` is a **separate heap allocation**
with its own reference count and vtable. During BSpline evaluation — the
innermost loop of curve/surface processing — the algorithm accesses `poles[i]`,
`weights[i]`, and `flatknots` from three different memory regions.

`Geom_BSplineSurface` has the same pattern with even more arrays (poles as a
2D array, plus U/V knots, U/V multiplicities, U/V flat knots).

### Why It's Very High Impact

- **BSpline is the dominant curve type in real CAD models.** STEP files from
  automotive and aerospace are >90% BSpline curves and surfaces.
- **Evaluation is the #1 hot path.** Meshing, intersection, projection, and
  display all evaluate BSplines millions of times per operation.
- **5 Handle indirections per curve × millions of curves = measurable
  cache pressure.** Consolidating into a single contiguous buffer turns
  5 scattered memory regions into 1 sequential access.

### Technical Approach

Allocate a single contiguous buffer and use `std::span` views for each
logical array:

```cpp
class Geom_BSplineCurve : public Geom_BoundedCurve {
private:
  // Single allocation for all data
  std::vector<std::byte> myData;

  // Views into the contiguous buffer (no ownership, no allocation)
  std::span<gp_Pnt> myPoles;
  std::span<double>  myWeights;      // empty span if non-rational
  std::span<double>  myFlatKnots;
  std::span<double>  myKnots;
  std::span<int>     myMults;

  bool   myRational;
  bool   myPeriodic;
  int    myDeg;
  // ...

  void AllocateData(int nbPoles, int nbKnots, int nbFlatKnots, bool rational);
};
```

**`AllocateData()`** computes the total buffer size, allocates once, and
assigns each `std::span` to point at the appropriate offset:

```
[ poles (nbPoles × 24 bytes) | weights (nbPoles × 8 bytes, if rational) |
  flatknots (nbFlatKnots × 8 bytes) | knots (nbKnots × 8 bytes) |
  mults (nbKnots × 4 bytes) ]
```

**Alignment:** `gp_Pnt` is 8-byte aligned (3 doubles). The buffer is
`alignas(8)` which satisfies all element types.

**For `Geom_BSplineSurface`:** Same pattern with a 2D pole grid stored
row-major in the contiguous buffer.

### Migration Path

Internal code accesses poles/knots through member functions like `Pole(i)`,
`Knot(i)`, `Weight(i)`. These accessors change from dereferencing a Handle
to indexing a span — the public API is unchanged:

```cpp
// Before:
const gp_Pnt& Pole(int Index) const {
  return poles->Value(Index);   // Handle dereference + bounds check
}

// After:
const gp_Pnt& Pole(int Index) const {
  return myPoles[Index - 1];    // Direct span access
}
```

### Risks

- **Sharing semantics change:** Currently, `poles` can be shared between
  curves via reference counting (e.g., after `Copy()`). With a single buffer,
  sharing requires copying the entire buffer or using `std::shared_ptr<>`.
  In practice, BSpline data is rarely shared — `Copy()` already deep-copies.
- Serialization (`BinTools`) reads/writes each array separately. The
  serialization code needs to write from spans instead of Handle arrays —
  a straightforward change.

---

## 3. `BRep_TEdge`: Tagged Union for Curve Representations

**Effort: Medium** | **Impact: Very High**

### Problem

Each `BRep_TEdge` stores its curve representations in a linked list of
polymorphic Handle objects (`BRep_TEdge.hxx:81`):

```cpp
NCollection_List<occ::handle<BRep_CurveRepresentation>> myCurves;
```

`BRep_CurveRepresentation` has **10 virtual `Is*()` type-checking methods**
(`BRep_CurveRepresentation.hxx:40-90`):

```cpp
virtual bool IsCurve3D() const;
virtual bool IsCurveOnSurface() const;
virtual bool IsRegularity() const;
virtual bool IsCurveOnClosedSurface() const;
virtual bool IsPolygon3D() const;
virtual bool IsPolygonOnTriangulation() const;
// ... and more
```

`BRep_Tool` — **the most-called API in the entire BRep system** — iterates
this linked list on every geometry access, calling virtual type-checking
methods on each element (`BRep_Tool.cxx:166-190`):

```cpp
NCollection_List<occ::handle<BRep_CurveRepresentation>>::Iterator itcr(TE->Curves());
while (itcr.More()) {
  const occ::handle<BRep_CurveRepresentation>& cr = itcr.Value();
  if (cr->IsCurve3D()) {                    // virtual call
    const BRep_Curve3D* GC = static_cast<const BRep_Curve3D*>(cr.get());
    // ...
  }
  itcr.Next();                               // linked list pointer chase
}
```

There are **7 concrete leaf types** in the `BRep_CurveRepresentation`
hierarchy:
`BRep_Curve3D`, `BRep_CurveOnSurface`, `BRep_CurveOnClosedSurface`,
`BRep_CurveOn2Surfaces`, `BRep_Polygon3D`, `BRep_PolygonOnSurface`,
`BRep_PolygonOnClosedSurface`, `BRep_PolygonOnTriangulation`,
`BRep_PolygonOnClosedTriangulation`.

This is: **linked list** (bad cache locality) + **virtual dispatch for type
identification** (branch misprediction) + **Handle indirection per
element** (pointer chasing), in the most frequently called code path.

### Why It's Very High Impact

- **`BRep_Tool::Curve()`, `BRep_Tool::Surface()`, `BRep_Tool::Pnt()` are
  called millions of times** per meshing/boolean/export operation.
- Eliminating linked-list traversal + virtual dispatch in favor of a tagged
  union with linear scan over a contiguous array is a fundamental data-access
  improvement.
- A typical edge has 1-4 curve representations (one 3D curve + 1-2 pcurves
  + optional polygon). This is a perfect fit for a small inline vector.

### Technical Approach

**Step 1: Define a variant for curve representation types.**

```cpp
// Tagged union of all concrete representation types
using BRep_CurveRepVariant = std::variant<
  BRep_Curve3D,
  BRep_CurveOnSurface,
  BRep_CurveOnClosedSurface,
  BRep_CurveOn2Surfaces,
  BRep_Polygon3D,
  BRep_PolygonOnSurface,
  BRep_PolygonOnClosedSurface,
  BRep_PolygonOnTriangulation,
  BRep_PolygonOnClosedTriangulation
>;
```

**Step 2: Replace linked list with small vector of variants.**

```cpp
class BRep_TEdge : public TopoDS_TShape {
private:
  SmallVector<BRep_CurveRepVariant, 3> myCurves;  // Inline for ≤3 reps
  double myTolerance;
};
```

**Step 3: Replace virtual `Is*()` dispatch with `std::visit`.**

```cpp
// Before (virtual dispatch over linked list):
for (auto it = TE->Curves().begin(); it != TE->Curves().end(); ++it) {
  if ((*it)->IsCurve3D()) {
    auto* GC = static_cast<const BRep_Curve3D*>(it->get());
    return GC->Curve3D();
  }
}

// After (std::visit over contiguous variant array):
for (const auto& rep : TE->Curves()) {
  if (auto* c3d = std::get_if<BRep_Curve3D>(&rep)) {
    return c3d->Curve3D();
  }
}
```

Or with `std::visit` for exhaustive handling:

```cpp
for (const auto& rep : TE->Curves()) {
  std::visit(overloaded{
    [&](const BRep_Curve3D& c)          { /* handle 3D curve */ },
    [&](const BRep_CurveOnSurface& c)   { /* handle pcurve */ },
    [](const auto&)                      { /* skip others */ }
  }, rep);
}
```

### Variant vs Polymorphism Tradeoffs

| Aspect | Current (Handle + virtual) | Proposed (variant) |
|--------|---------------------------|-------------------|
| Type check | Virtual call (`IsCurve3D()`) | Index comparison (0-cost) |
| Memory layout | Linked list + heap per repr | Contiguous inline array |
| Per-element overhead | ~40 bytes (Handle + vtable + refcount) | 0 (value storage) |
| Exhaustiveness | Silent if `Is*()` check forgotten | Compiler-enforced with `std::visit` |

### Risks

- **Medium effort:** The 7 concrete `BRep_CurveRepresentation` types vary
  in size. The variant will be sized to the largest alternative. Measure
  the actual sizes to ensure the inline storage overhead is acceptable.
- `BRep_CurveRepresentation` types currently inherit from
  `Standard_Transient` (reference counted). Moving them to value types
  inside a variant requires ensuring no external code holds `Handle<>`
  references to individual curve representations. If this is a concern,
  the variant can store non-Transient value copies of the same data.
- `GeomAdaptor_Curve` already uses `std::variant` internally
  (`GeomAdaptor_Curve.hxx:40-66`), demonstrating the pattern is viable
  in OCCT.

---

## 4. Scoped Enum (`enum class`) Migration

**Effort: Low** | **Impact: High**

### Problem

OCCT has **441 files** with old-style C `enum` declarations versus **only 4**
using `enum class`. This is the single largest C++ modernization gap in the
Foundation Classes and Modeling Data modules.

Old-style enums pollute the enclosing namespace, allow implicit integer
conversions, and create name collisions. Critical examples from the core
modules:

```cpp
// TopAbs_ShapeEnum — used in every topology operation
enum TopAbs_ShapeEnum {
  TopAbs_COMPOUND, TopAbs_COMPSOLID, TopAbs_SOLID, TopAbs_SHELL,
  TopAbs_FACE, TopAbs_WIRE, TopAbs_EDGE, TopAbs_VERTEX, TopAbs_SHAPE
};

// GeomAbs_CurveType — used in every geometry adaptor dispatch
enum GeomAbs_CurveType {
  GeomAbs_Line, GeomAbs_Circle, GeomAbs_Ellipse, GeomAbs_Hyperbola,
  GeomAbs_Parabola, GeomAbs_BezierCurve, GeomAbs_BSplineCurve,
  GeomAbs_OffsetCurve, GeomAbs_OtherCurve
};
```

Nothing prevents `TopAbs_SOLID == GeomAbs_Parabola` from compiling.

### Why It's High Impact

- **Type safety in the topology/geometry core:** Prevents silent
  cross-enum comparisons that currently compile without warning.
- **Since OCCT 8.0 breaks API:** This is the once-in-a-decade window.
- **Mechanical transformation:** clang-tidy can automate the migration.
  Every missed usage site becomes a compile error, not a runtime bug.

### Technical Approach

1. **Specify explicit underlying type** for serialization compatibility:

```cpp
enum class TopAbs_ShapeEnum : int {
  COMPOUND, COMPSOLID, SOLID, SHELL,
  FACE, WIRE, EDGE, VERTEX, SHAPE
};
```

2. **Phased approach by module:**
   - Phase 1: `TopAbs_*`, `GeomAbs_*`, `BRep_*` (Foundation/Modeling core)
   - Phase 2: `Graphic3d_*`, `AIS_*` (Visualization)
   - Phase 3: `StepData_*`, `IGESData_*` (Data Exchange)

3. **Note:** `TopoDS_TShape` already uses a scoped `enum BitLayout : uint16_t`
   (`TopoDS_TShape.hxx:69-82`) — the pattern is proven in the codebase.

### Risks

- Large number of files touched (441+), but the compiler enforces
  correctness — every missed usage site is a compile error.

---

## 5. `STANDARD_ALIGNED` → `alignas` + Dead Compiler Guard Removal

**Effort: Low** | **Impact: High**

### Problem

`Standard_DefineAlloc.hxx` contains:

1. **`STANDARD_ALIGNED` macro** (lines 77-91): Platform-specific alignment
   using `__declspec(align())` and `__attribute__((aligned()))`. Superseded
   by C++11 `alignas()`.

2. **Dead compiler guards** for:
   - Borland C++ (`__BORLANDC__`, lines 38-43) — discontinued 2009.
   - Sun Studio <= 5.3 (`__SUNPRO_CC <= 0x530`, lines 21-33) — released 2003.
   - Sun Studio <= 4.2 (`__SUNPRO_CC <= 0x420`, lines 67-75) — released ~2000.

These compilers cannot build C++17 code. The guards are dead code that adds
cognitive load for every developer who reads the file.

### Why It's High Impact

- Reduces the macro surface area in one of the most-included headers.
- Removes misleading code paths that suggest these compilers are supported.
- `alignas()` is cleaner and IDE-friendly (syntax highlighting, jump-to-def).

### Technical Approach

```cpp
// BEFORE:
static const STANDARD_ALIGNED(8, char, THE_ARRAY)[] = {0xFF, ...};

// AFTER:
alignas(8) static const char THE_ARRAY[] = {0xFF, ...};
```

Delete all `__BORLANDC__` and `__SUNPRO_CC` guarded code paths.
Delete `WORKAROUND_SUNPRO_NEW_PLACEMENT` block.
Simplify `DEFINE_STANDARD_ALLOC_ARRAY` and `DEFINE_STANDARD_ALLOC_PLACEMENT`
by removing the dead branches — only the standard C++ path remains.

### Risks

- Near zero. These compilers cannot build C++17 code.

---

## 6. CMake Presets + Sanitizer / Fuzz-Testing Targets

**Effort: Low** | **Impact: High**

### Problem

The build system (`CMakeLists.txt:1`) targets CMake 3.10 (but already
requires 3.16 for PCH at line 214) and has no built-in support for:

- **CMake Presets** (`CMakePresets.json`, CMake 3.21+) for reproducible builds.
- **AddressSanitizer / UBSanitizer** integration.
- **Fuzz testing** for geometry file parsers (STEP, IGES, STL, glTF).

The Foundation Classes contain code with sanitizer-detectable issues. For
example, `std::hash<gp_Pnt>` (`gp_Pnt.hxx:221-234`) uses union type-punning
(double→int reinterpretation) which is technically undefined behavior in C++.

### Why It's High Impact

- **Reproducible CI:** CMake Presets give every contributor identical build
  configurations across platforms.
- **Memory safety:** ASAN catches buffer overflows in mesh processing
  (`Poly_Triangulation`, BRep I/O) invisible to unit tests.
- **Parser robustness:** STEP/IGES parsers process untrusted external input.
  Fuzzing finds crash-inducing edge cases that manual tests miss.

### Technical Approach

1. **Bump minimum CMake to 3.16** (already effectively required for PCH).

2. **Add `CMakePresets.json`** with presets: `dev-debug`, `dev-asan`,
   `dev-ubsan`, `release`, `release-lto`.

3. **Add fuzz harnesses** (< 50 lines each) for STEP/IGES/STL readers.

4. **CI integration:** ASAN/UBSAN test job catches regressions immediately.

### Risks

- Purely additive build-system changes. No source code modifications.
- Fuzzing may find real bugs. This is the point.

---

## 7. `std::span` Views + Structured Bindings for `gp_*` Types

**Effort: Low** | **Impact: High**

### Problem

The `gp_*` value types (`gp_Pnt`, `gp_Vec`, `gp_Dir`, `gp_XYZ`) are already
well-modernized with `constexpr`, `[[nodiscard]]`, and operator overloads.
But two C++17 opportunities remain unused:

**A. No `std::span` support for array views.**

Code that passes geometry arrays currently requires concrete container types:

```cpp
void ProcessPoints(const NCollection_Array1<gp_Pnt>& points);
void ProcessPoints(const std::vector<gp_Pnt>& points);
// Two overloads for the same operation
```

**B. No structured bindings support.**

```cpp
gp_Pnt p(1.0, 2.0, 3.0);
auto [x, y, z] = p;  // Does not compile — no tuple protocol
```

Extracting coordinates requires verbose getter calls:
```cpp
double x = p.X(), y = p.Y(), z = p.Z();
```

### Why It's High Impact

- **`std::span<const gp_Pnt>`** unifies all contiguous array types under a
  single non-owning view. Algorithms accept any contiguous source
  (`NCollection_Array1`, `std::vector`, raw pointer + size, `Poly_ArrayOfNodes`)
  without templates or overloads.
- **Structured bindings** make geometry code dramatically more readable.
  In algorithms that compute with coordinates (distances, intersections,
  projections), `auto [x, y, z]` reduces noise significantly.

### Technical Approach

**A. `std::span` for array-accepting APIs:**

```cpp
// Single signature accepts any contiguous source:
void ProcessPoints(std::span<const gp_Pnt> points);

// Callers:
NCollection_Array1<gp_Pnt> arr(1, 100);
ProcessPoints(std::span(arr.begin(), arr.end()));

std::vector<gp_Pnt> vec;
ProcessPoints(vec);  // implicit conversion
```

`NCollection_Array1` already provides `begin()`/`end()` with random-access
iterators (`NCollection_Array1.hxx:73-80`), so `std::span` construction works
directly.

For `Poly_ArrayOfNodes` (which has a configurable stride and float/double
precision), a `std::span`-like accessor method can expose the underlying
contiguous memory:

```cpp
class Poly_ArrayOfNodes {
public:
  template<typename T>
  std::span<const T> Span() const;  // Returns view of the raw data
};
```

**B. Structured bindings via tuple protocol for `gp_XYZ`:**

```cpp
// In gp_XYZ.hxx — add tuple protocol:
namespace std {
  template<> struct tuple_size<gp_XYZ> : integral_constant<size_t, 3> {};
  template<size_t I> struct tuple_element<I, gp_XYZ> { using type = double; };
}

template<size_t I>
constexpr double get(const gp_XYZ& xyz) noexcept {
  if constexpr (I == 0) return xyz.X();
  else if constexpr (I == 1) return xyz.Y();
  else return xyz.Z();
}

// For gp_Pnt, gp_Vec, gp_Dir — delegate to their XYZ():
namespace std {
  template<> struct tuple_size<gp_Pnt> : integral_constant<size_t, 3> {};
  template<size_t I> struct tuple_element<I, gp_Pnt> { using type = double; };
}

template<size_t I>
constexpr double get(const gp_Pnt& p) noexcept { return get<I>(p.XYZ()); }
```

**Usage:**

```cpp
gp_Pnt p(1.0, 2.0, 3.0);
auto [x, y, z] = p;  // x=1.0, y=2.0, z=3.0

// In algorithm code:
for (const auto& pt : points) {
  auto [px, py, pz] = pt;
  // ... compute with px, py, pz directly
}
```

### Risks

- **Structured bindings:** Adding `std::tuple_size` and `get<>()` for a type
  is a non-breaking addition. It doesn't change the type's layout, size, or
  existing API. The only risk is name collision if `get()` is defined
  elsewhere — use a dedicated namespace or friend function to avoid this.
- **`std::span`:** Requires C++20 for `std::span` itself. For C++17, use a
  lightweight `tcb::span` polyfill or OCCT's own `NCollection_Span` if one
  exists. Alternatively, define API signatures using iterator pairs or
  `(const T* data, size_t count)`.

---

## Summary Matrix

| # | Proposal | Module | Effort | Impact | Key Benefit |
|---|----------|--------|--------|--------|-------------|
| 1 | GeomAdaptor: devirtualize elementary curves | Modeling Data | Low | Very High | Eliminate virtual dispatch for Line/Circle/Ellipse evaluation (~60% of real geometry) |
| 2 | BSpline single-allocation layout | Modeling Data | Medium | Very High | 5 scattered Handle arrays → 1 contiguous buffer for the dominant curve type |
| 3 | BRep_TEdge variant curve reps | Modeling Data | Medium | Very High | Remove virtual `Is*()` dispatch + linked list in `BRep_Tool` hot path |
| 4 | `enum class` migration | Foundation + Modeling | Low | High | Type safety across 441 enum definitions |
| 5 | `alignas` + dead guard removal | Foundation Classes | Low | High | Clean dead Borland/SunPro code, replace `STANDARD_ALIGNED` macro |
| 6 | CMake Presets + Sanitizers + Fuzzing | Build System | Low | High | Reproducible builds, parser robustness, memory safety CI |
| 7 | `std::span` + structured bindings | Foundation Classes | Low | High | Unified array views, `auto [x, y, z] = point` syntax |

---

## Recommended Priority Order

1. **Proposals 4, 5** (Low effort, mechanical cleanup) — do first. They
   clean up Foundation Classes and make subsequent work in Modeling Data
   easier.

2. **Proposal 6** (Low effort, infrastructure) — set up sanitizers before
   large refactoring to catch regressions early.

3. **Proposal 1** (Low effort, very high impact) — devirtualize elementary
   curve evaluation in `GeomAdaptor_Curve`. Lines and circles are 30-60%
   of real geometry; eliminating virtual dispatch for a ~6-FLOP computation
   is a large proportional win. Same pattern extends to `GeomAdaptor_Surface`.

4. **Proposal 7** (Low effort, developer experience) — small additions to
   well-understood `gp_*` types. Can be done incrementally.

5. **Proposals 2 and 3** (Medium effort, very high impact) — the biggest
   Modeling Data performance wins. Can be developed independently by
   separate contributors.
