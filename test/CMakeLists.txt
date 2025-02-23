include(cmtk/CppLibraryTests)
include(GoogleTest)

find_package(GTest 1.14 CONFIG REQUIRED)

add_cpp_library_basic_tests(${PROJECT_TARGET_NAME} GTest::gtest_main
    SOURCES
        project_version_tests.cpp
)

add_cpp_library_test(${PROJECT_TARGET_NAME}-inis_tests ${PROJECT_TARGET_NAME} GTest::gtest_main
    SOURCES
        inis_tests.cpp
)

target_compile_definitions(${PROJECT_TARGET_NAME}-inis_tests PUBLIC RSCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
