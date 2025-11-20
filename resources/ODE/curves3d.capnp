@0xfad460b01ca25d2d;
# ODE 3D Curves Schema
# Definitions for all Geom_Curve types

using import "ode.capnp".Vec3;
using import "ode.capnp".AxisPlacement;
using import "ode.capnp".ObjectRef;
using import "ode.capnp".ParameterRange;

# 3D Curve container with polymorphic union
struct Curve3d {
  index @0 :UInt32;

  union {
    line @1 :Line;
    circle @2 :Circle;
    ellipse @3 :Ellipse;
    hyperbola @4 :Hyperbola;
    parabola @5 :Parabola;
    bezierCurve @6 :BezierCurve;
    bsplineCurve @7 :BSplineCurve;
    trimmedCurve @8 :TrimmedCurve;
    offsetCurve @9 :OffsetCurve;
  }
}

# Straight line
struct Line {
  location @0 :Vec3;
  direction @1 :Vec3;  # Normalized direction vector
}

# Circle
struct Circle {
  position @0 :AxisPlacement;  # Center, axis (normal), refDirection
  radius @1 :Float64;
}

# Ellipse
struct Ellipse {
  position @0 :AxisPlacement;
  majorRadius @1 :Float64;
  minorRadius @2 :Float64;
}

# Hyperbola
struct Hyperbola {
  position @0 :AxisPlacement;
  majorRadius @1 :Float64;
  minorRadius @2 :Float64;
}

# Parabola
struct Parabola {
  position @0 :AxisPlacement;
  focalLength @1 :Float64;
}

# Bezier curve
struct BezierCurve {
  degree @0 :UInt32;
  poles @1 :List(Vec3);
  weights @2 :List(Float64);  # Empty if non-rational
  rational @3 :Bool;
  periodic @4 :Bool = false;
}

# B-Spline curve
struct BSplineCurve {
  degree @0 :UInt32;
  knots @1 :List(Float64);
  multiplicities @2 :List(UInt32);
  poles @3 :List(Vec3);
  weights @4 :List(Float64);  # Empty if non-rational
  rational @5 :Bool;
  periodic @6 :Bool = false;
  closed @7 :Bool = false;
}

# Trimmed curve
struct TrimmedCurve {
  basisCurve @0 :ObjectRef;   # Reference to another curve in this file
  firstParameter @1 :Float64;
  lastParameter @2 :Float64;
  sense @3 :Bool = true;      # true = same sense as basis curve
}

# Offset curve
struct OffsetCurve {
  basisCurve @0 :ObjectRef;   # Reference to another curve in this file
  offset @1 :Float64;
  direction @2 :Vec3;         # Offset direction
}

# Container for all 3D curves in file
struct Curve3dFile {
  curves @0 :List(Curve3d);
}
