@0x80b3c4b6c12d9ae1;
# ODE Base Types Schema
# Common data structures used across all ODE schemas

# 3D Vector
struct Vec3 {
  x @0 :Float64;
  y @1 :Float64;
  z @2 :Float64;
}

# 2D Vector
struct Vec2 {
  x @0 :Float64;
  y @1 :Float64;
}

# Quaternion for rotations
struct Quaternion {
  x @0 :Float64;
  y @1 :Float64;
  z @2 :Float64;
  w @3 :Float64;
}

# Transformation matrix (translation + rotation + scale)
struct Transform {
  translation @0 :Vec3;
  rotation @1 :Quaternion;
  scale @2 :Float64 = 1.0;
}

# Reference to an object in another file
struct ObjectRef {
  fileType @0 :Text;              # "surfaces", "curves3d", "curves2d", etc.
  index @1 :UInt32;               # Object index in that file
  subIndex @2 :UInt32 = 0xFFFFFFFF; # 0xFFFFFFFF = no sub-index (deep copy)
                                   # Other values = handle sharing group ID
}

# Optional ObjectRef (for nullable references)
struct OptionalObjectRef {
  union {
    none @0 :Void;
    ref @1 :ObjectRef;
  }
}

# Axis placement (location + direction + reference direction)
struct AxisPlacement {
  location @0 :Vec3;
  axis @1 :Vec3;       # Z direction
  refDirection @2 :Vec3; # X direction
}

# 2D Axis placement
struct AxisPlacement2d {
  location @0 :Vec2;
  refDirection @1 :Vec2; # X direction
}

# Parameter range
struct ParameterRange {
  first @0 :Float64;
  last @1 :Float64;
}

# UV parameter range (for surfaces)
struct UVRange {
  uMin @0 :Float64;
  uMax @1 :Float64;
  vMin @2 :Float64;
  vMax @3 :Float64;
}
