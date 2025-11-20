@0xc6f2a8b9d5e73401;
# ODE Polygons Schema
# Definitions for Poly_Polygon3D and Poly_Polygon2D

using import "ode.capnp".Vec3;
using import "ode.capnp".Vec2;

# Polygon container with polymorphic union
struct Polygon {
  index @0 :UInt32;

  union {
    polygon3d @1 :Polygon3D;
    polygon2d @2 :Polygon2D;
  }
}

# 3D polygon (polyline in 3D space)
struct Polygon3D {
  nodes @0 :List(Vec3);           # Sequential 3D points forming polyline
  parameters @1 :List(Float64);   # Optional curve parameters (empty if not present)
  deflection @2 :Float64;         # Maximum deviation from original curve
}

# 2D polygon (polyline in parametric space)
struct Polygon2D {
  nodes @0 :List(Vec2);           # Sequential 2D points in UV space
  parameters @1 :List(Float64);   # Optional curve parameters (empty if not present)
  deflection @2 :Float64;         # Maximum deviation from original curve
}

# Container for all polygons in file
struct PolygonFile {
  polygons @0 :List(Polygon);
}
