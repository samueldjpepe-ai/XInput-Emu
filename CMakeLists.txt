cmake_minimum_required(VERSION 3.15)
project(XInputEmu)

set(CMAKE_CXX_STANDARD 20)

# 1. Encontrar SDL2 (esto ya funciona bien)
find_package(SDL2 CONFIG REQUIRED)

# 2. Configurar ViGEmClient desde el código fuente que descargamos
set(VIGEM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ViGEmClient")
include_directories("${VIGEM_SRC_DIR}/include")

# Añadimos los archivos necesarios de ViGEm directamente al proyecto
add_executable(XInputEmu 
    main.cpp 
    "${VIGEM_SRC_DIR}/src/ViGEmClient.cpp"
)

# 3. Enlazar librerías y dependencias de Windows necesarias para ViGEm
target_link_libraries(XInputEmu PRIVATE SDL2::SDL2 setupapi.lib)
