# ODE Format Design Document
## OpenCascade Data Exchange - Next Generation Format

**Version:** 1.0
**Date:** 2025-11-20
**Author:** OCCT Development Team

---

## 1. Overview and Motivation

### 1.1 Goals
The ODE (OpenCascade Data Exchange) format aims to replace the legacy BRep format with a modern, efficient, and extensible serialization format that:

- **Reduces file size** through intelligent deduplication
- **Improves performance** with binary serialization (Cap'n Proto)
- **Maintains flexibility** with JSON conversion support
- **Preserves topology relationships** through indexed references
- **Supports handle sharing** to maintain memory efficiency
- **Enables incremental loading** through modular file structure
- **Provides schema versioning** for future compatibility

### 1.2 Key Features
- Non-compressed archive structure (external tools can compress)
- Multiple typed data files (topology, geometry, tessellation)
- UUID-based file identification
- Index-based cross-references between objects
- Optional sub-index for handle sharing
- Automatic deduplication of identical objects
- Cap'n Proto binary format with JSON fallback
- OCCT-compliant C++17 implementation

---

## 2. Archive Structure

### 2.1 Container Format
```
mymodel.ode/
├── manifest.json           # Entry point and file registry
├── topology.capnp          # TopoDS_Shape hierarchy
├── surfaces.capnp          # Geom_Surface objects
├── curves3d.capnp          # Geom_Curve objects
├── curves2d.capnp          # Geom2d_Curve objects
├── triangulation.capnp     # Poly_Triangulation data
└── polygons.capnp          # Poly_Polygon3D/2D data
```

### 2.2 Manifest Structure (manifest.json)
```json
{
  "format": "ODE",
  "version": "1.0",
  "generator": "OCCT 7.9.0",
  "created": "2025-11-20T10:30:00Z",
  "files": [
    {
      "uuid": "550e8400-e29b-41d4-a716-446655440000",
      "name": "topology.capnp",
      "type": "topology",
      "encoding": "capnp",
      "objectCount": 1247,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    {
      "uuid": "550e8400-e29b-41d4-a716-446655440001",
      "name": "surfaces.capnp",
      "type": "surfaces",
      "encoding": "capnp",
      "objectCount": 89,
      "sha256": "..."
    }
    // ... other files
  ],
  "metadata": {
    "units": "mm",
    "tolerance": 1e-7,
    "application": "Custom CAD System"
  }
}
```

---

## 3. Indexing and Reference System

### 3.1 Object Indexing
Each file contains objects indexed from **0** to **N-1**.

**Example:**
- `surfaces.capnp` contains:
  - Surface[0]: Plane at origin
  - Surface[1]: Cylinder radius=10
  - Surface[2]: BSpline surface
  - ...

### 3.2 Cross-File References
References use the format: `{fileType}#{objectIndex}` or `{fileType}#{objectIndex}.{subIndex}`

**Examples:**
```
surfaces#0       → Reference to Surface[0]
curves3d#15      → Reference to Curve3D[15]
surfaces#3.1     → Reference to Surface[3], shared handle group 1
```

### 3.3 Sub-Index for Handle Sharing

**Sub-Index Semantics:**
- **No sub-index** (e.g., `surfaces#5`): Create a **deep copy** of the object
- **With sub-index** (e.g., `surfaces#5.2`): Share the **same handle** pointer with all other `surfaces#5.2` references

**Use Case Example:**
```cpp
// Face1 references surfaces#0.1
// Face2 references surfaces#0.1
// Face3 references surfaces#0    (no sub-index)

// Result:
// Face1 and Face2 share the SAME Handle(Geom_Surface)
// Face3 gets its own deep copy of the surface
```

**Benefits:**
- Reduces memory usage for truly shared geometry
- Preserves OCCT's handle semantics
- Allows intentional deep copies when needed

---

## 4. Deduplication Strategy

### 4.1 Goals
- **Eliminate duplicate objects** even if they don't share the same handle in memory
- **Reduce file size** significantly
- **Maintain correctness** through deep comparison

### 4.2 Deduplication Phases

#### Phase 1: Collection
Traverse the entire model and collect all geometric objects by type:
```cpp
std::vector<Handle(Geom_Surface)> allSurfaces;
std::vector<Handle(Geom_Curve)> allCurves3D;
// etc.
```

#### Phase 2: Comparison and Hashing
For each object:
1. Compute a **content hash** (not pointer-based)
2. Compare objects with matching hashes using deep equality
3. Group identical objects together

**Example Hash Computation (following StepTidy pattern):**
```cpp
// For Geom_Plane - using opencascade::hashBytes from Standard_HashUtils.hxx
const gp_Pnt& aLoc = plane->Location();
const gp_Dir& aNorm = plane->Axis().Direction();

const Standard_Real aData[6] = {
  aLoc.X(), aLoc.Y(), aLoc.Z(),
  aNorm.X(), aNorm.Y(), aNorm.Z()
};

size_t hash = opencascade::hashBytes(aData, sizeof(aData));
```

**Note:** This approach follows the proven pattern established in OCCT's StepTidy module (see `src/DataExchange/TKDESTEP/StepTidy/*Hasher.pxx`).

#### Phase 3: Index Assignment
Assign unique indices only to deduplicated objects:
```cpp
// Original: 1000 surface references (many duplicates)
// After deduplication: 150 unique surfaces
// surfaces.capnp contains only 150 objects
```

#### Phase 4: Mapping
Build mapping from original handles to deduplicated indices:
```cpp
std::unordered_map<Handle(Geom_Surface), ObjectRef> surfaceMap;
// ObjectRef = {index, subIndex}
```

### 4.3 Hasher Implementation (StepTidy Pattern)

**OCCT-Compliant Dual-Operator Pattern:**

Following the established StepTidy pattern, hashers are stateless structs with two `operator()` overloads:

```cpp
// ODEHash_PlaneHasher.pxx
#include <Standard_HashUtils.hxx>
#include <Geom_Plane.hxx>

//! OCCT-style hasher for Geom_Plane entities.
//! Follows the pattern established in StepTidy module.
struct ODEHash_PlaneHasher
{
  // Operator 1: Compute hash value
  std::size_t operator()(const Handle(Geom_Surface)& theSurf) const noexcept
  {
    Handle(Geom_Plane) aPlane = Handle(Geom_Plane)::DownCast(theSurf);
    if (aPlane.IsNull()) return 0;

    const gp_Pnt& aLoc = aPlane->Location();
    const gp_Dir& aNorm = aPlane->Axis().Direction();

    const Standard_Real aData[6] = {
      aLoc.X(), aLoc.Y(), aLoc.Z(),
      aNorm.X(), aNorm.Y(), aNorm.Z()
    };

    return opencascade::hashBytes(aData, sizeof(aData));
  }

  // Operator 2: Compare for equality with tolerance
  bool operator()(const Handle(Geom_Surface)& theSurf1,
                  const Handle(Geom_Surface)& theSurf2) const noexcept
  {
    Handle(Geom_Plane) aPlane1 = Handle(Geom_Plane)::DownCast(theSurf1);
    Handle(Geom_Plane) aPlane2 = Handle(Geom_Plane)::DownCast(theSurf2);

    if (aPlane1.IsNull() || aPlane2.IsNull()) return false;

    constexpr double aTolerance = 1e-12;  // StepTidy standard

    const gp_Pnt& aLoc1 = aPlane1->Location();
    const gp_Pnt& aLoc2 = aPlane2->Location();

    if (std::abs(aLoc1.X() - aLoc2.X()) > aTolerance ||
        std::abs(aLoc1.Y() - aLoc2.Y()) > aTolerance ||
        std::abs(aLoc1.Z() - aLoc2.Z()) > aTolerance)
    {
      return false;
    }

    const gp_Dir& aNorm1 = aPlane1->Axis().Direction();
    const gp_Dir& aNorm2 = aPlane2->Axis().Direction();

    return std::abs(aNorm1.X() - aNorm2.X()) <= aTolerance &&
           std::abs(aNorm1.Y() - aNorm2.Y()) <= aTolerance &&
           std::abs(aNorm1.Z() - aNorm2.Z()) <= aTolerance;
  }
};
```

**Key Considerations:**
- **Tolerance**: Use fixed `1e-12` tolerance (StepTidy standard), not `Precision::Confusion()`
- **Stateless structs**: No member variables, marked `noexcept`
- **Dual operators**: First for hashing, second for equality comparison
- **Type safety**: Use `DownCast` to handle polymorphic geometry types
- **Compositional**: Complex hashers reuse simpler ones (see Section 6.3)
- **Handle optional fields**: Use `opencascade::MurmurHash::optimalSeed()` for missing data

---

## 5. Cap'n Proto Schema Design

### 5.1 Base Schema (ode.capnp)

```capnp
@0x9eb32e19f86ee174;

# Common types
struct Vec3 {
  x @0 :Float64;
  y @1 :Float64;
  z @2 :Float64;
}

struct Vec2 {
  x @0 :Float64;
  y @1 :Float64;
}

struct Quaternion {
  x @0 :Float64;
  y @1 :Float64;
  z @2 :Float64;
  w @3 :Float64;
}

struct Transform {
  translation @0 :Vec3;
  rotation @1 :Quaternion;
  scale @2 :Float64 = 1.0;
}

# Reference to object in another file
struct ObjectRef {
  fileType @0 :Text;      # "surfaces", "curves3d", etc.
  index @1 :UInt32;       # Object index in that file
  subIndex @2 :UInt32 = 0xFFFFFFFF;  # 0xFFFFFFFF means no sub-index (deep copy)
}
```

### 5.2 Surfaces Schema (surfaces.capnp)

```capnp
@0x9eb32e19f86ee175;

using import "ode.capnp".Vec3;
using import "ode.capnp".Transform;

struct Surface {
  index @0 :UInt32;

  union {
    plane @1 :Plane;
    cylinder @2 :Cylinder;
    cone @3 :Cone;
    sphere @4 :Sphere;
    torus @5 :Torus;
    bsplineSurface @6 :BSplineSurface;
    bezierSurface @7 :BezierSurface;
    surfaceOfRevolution @8 :SurfaceOfRevolution;
    surfaceOfExtrusion @9 :SurfaceOfExtrusion;
    offsetSurface @10 :OffsetSurface;
    # ... other types
  }
}

struct Plane {
  origin @0 :Vec3;
  normal @1 :Vec3;
  uDir @2 :Vec3;  # U direction for parametrization
}

struct Cylinder {
  origin @0 :Vec3;
  axis @1 :Vec3;
  radius @2 :Float64;
}

struct BSplineSurface {
  uDegree @0 :UInt32;
  vDegree @1 :UInt32;
  uPoles @2 :UInt32;
  vPoles @3 :UInt32;
  uKnots @4 :List(Float64);
  vKnots @5 :List(Float64);
  uMultiplicities @6 :List(UInt32);
  vMultiplicities @7 :List(UInt32);
  poles @8 :List(Vec3);        # Row-major: [u0v0, u0v1, ..., u1v0, ...]
  weights @9 :List(Float64);   # Empty if non-rational
  uPeriodic @10 :Bool;
  vPeriodic @11 :Bool;
}

# Container for all surfaces in file
struct SurfaceFile {
  surfaces @0 :List(Surface);
}
```

### 5.3 Curves3D Schema (curves3d.capnp)

```capnp
@0x9eb32e19f86ee176;

using import "ode.capnp".Vec3;

struct Curve3D {
  index @0 :UInt32;

  union {
    line @1 :Line3D;
    circle @2 :Circle3D;
    ellipse @3 :Ellipse3D;
    bsplineCurve @4 :BSplineCurve3D;
    bezierCurve @5 :BezierCurve3D;
    offsetCurve @6 :OffsetCurve3D;
    trimmedCurve @7 :TrimmedCurve3D;
  }
}

struct Line3D {
  origin @0 :Vec3;
  direction @1 :Vec3;
}

struct Circle3D {
  center @0 :Vec3;
  normal @1 :Vec3;
  xAxis @2 :Vec3;
  radius @3 :Float64;
}

struct BSplineCurve3D {
  degree @0 :UInt32;
  poles @1 :List(Vec3);
  knots @2 :List(Float64);
  multiplicities @3 :List(UInt32);
  weights @4 :List(Float64);  # Empty if non-rational
  periodic @5 :Bool;
}

struct TrimmedCurve3D {
  basisCurve @0 :UInt32;  # Index in same file
  firstParam @1 :Float64;
  lastParam @2 :Float64;
}

struct Curve3DFile {
  curves @0 :List(Curve3D);
}
```

### 5.4 Topology Schema (topology.capnp)

```capnp
@0x9eb32e19f86ee177;

using import "ode.capnp".ObjectRef;

struct Shape {
  index @0 :UInt32;
  shapeType @1 :ShapeType;
  orientation @2 :Orientation;

  union {
    vertex @3 :Vertex;
    edge @4 :Edge;
    wire @5 :Wire;
    face @6 :Face;
    shell @7 :Shell;
    solid @8 :Solid;
    compSolid @9 :CompSolid;
    compound @10 :Compound;
  }
}

enum ShapeType {
  vertex @0;
  edge @1;
  wire @2;
  face @3;
  shell @4;
  solid @5;
  compSolid @6;
  compound @7;
}

enum Orientation {
  forward @0;
  reversed @1;
  internal @2;
  external @3;
}

struct Vertex {
  point @0 :Vec3;
  tolerance @1 :Float64;
}

struct Edge {
  vertices @0 :List(UInt32);  # Indices in topology file
  curve3d @1 :ObjectRef;      # Reference to curves3d file
  firstParam @2 :Float64;
  lastParam @3 :Float64;
  tolerance @4 :Float64;

  # PCurves for each face using this edge
  pcurves @5 :List(PCurveOnFace);

  # Optional triangulation
  polygon3d @6 :ObjectRef;    # Reference to polygons file
}

struct PCurveOnFace {
  face @0 :UInt32;            # Face index in topology file
  curve2d @1 :ObjectRef;      # Reference to curves2d file
}

struct Face {
  surface @0 :ObjectRef;      # Reference to surfaces file
  wires @1 :List(UInt32);     # Wire indices (first is outer)
  orientation @2 :Orientation;
  tolerance @3 :Float64;

  # Optional triangulation
  triangulation @4 :ObjectRef; # Reference to triangulation file

  # Natural bounds (for infinite surfaces)
  uMin @5 :Float64;
  uMax @6 :Float64;
  vMin @7 :Float64;
  vMax @8 :Float64;
}

struct Wire {
  edges @0 :List(UInt32);     # Edge indices in topology file
  closed @1 :Bool;
}

struct Shell {
  faces @0 :List(UInt32);     # Face indices
  closed @1 :Bool;
}

struct Solid {
  shells @0 :List(UInt32);    # Shell indices (first is outer)
}

struct CompSolid {
  solids @0 :List(UInt32);
}

struct Compound {
  children @0 :List(UInt32);  # Mixed shape indices
}

struct TopologyFile {
  shapes @0 :List(Shape);
  rootShape @1 :UInt32;       # Index of the root shape
}
```

### 5.5 Triangulation Schema (triangulation.capnp)

```capnp
@0x9eb32e19f86ee178;

using import "ode.capnp".Vec3;
using import "ode.capnp".Vec2;

struct Triangulation {
  index @0 :UInt32;

  # Nodes (vertices)
  nodes @1 :List(Vec3);

  # Triangles (indices into nodes, 0-based)
  triangles @2 :List(Triangle);

  # Optional UV coordinates (for parametric space)
  uvNodes @3 :List(Vec2);

  # Optional normals
  normals @4 :List(Vec3);

  deflection @5 :Float64;
}

struct Triangle {
  n1 @0 :UInt32;
  n2 @1 :UInt32;
  n3 @2 :UInt32;
}

struct TriangulationFile {
  triangulations @0 :List(Triangulation);
}
```

---

## 6. Implementation Architecture

### 6.1 Class Structure (OCCT Naming Conventions)

#### 6.1.1 Core Classes

```cpp
// ODE_Object.hxx
class ODE_Object : public Standard_Transient {
public:
  DEFINE_STANDARD_RTTIEXT(ODE_Object, Standard_Transient)

  virtual ~ODE_Object() = default;

  // Serialization
  virtual void WriteToCapnp(capnp::MallocMessageBuilder& theBuilder) const = 0;
  virtual void ReadFromCapnp(const capnp::MessageReader& theReader) = 0;

  // JSON conversion
  virtual void WriteToJson(rapidjson::Value& theValue,
                          rapidjson::Document::AllocatorType& theAllocator) const = 0;
  virtual void ReadFromJson(const rapidjson::Value& theValue) = 0;
};

// ODE_ObjectRef.hxx
struct ODE_ObjectRef {
  TCollection_AsciiString FileType;  // "surfaces", "curves3d", etc.
  Standard_Integer Index;
  Standard_Integer SubIndex;         // -1 means no sub-index

  ODE_ObjectRef() : Index(-1), SubIndex(-1) {}

  Standard_Boolean HasSubIndex() const { return SubIndex >= 0; }
  TCollection_AsciiString ToString() const;
  static ODE_ObjectRef FromString(const TCollection_AsciiString& theStr);
};

// ODE_Archive.hxx
class ODE_Archive : public Standard_Transient {
public:
  DEFINE_STANDARD_RTTIEXT(ODE_Archive, Standard_Transient)

  // Write archive to directory
  Standard_Boolean Write(const TCollection_AsciiString& thePath) const;

  // Read archive from directory
  Standard_Boolean Read(const TCollection_AsciiString& thePath);

  // Access files
  Handle(ODE_TopologyFile) GetTopologyFile() const;
  Handle(ODE_SurfaceFile) GetSurfaceFile() const;
  // ... other getters

private:
  Handle(ODE_Manifest) myManifest;
  Handle(ODE_TopologyFile) myTopologyFile;
  Handle(ODE_SurfaceFile) mySurfaceFile;
  Handle(ODE_Curve3dFile) myCurve3dFile;
  Handle(ODE_Curve2dFile) myCurve2dFile;
  Handle(ODE_TriangulationFile) myTriangulationFile;
  Handle(ODE_PolygonFile) myPolygonFile;
};
```

#### 6.1.2 File Classes

```cpp
// ODE_TopologyFile.hxx
class ODE_TopologyFile : public Standard_Transient {
public:
  DEFINE_STANDARD_RTTIEXT(ODE_TopologyFile, Standard_Transient)

  // Add shape and get its index
  Standard_Integer AddShape(const TopoDS_Shape& theShape);

  // Get shape by index
  const TopoDS_Shape& GetShape(const Standard_Integer theIndex) const;

  // Write to Cap'n Proto
  void WriteToCapnp(const TCollection_AsciiString& thePath) const;
  void ReadFromCapnp(const TCollection_AsciiString& thePath);

  // JSON conversion
  void WriteToJson(const TCollection_AsciiString& thePath) const;
  void ReadFromJson(const TCollection_AsciiString& thePath);

  Standard_Integer Count() const { return myShapes.Size(); }

private:
  NCollection_DataMap<Standard_Integer, TopoDS_Shape> myShapes;
  Standard_Integer myRootShapeIndex;
};

// ODE_SurfaceFile.hxx
class ODE_SurfaceFile : public Standard_Transient {
public:
  DEFINE_STANDARD_RTTIEXT(ODE_SurfaceFile, Standard_Transient)

  // Add surface and get its index (with deduplication)
  Standard_Integer AddSurface(
    const Handle(Geom_Surface)& theSurface,
    NCollection_DataMap<Handle(Geom_Surface), ODE_ObjectRef>& theMap,
    const Standard_Boolean theEnableDedup = Standard_True
  );

  // Get surface by index
  Handle(Geom_Surface) GetSurface(const Standard_Integer theIndex) const;

  void WriteToCapnp(const TCollection_AsciiString& thePath) const;
  void ReadFromCapnp(const TCollection_AsciiString& thePath);

  Standard_Integer Count() const { return mySurfaces.Size(); }

private:
  NCollection_Vector<Handle(Geom_Surface)> mySurfaces;

  // For deduplication during write
  struct SurfaceHasher;
  NCollection_DataMap<size_t, NCollection_Vector<Standard_Integer>> myHashMap;
};
```

#### 6.1.3 Writer/Reader Classes

```cpp
// ODE_Writer.hxx
class ODE_Writer {
public:
  ODE_Writer();

  // Main write method
  Standard_Boolean Write(
    const TopoDS_Shape& theShape,
    const TCollection_AsciiString& thePath
  );

  // Configuration
  void SetEnableDeduplication(const Standard_Boolean theEnable) {
    myEnableDedup = theEnable;
  }

  void SetEncoding(const ODE_Encoding theEncoding) {
    myEncoding = theEncoding;  // CAPNP or JSON
  }

  // Statistics
  Standard_Integer GetOriginalObjectCount() const;
  Standard_Integer GetDeduplicatedObjectCount() const;

private:
  Standard_Boolean myEnableDedup;
  ODE_Encoding myEncoding;

  // Temporary collections during write
  NCollection_DataMap<Handle(Geom_Surface), ODE_ObjectRef> mySurfaceMap;
  NCollection_DataMap<Handle(Geom_Curve), ODE_ObjectRef> myCurve3dMap;
  // ...

  // Phase 1: Collect all objects
  void collectObjects(const TopoDS_Shape& theShape);

  // Phase 2: Deduplicate
  void deduplicateObjects();

  // Phase 3: Build files
  void buildFiles();

  // Phase 4: Write to disk
  Standard_Boolean writeArchive(const TCollection_AsciiString& thePath);
};

// ODE_Reader.hxx
class ODE_Reader {
public:
  ODE_Reader();

  // Main read method
  Standard_Boolean Read(
    const TCollection_AsciiString& thePath,
    TopoDS_Shape& theShape
  );

private:
  Handle(ODE_Archive) myArchive;

  // Cache for handle sharing
  NCollection_DataMap<ODE_ObjectRef, Handle(Geom_Surface)> mySurfaceCache;
  NCollection_DataMap<ODE_ObjectRef, Handle(Geom_Curve)> myCurve3dCache;
  // ...

  // Resolve reference (with handle sharing)
  Handle(Geom_Surface) resolveSurfaceRef(const ODE_ObjectRef& theRef);
  Handle(Geom_Curve) resolveCurve3dRef(const ODE_ObjectRef& theRef);
};
```

### 6.2 Deduplication with NCollection_DataMap (StepTidy Pattern)

**Key Insight:** OCCT's `NCollection_DataMap` natively supports custom hashers using the dual-operator pattern. This eliminates the need for separate deduplication classes.

#### 6.2.1 Polymorphic Surface Hasher

Since `Geom_Surface` is polymorphic, we need a hasher that dispatches to type-specific hashers:

```cpp
// ODEHash_SurfaceHasher.pxx
#include <Standard_HashUtils.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_BSplineSurface.hxx>
// ... other surface types

#include "ODEHash_PlaneHasher.pxx"
#include "ODEHash_CylindricalSurfaceHasher.pxx"
#include "ODEHash_BSplineSurfaceHasher.pxx"
// ... other hasher includes

//! Polymorphic hasher for Geom_Surface - dispatches to type-specific hashers
struct ODEHash_SurfaceHasher
{
  std::size_t operator()(const Handle(Geom_Surface)& theSurf) const noexcept
  {
    if (theSurf.IsNull()) return 0;

    // Dispatch based on runtime type
    if (theSurf->IsKind(STANDARD_TYPE(Geom_Plane))) {
      return ODEHash_PlaneHasher{}(theSurf);
    }
    else if (theSurf->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
      return ODEHash_CylindricalSurfaceHasher{}(theSurf);
    }
    else if (theSurf->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
      return ODEHash_BSplineSurfaceHasher{}(theSurf);
    }
    // ... other types

    return 0;  // Unknown type
  }

  bool operator()(const Handle(Geom_Surface)& theSurf1,
                  const Handle(Geom_Surface)& theSurf2) const noexcept
  {
    if (theSurf1.IsNull() || theSurf2.IsNull()) return false;

    // Types must match
    if (theSurf1->DynamicType() != theSurf2->DynamicType()) {
      return false;
    }

    // Dispatch to type-specific comparator
    if (theSurf1->IsKind(STANDARD_TYPE(Geom_Plane))) {
      return ODEHash_PlaneHasher{}(theSurf1, theSurf2);
    }
    else if (theSurf1->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
      return ODEHash_CylindricalSurfaceHasher{}(theSurf1, theSurf2);
    }
    else if (theSurf1->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
      return ODEHash_BSplineSurfaceHasher{}(theSurf1, theSurf2);
    }
    // ... other types

    return false;  // Unknown type
  }
};
```

#### 6.2.2 Deduplication Map

Use `NCollection_DataMap` with the hasher directly:

```cpp
// In ODE_Writer or deduplication module
using SurfaceDedupMap = NCollection_DataMap<
  Handle(Geom_Surface),              // Key: surface handle
  std::vector<Handle(Geom_Surface)>, // Value: list of duplicate handles
  ODEHash_SurfaceHasher              // Hasher with dual operators
>;

SurfaceDedupMap mySurfaceDuplicates;
```

**How it works:**
1. `operator()(surface)` computes hash for bucket assignment
2. `operator()(surf1, surf2)` checks equality when hash collisions occur
3. All duplicates are automatically grouped together

#### 6.2.3 Processing Algorithm

```cpp
void ODE_Writer::deduplicateSurfaces()
{
  SurfaceDedupMap aDedupMap;

  // Phase 1: Group duplicates
  for (const auto& aSurf : myCollectedSurfaces)
  {
    if (!aDedupMap.IsBound(aSurf)) {
      // First occurrence - create new entry
      std::vector<Handle(Geom_Surface)> aDuplicates;
      aDuplicates.push_back(aSurf);
      aDedupMap.Bind(aSurf, aDuplicates);
    }
    else {
      // Duplicate found - add to list
      aDedupMap.ChangeFind(aSurf).push_back(aSurf);
    }
  }

  // Phase 2: Build unique surface list and mapping
  Standard_Integer anIndex = 0;
  for (auto it = aDedupMap.cbegin(); it != aDedupMap.cend(); ++it)
  {
    const Handle(Geom_Surface)& aCanonical = it->first;
    const std::vector<Handle(Geom_Surface)>& aDuplicates = it->second;

    // Add canonical surface to output
    myUniqueSurfaces.Append(aCanonical);

    // Map all duplicates to this index with sub-indices
    Standard_Integer aSubIndex = 0;
    for (const auto& aDup : aDuplicates) {
      ODE_ObjectRef aRef;
      aRef.FileType = "surfaces";
      aRef.Index = anIndex;
      aRef.SubIndex = aSubIndex++;

      mySurfaceToRef.Bind(aDup, aRef);
    }

    ++anIndex;
  }
}
```

**Benefits of this approach:**
- Leverages OCCT's built-in hash map with custom hasher
- No need for separate deduplication class hierarchy
- Follows proven StepTidy pattern
- Efficient: O(1) average case for duplicate detection
- Type-safe with compile-time checks

### 6.3 Complete Hasher Examples

This section provides complete hasher implementations for various geometry types, demonstrating the compositional pattern.

#### 6.3.1 Point Hasher (Foundational)

```cpp
// ODEHash_PointHasher.pxx
#include <Standard_HashUtils.hxx>
#include <gp_Pnt.hxx>

//! Hasher for gp_Pnt (3D points)
struct ODEHash_PointHasher
{
  std::size_t operator()(const gp_Pnt& thePnt) const noexcept
  {
    const Standard_Real aData[3] = {thePnt.X(), thePnt.Y(), thePnt.Z()};
    return opencascade::hashBytes(aData, sizeof(aData));
  }

  bool operator()(const gp_Pnt& thePnt1, const gp_Pnt& thePnt2) const noexcept
  {
    constexpr double aTol = 1e-12;
    return std::abs(thePnt1.X() - thePnt2.X()) <= aTol &&
           std::abs(thePnt1.Y() - thePnt2.Y()) <= aTol &&
           std::abs(thePnt1.Z() - thePnt2.Z()) <= aTol;
  }
};
```

#### 6.3.2 Direction Hasher (Foundational)

```cpp
// ODEHash_DirectionHasher.pxx
#include <Standard_HashUtils.hxx>
#include <gp_Dir.hxx>

//! Hasher for gp_Dir (direction vectors)
struct ODEHash_DirectionHasher
{
  std::size_t operator()(const gp_Dir& theDir) const noexcept
  {
    const Standard_Real aData[3] = {theDir.X(), theDir.Y(), theDir.Z()};
    return opencascade::hashBytes(aData, sizeof(aData));
  }

  bool operator()(const gp_Dir& theDir1, const gp_Dir& theDir2) const noexcept
  {
    constexpr double aTol = 1e-12;
    return std::abs(theDir1.X() - theDir2.X()) <= aTol &&
           std::abs(theDir1.Y() - theDir2.Y()) <= aTol &&
           std::abs(theDir1.Z() - theDir2.Z()) <= aTol;
  }
};
```

#### 6.3.3 Cylinder Hasher (Compositional)

```cpp
// ODEHash_CylindricalSurfaceHasher.pxx
#include <Standard_HashUtils.hxx>
#include <Geom_CylindricalSurface.hxx>
#include "ODEHash_PointHasher.pxx"
#include "ODEHash_DirectionHasher.pxx"

//! Hasher for Geom_CylindricalSurface
struct ODEHash_CylindricalSurfaceHasher
{
  std::size_t operator()(const Handle(Geom_Surface)& theSurf) const noexcept
  {
    Handle(Geom_CylindricalSurface) aCyl =
      Handle(Geom_CylindricalSurface)::DownCast(theSurf);
    if (aCyl.IsNull()) return 0;

    // Combine hashes: location + axis + radius
    const size_t aHashes[3] = {
      ODEHash_PointHasher{}(aCyl->Location()),
      ODEHash_DirectionHasher{}(aCyl->Axis().Direction()),
      opencascade::hash(aCyl->Radius())
    };

    return opencascade::hashBytes(aHashes, sizeof(aHashes));
  }

  bool operator()(const Handle(Geom_Surface)& theSurf1,
                  const Handle(Geom_Surface)& theSurf2) const noexcept
  {
    Handle(Geom_CylindricalSurface) aCyl1 =
      Handle(Geom_CylindricalSurface)::DownCast(theSurf1);
    Handle(Geom_CylindricalSurface) aCyl2 =
      Handle(Geom_CylindricalSurface)::DownCast(theSurf2);

    if (aCyl1.IsNull() || aCyl2.IsNull()) return false;

    // Compare components using sub-hashers
    if (!ODEHash_PointHasher{}(aCyl1->Location(), aCyl2->Location())) {
      return false;
    }

    if (!ODEHash_DirectionHasher{}(aCyl1->Axis().Direction(),
                                   aCyl2->Axis().Direction())) {
      return false;
    }

    constexpr double aTol = 1e-12;
    return std::abs(aCyl1->Radius() - aCyl2->Radius()) <= aTol;
  }
};
```

#### 6.3.4 BSpline Surface Hasher (Complex)

```cpp
// ODEHash_BSplineSurfaceHasher.pxx
#include <Standard_HashUtils.hxx>
#include <Geom_BSplineSurface.hxx>

//! Hasher for Geom_BSplineSurface
struct ODEHash_BSplineSurfaceHasher
{
  std::size_t operator()(const Handle(Geom_Surface)& theSurf) const noexcept
  {
    Handle(Geom_BSplineSurface) aBSpl =
      Handle(Geom_BSplineSurface)::DownCast(theSurf);
    if (aBSpl.IsNull()) return 0;

    size_t aHash = opencascade::MurmurHash::optimalSeed();

    // Hash degrees
    const Standard_Integer aDegrees[2] = {aBSpl->UDegree(), aBSpl->VDegree()};
    aHash = opencascade::hashBytes(aDegrees, sizeof(aDegrees), aHash);

    // Hash dimensions
    const Standard_Integer aDims[2] = {aBSpl->NbUPoles(), aBSpl->NbVPoles()};
    aHash = opencascade::hashBytes(aDims, sizeof(aDims), aHash);

    // Hash U knots
    const Standard_Integer aNbUKnots = aBSpl->NbUKnots();
    for (Standard_Integer i = 1; i <= aNbUKnots; ++i) {
      const Standard_Real aKnot = aBSpl->UKnot(i);
      aHash = opencascade::hash(aKnot, aHash);
    }

    // Hash V knots
    const Standard_Integer aNbVKnots = aBSpl->NbVKnots();
    for (Standard_Integer i = 1; i <= aNbVKnots; ++i) {
      const Standard_Real aKnot = aBSpl->VKnot(i);
      aHash = opencascade::hash(aKnot, aHash);
    }

    // Hash poles (sample a few for performance)
    const Standard_Integer aNbUPoles = aBSpl->NbUPoles();
    const Standard_Integer aNbVPoles = aBSpl->NbVPoles();
    for (Standard_Integer u = 1; u <= aNbUPoles; u += std::max(1, aNbUPoles / 5)) {
      for (Standard_Integer v = 1; v <= aNbVPoles; v += std::max(1, aNbVPoles / 5)) {
        const gp_Pnt& aPole = aBSpl->Pole(u, v);
        aHash = opencascade::hash(aPole.X(), aHash);
        aHash = opencascade::hash(aPole.Y(), aHash);
        aHash = opencascade::hash(aPole.Z(), aHash);
      }
    }

    return aHash;
  }

  bool operator()(const Handle(Geom_Surface)& theSurf1,
                  const Handle(Geom_Surface)& theSurf2) const noexcept
  {
    Handle(Geom_BSplineSurface) aBSpl1 =
      Handle(Geom_BSplineSurface)::DownCast(theSurf1);
    Handle(Geom_BSplineSurface) aBSpl2 =
      Handle(Geom_BSplineSurface)::DownCast(theSurf2);

    if (aBSpl1.IsNull() || aBSpl2.IsNull()) return false;

    // Compare degrees
    if (aBSpl1->UDegree() != aBSpl2->UDegree() ||
        aBSpl1->VDegree() != aBSpl2->VDegree()) {
      return false;
    }

    // Compare dimensions
    if (aBSpl1->NbUPoles() != aBSpl2->NbUPoles() ||
        aBSpl1->NbVPoles() != aBSpl2->NbVPoles() ||
        aBSpl1->NbUKnots() != aBSpl2->NbUKnots() ||
        aBSpl1->NbVKnots() != aBSpl2->NbVKnots()) {
      return false;
    }

    constexpr double aTol = 1e-12;

    // Compare U knots
    for (Standard_Integer i = 1; i <= aBSpl1->NbUKnots(); ++i) {
      if (std::abs(aBSpl1->UKnot(i) - aBSpl2->UKnot(i)) > aTol) {
        return false;
      }
    }

    // Compare V knots
    for (Standard_Integer i = 1; i <= aBSpl1->NbVKnots(); ++i) {
      if (std::abs(aBSpl1->VKnot(i) - aBSpl2->VKnot(i)) > aTol) {
        return false;
      }
    }

    // Compare all poles
    for (Standard_Integer u = 1; u <= aBSpl1->NbUPoles(); ++u) {
      for (Standard_Integer v = 1; v <= aBSpl1->NbVPoles(); ++v) {
        const gp_Pnt& aPole1 = aBSpl1->Pole(u, v);
        const gp_Pnt& aPole2 = aBSpl2->Pole(u, v);

        if (std::abs(aPole1.X() - aPole2.X()) > aTol ||
            std::abs(aPole1.Y() - aPole2.Y()) > aTol ||
            std::abs(aPole1.Z() - aPole2.Z()) > aTol) {
          return false;
        }
      }
    }

    // Compare weights if rational
    if (aBSpl1->IsURational() || aBSpl1->IsVRational()) {
      for (Standard_Integer u = 1; u <= aBSpl1->NbUPoles(); ++u) {
        for (Standard_Integer v = 1; v <= aBSpl1->NbVPoles(); ++v) {
          if (std::abs(aBSpl1->Weight(u, v) - aBSpl2->Weight(u, v)) > aTol) {
            return false;
          }
        }
      }
    }

    return true;
  }
};
```

#### 6.3.5 Line Hasher (3D Curve, Compositional)

```cpp
// ODEHash_LineHasher.pxx
#include <Standard_HashUtils.hxx>
#include <Geom_Line.hxx>
#include "ODEHash_PointHasher.pxx"
#include "ODEHash_DirectionHasher.pxx"

//! Hasher for Geom_Line
struct ODEHash_LineHasher
{
  std::size_t operator()(const Handle(Geom_Curve)& theCurve) const noexcept
  {
    Handle(Geom_Line) aLine = Handle(Geom_Line)::DownCast(theCurve);
    if (aLine.IsNull()) return 0;

    const size_t aHashes[2] = {
      ODEHash_PointHasher{}(aLine->Location()),
      ODEHash_DirectionHasher{}(aLine->Direction())
    };

    return opencascade::hashBytes(aHashes, sizeof(aHashes));
  }

  bool operator()(const Handle(Geom_Curve)& theCurve1,
                  const Handle(Geom_Curve)& theCurve2) const noexcept
  {
    Handle(Geom_Line) aLine1 = Handle(Geom_Line)::DownCast(theCurve1);
    Handle(Geom_Line) aLine2 = Handle(Geom_Line)::DownCast(theCurve2);

    if (aLine1.IsNull() || aLine2.IsNull()) return false;

    return ODEHash_PointHasher{}(aLine1->Location(), aLine2->Location()) &&
           ODEHash_DirectionHasher{}(aLine1->Direction(), aLine2->Direction());
  }
};
```

**Key Patterns Demonstrated:**
1. **Foundational hashers** (Point, Direction) are used by complex hashers
2. **Compositional design** - reuse simpler hashers via `{}()` syntax
3. **Performance optimization** - BSpline hasher samples poles instead of hashing all
4. **Tolerance consistency** - all use `1e-12` tolerance
5. **Type safety** - use `DownCast` and null checks
6. **Hash combination** - use `opencascade::hashBytes` for arrays of hashes

### 6.4 Hasher Integration with NCollection_DataMap

OCCT's `NCollection_DataMap` supports custom hashers through template specialization. Here's how hashers integrate with the collection classes.

#### 6.4.1 NCollection_DataMap Signature

```cpp
template<class TheKeyType,
         class TheValueType,
         class TheHasher = NCollection_DefaultHasher<TheKeyType>>
class NCollection_DataMap;
```

**The Hasher Requirements:**
- Must provide `static size_t HashCode(const TheKeyType& K, const int Upper)`
- Must provide `static Standard_Boolean IsEqual(const TheKeyType& K1, const TheKeyType& K2)`

#### 6.4.2 Adapting Dual-Operator Hashers for NCollection

Our dual-operator hashers need a thin wrapper to work with `NCollection_DataMap`:

```cpp
// ODEHash_SurfaceHasherAdapter.hxx
#include "ODEHash_SurfaceHasher.pxx"

//! Adapter to make ODEHash_SurfaceHasher compatible with NCollection_DataMap
struct ODEHash_SurfaceHasherAdapter
{
  //! Computes hash code for surface, in range [1, Upper]
  static size_t HashCode(const Handle(Geom_Surface)& theSurf, const int theUpper)
  {
    const size_t aHash = ODEHash_SurfaceHasher{}(theSurf);
    return static_cast<size_t>(aHash % theUpper) + 1;
  }

  //! Tests surfaces for equality
  static Standard_Boolean IsEqual(const Handle(Geom_Surface)& theSurf1,
                                   const Handle(Geom_Surface)& theSurf2)
  {
    return ODEHash_SurfaceHasher{}(theSurf1, theSurf2);
  }
};
```

#### 6.4.3 Usage with NCollection_DataMap

```cpp
// Define deduplication map
using SurfaceDedupMap = NCollection_DataMap<
  Handle(Geom_Surface),              // Key type
  NCollection_Vector<ODE_ObjectRef>, // Value type
  ODEHash_SurfaceHasherAdapter       // Hasher adapter
>;

// Usage
SurfaceDedupMap aSurfaceMap;

// Add surfaces
for (const auto& aSurf : myCollectedSurfaces)
{
  if (!aSurfaceMap.IsBound(aSurf)) {
    // First occurrence
    NCollection_Vector<ODE_ObjectRef> aRefs;
    aSurfaceMap.Bind(aSurf, aRefs);
  }
  else {
    // Duplicate detected via hasher equality check
    // ... handle duplicate
  }
}
```

#### 6.4.4 Alternative: Direct std::unordered_map Usage

For simpler code, use C++17's `std::unordered_map` directly with dual-operator hashers:

```cpp
#include <unordered_map>
#include "ODEHash_SurfaceHasher.pxx"

// Define map with hasher as template parameters
std::unordered_map<
  Handle(Geom_Surface),
  std::vector<ODE_ObjectRef>,
  ODEHash_SurfaceHasher,       // Hash function object
  ODEHash_SurfaceHasher        // Equality function object (yes, same struct!)
> surfaceMap;

// The hasher's operator()(surface) is used for hashing
// The hasher's operator()(surf1, surf2) is used for equality
```

**Advantages of std::unordered_map approach:**
- No adapter needed - dual-operator pattern matches C++ standard
- More familiar to developers
- Better C++17 integration
- Cleaner code

**Advantages of NCollection_DataMap approach:**
- Consistent with OCCT conventions
- Better integration with OCCT collections
- May have OCCT-specific optimizations

**Recommendation:** Use `std::unordered_map` for new ODE code (C++17 target), unless tight integration with existing OCCT code requires `NCollection_DataMap`.

#### 6.4.5 Complete Example

```cpp
// In ODE_Writer.cxx
#include <unordered_map>
#include "ODEHash_SurfaceHasher.pxx"

void ODE_Writer::deduplicateSurfaces()
{
  // Use std::unordered_map with dual-operator hasher
  std::unordered_map<
    Handle(Geom_Surface),
    std::vector<Handle(Geom_Surface)>,
    ODEHash_SurfaceHasher,  // Hash operator
    ODEHash_SurfaceHasher   // Equality operator
  > dedupMap;

  // Phase 1: Collect duplicates
  for (const auto& aSurf : myCollectedSurfaces)
  {
    // find() uses hasher's operator()(surf1, surf2) for comparison
    auto it = dedupMap.find(aSurf);

    if (it == dedupMap.end()) {
      // First occurrence - insert new entry
      dedupMap[aSurf] = {aSurf};
    }
    else {
      // Duplicate found - add to existing entry
      it->second.push_back(aSurf);
    }
  }

  // Phase 2: Build index mapping
  Standard_Integer anIndex = 0;
  for (const auto& [canonical, duplicates] : dedupMap)
  {
    // Add canonical surface to output file
    myUniqueSurfaces.Append(canonical);

    // Map all instances to this index
    Standard_Integer aSubIndex = 0;
    for (const auto& aSurf : duplicates) {
      ODE_ObjectRef aRef;
      aRef.FileType = "surfaces";
      aRef.Index = anIndex;
      aRef.SubIndex = aSubIndex++;

      mySurfaceToRef.Bind(aSurf, aRef);
    }

    ++anIndex;
  }

  // Statistics
  const size_t aOriginalCount = myCollectedSurfaces.Size();
  const size_t aUniqueCount = myUniqueSurfaces.Size();
  const double aReduction = 100.0 * (1.0 - aUniqueCount / aOriginalCount);

  std::cout << "Surface deduplication: "
            << aOriginalCount << " → " << aUniqueCount
            << " (" << aReduction << "% reduction)" << std::endl;
}
```

---

## 7. C++17 Features Usage

### 7.1 Recommended C++17 Features

#### std::optional for Optional Fields
```cpp
class ODE_Edge {
public:
  std::optional<ODE_ObjectRef> GetPolygon3D() const {
    return myPolygon3D;
  }

  void SetPolygon3D(const ODE_ObjectRef& theRef) {
    myPolygon3D = theRef;
  }

private:
  std::optional<ODE_ObjectRef> myPolygon3D;
};
```

#### std::variant for Type Unions
```cpp
using SurfaceVariant = std::variant<
  Handle(Geom_Plane),
  Handle(Geom_CylindricalSurface),
  Handle(Geom_ConicalSurface),
  Handle(Geom_SphericalSurface),
  Handle(Geom_BSplineSurface)
  // ...
>;

// Visitor pattern for serialization
struct SurfaceCapnpWriter {
  capnp::Surface::Builder& builder;

  void operator()(const Handle(Geom_Plane)& thePlane) {
    auto plane = builder.initPlane();
    // ... write plane data
  }

  void operator()(const Handle(Geom_CylindricalSurface)& theCyl) {
    auto cyl = builder.initCylinder();
    // ... write cylinder data
  }
  // ...
};

// Usage:
std::visit(SurfaceCapnpWriter{builder}, surfaceVariant);
```

#### Structured Bindings
```cpp
// Reading from map
for (const auto& [handle, ref] : mySurfaceMap) {
  std::cout << "Surface " << handle.get()
            << " -> " << ref.ToString() << std::endl;
}

// Tuple unpacking
auto [success, shape] = reader.Read("/path/to/archive");
```

#### if constexpr for Type Dispatch
```cpp
template<typename T>
ODE_ObjectRef addGeometry(const Handle(T)& theGeom) {
  if constexpr (std::is_base_of_v<Geom_Surface, T>) {
    return mySurfaceFile->AddSurface(theGeom, mySurfaceMap);
  } else if constexpr (std::is_base_of_v<Geom_Curve, T>) {
    return myCurve3dFile->AddCurve(theGeom, myCurve3dMap);
  } else if constexpr (std::is_base_of_v<Geom2d_Curve, T>) {
    return myCurve2dFile->AddCurve(theGeom, myCurve2dMap);
  }
}
```

#### std::filesystem for Archive Management
```cpp
#include <filesystem>

namespace fs = std::filesystem;

Standard_Boolean ODE_Archive::Write(const TCollection_AsciiString& thePath) {
  fs::path archivePath(thePath.ToCString());

  // Create directory
  if (!fs::create_directories(archivePath)) {
    return Standard_False;
  }

  // Write manifest
  fs::path manifestPath = archivePath / "manifest.json";
  myManifest->Write(manifestPath.string().c_str());

  // Write each file
  for (const auto& entry : fs::directory_iterator(archivePath)) {
    // ...
  }

  return Standard_True;
}
```

#### Fold Expressions for Hash Combination
```cpp
template<typename... Args>
size_t hashCombine(size_t seed, const Args&... args) {
  ((seed ^= std::hash<Args>{}(args) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
  return seed;
}

// Usage:
size_t hash = hashCombine(0,
  plane.Location().X(),
  plane.Location().Y(),
  plane.Location().Z(),
  plane.Axis().Direction().X()
  // ...
);
```

---

## 8. Cap'n Proto and JSON Interoperability

### 8.1 Bidirectional Conversion

Cap'n Proto supports JSON encoding natively:

```cpp
// Cap'n Proto -> JSON
void ODE_SurfaceFile::WriteToJson(const TCollection_AsciiString& thePath) const {
  // Build Cap'n Proto message
  capnp::MallocMessageBuilder message;
  auto root = message.initRoot<SurfaceFile>();

  // ... populate message

  // Convert to JSON
  capnp::JsonCodec json;
  kj::String jsonStr = json.encode(root);

  // Write to file
  std::ofstream file(thePath.ToCString());
  file << jsonStr.cStr();
}

// JSON -> Cap'n Proto
void ODE_SurfaceFile::ReadFromJson(const TCollection_AsciiString& thePath) {
  // Read JSON
  std::ifstream file(thePath.ToCString());
  std::string jsonStr((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  // Parse JSON to Cap'n Proto
  capnp::JsonCodec json;
  capnp::MallocMessageBuilder message;
  json.decode(kj::StringPtr(jsonStr.c_str()), message);

  auto root = message.getRoot<SurfaceFile>();

  // ... extract data
}
```

### 8.2 Manifest JSON with RapidJSON

```cpp
// ODE_Manifest.hxx
class ODE_Manifest : public Standard_Transient {
public:
  void WriteToJson(const TCollection_AsciiString& thePath) const;
  void ReadFromJson(const TCollection_AsciiString& thePath);

private:
  struct FileEntry {
    TCollection_AsciiString UUID;
    TCollection_AsciiString Name;
    TCollection_AsciiString Type;
    TCollection_AsciiString Encoding;  // "capnp" or "json"
    Standard_Integer ObjectCount;
    TCollection_AsciiString SHA256;
  };

  NCollection_Vector<FileEntry> myFiles;
  // ... metadata fields
};

// Implementation
void ODE_Manifest::WriteToJson(const TCollection_AsciiString& thePath) const {
  rapidjson::Document doc;
  doc.SetObject();
  auto& allocator = doc.GetAllocator();

  doc.AddMember("format", "ODE", allocator);
  doc.AddMember("version", "1.0", allocator);

  rapidjson::Value files(rapidjson::kArrayType);
  for (const auto& entry : myFiles) {
    rapidjson::Value file(rapidjson::kObjectType);
    file.AddMember("uuid",
      rapidjson::Value(entry.UUID.ToCString(), allocator), allocator);
    file.AddMember("name",
      rapidjson::Value(entry.Name.ToCString(), allocator), allocator);
    // ... other fields
    files.PushBack(file, allocator);
  }
  doc.AddMember("files", files, allocator);

  // Write to file
  std::ofstream ofs(thePath.ToCString());
  rapidjson::OStreamWrapper osw(ofs);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
  doc.Accept(writer);
}
```

---

## 9. Serialization Workflow

### 9.1 Write Process

```
┌─────────────────────────────────────────────────────────┐
│ 1. Input: TopoDS_Shape                                  │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 2. Collection Phase                                     │
│   - Traverse shape hierarchy                            │
│   - Collect all: vertices, edges, wires, faces, etc.   │
│   - Extract: surfaces, curves3d, curves2d, pcurves     │
│   - Gather: triangulations, polygons                    │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 3. Deduplication Phase (optional)                       │
│   - Hash each geometric object                          │
│   - Group by hash                                       │
│   - Deep compare within groups                          │
│   - Assign indices and sub-indices                      │
│   - Build mapping: Handle -> ODE_ObjectRef             │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 4. File Building Phase                                  │
│   - Create ODE_SurfaceFile (unique surfaces only)       │
│   - Create ODE_Curve3dFile (unique curves only)         │
│   - Create ODE_TopologyFile (with ObjectRefs)           │
│   - Create other files...                               │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 5. Serialization Phase                                  │
│   - Convert each file to Cap'n Proto                    │
│   - Optionally convert to JSON                          │
│   - Compute SHA-256 checksums                           │
│   - Generate UUIDs                                      │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 6. Archive Writing Phase                                │
│   - Create archive directory                            │
│   - Write manifest.json                                 │
│   - Write all data files (*.capnp or *.json)            │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
                  ✓ Complete
```

### 9.2 Example Code Flow

```cpp
Standard_Boolean ODE_Writer::Write(
  const TopoDS_Shape& theShape,
  const TCollection_AsciiString& thePath)
{
  // Phase 1: Collection
  collectObjects(theShape);

  // Phase 2: Deduplication
  if (myEnableDedup) {
    deduplicateObjects();
  }

  // Phase 3: Build files
  buildFiles();

  // Phase 4: Write archive
  return writeArchive(thePath);
}

void ODE_Writer::collectObjects(const TopoDS_Shape& theShape) {
  // Traverse shape
  for (TopExp_Explorer exp(theShape, TopAbs_FACE); exp.More(); exp.Next()) {
    const TopoDS_Face& aFace = TopoDS::Face(exp.Current());

    // Get surface
    Handle(Geom_Surface) aSurf = BRep_Tool::Surface(aFace);
    if (!aSurf.IsNull()) {
      myTempSurfaces.Append(aSurf);
    }

    // Get triangulation
    TopLoc_Location aLoc;
    Handle(Poly_Triangulation) aTri = BRep_Tool::Triangulation(aFace, aLoc);
    if (!aTri.IsNull()) {
      myTempTriangulations.Append(aTri);
    }
  }

  // Edges...
  for (TopExp_Explorer exp(theShape, TopAbs_EDGE); exp.More(); exp.Next()) {
    const TopoDS_Edge& anEdge = TopoDS::Edge(exp.Current());

    Standard_Real f, l;
    Handle(Geom_Curve) aCurve = BRep_Tool::Curve(anEdge, f, l);
    if (!aCurve.IsNull()) {
      myTempCurves3d.Append(aCurve);
    }

    // PCurves...
  }
}

void ODE_Writer::deduplicateObjects() {
  ODE_SurfaceDeduplicator aSurfDedup;
  ODE_CurveDeduplicator aCurveDedup;

  // Deduplicate surfaces
  for (const auto& aSurf : myTempSurfaces) {
    ODE_ObjectRef aRef = aSurfDedup.AddSurface(aSurf);
    mySurfaceMap.Bind(aSurf, aRef);
  }

  myUniqueSurfaces = aSurfDedup.GetUniqueSurfaces();

  // Deduplicate curves...
}
```

---

## 10. Deserialization Workflow

### 10.1 Read Process

```
┌─────────────────────────────────────────────────────────┐
│ 1. Input: Archive path                                  │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 2. Manifest Loading                                     │
│   - Read manifest.json                                  │
│   - Validate format version                             │
│   - Get file list and UUIDs                             │
│   - Verify checksums                                    │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 3. File Deserialization (Lazy or Eager)                │
│   - Read surfaces.capnp -> ODE_SurfaceFile              │
│   - Read curves3d.capnp -> ODE_Curve3dFile              │
│   - Read topology.capnp -> ODE_TopologyFile             │
│   - Parse Cap'n Proto or JSON                           │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 4. Object Reconstruction                                │
│   - Create Geom_Surface objects from indices            │
│   - Create Geom_Curve objects                           │
│   - Handle sub-indices for handle sharing               │
│   - Cache objects by ODE_ObjectRef                      │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 5. Topology Reconstruction                              │
│   - Build vertices                                      │
│   - Build edges (resolve curve refs)                    │
│   - Build wires                                         │
│   - Build faces (resolve surface refs)                  │
│   - Build shells, solids, compounds                     │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────┐
│ 6. Shape Assembly                                       │
│   - Assemble final TopoDS_Shape                         │
│   - Apply orientations                                  │
│   - Set tolerances                                      │
└──────────────────────┬──────────────────────────────────┘
                       │
                       ▼
                  ✓ Complete
```

### 10.2 Handle Sharing Implementation

```cpp
Handle(Geom_Surface) ODE_Reader::resolveSurfaceRef(const ODE_ObjectRef& theRef) {
  // Check cache first
  if (mySurfaceCache.IsBound(theRef)) {
    return mySurfaceCache.Find(theRef);
  }

  // Load surface from file
  Handle(Geom_Surface) aSurf =
    myArchive->GetSurfaceFile()->GetSurface(theRef.Index);

  if (aSurf.IsNull()) {
    return nullptr;
  }

  // Handle sharing based on sub-index
  if (theRef.HasSubIndex()) {
    // Check if another object with same index+subIndex exists
    ODE_ObjectRef aCacheKey = theRef;  // Same index + subIndex

    if (mySurfaceCache.IsBound(aCacheKey)) {
      // Reuse existing handle
      return mySurfaceCache.Find(aCacheKey);
    } else {
      // First occurrence - cache it
      mySurfaceCache.Bind(aCacheKey, aSurf);
      return aSurf;
    }
  } else {
    // No sub-index = deep copy (don't cache)
    return aSurf->Copy();  // Returns Handle(Geom_Geometry), cast to Surface
  }
}
```

**Key Points:**
- **With sub-index**: Cache and reuse the same handle
- **Without sub-index**: Always return a deep copy (don't cache)

---

## 11. Performance Considerations

### 11.1 Memory Efficiency
- **Deduplication** can reduce file size by 50-80% for typical CAD models
- **Handle sharing** reduces runtime memory by avoiding duplicate geometry
- **Lazy loading** of files allows processing large models incrementally

### 11.2 Speed Optimizations
- **Cap'n Proto** is zero-copy for deserialization (much faster than JSON)
- **Index-based references** are O(1) lookups (no string parsing)
- **Parallel file writing** using C++17 `std::async`:

```cpp
// Write files in parallel
auto futSurfaces = std::async(std::launch::async, [&]() {
  mySurfaceFile->WriteToCapnp(archivePath + "/surfaces.capnp");
});

auto futCurves = std::async(std::launch::async, [&]() {
  myCurve3dFile->WriteToCapnp(archivePath + "/curves3d.capnp");
});

// Wait for completion
futSurfaces.wait();
futCurves.wait();
```

### 11.3 Hash Function Performance
Use fast, non-cryptographic hash for deduplication:

```cpp
// xxHash or FNV-1a for geometry hashing
#include <xxhash.h>

size_t ODE_SurfaceDeduplicator::computeHash(
  const Handle(Geom_Surface)& theSurface)
{
  XXH64_state_t* state = XXH64_createState();
  XXH64_reset(state, 0);

  // Hash surface data
  if (theSurface->IsKind(STANDARD_TYPE(Geom_Plane))) {
    Handle(Geom_Plane) aPlane = Handle(Geom_Plane)::DownCast(theSurface);
    const gp_Pnt& aLoc = aPlane->Location();
    XXH64_update(state, &aLoc, sizeof(gp_Pnt));
    // ... hash other fields
  }
  // ... other surface types

  return XXH64_digest(state);
}
```

---

## 12. OCCT Coding Standards Compliance

### 12.1 Naming Conventions
- **Classes**: `ODE_ClassName` (module prefix + PascalCase)
- **Methods**: `MethodName()` (PascalCase, no prefix)
- **Member variables**: `myVariable` (camelCase with 'my' prefix)
- **Parameters**: `theParameter` (camelCase with 'the' prefix)
- **Local variables**: `aVariable` (camelCase with 'a' prefix)
- **Constants**: `THE_CONSTANT` (UPPER_SNAKE_CASE with 'THE_' prefix)

### 12.2 Handle Usage
```cpp
// Always use Handle() for Standard_Transient derived classes
Handle(ODE_Archive) anArchive = new ODE_Archive();

// Use DEFINE_STANDARD_RTTIEXT for RTTI
DEFINE_STANDARD_RTTIEXT(ODE_Archive, Standard_Transient)

// Use IsKind() for type checking
if (aSurface->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
  Handle(Geom_BSplineSurface) aBSpline =
    Handle(Geom_BSplineSurface)::DownCast(aSurface);
}
```

### 12.3 Error Handling

**IMPORTANT:** Avoid exceptions. Use status codes and error state tracking instead.

#### 12.3.1 Status Enumeration

```cpp
// ODE_Status.hxx
//! Status codes for ODE operations
enum ODE_Status
{
  ODE_Status_OK,                    //!< Operation completed successfully
  ODE_Status_FileNotFound,          //!< Archive or file not found
  ODE_Status_InvalidArchive,        //!< Archive structure is invalid
  ODE_Status_InvalidManifest,       //!< Manifest.json is malformed
  ODE_Status_UnsupportedVersion,    //!< ODE format version not supported
  ODE_Status_CorruptedData,         //!< Data file is corrupted
  ODE_Status_MissingFile,           //!< Required file missing from archive
  ODE_Status_InvalidShape,          //!< Input shape is invalid
  ODE_Status_WriteFailed,           //!< Failed to write file
  ODE_Status_ReadFailed,            //!< Failed to read file
  ODE_Status_CapnpError,            //!< Cap'n Proto serialization error
  ODE_Status_JsonError,             //!< JSON parsing error
  ODE_Status_InvalidReference,      //!< ObjectRef cannot be resolved
  ODE_Status_InternalError          //!< Internal error
};
```

#### 12.3.2 Error State Tracking

```cpp
// ODE_Reader.hxx
class ODE_Reader {
public:
  ODE_Reader();

  //! Main read method - returns success/failure
  Standard_Boolean Read(
    const TCollection_AsciiString& thePath,
    TopoDS_Shape& theShape
  );

  //! Get detailed status of last operation
  ODE_Status GetStatus() const { return myStatus; }

  //! Get human-readable error message
  const TCollection_AsciiString& GetErrorMessage() const { return myErrorMsg; }

  //! Check if last operation succeeded
  Standard_Boolean IsOK() const { return myStatus == ODE_Status_OK; }

private:
  ODE_Status myStatus;
  TCollection_AsciiString myErrorMsg;

  //! Set error state
  void setError(const ODE_Status theStatus, const TCollection_AsciiString& theMsg) {
    myStatus = theStatus;
    myErrorMsg = theMsg;
  }

  //! Clear error state
  void clearError() {
    myStatus = ODE_Status_OK;
    myErrorMsg.Clear();
  }
};
```

#### 12.3.3 Implementation Pattern

```cpp
// ODE_Reader.cxx
Standard_Boolean ODE_Reader::Read(
  const TCollection_AsciiString& thePath,
  TopoDS_Shape& theShape)
{
  clearError();

  // Check if path exists
  if (!fs::exists(thePath.ToCString())) {
    setError(ODE_Status_FileNotFound,
             "Archive directory does not exist: " + thePath);
    return Standard_False;
  }

  // Check manifest
  TCollection_AsciiString aManifestPath = thePath + "/manifest.json";
  if (!fs::exists(aManifestPath.ToCString())) {
    setError(ODE_Status_InvalidArchive,
             "Manifest file not found: " + aManifestPath);
    return Standard_False;
  }

  // Read manifest
  myManifest = new ODE_Manifest();
  if (!myManifest->Read(aManifestPath)) {
    setError(ODE_Status_InvalidManifest,
             "Failed to parse manifest: " + myManifest->GetErrorMessage());
    return Standard_False;
  }

  // Check version
  if (!isVersionSupported(myManifest->GetVersion())) {
    setError(ODE_Status_UnsupportedVersion,
             "ODE version not supported: " + myManifest->GetVersion());
    return Standard_False;
  }

  // Load files and build shape...
  if (!loadTopologyFile()) {
    return Standard_False;  // Error already set by loadTopologyFile
  }

  if (!buildShape(theShape)) {
    return Standard_False;  // Error already set by buildShape
  }

  return Standard_True;
}
```

#### 12.3.4 Usage Pattern

```cpp
// User code
ODE_Reader aReader;
TopoDS_Shape aShape;

if (!aReader.Read("/path/to/model.ode", aShape)) {
  // Error occurred - check details
  switch (aReader.GetStatus()) {
    case ODE_Status_FileNotFound:
      std::cerr << "File not found: " << aReader.GetErrorMessage() << std::endl;
      break;
    case ODE_Status_InvalidArchive:
      std::cerr << "Invalid archive: " << aReader.GetErrorMessage() << std::endl;
      break;
    case ODE_Status_UnsupportedVersion:
      std::cerr << "Unsupported version: " << aReader.GetErrorMessage() << std::endl;
      break;
    default:
      std::cerr << "Error: " << aReader.GetErrorMessage() << std::endl;
  }
  return 1;
}

// Success - use shape
std::cout << "Shape loaded successfully" << std::endl;
```

#### 12.3.5 Integration with Message_Messenger

For reporting errors to OCCT's messaging system:

```cpp
#include <Message.hxx>
#include <Message_Messenger.hxx>

void ODE_Reader::reportError(const TCollection_AsciiString& theMsg) {
  setError(ODE_Status_ReadFailed, theMsg);

  // Also report to OCCT message system
  Handle(Message_Messenger) aMessenger = Message::DefaultMessenger();
  if (!aMessenger.IsNull()) {
    aMessenger->Send(theMsg, Message_Fail);
  }
}

void ODE_Reader::reportWarning(const TCollection_AsciiString& theMsg) {
  Handle(Message_Messenger) aMessenger = Message::DefaultMessenger();
  if (!aMessenger.IsNull()) {
    aMessenger->Send(theMsg, Message_Warning);
  }
}
```

#### 12.3.6 Optional Result Type (C++17)

For methods that return values, use `std::optional`:

```cpp
// Returns surface or nullopt on error
std::optional<Handle(Geom_Surface)> ODE_SurfaceFile::GetSurface(
  const Standard_Integer theIndex)
{
  if (theIndex < 0 || theIndex >= mySurfaces.Size()) {
    return std::nullopt;  // Invalid index
  }

  return mySurfaces.Value(theIndex);
}

// Usage
auto aSurfOpt = surfaceFile->GetSurface(42);
if (aSurfOpt.has_value()) {
  Handle(Geom_Surface) aSurf = aSurfOpt.value();
  // ... use surface
} else {
  // Handle error
}
```

**Key Principles:**
- **No exceptions** - use return codes
- **Standard_Boolean** for success/failure
- **Status enums** for detailed error codes
- **Error messages** for human-readable descriptions
- **State tracking** - store last error in object
- **Message_Messenger** for integration with OCCT logging
- **std::optional** for optional return values

### 12.4 Documentation
```cpp
//! Writes a TopoDS_Shape to ODE archive format.
//! @param theShape [in] the shape to serialize
//! @param thePath [in] the output directory path
//! @return Standard_True on success, Standard_False otherwise
Standard_Boolean Write(
  const TopoDS_Shape& theShape,
  const TCollection_AsciiString& thePath
);
```

---

## 13. File Structure Recommendation

### 13.1 Main ODE Module

```
src/DataExchange/TKDE/ODE/
├── CMakeLists.txt
├── FILES.cmake
├── ODE_FORMAT_DESIGN.md              # This document
├── ODE_Status.hxx                    # Status enumeration
├── ODE_Archive.hxx / .cxx
├── ODE_Object.hxx
├── ODE_ObjectRef.hxx / .cxx
├── ODE_Manifest.hxx / .cxx
├── ODE_TopologyFile.hxx / .cxx
├── ODE_SurfaceFile.hxx / .cxx
├── ODE_Curve3dFile.hxx / .cxx
├── ODE_Curve2dFile.hxx / .cxx
├── ODE_TriangulationFile.hxx / .cxx
├── ODE_PolygonFile.hxx / .cxx
├── ODE_Writer.hxx / .cxx
├── ODE_Reader.hxx / .cxx
└── ODE_Provider.hxx / .cxx           # DE_Provider subclass
```

### 13.2 ODEHash Module (Separate Module)

```
src/DataExchange/TKDE/ODEHash/
├── CMakeLists.txt
├── FILES.cmake
│
├── Foundational Hashers (geometry primitives)
├── ODEHash_PointHasher.pxx           # gp_Pnt
├── ODEHash_Point2dHasher.pxx         # gp_Pnt2d
├── ODEHash_DirectionHasher.pxx       # gp_Dir
├── ODEHash_Direction2dHasher.pxx     # gp_Dir2d
├── ODEHash_VectorHasher.pxx          # gp_Vec
├── ODEHash_AxisPlacement.pxx         # gp_Ax1, gp_Ax2, gp_Ax3
│
├── Surface Hashers (Geom_Surface - 12 types)
├── ODEHash_PlaneHasher.pxx                       # Geom_Plane
├── ODEHash_CylindricalSurfaceHasher.pxx          # Geom_CylindricalSurface
├── ODEHash_ConicalSurfaceHasher.pxx              # Geom_ConicalSurface
├── ODEHash_SphericalSurfaceHasher.pxx            # Geom_SphericalSurface
├── ODEHash_ToroidalSurfaceHasher.pxx             # Geom_ToroidalSurface
├── ODEHash_SurfaceOfRevolutionHasher.pxx         # Geom_SurfaceOfRevolution
├── ODEHash_SurfaceOfLinearExtrusionHasher.pxx    # Geom_SurfaceOfLinearExtrusion
├── ODEHash_BezierSurfaceHasher.pxx               # Geom_BezierSurface
├── ODEHash_BSplineSurfaceHasher.pxx              # Geom_BSplineSurface
├── ODEHash_RectangularTrimmedSurfaceHasher.pxx   # Geom_RectangularTrimmedSurface
├── ODEHash_OffsetSurfaceHasher.pxx               # Geom_OffsetSurface
├── ODEHash_SurfaceHasher.pxx                     # Polymorphic dispatcher
│
├── Curve3D Hashers (Geom_Curve - 11 types)
├── ODEHash_LineHasher.pxx                        # Geom_Line
├── ODEHash_CircleHasher.pxx                      # Geom_Circle
├── ODEHash_EllipseHasher.pxx                     # Geom_Ellipse
├── ODEHash_HyperbolaHasher.pxx                   # Geom_Hyperbola
├── ODEHash_ParabolaHasher.pxx                    # Geom_Parabola
├── ODEHash_BezierCurveHasher.pxx                 # Geom_BezierCurve
├── ODEHash_BSplineCurveHasher.pxx                # Geom_BSplineCurve
├── ODEHash_TrimmedCurveHasher.pxx                # Geom_TrimmedCurve
├── ODEHash_OffsetCurveHasher.pxx                 # Geom_OffsetCurve
├── ODEHash_Curve3dHasher.pxx                     # Polymorphic dispatcher
│
├── Curve2D Hashers (Geom2d_Curve - 11 types)
├── ODEHash_Line2dHasher.pxx                      # Geom2d_Line
├── ODEHash_Circle2dHasher.pxx                    # Geom2d_Circle
├── ODEHash_Ellipse2dHasher.pxx                   # Geom2d_Ellipse
├── ODEHash_Hyperbola2dHasher.pxx                 # Geom2d_Hyperbola
├── ODEHash_Parabola2dHasher.pxx                  # Geom2d_Parabola
├── ODEHash_BezierCurve2dHasher.pxx               # Geom2d_BezierCurve
├── ODEHash_BSplineCurve2dHasher.pxx              # Geom2d_BSplineCurve
├── ODEHash_TrimmedCurve2dHasher.pxx              # Geom2d_TrimmedCurve
├── ODEHash_OffsetCurve2dHasher.pxx               # Geom2d_OffsetCurve
└── ODEHash_Curve2dHasher.pxx                     # Polymorphic dispatcher
```

**Total: 40 hasher files** (6 foundational + 12 surface + 11 curve3d + 11 curve2d)

### 13.3 Cap'n Proto Schemas

```
resources/ODE/
├── ode.capnp                         # Base types (Vec3, Vec2, ObjectRef, etc.)
├── surfaces.capnp                    # All 12 surface types
├── curves3d.capnp                    # All 11 3D curve types
├── curves2d.capnp                    # All 11 2D curve types
├── topology.capnp                    # TopoDS topology structures
├── triangulation.capnp               # Poly_Triangulation data
└── polygons.capnp                    # Poly_Polygon3D/2D data
```

### 13.4 Notes

**Why separate ODEHash module?**
- Hashers are reusable across different serialization formats (not just ODE)
- Clear separation of concerns: ODE = format, ODEHash = comparison utilities
- Can be tested independently
- Follows OCCT modularity principles
- Other modules could use these hashers for deduplication

**ODEHash_ prefix:**
- Distinguishes from ODE_ (format-specific classes)
- Follows OCCT naming: module prefix + class name
- Consistent with StepTidy pattern (StepTidy_*Hasher)

**.pxx extension:**
- OCCT convention for header-only template/inline code
- Used by StepTidy module for all hasher implementations
- Contains only struct definitions with inline methods
- No corresponding `.cxx` file needed
- Included directly where needed via `#include`

**Schemas in resources/:**
- Cap'n Proto schemas are data files, not source code
- Resources directory is standard location for data files
- Allows easy distribution with OCCT
- Can be accessed by external tools for schema inspection

---

## 14. Integration with OCCT DataExchange Module

### 14.1 DE_Provider Integration

```cpp
// ODE_Provider.hxx
#include <DE_Provider.hxx>

class ODE_Provider : public DE_Provider {
public:
  DEFINE_STANDARD_RTTIEXT(ODE_Provider, DE_Provider)

  //! Gets provider name
  virtual TCollection_AsciiString GetVendor() const override {
    return "OPEN CASCADE";
  }

  virtual TCollection_AsciiString GetFormat() const override {
    return "ODE";
  }

  //! Reads document from file
  virtual Standard_Boolean Read(
    const TCollection_AsciiString& thePath,
    const Handle(TDocStd_Document)& theDocument,
    Handle(XSControl_WorkSession)& theWS,
    const Message_ProgressRange& theProgress = Message_ProgressRange()) override;

  //! Writes document to file
  virtual Standard_Boolean Write(
    const TCollection_AsciiString& thePath,
    const Handle(TDocStd_Document)& theDocument,
    Handle(XSControl_WorkSession)& theWS,
    const Message_ProgressRange& theProgress = Message_ProgressRange()) override;

  //! Reads shape from file
  virtual Standard_Boolean Read(
    const TCollection_AsciiString& thePath,
    TopoDS_Shape& theShape,
    Handle(XSControl_WorkSession)& theWS,
    const Message_ProgressRange& theProgress = Message_ProgressRange()) override;

  //! Writes shape to file
  virtual Standard_Boolean Write(
    const TCollection_AsciiString& thePath,
    const TopoDS_Shape& theShape,
    Handle(XSControl_WorkSession)& theWS,
    const Message_ProgressRange& theProgress = Message_ProgressRange()) override;
};
```

### 14.2 Registration

```cpp
// In ODE_Provider.cxx static initialization
static Standard_Boolean registerProvider() {
  Handle(ODE_Provider) aProvider = new ODE_Provider();
  DE_Wrapper::GlobalWrapper()->Bind(aProvider);
  return Standard_True;
}

static Standard_Boolean isRegistered = registerProvider();
```

---

## 15. Testing Strategy

### 15.1 Unit Tests
- Test each deduplicator (surfaces, curves, etc.)
- Test ObjectRef string conversion
- Test hash collision handling
- Test deep equality comparisons
- Test handle sharing logic

### 15.2 Integration Tests
- Write and read simple shapes (box, cylinder, sphere)
- Write and read complex assemblies
- Test with large models (> 10,000 faces)
- Compare with original BRep format
- Validate deduplication statistics

### 15.3 Performance Benchmarks
- Measure write time vs BRep
- Measure read time vs BRep
- Measure file size reduction
- Memory usage comparison
- Deduplication effectiveness

---

## 16. Future Extensions

### 16.1 Potential Enhancements
1. **Compression**: Add optional gzip/zstd compression per file
2. **Streaming**: Support streaming read/write for huge models
3. **Partial updates**: Modify individual files without rewriting entire archive
4. **Attributes**: Add support for XCAFDoc attributes (colors, layers, metadata)
5. **Assembly structure**: Preserve XCAF assembly hierarchy
6. **External references**: Support references to other ODE archives
7. **Versioning**: Schema evolution with backward compatibility
8. **Validation**: JSON schema validation for manifest
9. **Encryption**: Optional AES encryption for IP protection
10. **Cloud storage**: Direct read/write to S3, Azure Blob, etc.

### 16.2 Interoperability
- **ODE → STEP**: Converter to STEP AP242
- **ODE → glTF**: Direct export to glTF 2.0 (reuse triangulation)
- **ODE → IFC**: BIM integration
- **ODE → USD**: Integration with Pixar's Universal Scene Description

---

## 17. Implementation Phases

### Phase 1: Foundation (Weeks 1-2)
- [ ] Define Cap'n Proto schemas
- [ ] Implement base classes (ODE_Object, ODE_ObjectRef, ODE_Archive)
- [ ] Implement manifest read/write with RapidJSON
- [ ] Set up CMake build with Cap'n Proto integration

### Phase 2: Geometry Serialization (Weeks 3-5)
- [ ] Implement ODE_SurfaceFile
- [ ] Implement ODE_Curve3dFile
- [ ] Implement ODE_Curve2dFile
- [ ] Write Cap'n Proto serializers for each geometry type
- [ ] Test geometry round-trip (write + read)

### Phase 3: Topology Serialization (Weeks 6-7)
- [ ] Implement ODE_TopologyFile
- [ ] Handle vertex, edge, wire, face, shell, solid, compound
- [ ] Implement cross-reference resolution
- [ ] Test topology round-trip

### Phase 4: Deduplication (Weeks 8-10)
- [ ] Implement hash functions for all geometry types
- [ ] Implement deep equality comparators
- [ ] Implement ODE_SurfaceDeduplicator
- [ ] Implement ODE_CurveDeduplicator
- [ ] Add sub-index logic for handle sharing
- [ ] Benchmark deduplication effectiveness

### Phase 5: Reader/Writer (Weeks 11-12)
- [ ] Implement ODE_Writer with full workflow
- [ ] Implement ODE_Reader with handle sharing
- [ ] Add progress reporting
- [ ] Error handling and validation

### Phase 6: Integration (Week 13)
- [ ] Implement ODE_Provider for DE module
- [ ] Register provider with DE_Wrapper
- [ ] Command-line tool integration
- [ ] Documentation

### Phase 7: Testing & Optimization (Weeks 14-16)
- [ ] Comprehensive test suite
- [ ] Performance benchmarks
- [ ] Memory profiling
- [ ] Parallel I/O optimization
- [ ] Bug fixes

---

## 18. Success Metrics

### 18.1 Performance Targets
- **File size**: 30-50% smaller than BRep for typical models
- **Write speed**: Within 2x of BRep write time
- **Read speed**: Within 1.5x of BRep read time (accounting for handle sharing setup)
- **Memory**: 20-40% less runtime memory due to handle sharing

### 18.2 Quality Metrics
- **Fidelity**: 100% topology and geometry preservation
- **Tolerance**: No loss of precision beyond OCCT's native tolerance
- **Compatibility**: Works with all OCCT topology types
- **Stability**: No memory leaks, no crashes on malformed files

---

## Conclusion

The ODE format represents a significant modernization of OCCT's data exchange capabilities. By leveraging:
- **Cap'n Proto** for efficient serialization
- **Intelligent deduplication** for size reduction
- **Handle sharing** for memory efficiency
- **Modular architecture** for extensibility
- **C++17 features** for clean, maintainable code

This format can serve as OCCT's next-generation native format, suitable for CAD, BIM, and digital twin applications.

**Next Steps:**
1. Review and approve this design document
2. Create Cap'n Proto schema files
3. Set up build infrastructure
4. Begin Phase 1 implementation

---

**Document End**
