cmake_minimum_required(VERSION 3.19)

project(Main_app LANGUAGES CXX)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core SerialPort)

qt_standard_project_setup()

qt_add_executable(Main_app
    main.cpp
)

target_link_libraries(Main_app
    PRIVATE
        Qt::Core
        Qt::SerialPort
)
