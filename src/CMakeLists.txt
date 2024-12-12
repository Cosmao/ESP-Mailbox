# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

idf_component_register(SRCS ${app_sources})
