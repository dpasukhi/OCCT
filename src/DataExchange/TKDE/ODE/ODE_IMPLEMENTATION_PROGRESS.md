# ODE Format Implementation Progress

**Last Updated:** 2025-11-20
**Version:** 1.0
**Status:** Planning Phase

---

## Overview

This document tracks the implementation progress of the ODE (OpenCascade Data Exchange) format. It works alongside `ODE_FORMAT_DESIGN.md` which contains the complete technical specification.

### Quick Status

| Component | Status | Progress | Notes |
|-----------|--------|----------|-------|
| **Infrastructure** | 🔴 Not Started | 0% | CMake, Cap'n Proto setup |
| **Schemas** | 🔴 Not Started | 0% | Cap'n Proto schema files |
| **ODEHash Module** | 🔴 Not Started | 0% | 40 hasher implementations |
| **ODE Core** | 🔴 Not Started | 0% | Archive, Manifest, Status |
| **File Classes** | 🔴 Not Started | 0% | Topology, Surface, Curve files |
| **Writer** | 🔴 Not Started | 0% | Serialization with deduplication |
| **Reader** | 🔴 Not Started | 0% | Deserialization with handle sharing |
| **Provider** | 🔴 Not Started | 0% | DE_Provider integration |
| **Tests** | 🔴 Not Started | 0% | Unit and integration tests |

**Legend:**
🔴 Not Started | 🟡 In Progress | 🟢 Complete | ⚠️ Blocked

---

## Phase 1: Foundation (Weeks 1-2)

**Goal:** Set up build infrastructure and define schemas

### 1.1 Build System Setup

- [ ] Create `src/DataExchange/TKDE/ODE/CMakeLists.txt`
  - [ ] Add Cap'n Proto dependency
  - [ ] Configure schema compilation
  - [ ] Set up file lists
- [ ] Create `src/DataExchange/TKDE/ODE/FILES.cmake`
- [ ] Create `src/DataExchange/TKDE/ODEHash/CMakeLists.txt`
- [ ] Create `src/DataExchange/TKDE/ODEHash/FILES.cmake`
- [ ] Update parent CMakeLists to include ODE and ODEHash modules
- [ ] Verify Cap'n Proto integration (version 1.2.0 from Homebrew)

**Notes:**
- Cap'n Proto already linked in TKDE via main CMakeLists.txt
- Need to add schema compilation step

### 1.2 Cap'n Proto Schema Definitions

- [ ] Create `resources/ODE/` directory
- [ ] **Base Schema** (`resources/ODE/ode.capnp`)
  - [ ] Vec3, Vec2, Quaternion structs
  - [ ] Transform struct
  - [ ] ObjectRef struct
- [ ] **Surfaces Schema** (`resources/ODE/surfaces.capnp`)
  - [ ] Plane, Cylinder, Cone, Sphere, Torus
  - [ ] Revolution, Linear Extrusion
  - [ ] Bezier, BSpline, Rectangular Trimmed
  - [ ] Offset Surface
  - [ ] SurfaceFile container
- [ ] **Curves3D Schema** (`resources/ODE/curves3d.capnp`)
  - [ ] Line, Circle, Ellipse, Hyperbola, Parabola
  - [ ] Bezier, BSpline, Trimmed, Offset
  - [ ] Curve3DFile container
- [ ] **Curves2D Schema** (`resources/ODE/curves2d.capnp`)
  - [ ] All 2D curve types (mirror of 3D)
  - [ ] Curve2DFile container
- [ ] **Topology Schema** (`resources/ODE/topology.capnp`)
  - [ ] Shape, Vertex, Edge, Wire, Face, Shell, Solid, Compound
  - [ ] Enums: ShapeType, Orientation
  - [ ] PCurveOnFace struct
  - [ ] TopologyFile container
- [ ] **Triangulation Schema** (`resources/ODE/triangulation.capnp`)
  - [ ] Triangulation struct
  - [ ] Triangle struct
  - [ ] TriangulationFile container
