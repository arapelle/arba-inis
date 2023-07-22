find_or_fetch_GTest()

add_cpp_library_basic_tests(GTest::gtest_main
    SHARED ${PROJECT_NAME}
    STATIC ${PROJECT_NAME}-static
    SOURCES
        project_version_tests.cpp
)

add_cpp_library_basic_tests(GTest::gtest
    SHARED ${PROJECT_NAME}
    STATIC ${PROJECT_NAME}-static
    SOURCES
        inis_tests.cpp
)

file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/inis" "${CMAKE_CURRENT_BINARY_DIR}/inis" COPY_ON_ERROR SYMBOLIC)
