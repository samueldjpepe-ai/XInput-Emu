cmake_minimum_required(VERSION 3.15)
project(XInputEmu)

set(CMAKE_CXX_STANDARD 20)

# Forzamos la búsqueda de paquetes instalados por vcpkg
find_package(SDL2 CONFIG REQUIRED)
find_package(ViGEmClient CONFIG REQUIRED)

add_executable(XInputEmu main.cpp)

# Usamos los nombres de objetivos específicos que vcpkg genera
target_link_libraries(XInputEmu PRIVATE SDL2::SDL2 ViGEmClient::ViGEmClient)
