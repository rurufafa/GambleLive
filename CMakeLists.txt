cmake_minimum_required(VERSION 3.21.1)
project(GambleLive VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core5Compat)

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/ui
)

qt_add_executable(GambleLive WIN32
    main.cpp
    src/core/config_manager.cpp 
    src/core/config_manager.h 
    src/core/log_parser.cpp 
    src/core/log_parser.h 
    src/core/log_watcher.cpp 
    src/core/log_watcher.h 
    src/core/slot_tab_controller.cpp
    src/core/slot_tab_controller.h
    src/window/infowidget.cpp 
    src/window/infowidget.h
    src/window/mainwindow.cpp
    src/window/mainwindow.h
    ui/infowidget.ui 
    ui/mainwindow.ui
)

target_include_directories(GambleLive PRIVATE
    ${CMAKE_SOURCE_DIR}/src/window
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(GambleLive PRIVATE Qt6::Widgets Qt6::Core5Compat)