cmake_minimum_required(VERSION 3.26)

project(basic_cmake_project)

add_executable(${PROJECT_NAME} main.cpp)
add_executable(${PROJECT_NAME}-static main.cpp)
# Find package arba-inis:
find_package(arba-inis 0.3.0 CONFIG REQUIRED)
# Use arba-inis release shared target:
target_link_libraries(${PROJECT_NAME} PRIVATE arba-inis)
# Use arba-inis release static target:
target_link_libraries(${PROJECT_NAME}-static PRIVATE arba-inis-static)