- [ ] **Polygons Schema** (`resources/ODE/polygons.capnp`)
  - [ ] Polygon3D, Polygon2D structs
  - [ ] PolygonFile container

**Notes:**
- Follow Cap'n Proto naming conventions (camelCase for fields)
- Add schema IDs using `capnp id` tool

### 1.3 Base Classes

- [ ] `ODE_Status.hxx` - Status enumeration (14 codes)
- [ ] `ODE_Object.hxx` - Base class for ODE objects
- [ ] `ODE_ObjectRef.hxx/.cxx` - Reference structure
  - [ ] Constructor, ToString(), FromString()
  - [ ] HasSubIndex() helper

**Current Status:** 🔴 Not Started
**Blockers:** None
**Next Steps:** Create CMake files and directory structure

---

## Phase 2: ODEHash Module (Weeks 3-4)

**Goal:** Implement all 40 hasher structs

### 2.1 Foundational Hashers (6 files)

- [ ] `ODEHash_PointHasher.pxx` - gp_Pnt
- [ ] `ODEHash_Point2dHasher.pxx` - gp_Pnt2d
- [ ] `ODEHash_DirectionHasher.pxx` - gp_Dir
- [ ] `ODEHash_Direction2dHasher.pxx` - gp_Dir2d
- [ ] `ODEHash_VectorHasher.pxx` - gp_Vec
- [ ] `ODEHash_AxisPlacement.pxx` - gp_Ax1, gp_Ax2, gp_Ax3

### 2.2 Surface Hashers (12 files)

- [ ] `ODEHash_PlaneHasher.pxx`
- [ ] `ODEHash_CylindricalSurfaceHasher.pxx`
- [ ] `ODEHash_ConicalSurfaceHasher.pxx`
- [ ] `ODEHash_SphericalSurfaceHasher.pxx`
- [ ] `ODEHash_ToroidalSurfaceHasher.pxx`
- [ ] `ODEHash_SurfaceOfRevolutionHasher.pxx`
- [ ] `ODEHash_SurfaceOfLinearExtrusionHasher.pxx`
- [ ] `ODEHash_BezierSurfaceHasher.pxx`
- [ ] `ODEHash_BSplineSurfaceHasher.pxx`
- [ ] `ODEHash_RectangularTrimmedSurfaceHasher.pxx`
- [ ] `ODEHash_OffsetSurfaceHasher.pxx`
- [ ] `ODEHash_SurfaceHasher.pxx` - Polymorphic dispatcher

### 2.3 Curve3D Hashers (11 files)

- [ ] `ODEHash_LineHasher.pxx`
- [ ] `ODEHash_CircleHasher.pxx`
- [ ] `ODEHash_EllipseHasher.pxx`
- [ ] `ODEHash_HyperbolaHasher.pxx`
- [ ] `ODEHash_ParabolaHasher.pxx`
- [ ] `ODEHash_BezierCurveHasher.pxx`
- [ ] `ODEHash_BSplineCurveHasher.pxx`
- [ ] `ODEHash_TrimmedCurveHasher.pxx`
- [ ] `ODEHash_OffsetCurveHasher.pxx`
- [ ] `ODEHash_Curve3dHasher.pxx` - Polymorphic dispatcher

### 2.4 Curve2D Hashers (11 files)

- [ ] `ODEHash_Line2dHasher.pxx`
- [ ] `ODEHash_Circle2dHasher.pxx`
- [ ] `ODEHash_Ellipse2dHasher.pxx`
- [ ] `ODEHash_Hyperbola2dHasher.pxx`
- [ ] `ODEHash_Parabola2dHasher.pxx`
- [ ] `ODEHash_BezierCurve2dHasher.pxx`
- [ ] `ODEHash_BSplineCurve2dHasher.pxx`
- [ ] `ODEHash_TrimmedCurve2dHasher.pxx`
- [ ] `ODEHash_OffsetCurve2dHasher.pxx`
- [ ] `ODEHash_Curve2dHasher.pxx` - Polymorphic dispatcher

