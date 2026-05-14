cmake_minimum_required(VERSION 3.10)
project(XInputEmu)

set(CMAKE_CXX_STANDARD 20)

# Buscar paquetes en el entorno de GitHub
find_package(SDL2 REQUIRED)
find_package(ViGEmClient REQUIRED)

add_executable(XInputEmu main.cpp)

target_link_libraries(XInputEmu SDL2::SDL2 ViGEmClient::ViGEmClient)
