#include <windows.h>
#include <iostream>
#include <SDL.h>
#include "ViGEm/Client.h"

// Enlace manual para evitar errores de configuración
#pragma comment(lib, "setupapi.lib")

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Error SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    auto client = vigem_alloc();
    if (client == nullptr) {
        std::cerr << "No se pudo asignar el cliente ViGEm." << std::endl;
        return 1;
    }

    const auto retval = vigem_connect(client);
    if (!VIGEM_SUCCESS(retval)) {
        std::cerr << "Error de conexión ViGEm: " << retval << std::endl;
        return 1;
    }

    std::cout << "Emulador iniciado correctamente. Presiona Ctrl+C para salir." << std::endl;

    // Bucle simple de espera
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        Sleep(10);
    }

    vigem_disconnect(client);
    vigem_free(client);
    SDL_Quit();
    return 0;
}
