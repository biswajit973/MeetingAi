cmake_minimum_required(VERSION 3.10)
project(SimpleGoodMorning)

set(CMAKE_CXX_STANDARD 17)

# Add source files
set(SOURCES
    simple_good_morning.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)

add_executable(SimpleGoodMorning ${SOURCES})
set_target_properties(SimpleGoodMorning PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(SimpleGoodMorning PRIVATE imgui imgui/backends ${CMAKE_SOURCE_DIR}/nlohmann)
target_link_libraries(SimpleGoodMorning d3d11 dwmapi)
