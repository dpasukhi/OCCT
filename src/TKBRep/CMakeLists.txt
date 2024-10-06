project(TKBRep)

OCCT_INCLUDE_CMAKE_FILE (adm/cmake/occt_toolkit)

# Add the precompiled header
add_precompiled_header(TKernel "${CMAKE_CURRENT_SOURCE_DIR}/TKBRep_pch.hxx")
