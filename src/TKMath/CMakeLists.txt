project(TKMath)

OCCT_INCLUDE_CMAKE_FILE (adm/cmake/occt_toolkit)

# Add the precompiled header
add_precompiled_header(TKMath "${CMAKE_CURRENT_SOURCE_DIR}/TKMath_pch.hxx")
