find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(mlms_tests
    unit/DateTimeTest.cpp
    unit/DomainTest.cpp
    unit/PenaltyPolicyTest.cpp
    unit/SubmissionStateTest.cpp
    unit/InfrastructureTest.cpp
    integration/DatabaseTest.cpp
    integration/AuthServiceTest.cpp
    integration/AdminServiceTest.cpp
    integration/SubmissionServiceTest.cpp
    integration/MarkingWorkflowTest.cpp
)
target_include_directories(mlms_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mlms_tests PRIVATE mlms_core mlms_warnings GTest::gtest_main)
set(MLMS_TEST_TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp")
target_compile_definitions(mlms_tests PRIVATE MLMS_TEST_TMP_DIR="${MLMS_TEST_TMP_DIR}")

include(GoogleTest)
gtest_discover_tests(mlms_tests DISCOVERY_TIMEOUT 60)

if(MLMS_BUILD_GUI)
    # Opens every role's window on the "offscreen" Qt platform (no display needed, works in CI).
    add_executable(mlms_gui_tests gui/GuiSmokeTest.cpp)
    target_include_directories(mlms_gui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(mlms_gui_tests PRIVATE mlms_ui mlms_warnings GTest::gtest)
    target_compile_definitions(mlms_gui_tests PRIVATE MLMS_TEST_TMP_DIR="${MLMS_TEST_TMP_DIR}")
    add_test(NAME gui_smoke COMMAND mlms_gui_tests)
    set_tests_properties(gui_smoke PROPERTIES TIMEOUT 60 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
