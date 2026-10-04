cmake_minimum_required(VERSION 3.20)
project(ChronoGlass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ChronoGlass WIN32 main.cpp app.rc)
target_link_libraries(ChronoGlass PRIVATE user32 gdi32 shell32 ole32)
target_compile_definitions(ChronoGlass PRIVATE UNICODE _UNICODE NOMINMAX)
