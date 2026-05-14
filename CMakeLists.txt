cmake_minimum_required(VERSION 3.15)
project(XInputEmu)

set(CMAKE_CXX_STANDARD 20)

# Encontrar SDL2
find_package(SDL2 CONFIG REQUIRED)

# Crear ejecutable
add_executable(XInputEmu main.cpp)

# Enlazar librerías
target_link_libraries(XInputEmu PRIVATE SDL2::SDL2 setupapi.lib winmm.lib)
