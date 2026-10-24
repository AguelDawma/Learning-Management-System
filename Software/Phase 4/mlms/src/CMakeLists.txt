add_library(mlms_domain INTERFACE)
add_library(mlms_data INTERFACE)
add_library(mlms_app INTERFACE)

add_executable(mlms 
    main.cpp
    domain/User.cpp
    ui/LoginWindow.cpp
    ui/DashboardWindow.cpp
    ui/AssignmentDialog.cpp
    ui/AssignmentDialog.hpp
)

target_link_libraries(mlms PRIVATE
    SQLite::SQLite3
    ${ARGON2_LIBRARIES}
)
target_include_directories(mlms PRIVATE ${ARGON2_INCLUDE_DIRS})

if(MLMS_BUILD_GUI)
    target_link_libraries(mlms PRIVATE SQLite::SQLite3 argon2 Qt6::Widgets Qt6::Gui Qt6::Core)
endif()