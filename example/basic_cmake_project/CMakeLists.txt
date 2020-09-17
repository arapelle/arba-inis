cmake_minimum_required(VERSION 3.18)

project(basic_cmake_project)

add_executable(${PROJECT_NAME} main.cpp)
# Find package inis:
find_package(inis 0.1.0 CONFIG REQUIRED)
# Use inis release shared target:
target_link_libraries(${PROJECT_NAME} PRIVATE arba::inis)
# Use inis release static target:
#target_link_libraries(${PROJECT_NAME} PRIVATE arba::inis-static)

