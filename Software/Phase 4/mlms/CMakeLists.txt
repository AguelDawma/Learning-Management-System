cmake_minimum_required(VERSION 3.16)
project(mlms VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy / cppcheck

option(MLMS_BUILD_GUI "Build the Qt desktop application" ON)
option(MLMS_BUILD_TESTS "Build the GoogleTest suite" ON)
option(MLMS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(MLMS_COVERAGE "Instrument mlms_core and tests for gcov line coverage" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Warnings are applied per target (not globally) so third-party code such as
# GoogleTest is not built with -Werror.
add_library(mlms_warnings INTERFACE)
if(MSVC)
    target_compile_options(mlms_warnings INTERFACE /W4 $<$<BOOL:${MLMS_WARNINGS_AS_ERRORS}>:/WX>)
else()
    target_compile_options(mlms_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wno-sign-conversion $<$<BOOL:${MLMS_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ARGON2 REQUIRED IMPORTED_TARGET libargon2)

# The schema is embedded in the executable so it can never go missing at run time.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql MLMS_SCHEMA_SQL)
configure_file(src/data/SchemaSql.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/SchemaSql.hpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS sql/schema.sql)

add_subdirectory(src)

if(MLMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
