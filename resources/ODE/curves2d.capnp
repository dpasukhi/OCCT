@0xad8e8cb37860563b;
# ODE 2D Curves Schema
# Definitions for all Geom2d_Curve types

using import "ode.capnp".Vec2;
using import "ode.capnp".AxisPlacement2d;
using import "ode.capnp".ObjectRef;
using import "ode.capnp".ParameterRange;

# 2D Curve container with polymorphic union
struct Curve2d {
  index @0 :UInt32;

  union {
    line @1 :Line2d;
    circle @2 :Circle2d;
    ellipse @3 :Ellipse2d;
    hyperbola @4 :Hyperbola2d;
    parabola @5 :Parabola2d;
    bezierCurve @6 :BezierCurve2d;
    bsplineCurve @7 :BSplineCurve2d;
    trimmedCurve @8 :TrimmedCurve2d;
    offsetCurve @9 :OffsetCurve2d;
  }
}

# Straight line in 2D
struct Line2d {
  location @0 :Vec2;
  direction @1 :Vec2;  # Normalized direction vector
}

# Circle in 2D
struct Circle2d {
  position @0 :AxisPlacement2d;  # Center and reference direction
  radius @1 :Float64;
}

# Ellipse in 2D
struct Ellipse2d {
  position @0 :AxisPlacement2d;
  majorRadius @1 :Float64;
  minorRadius @2 :Float64;
}

# Hyperbola in 2D
struct Hyperbola2d {
  position @0 :AxisPlacement2d;
  majorRadius @1 :Float64;
  minorRadius @2 :Float64;
}

# Parabola in 2D
struct Parabola2d {
  position @0 :AxisPlacement2d;
  focalLength @1 :Float64;
}

# Bezier curve in 2D
struct BezierCurve2d {
  degree @0 :UInt32;
  poles @1 :List(Vec2);
  weights @2 :List(Float64);  # Empty if non-rational
  rational @3 :Bool;
  periodic @4 :Bool = false;
}

# B-Spline curve in 2D
struct BSplineCurve2d {
  degree @0 :UInt32;
  knots @1 :List(Float64);
  multiplicities @2 :List(UInt32);
  poles @3 :List(Vec2);
  weights @4 :List(Float64);  # Empty if non-rational
  rational @5 :Bool;
  periodic @6 :Bool = false;
  closed @7 :Bool = false;
}

# Trimmed curve in 2D
struct TrimmedCurve2d {
  basisCurve @0 :ObjectRef;   # Reference to another curve in this file
  firstParameter @1 :Float64;
  lastParameter @2 :Float64;
  sense @3 :Bool = true;      # true = same sense as basis curve
}

# Offset curve in 2D
struct OffsetCurve2d {
  basisCurve @0 :ObjectRef;   # Reference to another curve in this file
  offset @1 :Float64;
}

# Container for all 2D curves in file
struct Curve2dFile {
  curves @0 :List(Curve2d);
}