### 2.5 Unit Tests for Hashers

- [ ] Test foundational hashers
- [ ] Test hash collision rate
- [ ] Test equality with tolerance
- [ ] Test polymorphic dispatchers
- [ ] Benchmark performance

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 1 complete
**Next Steps:** Implement foundational hashers first

---

## Phase 3: Geometry Serialization (Weeks 5-7)

**Goal:** Implement file classes for geometry storage

### 3.1 Manifest and Archive

- [ ] `ODE_Manifest.hxx/.cxx`
  - [ ] Read/Write JSON with RapidJSON
  - [ ] Version checking
  - [ ] File entry management
  - [ ] SHA-256 checksum support
- [ ] `ODE_Archive.hxx/.cxx`
  - [ ] Directory creation/reading with std::filesystem
  - [ ] Manifest integration
  - [ ] File accessor methods

### 3.2 Surface File

- [ ] `ODE_SurfaceFile.hxx/.cxx`
  - [ ] AddSurface() with deduplication
  - [ ] GetSurface() by index
  - [ ] WriteToCapnp() implementation
  - [ ] ReadFromCapnp() implementation
  - [ ] Cap'n Proto → Geom_Surface conversion (all 12 types)
  - [ ] Geom_Surface → Cap'n Proto conversion (all 12 types)

### 3.3 Curve Files

- [ ] `ODE_Curve3dFile.hxx/.cxx`
  - [ ] AddCurve() with deduplication
  - [ ] GetCurve() by index
  - [ ] WriteToCapnp() for all 11 curve types
  - [ ] ReadFromCapnp() for all 11 curve types
- [ ] `ODE_Curve2dFile.hxx/.cxx`
  - [ ] AddCurve() with deduplication
  - [ ] GetCurve() by index
  - [ ] WriteToCapnp() for all 11 curve2d types
  - [ ] ReadFromCapnp() for all 11 curve2d types

### 3.4 Round-Trip Tests

- [ ] Test each surface type (write + read)
- [ ] Test each 3D curve type (write + read)
- [ ] Test each 2D curve type (write + read)
- [ ] Verify precision preservation
- [ ] Test handle sharing

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 2 complete
**Next Steps:** Start with simple surfaces (Plane, Cylinder)

---

## Phase 4: Topology Serialization (Weeks 8-9)

**Goal:** Implement topology file and cross-references

### 4.1 Topology File

- [ ] `ODE_TopologyFile.hxx/.cxx`
  - [ ] AddShape() recursive traversal
  - [ ] GetShape() reconstruction
  - [ ] WriteToCapnp() with ObjectRef resolution
  - [ ] ReadFromCapnp() with reference resolution
  - [ ] Handle all shape types: Vertex, Edge, Wire, Face, Shell, Solid, Compound
  - [ ] PCurve handling
  - [ ] Orientation preservation
  - [ ] Tolerance preservation

### 4.2 Triangulation and Polygon Files

- [ ] `ODE_TriangulationFile.hxx/.cxx`
  - [ ] Poly_Triangulation serialization
  - [ ] UV coordinates handling
  - [ ] Normal vectors handling
- [ ] `ODE_PolygonFile.hxx/.cxx`
  - [ ] Poly_Polygon3D serialization
  - [ ] Poly_Polygon2D serialization

### 4.3 Integration Tests

- [ ] Test simple shapes (box, cylinder, sphere)
- [ ] Test complex shapes with faces referencing surfaces
- [ ] Test edges with 3D curves and PCurves
- [ ] Test compound shapes
- [ ] Verify topology integrity after round-trip

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 3 complete
**Next Steps:** Implement vertex and edge handling first

---

## Phase 5: Deduplication (Weeks 10-12)

**Goal:** Implement full deduplication with handle sharing

### 5.1 Deduplication Logic

- [ ] Surface deduplication using `std::unordered_map` + `ODEHash_SurfaceHasher`
- [ ] Curve3D deduplication using `ODEHash_Curve3dHasher`
- [ ] Curve2D deduplication using `ODEHash_Curve2dHasher`
- [ ] Sub-index assignment for handle sharing
- [ ] Mapping: Handle → ODE_ObjectRef

