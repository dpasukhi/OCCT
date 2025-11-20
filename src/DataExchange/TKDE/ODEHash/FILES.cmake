# Source files for ODEHash package
# Most hashers are header-only .pxx files
# Polymorphic hashers (SurfaceHasher, Curve3dHasher, Curve2dHasher) have .hxx/.cxx pairs
set(OCCT_ODEHash_FILES_LOCATION "${CMAKE_CURRENT_LIST_DIR}")

set(OCCT_ODEHash_FILES
  # Foundational Hashers
  ODEHash_PointHasher.pxx
  ODEHash_Point2dHasher.pxx
  ODEHash_DirectionHasher.pxx
  ODEHash_Direction2dHasher.pxx
  ODEHash_VectorHasher.pxx
  ODEHash_AxisPlacement.pxx
  ODEHash_AxisPlacement2d.pxx

  # Surface Hashers
  ODEHash_PlaneHasher.pxx
  ODEHash_CylindricalSurfaceHasher.pxx
  ODEHash_ConicalSurfaceHasher.pxx
  ODEHash_SphericalSurfaceHasher.pxx
  ODEHash_ToroidalSurfaceHasher.pxx
  ODEHash_SurfaceOfRevolutionHasher.pxx
  ODEHash_SurfaceOfLinearExtrusionHasher.pxx
  ODEHash_BezierSurfaceHasher.pxx
  ODEHash_BSplineSurfaceHasher.pxx
  ODEHash_RectangularTrimmedSurfaceHasher.pxx
  ODEHash_OffsetSurfaceHasher.pxx
  ODEHash_SurfaceHasher.hxx
  ODEHash_SurfaceHasher.cxx

  # Curve3D Hashers
  ODEHash_LineHasher.pxx
  ODEHash_CircleHasher.pxx
  ODEHash_EllipseHasher.pxx
  ODEHash_HyperbolaHasher.pxx
  ODEHash_ParabolaHasher.pxx
  ODEHash_BezierCurveHasher.pxx
  ODEHash_BSplineCurveHasher.pxx
  ODEHash_TrimmedCurveHasher.pxx
  ODEHash_OffsetCurveHasher.pxx
  ODEHash_Curve3dHasher.hxx
  ODEHash_Curve3dHasher.cxx

  # Curve2D Hashers
  ODEHash_Line2dHasher.pxx
  ODEHash_Circle2dHasher.pxx
  ODEHash_Ellipse2dHasher.pxx
  ODEHash_Hyperbola2dHasher.pxx
  ODEHash_Parabola2dHasher.pxx
  ODEHash_BezierCurve2dHasher.pxx
  ODEHash_BSplineCurve2dHasher.pxx
  ODEHash_TrimmedCurve2dHasher.pxx
  ODEHash_OffsetCurve2dHasher.pxx
  ODEHash_Curve2dHasher.hxx
  ODEHash_Curve2dHasher.cxx
)
