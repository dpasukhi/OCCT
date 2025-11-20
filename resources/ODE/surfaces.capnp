@0x964c3cc577fcd75d;
# ODE Surfaces Schema
# Definitions for all Geom_Surface types

using import "ode.capnp".Vec3;
using import "ode.capnp".AxisPlacement;
using import "ode.capnp".ObjectRef;
using import "ode.capnp".UVRange;

# Surface container with polymorphic union
struct Surface {
  index @0 :UInt32;

  union {
    plane @1 :Plane;
    cylindricalSurface @2 :CylindricalSurface;
    conicalSurface @3 :ConicalSurface;
    sphericalSurface @4 :SphericalSurface;
    toroidalSurface @5 :ToroidalSurface;
    surfaceOfRevolution @6 :SurfaceOfRevolution;
    surfaceOfLinearExtrusion @7 :SurfaceOfLinearExtrusion;
    bezierSurface @8 :BezierSurface;
    bsplineSurface @9 :BSplineSurface;
    rectangularTrimmedSurface @10 :RectangularTrimmedSurface;
    offsetSurface @11 :OffsetSurface;
  }
}

# Plane surface
struct Plane {
  position @0 :AxisPlacement;  # Location, axis (normal), refDirection
}

# Cylindrical surface
struct CylindricalSurface {
  position @0 :AxisPlacement;
  radius @1 :Float64;
}

# Conical surface
struct ConicalSurface {
  position @0 :AxisPlacement;
  radius @1 :Float64;      # Radius at apex
  semiAngle @2 :Float64;   # Half-angle in radians
}

# Spherical surface
struct SphericalSurface {
  position @0 :AxisPlacement;
  radius @1 :Float64;
}

# Toroidal surface
struct ToroidalSurface {
  position @0 :AxisPlacement;
  majorRadius @1 :Float64;
  minorRadius @2 :Float64;
}

# Surface of revolution
struct SurfaceOfRevolution {
  basisCurve @0 :ObjectRef;  # Reference to curves3d file
  axisPosition @1 :AxisPlacement;
}

# Surface of linear extrusion
struct SurfaceOfLinearExtrusion {
  basisCurve @0 :ObjectRef;  # Reference to curves3d file
  direction @1 :Vec3;
}

# Bezier surface
struct BezierSurface {
  uDegree @0 :UInt32;
  vDegree @1 :UInt32;
  poles @2 :List(Vec3);      # Row-major: [u0v0, u0v1, ..., u1v0, ...]
  weights @3 :List(Float64); # Empty if non-rational
  uRational @4 :Bool;
  vRational @5 :Bool;
  uPeriodic @6 :Bool = false;
  vPeriodic @7 :Bool = false;
}

# B-Spline surface
struct BSplineSurface {
  uDegree @0 :UInt32;
  vDegree @1 :UInt32;
  uKnots @2 :List(Float64);
  vKnots @3 :List(Float64);
  uMultiplicities @4 :List(UInt32);
  vMultiplicities @5 :List(UInt32);
  poles @6 :List(Vec3);      # Row-major: [u0v0, u0v1, ..., u1v0, ...]
  weights @7 :List(Float64); # Empty if non-rational
  uRational @8 :Bool;
  vRational @9 :Bool;
  uPeriodic @10 :Bool = false;
  vPeriodic @11 :Bool = false;
}

# Rectangular trimmed surface
struct RectangularTrimmedSurface {
  basisSurface @0 :ObjectRef; # Reference to another surface in this file
  uMin @1 :Float64;
  uMax @2 :Float64;
  vMin @3 :Float64;
  vMax @4 :Float64;
  usense @5 :Bool = true;     # U direction sense
  vsense @6 :Bool = true;     # V direction sense
}

# Offset surface
struct OffsetSurface {
  basisSurface @0 :ObjectRef; # Reference to another surface in this file
  offset @1 :Float64;
  osculating @2 :Bool = false;
}

# Container for all surfaces in file
struct SurfaceFile {
  surfaces @0 :List(Surface);
}
