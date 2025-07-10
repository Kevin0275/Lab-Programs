cmake_minimum_required(VERSION 3.19)

project(Main_app LANGUAGES CXX)

# Find Qt components
find_package(Qt6 6.9 REQUIRED COMPONENTS Core Widgets SerialPort)

# Basic Qt setup
qt_standard_project_setup()

# Include PortAudio headers and libs
include_directories("C:/portaudio/include")
link_directories("C:/portaudio/build")

# Add your sources including UI and QCustomPlot
qt_add_executable(Main_app
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    qcustomplot.cpp
)

# Link Qt and system libraries
target_link_libraries(Main_app
    PRIVATE
        Qt::Core
        Qt::Widgets       # Required for MainWindow and UI
        Qt::SerialPort
        portaudio
        winmm
        setupapi
)

# Optional: Add device listing tool (if still needed)
add_executable(ListDevices
    ListDevices.cpp
)

target_link_libraries(ListDevices
    PRIVATE
        portaudio
        winmm
        setupapi
)
