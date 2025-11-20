project(TKDE)

OCCT_INCLUDE_CMAKE_FILE (adm/cmake/occt_toolkit)

# Link Cap'n Proto for ODE format
if (TARGET CapnProto::capnp)
  target_link_libraries(TKDE PRIVATE CapnProto::capnp CapnProto::kj)

  # Compile ODE Cap'n Proto schemas
  set(ODE_SCHEMA_DIR "${CMAKE_SOURCE_DIR}/resources/ODE")
  set(ODE_SCHEMA_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/ODE")
  file(MAKE_DIRECTORY ${ODE_SCHEMA_OUTPUT_DIR})

  set(ODE_SCHEMA_FILES
    ${ODE_SCHEMA_DIR}/ode.capnp
    ${ODE_SCHEMA_DIR}/surfaces.capnp
    ${ODE_SCHEMA_DIR}/curves3d.capnp
    ${ODE_SCHEMA_DIR}/curves2d.capnp
    ${ODE_SCHEMA_DIR}/topology.capnp
    ${ODE_SCHEMA_DIR}/triangulation.capnp
    ${ODE_SCHEMA_DIR}/polygons.capnp
  )

  foreach(SCHEMA ${ODE_SCHEMA_FILES})
    get_filename_component(SCHEMA_NAME ${SCHEMA} NAME_WE)
    set(SCHEMA_OUTPUT_H "${ODE_SCHEMA_OUTPUT_DIR}/${SCHEMA_NAME}.capnp.h")
    set(SCHEMA_OUTPUT_CPP "${ODE_SCHEMA_OUTPUT_DIR}/${SCHEMA_NAME}.capnp.c++")

    add_custom_command(
      OUTPUT ${SCHEMA_OUTPUT_H} ${SCHEMA_OUTPUT_CPP}
      COMMAND ${CAPNP_EXECUTABLE} compile
              -o${CAPNPC_CXX_EXECUTABLE}
              --src-prefix=${ODE_SCHEMA_DIR}
              ${SCHEMA}
      DEPENDS ${SCHEMA}
      WORKING_DIRECTORY ${ODE_SCHEMA_DIR}
      COMMENT "Compiling ODE schema: ${SCHEMA_NAME}.capnp"
      VERBATIM
    )

    list(APPEND ODE_SCHEMA_OUTPUTS ${SCHEMA_OUTPUT_H} ${SCHEMA_OUTPUT_CPP})
  endforeach()

  # Create custom target for schema compilation
  add_custom_target(ODESchemas ALL DEPENDS ${ODE_SCHEMA_OUTPUTS})
  add_dependencies(TKDE ODESchemas)

  # Add generated headers to TKDE include path
  target_include_directories(TKDE PRIVATE ${ODE_SCHEMA_OUTPUT_DIR})

  message(STATUS "ODE schemas will be generated in: ${ODE_SCHEMA_OUTPUT_DIR}")
endif()
