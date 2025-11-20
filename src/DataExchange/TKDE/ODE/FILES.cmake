# Source files for ODE package
set(OCCT_ODE_FILES_LOCATION "${CMAKE_CURRENT_LIST_DIR}")

set(OCCT_ODE_FILES
  # Status enumeration
  ODE_Status.hxx

  # Base classes
  ODE_Object.hxx
  ODE_Object.cxx
  ODE_ObjectRef.hxx
  ODE_ObjectRef.cxx

  # Archive and Manifest
  ODE_Archive.hxx
  ODE_Archive.cxx
  ODE_Manifest.hxx
  ODE_Manifest.cxx

  # File classes
  ODE_SurfaceFile.hxx
  ODE_SurfaceFile.cxx
  ODE_Curve3dFile.hxx
  ODE_Curve3dFile.cxx
  ODE_Curve2dFile.hxx
  ODE_Curve2dFile.cxx
  ODE_TopologyFile.hxx
  ODE_TopologyFile.cxx
  ODE_TriangulationFile.hxx
  ODE_TriangulationFile.cxx
  ODE_PolygonFile.hxx
  ODE_PolygonFile.cxx
#
  ## Writer and Reader
  #ODE_Writer.hxx
  #ODE_Writer.cxx
  #ODE_Reader.hxx
  #ODE_Reader.cxx
#
  ## Provider
  #ODE_Provider.hxx
  #ODE_Provider.cxx
)
