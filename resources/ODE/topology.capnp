@0xadc52e5541851838;
# ODE Topology Schema
# Definitions for TopoDS shape hierarchy

using import "ode.capnp".Vec3;
using import "ode.capnp".ObjectRef;
using import "ode.capnp".OptionalObjectRef;
using import "ode.capnp".UVRange;

# Shape type enumeration
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

# Orientation enumeration
enum Orientation {
  forward @0;
  reversed @1;
  internal @2;
  external @3;
}

# Generic shape container with polymorphic union
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

# Vertex
struct Vertex {
  point @0 :Vec3;
  tolerance @1 :Float64;
}

# Edge
struct Edge {
  vertices @0 :List(UInt32);      # Indices to vertex shapes (0 or 2 vertices)
  curve3d @1 :OptionalObjectRef;  # Reference to curves3d file (optional)
  firstParameter @2 :Float64;
  lastParameter @3 :Float64;
  tolerance @4 :Float64;
  degenerated @5 :Bool = false;

  # PCurves (parametric curves on faces)
  pcurves @6 :List(PCurveOnFace);

  # Optional polygon representation
  polygon3d @7 :OptionalObjectRef;  # Reference to polygons file
}

# Parametric curve on a face
struct PCurveOnFace {
  face @0 :UInt32;                # Index to face shape
  curve2d @1 :ObjectRef;          # Reference to curves2d file
  uvRange @2 :UVRange;            # Parameter range on surface
}

# Wire
struct Wire {
  edges @0 :List(UInt32);         # Indices to edge shapes
  closed @1 :Bool = false;
}

# Face
struct Face {
  surface @0 :ObjectRef;          # Reference to surfaces file
  wires @1 :List(UInt32);         # Indices to wire shapes (first is outer boundary)
  orientation @2 :Orientation;
  tolerance @3 :Float64;

  # Natural bounds for infinite surfaces
  naturalBounds @4 :UVRange;

  # Optional triangulation
  triangulation @5 :OptionalObjectRef;  # Reference to triangulation file
}

# Shell
struct Shell {
  faces @0 :List(UInt32);         # Indices to face shapes
  closed @1 :Bool = false;
}

# Solid
struct Solid {
  shells @0 :List(UInt32);        # Indices to shell shapes (first is outer shell)
}

# Compound solid
struct CompSolid {
  solids @0 :List(UInt32);        # Indices to solid shapes
}

# Compound
struct Compound {
  children @0 :List(UInt32);      # Indices to mixed shape types
}

# Container for all shapes in file
struct TopologyFile {
  shapes @0 :List(Shape);
  rootShape @1 :UInt32;           # Index of the root shape
}
