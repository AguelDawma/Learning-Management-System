# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  [[src\CMakeFiles\mlms_autogen.dir\AutogenUsed.txt]]
  [[src\CMakeFiles\mlms_autogen.dir\ParseCache.txt]]
  [[src\CMakeFiles\mlms_ui_autogen.dir\AutogenUsed.txt]]
  [[src\CMakeFiles\mlms_ui_autogen.dir\ParseCache.txt]]
  [[src\mlms_autogen]]
  [[src\mlms_ui_autogen]]
  )
endif()
