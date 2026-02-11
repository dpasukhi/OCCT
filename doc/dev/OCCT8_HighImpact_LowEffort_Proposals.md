# OCCT 8.0.0 — High-Impact / Low-Effort Architectural Proposals

**Status:** RFC (Request for Comments)
**Target Release:** OCCT 8.0.0
**Baseline Assumptions:** C++17 minimum, ABI/API breakage permitted

This document proposes 7 specific architectural changes that are **not** on the
current roadmap. Each is selected for maximum value-to-effort ratio.

> **Already on the roadmap (not covered here):**
> Robust Geometric Predicates, Geom_Ellipsoid/Paraboloid (AP242),
> NCollection STL iterators + `std::pmr`, devirtualized math solvers,
> KD-Tree for point clouds.

---

## Table of Contents

| #  | Proposal | Effort | Impact |
|----|----------|--------|--------|
| 1  | Eliminate `DEFINE_STANDARD_ALLOC` macro | Low | Very High |
| 2  | GPU ID-Buffer Selection (Color Picking) | Medium | Very High |
| 3  | Scoped Enum (`enum class`) Migration | Low | High |
| 4  | CMake Presets + Sanitizer / Fuzz-Testing Targets | Low | High |
| 5  | Replace `STANDARD_ALIGNED` and Dead Compiler Guards | Low | High |
| 6  | `std::string_view` at API Boundaries | Medium | High |
| 7  | Flat Indexed BRep Representation for Large Assemblies | Medium | Very High |

---

## 1. Eliminate `DEFINE_STANDARD_ALLOC` Macro

**Effort: Low** | **Impact: Very High**

### Problem

The `DEFINE_STANDARD_ALLOC` macro (`Standard_DefineAlloc.hxx:54-64`) injects
per-class `operator new`/`operator delete` overrides that redirect to
`Standard::Allocate()` / `Standard::Free()`. It appears in virtually every
class in the codebase — including trivially copyable value types like `gp_Pnt`
(`gp_Pnt.hxx:34`) where it is semantically meaningless noise.

The macro also includes dead workarounds for Borland C++ and Sun Studio <= 5.3,
compilers that no modern CI will ever target.

### Why It's High Impact

- **Code noise reduction:** Removing a macro from thousands of class
  definitions massively improves readability.
- **Simplifies value types:** Types like `gp_Pnt`, `gp_Vec`, `gp_Dir`,
  `gp_XYZ` are `constexpr`-constructible POD-like types. They should not carry
  custom allocation operators. This enables compilers to treat them as truly
  trivial where applicable.
- **Enables standard tooling:** Custom `operator new` interferes with
  AddressSanitizer's allocator replacement, `std::make_shared` optimization,
  and allocator-aware containers.

### Technical Approach

1. **For non-`Standard_Transient` value types** (gp_*, TColgp_*, Bnd_Box,
   etc.): Remove `DEFINE_STANDARD_ALLOC` entirely. These types should use
   the global allocator (which can be jemalloc/TBB at the process level).

2. **For `Standard_Transient`-derived types**: The memory manager override can
   be handled once in `Standard_Transient::operator new/delete` via
   inheritance, eliminating the need for the macro in every derived class.

3. **For the `USE_MMGR_TYPE` CMake option**: Switch to a global
   `operator new`/`delete` replacement strategy (the same approach jemalloc
   and TBB already support natively via `LD_PRELOAD` or link-time injection).

4. **Remove `STANDARD_ALIGNED` macro** — replaced by C++11 `alignas()`.

5. **Remove all Borland/SunPro compiler guards** in `Standard_DefineAlloc.hxx`.

### Migration Path

```cpp
// BEFORE (every class, everywhere):
class gp_Pnt {
public:
  DEFINE_STANDARD_ALLOC    // <-- noise
  constexpr gp_Pnt() = default;
  ...
};

// AFTER:
class gp_Pnt {
public:
  constexpr gp_Pnt() = default;
  ...
};
```

For `Standard_Transient` hierarchy, the override lives in the base:

