cmake_minimum_required(VERSION 3.19)
project(Without_Qt)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/imgui/backends  # <--- ADD THIS LINE!
    ${CMAKE_SOURCE_DIR}/implot
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/portaudio/include
)


link_directories(
    ${CMAKE_SOURCE_DIR}/portaudio/install/lib
)

add_executable(Without_Qt
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
    implot/implot.cpp
    implot/implot_items.cpp
)

target_link_libraries(Without_Qt
    portaudio
    winmm
    setupapi
    d3d11        # If you use DX11 backend for ImGui (recommended for Windows)
    dxgi
    d3dcompiler
    dwmapi
)
