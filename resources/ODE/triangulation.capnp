@0xb7c4f9d3e8a21c5f;
# ODE Triangulation Schema
# Definitions for Poly_Triangulation data

using import "ode.capnp".Vec3;
using import "ode.capnp".Vec2;

# Triangulation structure
struct Triangulation {
  index @0 :UInt32;

  # Mesh data
  nodes @1 :List(Vec3);           # 3D vertex positions
  triangles @2 :List(UInt32);     # Flat array: [i0,i1,i2, i3,i4,i5, ...] (indices into nodes)

  # Optional data
  normals @3 :List(Vec3);         # Per-node normals (empty if not present)
  uvNodes @4 :List(Vec2);         # Parametric coordinates (empty if not present)

  # Quality parameters
  deflection @5 :Float64;         # Maximum deviation from original surface
}

# Container for all triangulations in file
struct TriangulationFile {
  triangulations @0 :List(Triangulation);
}