```cpp
class Standard_Transient {
public:
  // Single point of memory manager integration
  static void* operator new(std::size_t sz);
  static void  operator delete(void* ptr) noexcept;
  ...
};
// All derived classes inherit this automatically — no macro needed.
```

### Risks

- Low. This is a mechanical transformation. The `USE_MMGR_TYPE` feature is
  preserved by moving the override to the base class or using global
  replacement. A `static_assert` or compile-time check can verify no class
  accidentally reintroduces custom allocation.

---

## 2. GPU ID-Buffer Selection (Color Picking)

**Effort: Medium** | **Impact: Very High**

### Problem

All selection in OCCT is currently CPU-based: BVH traversal in
`SelectMgr_SelectingVolumeManager` with frustum overlap tests on every
`Select3D_SensitiveEntity`. For scenes with millions of triangles, point-click
selection becomes a bottleneck, especially on-demand interactive picking.

There is no GPU-accelerated selection path (confirmed: no `OpenGl_Selection`,
no compute-shader picking, no depth/ID readback in `TKOpenGl`).

### Why It's High Impact

- **O(1) pick under cursor** instead of O(n log n) BVH traversal.
- **Scales to arbitrarily complex scenes** — rendering is already GPU-bound;
  ID pass adds negligible overhead.
- **Industry standard:** Every modern CAD viewer (Hoops, VTK, Three.js) uses
  GPU picking for interactive selection. Its absence is a visible competitive
  gap.

### Technical Approach

1. **Offscreen FBO with integer color attachment**: Allocate an
   `OpenGl_FrameBuffer` with `GL_R32UI` (or `GL_RG32UI` for sub-element IDs)
   as a color attachment and a depth attachment.

2. **ID encoding shader**: A minimal fragment shader that writes a packed
   `(objectID, triangleID)` pair. Object IDs come from
   `SelectMgr_SelectableObject::GlobalSelectionId()` (already assigned).
   Triangle/element IDs are passed via a uniform or vertex attribute.

3. **Readback**: On pick, call `glReadPixels` for a 1x1 (or small NxN for
   proximity tolerance) region. Decode the ID, map back to the
   `SelectMgr_EntityOwner`.

4. **Integration point**: Add `OpenGl_View::SelectByIdBuffer()` as an
   alternative path in `SelectMgr_ViewerSelector::Pick()`. The BVH path
   remains available for rubber-band / polyline selection where GPU picking
   is less natural.

5. **Sub-element selection**: For face/edge/vertex level selection (common in
   CAD), encode the sub-element mode in the upper bits of the ID. The
   `Select3D_SensitivePrimitiveArray` patch size mechanism
   (`Select3D_SensitivePrimitiveArray.hxx:39-43`) can directly map to GPU
   sub-triangle IDs.

### Architecture Sketch

```
User Click (x, y)
       │
       ▼
┌──────────────────────┐
│ OpenGl_View          │
│  ├─ Render ID pass   │ ← Offscreen FBO, ID shader
│  ├─ glReadPixels     │ ← 1x1 readback at (x,y)
│  └─ Decode ID        │
└──────────┬───────────┘
           │ (objectID, subElementID)
           ▼
┌──────────────────────┐
│ SelectMgr            │
│  └─ Map ID → Owner   │ ← O(1) hash lookup
└──────────────────────┘
```

### Risks

- Requires OpenGL 3.0+ (integer textures) — but OCCT already targets 3.3+
  core profile.
- `glReadPixels` has a GPU-CPU sync cost on the first call. Mitigated by
  async PBO readback or by deferring the decode to the next frame.
- Does not replace BVH selection for non-interactive queries (ray casting,
  proximity analysis). Both paths coexist.

---

## 3. Scoped Enum (`enum class`) Migration

**Effort: Low** | **Impact: High**

### Problem

OCCT has **441 files** with old-style C `enum` declarations versus **only 4**
using `enum class`. This is the single largest C++ modernization gap in the
codebase.