### 5.2 Performance Optimization

- [ ] Benchmark hash collision rate
- [ ] Optimize BSpline hashing (pole sampling)
- [ ] Profile deduplication performance
- [ ] Test with large models (>10k surfaces)

### 5.3 Deduplication Tests

- [ ] Test duplicate detection accuracy
- [ ] Test false positive rate (should be 0)
- [ ] Test sub-index assignment
- [ ] Measure file size reduction
- [ ] Verify handle sharing semantics

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 4 complete
**Next Steps:** Start with surface deduplication

---

## Phase 6: Writer & Reader (Weeks 13-14)

**Goal:** Complete end-to-end serialization workflow

### 6.1 Writer Implementation

- [ ] `ODE_Writer.hxx/.cxx`
  - [ ] Write() main entry point
  - [ ] Phase 1: collectObjects() - traverse shape
  - [ ] Phase 2: deduplicateObjects() - using hashers
  - [ ] Phase 3: buildFiles() - create file objects
  - [ ] Phase 4: writeArchive() - write to disk
  - [ ] Progress reporting with Message_ProgressIndicator
  - [ ] Statistics: original vs deduplicated counts
  - [ ] Error handling (no exceptions)

### 6.2 Reader Implementation

- [ ] `ODE_Reader.hxx/.cxx`
  - [ ] Read() main entry point
  - [ ] Phase 1: loadManifest()
  - [ ] Phase 2: loadFiles() - lazy or eager
  - [ ] Phase 3: resolveReferences() - ObjectRef → Handle
  - [ ] Phase 4: buildShape() - reconstruct TopoDS_Shape
  - [ ] Handle sharing cache
  - [ ] Deep copy for no-subindex refs
  - [ ] Error handling (no exceptions)

### 6.3 Integration Tests

- [ ] Test full write-read cycle
- [ ] Test with OCCT sample models
- [ ] Test with user-provided models
- [ ] Compare with BRep format (size, speed)
- [ ] Verify shape equivalence after round-trip

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 5 complete
**Next Steps:** Implement writer first, then reader

---

## Phase 7: Provider Integration (Week 15)

**Goal:** Integrate with OCCT DataExchange module

### 7.1 DE_Provider Implementation

- [ ] `ODE_Provider.hxx/.cxx`
  - [ ] Inherit from DE_Provider
  - [ ] Implement GetVendor(), GetFormat()
  - [ ] Implement Read() for shapes
  - [ ] Implement Write() for shapes
  - [ ] Implement Read() for documents (optional)
  - [ ] Implement Write() for documents (optional)
- [ ] Register with DE_Wrapper::GlobalWrapper()

### 7.2 Command-Line Tools

- [ ] Add ODE support to DRAWEXE commands
- [ ] `readode` / `writeode` commands
- [ ] Help documentation

### 7.3 Documentation

- [ ] Update OCCT documentation
- [ ] Add ODE format examples
- [ ] Migration guide from BRep

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 6 complete
**Next Steps:** Implement basic provider

---

## Phase 8: Testing & Optimization (Weeks 16-18)

**Goal:** Comprehensive testing and performance tuning

### 8.1 Unit Tests

- [ ] Test each hasher independently
- [ ] Test each file class independently
- [ ] Test ObjectRef string conversion
- [ ] Test error handling paths
- [ ] Test edge cases (null handles, empty shapes, etc.)

### 8.2 Integration Tests

- [ ] Simple shapes (primitives)
- [ ] Complex assemblies
- [ ] Large models (>100k faces)
- [ ] Models with duplicate geometry
- [ ] Models with shared edges/faces

### 8.3 Performance Benchmarks

- [ ] Write speed vs BRep
- [ ] Read speed vs BRep
- [ ] File size vs BRep
- [ ] Memory usage
- [ ] Deduplication effectiveness

### 8.4 Optimization

