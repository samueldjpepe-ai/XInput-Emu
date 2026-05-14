cmake_minimum_required(VERSION 3.15)
project(XInputEmu)

set(CMAKE_CXX_STANDARD 20)

# Encontrar SDL2 vía vcpkg
find_package(SDL2 CONFIG REQUIRED)

# Configurar ViGEmClient manualmente
include_directories(${VIGEM_INCLUDE_DIR})

add_executable(XInputEmu main.cpp)

# Enlazar librerías
target_link_libraries(XInputEmu PRIVATE SDL2::SDL2)
