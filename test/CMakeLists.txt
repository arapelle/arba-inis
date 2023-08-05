find_or_fetch_GTest()

add_cpp_library_basic_tests(GTest::gtest_main
    SHARED ${PROJECT_NAME}
    STATIC ${PROJECT_NAME}-static
    SOURCES
        project_version_tests.cpp
)

add_cpp_library_test(${PROJECT_NAME}-inis_tests GTest::gtest_main
    SHARED ${PROJECT_NAME}
    STATIC ${PROJECT_NAME}-static
    SOURCES
        inis_tests.cpp
)

target_compile_definitions(${PROJECT_NAME}-inis_tests PUBLIC RSCDIR="${CMAKE_CURRENT_SOURCE_DIR}")
