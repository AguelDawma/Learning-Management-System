# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  [[src\CMakeFiles\mlms_autogen.dir\AutogenUsed.txt]]
  [[src\CMakeFiles\mlms_autogen.dir\ParseCache.txt]]
  [[src\mlms_autogen]]
  )
endif()
