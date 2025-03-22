# Google Test integration for OCCT toolkits

# Custom macro to add a Google Test project for a toolkit
macro(OCCT_ADD_GTEST_PROJECT TEST_PROJECT_NAME TOOLKIT_NAME)
  if (NOT GOOGLETEST_FOUND)
    message(STATUS "Google Test not available. Skipping test project ${TEST_PROJECT_NAME}")
    return()
  endif()

  # Extract test source files from FILES.cmake
  set(FILES_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/FILES.cmake")
  if(EXISTS "${FILES_CMAKE_PATH}")
    include("${FILES_CMAKE_PATH}")
    set(TEST_SOURCE_FILES "${OCCT_${TOOLKIT_NAME}_GTests_FILES}")
  endif()

  # Skip if no test source files defined
  if ("${TEST_SOURCE_FILES}" STREQUAL "")
    message(STATUS "No test files found for ${TEST_PROJECT_NAME}, skipping...")
    return()
  endif()

  # Get module name to organize tests in solution explorer
  get_target_property(TOOLKIT_MODULE ${TOOLKIT_NAME} MODULE)
  if(NOT TOOLKIT_MODULE)
    set(TOOLKIT_MODULE "Unknown")
  endif()

  # Disable testing for DRAW plugin
  if ("${TOOLKIT_MODULE}" STREQUAL "Draw")
    return()
  endif()

  # Create the test executable
  add_executable(${TEST_PROJECT_NAME} ${TEST_SOURCE_FILES})

  # Set the folder property for solution explorer organization by module
  set_target_properties(${TEST_PROJECT_NAME} PROPERTIES 
    FOLDER "Tests/${TOOLKIT_MODULE}" 
    MODULE "${TOOLKIT_MODULE}"
  )

  # Add toolkit-specific definition to identify tests belonging to this toolkit
  target_compile_definitions(${TEST_PROJECT_NAME} PRIVATE 
    OCCT_TEST_TOOLKIT=${TOOLKIT_NAME}
    OCCT_TEST_TOOLKIT_${TOOLKIT_NAME}
  )

  # Link with Google Test
  if(TARGET gtest AND TARGET gtest_main)
    # Use targets from FetchContent
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE gtest gtest_main)
  elseif(TARGET GTest::gtest AND TARGET GTest::gtest_main)
    # Use targets from find_package with imported targets
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)
  else()
    # Fall back to direct library paths
    target_include_directories(${TEST_PROJECT_NAME} PRIVATE ${GTEST_INCLUDE_DIRS})
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE ${GTEST_BOTH_LIBRARIES})
  endif()
  
  # Link with the toolkit being tested
  target_link_libraries(${TEST_PROJECT_NAME} PRIVATE ${TOOLKIT_NAME})
  
  # Link with all dependencies of the toolkit
  get_target_property(TOOLKIT_DEPS ${TOOLKIT_NAME} LINK_LIBRARIES)
  if(TOOLKIT_DEPS)
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE ${TOOLKIT_DEPS})
  endif()
  
  # Add pthreads if necessary (for Linux)
  if (UNIX AND NOT APPLE)
    target_link_libraries(${TEST_PROJECT_NAME} PRIVATE pthread)
  endif()

  # Collect absolute paths of test source files
  set(TEST_SOURCE_FILES_ABS "")
  foreach (TEST_SOURCE_FILE ${TEST_SOURCE_FILES})
    get_filename_component(TEST_SOURCE_FILE_ABS "${TEST_SOURCE_FILE}" ABSOLUTE)
    list(APPEND TEST_SOURCE_FILES_ABS "${TEST_SOURCE_FILE_ABS}")
  endforeach()
  # Register tests with CTest using test discovery using the actual source files
  gtest_add_tests (
    TARGET ${TEST_PROJECT_NAME}
    TEST_PREFIX "${TOOLKIT_MODULE}::${TOOLKIT_NAME}::"
    SOURCES ${TEST_SOURCE_FILES_ABS}
    TEST_LIST ${TOOLKIT_NAME}_Tests
    TEST_SOURCES ${TEST_SOURCE_FILES_ABS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTIES 
      LABELS ${TOOLKIT_NAME}
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      VS_DEBUGGER_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${TEST_PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
  )
endmacro()
