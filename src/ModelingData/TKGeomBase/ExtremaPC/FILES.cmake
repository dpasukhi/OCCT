# Auto-generated list of source files for ExtremaPC package
set(OCCT_ExtremaPC_FILES_LOCATION "${CMAKE_CURRENT_LIST_DIR}")

set(OCCT_ExtremaPC_FILES
  # Core types
  ExtremaPC.hxx
  ExtremaPC.cxx
  ExtremaPC_Planar.hxx
  ExtremaPC_Planar.cxx

  # Elementary curves (analytical solutions)
  ExtremaPC_Line.hxx
  ExtremaPC_Line.cxx
  ExtremaPC_Circle.hxx
  ExtremaPC_Circle.cxx
  ExtremaPC_Ellipse.hxx
  ExtremaPC_Ellipse.cxx
  ExtremaPC_Hyperbola.hxx
  ExtremaPC_Hyperbola.cxx
  ExtremaPC_Parabola.hxx
  ExtremaPC_Parabola.cxx

  # Grid-based infrastructure for numerical curves
  ExtremaPC_DistanceFunction.hxx
  ExtremaPC_GridEvaluator.hxx
  ExtremaPC_GridEvaluator.cxx

  # Numerical curve evaluators (grid-based)
  ExtremaPC_BezierCurve.hxx
  ExtremaPC_BezierCurve.cxx
  ExtremaPC_BSplineCurve.hxx
  ExtremaPC_BSplineCurve.cxx
  ExtremaPC_OffsetCurve.hxx
  ExtremaPC_OffsetCurve.cxx
  ExtremaPC_OtherCurve.hxx
  ExtremaPC_OtherCurve.cxx

  # Main aggregator with std::variant dispatch
  ExtremaPC_Curve.hxx
  ExtremaPC_Curve.cxx
)