- [ ] Profile hotspots
- [ ] Parallel file I/O (std::async)
- [ ] Optimize hash functions
- [ ] Reduce memory allocations
- [ ] Cache optimization

**Current Status:** 🔴 Not Started
**Blockers:** Need Phase 7 complete
**Next Steps:** Set up GTest framework

---

## Implementation Notes

### Decisions Made

1. **2025-11-20:** Use `std::unordered_map` with dual-operator hashers instead of `NCollection_DataMap` for deduplication (simpler, C++17-native)
2. **2025-11-20:** Use status codes instead of exceptions for error handling
3. **2025-11-20:** Separate ODEHash module for reusability
4. **2025-11-20:** Schemas in `resources/ODE/` for proper data file organization

### Open Questions

- [ ] Should JSON encoding be supported in addition to Cap'n Proto binary?
- [ ] Should we support streaming/incremental write for huge models?
- [ ] Should deduplication be optional (config flag)?
- [ ] Should we add compression (gzip/zstd)?

### Technical Debt

None yet.

---

## Issues and Blockers

### Current Blockers

None - project in planning phase.

### Known Issues

None yet.

---

## Testing Status

### Unit Tests

| Component | Tests Written | Tests Passing | Coverage |
|-----------|---------------|---------------|----------|
| ODEHash Foundational | 0 | 0 | 0% |
| ODEHash Surfaces | 0 | 0 | 0% |
| ODEHash Curves | 0 | 0 | 0% |
| ODE Core | 0 | 0 | 0% |
| ODE Files | 0 | 0 | 0% |
| ODE Writer | 0 | 0 | 0% |
| ODE Reader | 0 | 0 | 0% |

### Integration Tests

| Test Category | Status | Notes |
|---------------|--------|-------|
| Simple Primitives | 🔴 Not Started | Box, Cylinder, Sphere |
| Complex Assemblies | 🔴 Not Started | Multi-body models |
| Large Models | 🔴 Not Started | >10k faces |
| Deduplication | 🔴 Not Started | Duplicate detection |
| Round-Trip | 🔴 Not Started | Write + Read equivalence |

### Performance Benchmarks

| Benchmark | Target | Actual | Status |
|-----------|--------|--------|--------|
| File Size Reduction | 30-50% vs BRep | - | 🔴 Not Started |
| Write Speed | <2x BRep time | - | 🔴 Not Started |
| Read Speed | <1.5x BRep time | - | 🔴 Not Started |
| Memory Usage | 20-40% less | - | 🔴 Not Started |

---

## Success Criteria

### Phase Completion

- [x] Phase 1: All schemas defined, build system working
- [ ] Phase 2: All 40 hashers implemented and tested
- [ ] Phase 3: Geometry serialization working for all types
- [ ] Phase 4: Topology round-trip successful
- [ ] Phase 5: Deduplication reducing file size by >30%
- [ ] Phase 6: End-to-end write-read working
- [ ] Phase 7: Provider integrated with OCCT
- [ ] Phase 8: All tests passing, performance targets met

### Final Acceptance

- [ ] All unit tests passing
- [ ] All integration tests passing
- [ ] Performance targets achieved
- [ ] Code review complete
- [ ] Documentation complete
- [ ] No memory leaks (Valgrind clean)
- [ ] No warnings on supported compilers

---

## Resources

### Documentation

- Design Document: `ODE_FORMAT_DESIGN.md`
- Cap'n Proto Docs: https://capnproto.org/
- OCCT Docs: https://dev.opencascade.org/

### References

- StepTidy Module: `src/DataExchange/TKDESTEP/StepTidy/`
- DE_Provider Pattern: `src/DataExchange/TKDE/DE/DE_Provider.hxx`
- BRep Format: `src/ModelingData/TKBRep/BRep/`

### Tools

- Cap'n Proto Compiler: `capnp` (v1.2.0)
- Build System: CMake 3.20+
- Compiler: Clang 14+ or GCC 11+ (C++17)
- Testing: GTest

---

**End of Progress Document**