Old-style enums pollute the enclosing namespace, allow implicit integer
conversions, and create name collisions. Example from
`TDataXtd_ConstraintEnum.hxx`:

```cpp
enum TDataXtd_ConstraintEnum {
  TDataXtd_RADIUS,
  TDataXtd_DIAMETER,
  TDataXtd_MINOR_RADIUS,
  // ... 26 values, all in global scope
};
```

### Why It's High Impact

- **Type safety:** Prevents a `TopAbs_ShapeEnum` from being silently compared
  with a `GeomAbs_CurveType`. These bugs are real and subtle.
- **Namespace cleanliness:** Every enum value currently pollutes the containing
  namespace. With `enum class`, values are scoped.
- **IDE support:** Better autocomplete, refactoring, and static analysis.
- **Since OCCT 8.0 breaks API:** This is the once-in-a-decade window to do
  this migration.

### Technical Approach

1. **Automated migration with clang-tidy**: The
   `modernize-use-using` and custom AST matchers can convert `enum` to
   `enum class` and update all usage sites in a single pass.

2. **Explicit underlying type**: Specify `enum class Foo : int` (or
   `: uint8_t` where range allows) to control serialization width. This is
   important for binary persistence (`BinTools`) compatibility.

3. **Serialization compatibility**: Binary format readers/writers already use
   `Standard_Integer` for enum I/O. Adding a `static_cast` at the
   serialization boundary is sufficient.

4. **Phased approach** (if preferred):
   - Phase 1: Core math/geometry enums (`TopAbs_ShapeEnum`, `GeomAbs_*`,
     `BRepBuilderAPI_*`) — most impactful, most referenced.
   - Phase 2: Visualization enums (`Graphic3d_*`, `AIS_*`, `V3d_*`).
   - Phase 3: Data exchange enums (`StepData_*`, `IGESData_*`).

### Migration Example

```cpp
// BEFORE:
enum TopAbs_ShapeEnum {
  TopAbs_COMPOUND,
  TopAbs_COMPSOLID,
  TopAbs_SOLID,
  // ...
};

// AFTER:
enum class TopAbs_ShapeEnum : int {
  COMPOUND,
  COMPSOLID,
  SOLID,
  // ...
};
// Usage: TopAbs_ShapeEnum::COMPOUND
```

### Compatibility Utilities

For downstream code, provide a temporary header with `using` aliases or
a migration guide. Since 8.0 breaks API, the old names do not need to be
preserved indefinitely.

### Risks

- Large number of files touched (441+). But the transformation is mechanical
  and verifiable by the compiler — every missed usage site becomes a compile
  error, not a silent runtime bug.

---

## 4. CMake Presets + Sanitizer / Fuzz-Testing Targets

**Effort: Low** | **Impact: High**

### Problem

The current `CMakeLists.txt` targets CMake 3.10 (`CMakeLists.txt:1`) and has no
built-in support for:

- **CMake Presets** (`CMakePresets.json`, requires 3.21+) for reproducible
  build configurations.
- **AddressSanitizer / UndefinedBehaviorSanitizer** integration.
- **Fuzz testing** for geometry file parsers (STEP, IGES, STL, OBJ, glTF).

STEP/IGES parsers process untrusted input from external files. The
`std::hash<gp_Pnt>` specialization (`gp_Pnt.hxx:221-234`) uses a union
type-punning trick that is technically UB in C++. These are exactly the
kinds of issues sanitizers and fuzzers catch.

### Why It's High Impact

- **Reproducible CI**: CMake Presets give every contributor identical build
  configurations. No more "it works on my machine."
- **Memory safety**: ASAN catches heap buffer overflows in mesh processing
  (Poly_Triangulation, BRep I/O) that are invisible to unit tests.
- **Stability of file parsers**: STEP and IGES parsers handle complex grammars
  on untrusted input. Fuzzing finds edge cases that manual tests miss. This
  directly prevents CVEs.
- **Minimal implementation cost**: These are purely build-system additions.
  No source code changes required.

### Technical Approach

