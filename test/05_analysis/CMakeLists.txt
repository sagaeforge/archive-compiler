# Analysis Module Tests
project(compiler_05_analysis_tests)

# Collect all existing test files
set(ANALYSIS_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisConfigTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnalysisManagerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/errors/AnalysisErrorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/errors/ErrorReporterTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/semantic/SymbolTableTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/control_flow/ControlFlowGraphTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dataflow/LiveVariablesTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dataflow/DataFlowAnalysisManagerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimization/AlgebraicSimplifierTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimization/FunctionInliningTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimization/DeadCodeEliminatorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimization/ConstantFolderTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/optimization/OptimizationPassTest.cpp
)

# Create test executable
add_executable(${PROJECT_NAME} ${ANALYSIS_TEST_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    compiler_lib
    GTest::gtest_main
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
)

# Set compile features
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Test discovery
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} 
    TEST_PREFIX "05_analysis."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)