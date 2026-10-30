# Run with: cmake -DEXE=<path to exe> -DSEARCH_DIR=<compiler bin dir> -P CopyRuntimeDlls.cmake
# Copies every non-system DLL that the executable and its Qt plugins need into
# the executable's folder, so the program runs on a PC without MSYS2 installed.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW) # normalise paths before matching the exclude patterns
endif()

# Script mode (-P) does not know the target platform, so say how to read PE files.
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${SEARCH_DIR}/objdump.exe")

get_filename_component(dest "${EXE}" DIRECTORY)
file(GLOB_RECURSE plugins "${dest}/*/*.dll")

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXE}"
    LIBRARIES ${plugins}
    DIRECTORIES "${dest}" "${SEARCH_DIR}"
    PRE_EXCLUDE_REGEXES "^api-ms-.*" "^ext-ms-.*"
    POST_EXCLUDE_REGEXES "^[A-Za-z]:[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved
    # A DLL already copied by windeployqt is also found in SEARCH_DIR; both are the same file.
    CONFLICTING_DEPENDENCIES_PREFIX conflicts)

set(copied 0)
foreach(dll IN LISTS resolved)
    get_filename_component(name "${dll}" NAME)
    if(NOT EXISTS "${dest}/${name}")
        file(COPY "${dll}" DESTINATION "${dest}")
        math(EXPR copied "${copied} + 1")
    endif()
endforeach()
message(STATUS "Copied ${copied} more runtime DLLs next to ${EXE}")
if(unresolved)
    message(WARNING "Could not find: ${unresolved}")
endif()