1. **Bump minimum CMake to 3.16** (already required for PCH, line 214).
   Recommend 3.21+ for Presets support.

2. **Add `CMakePresets.json`**:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "dev-debug",
      "displayName": "Developer Debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "BUILD_CPP_STANDARD": "C++17",
        "BUILD_WITH_DEBUG": "ON",
        "BUILD_GTEST": "ON"
      }
    },
    {
      "name": "dev-asan",
      "displayName": "Debug + AddressSanitizer",
      "inherits": "dev-debug",
      "binaryDir": "${sourceDir}/build/asan",
      "cacheVariables": {
        "CMAKE_CXX_FLAGS": "-fsanitize=address,undefined -fno-omit-frame-pointer",
        "CMAKE_EXE_LINKER_FLAGS": "-fsanitize=address,undefined"
      }
    },
    {
      "name": "release",
      "displayName": "Production Release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BUILD_OPT_PROFILE": "Production"
      }
    }
  ]
}
```

3. **Add fuzz targets** for file parsers:

```cmake
# adm/cmake/occt_fuzz.cmake
if (BUILD_FUZZ_TARGETS)
  add_executable(fuzz_step_reader fuzz/fuzz_step_reader.cpp)
  target_link_libraries(fuzz_step_reader TKDESTEP TKernel TKMath)
  target_compile_options(fuzz_step_reader PRIVATE -fsanitize=fuzzer)
  target_link_options(fuzz_step_reader PRIVATE -fsanitize=fuzzer)
endif()
```

Fuzz harnesses are small (< 50 lines each). A STEP reader fuzz harness:

```cpp
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string input(reinterpret_cast<const char*>(data), size);
  std::istringstream stream(input);
  STEPControl_Reader reader;
  reader.ReadStream("fuzz.step", stream);
  return 0;
}
```

4. **CI integration**: Add a GitHub Actions / GitLab CI job that runs
   ASAN/UBSAN on the test suite. This catches regressions immediately.

### Risks

- None for the Presets or sanitizer integration — purely additive.
- Fuzzing may find real bugs that need fixing. This is a feature.

---

## 5. Replace `STANDARD_ALIGNED` and Remove Dead Compiler Guards

**Effort: Low** | **Impact: High**

### Problem

`Standard_DefineAlloc.hxx:77-91` defines a `STANDARD_ALIGNED` macro with
platform-specific `__declspec(align())` and `__attribute__((aligned()))`.
C++11 introduced `alignas()` which works on all compilers OCCT supports.

The same file contains dead code paths for:
- Borland C++ (`__BORLANDC__`) — discontinued in 2009.
- Sun Studio <= 5.3 (`__SUNPRO_CC <= 0x530`) — released in 2003.
- Sun Studio <= 4.2 (`__SUNPRO_CC <= 0x420`) — released ~2000.

These guards appear throughout the codebase and add cognitive load and
maintenance burden for compilers that no one has tested against in over 15
years.

### Why It's High Impact

- **Reduces macro surface area**: Every eliminated macro is one less thing to
  debug, document, and maintain.
- **Modern C++ credibility**: Dead Borland/SunPro guards signal stale code to
  new contributors and potential adopters.
- **Enables further cleanup**: Once `STANDARD_ALIGNED` is gone, `alignas()`
  can be used directly in data structures like `NCollection_AliasedArray`
  (currently uses 16-byte template alignment parameter).

### Technical Approach

1. **Replace all `STANDARD_ALIGNED(N, T, V)` with `alignas(N) T V`:**

```cpp
// BEFORE:
static const STANDARD_ALIGNED(8, char, THE_ARRAY)[] = {0xFF, ...};

// AFTER:
alignas(8) static const char THE_ARRAY[] = {0xFF, ...};
```

2. **Delete all `#if defined(__BORLANDC__)` and `__SUNPRO_CC` guards.**

3. **Delete `WORKAROUND_SUNPRO_NEW_PLACEMENT` block** (lines 67-75).

