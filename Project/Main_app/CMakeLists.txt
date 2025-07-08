cmake_minimum_required(VERSION 3.19)

project(Main_app LANGUAGES CXX)

# Find Qt components
find_package(Qt6 6.9 REQUIRED COMPONENTS Core SerialPort)

# Basic Qt setup
qt_standard_project_setup()

# Include PortAudio headers and libs
include_directories("C:/portaudio/include")
link_directories("C:/portaudio/build")

# Main Application
qt_add_executable(Main_app
    main.cpp
)

target_link_libraries(Main_app
    PRIVATE
        Qt::Core
        Qt::SerialPort
        portaudio      # or use "libportaudio.a" if absolutely needed
        winmm          # for timeGetTime etc.
        setupapi       # for SetupDi* functions
)

# Device Listing Tool
add_executable(ListDevices
    ListDevices.cpp
)

target_link_libraries(ListDevices
    PRIVATE
        portaudio
        winmm
        setupapi
)
