add_library(mlms_domain INTERFACE)
add_library(mlms_data INTERFACE)
add_library(mlms_app INTERFACE)

add_executable(mlms 
    main.cpp
    domain/User.cpp
)

target_link_libraries(mlms PRIVATE
    SQLite::SQLite3
    ${ARGON2_LIBRARIES}
)
target_include_directories(mlms PRIVATE ${ARGON2_INCLUDE_DIRS})

if(MLMS_BUILD_GUI)
    target_link_libraries(mlms PRIVATE Qt6::Widgets)
endif()