4. **Audit remaining platform macros** across the codebase for similar
   dead-compiler paths.

### Risks

- Near zero. These compilers cannot build modern C++17 code. The guards are
  unreachable.

---

## 6. `std::string_view` at Public API Boundaries

**Effort: Medium** | **Impact: High**

### Problem

OCCT's public API extensively uses `const TCollection_AsciiString&` for string
parameters. This forces callers to construct an `TCollection_AsciiString`
(which heap-allocates) even when passing a string literal or a `std::string`.

`TCollection_AsciiString` already accepts `std::string_view` in its constructor
(`TCollection_AsciiString.hxx:58-87`), so the type can interoperate. But the
API boundary forces the allocation.

### Why It's High Impact

- **Zero-copy string passing**: `std::string_view` avoids heap allocation for
  read-only parameters. This matters in hot paths like STEP file parsing
  where thousands of entity names are compared.
- **Interoperability**: `std::string_view` works with `std::string`, C string
  literals, `std::string_view`, and `TCollection_AsciiString` (which is
  contiguous and null-terminated). This eliminates a common friction point
  for users integrating OCCT with other C++ libraries.
- **Reduced header coupling**: Functions taking `std::string_view` don't need
  to include `TCollection_AsciiString.hxx`.

### Technical Approach

1. **Identify read-only string parameters** in public APIs: functions that
   take `const TCollection_AsciiString&` but only read the string content
   (compare, search, hash, output).

2. **Replace with `std::string_view`** at the signature level:

```cpp
// BEFORE:
class StepData_StepModel {
  Handle(Standard_Transient) EntityByName(
    const TCollection_AsciiString& theName) const;
};

// AFTER:
class StepData_StepModel {
  Handle(Standard_Transient) EntityByName(std::string_view theName) const;
};
```

3. **Add `std::string_view` conversion** to `TCollection_AsciiString`:

```cpp
class TCollection_AsciiString {
public:
  // Implicit conversion to string_view (zero-cost, no allocation)
  operator std::string_view() const noexcept {
    return std::string_view(ToCString(), Length());
  }
};
```

This makes existing code that passes `TCollection_AsciiString` to the
new `string_view` APIs work without changes.

4. **Keep `TCollection_AsciiString` for owned strings**: Don't replace
   member variables or return types — those need ownership semantics.
   Only change read-only input parameters.

### Risks

- Moderate scope: many API functions to audit. But the change is mechanical
  and the compiler catches mismatches.
- `std::string_view` is not null-terminated. Functions that pass to C APIs
  (e.g., `fopen()`) need a small `std::string` intermediate. This is already
  the case for `TCollection_ExtendedString` which is UTF-16.

---

## 7. Flat Indexed BRep Representation for Large Assemblies

**Effort: Medium** | **Impact: Very High**

### Problem

The BRep representation uses a deep tree of handle-linked objects:
`TopoDS_Shape` → `TopoDS_TShape` (handle) → sub-shapes (list of handles) →
geometry (handles to `Geom_Curve`, `Geom_Surface`) → etc.

For a typical automotive assembly with 100K+ faces, this creates:
- **Millions of small heap allocations** (one per TShape, per curve, per
  surface, per PCurve, per Location, per Representation).
- **Poor cache locality**: traversing the B-Rep graph chases pointers across
  the heap.
- **High overhead for serialization**: each handle is a separate
  reference-counted object that must be serialized individually.

### Why It's Very High Impact

- **Massive speedup for model traversal**: Boolean operations, mesh generation,
  and STEP export iterate over the B-Rep graph repeatedly. Flat indexed layout
  with contiguous memory turns pointer-chasing into sequential array access.
- **Reduced memory footprint**: Eliminates per-object refcount overhead
  (~24 bytes per `Standard_Transient` including vtable + refcount + padding)
  for millions of internal geometry objects.
- **Parallel-friendly**: Flat arrays with indices are trivially parallelizable
  and SIMD-friendly, unlike pointer-based graphs.

### Technical Approach

