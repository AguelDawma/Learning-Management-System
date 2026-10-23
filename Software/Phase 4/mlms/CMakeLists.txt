cmake_minimum_required(VERSION 3.16)
project(mlms VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MLMS_BUILD_GUI "Build Qt GUI" ON)
option(MLMS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(MLMS_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
endif()

find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ARGON2 REQUIRED libargon2)

if(MLMS_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sql/schema.sql DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sql)

add_subdirectory(src)