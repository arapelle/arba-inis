add_cpp_library_tests(SHARED ${PROJECT_NAME}
                      STATIC ${PROJECT_NAME}-static
                      SOURCES
                          inis_tests.cpp
                      )

file(CREATE_LINK "${CMAKE_CURRENT_LIST_DIR}/inis" "${CMAKE_CURRENT_BINARY_DIR}/inis" COPY_ON_ERROR SYMBOLIC)