Create an **alternate flat representation** (`BRep_IndexedMesh` or
`BRep_CompactShape`) that coexists with the existing handle-based BRep.
It is constructed from a `TopoDS_Shape` and used where performance matters.

1. **Geometry pool**: All curves, surfaces, and locations are stored in typed
   contiguous arrays. References become 32-bit indices:

```cpp
struct BRep_CompactShape {
  // Geometry pools (contiguous, cache-friendly)
  std::vector<Geom_Line>          lines;
  std::vector<Geom_Circle>        circles;
  std::vector<Geom_BSplineCurve>  bsplineCurves;
  std::vector<Geom_Plane>         planes;
  std::vector<Geom_BSplineSurface> bsplineSurfaces;
  // ... one vector per concrete geometry type

  // Topology (indexed)
  struct Face {
    uint32_t surfaceType;    // discriminator
    uint32_t surfaceIndex;   // index into the appropriate pool
    uint32_t firstEdge;      // index into edges array
    uint16_t edgeCount;
    uint8_t  orientation;
  };

  struct Edge {
    uint32_t curveType;
    uint32_t curveIndex;
    uint32_t startVertex;    // index into vertices array
    uint32_t endVertex;
    double   firstParam;
    double   lastParam;
  };

  struct Vertex {
    gp_Pnt   point;
    double   tolerance;
  };

  std::vector<Face>    faces;
  std::vector<Edge>    edges;
  std::vector<Vertex>  vertices;
};
```

2. **Construction**: A builder traverses the existing `TopoDS_Shape`,
   deduplicates shared geometry (edges shared by faces, vertices shared by
   edges), and populates the flat arrays. This is a one-time O(n) pass.

3. **Usage**: Algorithms that traverse the BRep (meshing, boolean
   preprocessing, healing, STEP export) can accept `BRep_CompactShape`
   as input. The main benefit is in hot inner loops.

4. **Coexistence**: The existing `TopoDS_Shape` API remains unchanged. The
   flat representation is opt-in, constructed when needed, and discarded
   after use. No migration required.

### Risks

- Medium implementation effort: requires a builder, accessor API, and
  adaptation of key algorithms.
- Does not replace the existing BRep for editing (adding/removing faces).
  It is a read-optimized snapshot.
- Shared sub-shape semantics (e.g., same edge in two faces with different
  orientations) must be encoded carefully in the index scheme.

---

## Summary Matrix

| # | Proposal | Effort | Impact | Key Benefit |
|---|----------|--------|--------|-------------|
| 1 | Eliminate `DEFINE_STANDARD_ALLOC` | Low | Very High | Remove ~2000+ macro instances, simplify every class |
| 2 | GPU ID-Buffer Selection | Medium | Very High | O(1) interactive picking for million-triangle scenes |
| 3 | `enum class` Migration | Low | High | Type safety across 441 enum definitions |
| 4 | CMake Presets + Sanitizers + Fuzzing | Low | High | CI quality, parser robustness, reproducible builds |
| 5 | Remove Dead Compiler Guards & Macros | Low | High | Eliminate dead Borland/SunPro code, replace `STANDARD_ALIGNED` with `alignas()` |
| 6 | `std::string_view` API Boundaries | Medium | High | Zero-copy string passing, reduced coupling |
| 7 | Flat Indexed BRep Representation | Medium | Very High | Cache-friendly traversal, reduced memory for 100K+ face assemblies |

---

## Recommended Priority Order

1. **Proposals 1, 3, 5** (Low effort, mechanical) — do first. They clean up
   the codebase and make subsequent work easier. Proposal 1 alone touches
   the most files but is the most straightforward.

2. **Proposal 4** (Low effort, infrastructure) — enables catching bugs
   introduced by the other changes. Set up sanitizers before large
   refactoring.

3. **Proposal 6** (Medium effort, API quality) — do alongside the API-breaking
   enum migration since both touch public headers.

4. **Proposals 2 and 7** (Medium effort, performance) — these are the biggest
   performance wins and can be developed independently by separate
   contributors.
