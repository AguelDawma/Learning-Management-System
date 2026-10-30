# Domain, data access, infrastructure and application services. No Qt here,
# so the business logic is unit tested without a GUI.
add_library(mlms_core STATIC
    domain/Assignment.cpp
    domain/DateTime.cpp
    domain/LatePenaltyPolicy.cpp
    domain/Mark.cpp
    domain/Submission.cpp
    domain/User.cpp
    domain/UserFactory.cpp
    domain/Validation.cpp
    data/Database.cpp
    data/FileStore.cpp
    data/SqliteRepositories.cpp
    infrastructure/Infrastructure.cpp
    infrastructure/Sha256.cpp
    services/AdminService.cpp
    services/AssignmentService.cpp
    services/AuditService.cpp
    services/AuthService.cpp
    services/Bootstrap.cpp
    services/MarkEvents.cpp
    services/MarkingService.cpp
    services/MarkingSheet.cpp
    services/ModerationService.cpp
    services/SubmissionService.cpp
)
target_include_directories(mlms_core
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${PROJECT_BINARY_DIR}/generated)
# CMake 4.x renamed the imported target; older versions (e.g. on CI) only have the old name.
if(TARGET SQLite3::SQLite3)
    set(MLMS_SQLITE_TARGET SQLite3::SQLite3)
else()
    set(MLMS_SQLITE_TARGET SQLite::SQLite3)
endif()
target_link_libraries(mlms_core
    PUBLIC ${MLMS_SQLITE_TARGET} PkgConfig::ARGON2
    PRIVATE mlms_warnings)
if(MLMS_COVERAGE)
    target_compile_options(mlms_core PUBLIC --coverage -O0)
    target_link_options(mlms_core PUBLIC --coverage)
endif()

if(MLMS_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    qt_standard_project_setup()

    # Presentation layer, a library so the GUI smoke tests can link it too.
    add_library(mlms_ui STATIC
        ui/AdminView.cpp
        ui/Dialogs.cpp
        ui/HoDView.cpp
        ui/LecturerView.cpp
        ui/LoginWindow.cpp
        ui/MainWindow.cpp
        ui/StudentView.cpp
        ui/UiHelpers.cpp
    )
    target_link_libraries(mlms_ui PUBLIC mlms_core Qt6::Widgets PRIVATE mlms_warnings)

    add_executable(mlms WIN32 main.cpp)
    target_link_libraries(mlms PRIVATE mlms_ui mlms_warnings)
    target_compile_definitions(mlms PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

    # Windows: put Qt, its plugins and every other required DLL next to mlms.exe
    # so it starts from any Command Prompt or by double-clicking (no MSYS2 on PATH needed).
    if(WIN32)
        find_program(WINDEPLOYQT NAMES windeployqt6 windeployqt HINTS "${Qt6_DIR}/../../../bin")
        get_filename_component(MLMS_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
        if(WINDEPLOYQT)
            add_custom_command(TARGET mlms POST_BUILD
                COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                        "$<TARGET_FILE:mlms>"
                COMMAND "${CMAKE_COMMAND}" "-DEXE=$<TARGET_FILE:mlms>" "-DSEARCH_DIR=${MLMS_COMPILER_BIN}"
                        -P "${PROJECT_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake"
                COMMENT "Copying Qt and runtime DLLs next to mlms.exe"
                VERBATIM)
        endif()
    endif()
endif()
