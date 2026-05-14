#define SDL_MAIN_HANDLED
#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <SDL.h>
#include "ViGEm/Client.h"

#pragma comment(lib, "setupapi.lib")

// Estructura para manejar cada mando conectado
struct ControllerPair {
    SDL_GameController* physical;
    PVIGEM_TARGET virtualPad;
};

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) return 1;

    PVIGEM_CLIENT client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cerr << "ERROR: Driver ViGEmBus no encontrado." << std::endl;
        return 1;
    }

    std::vector<ControllerPair> controllers;
    std::cout << "=== XInput Master Emulator (Win 7/10/11) ===" << std::endl;
    std::cout << "Esperando mandos... (Presiona Ctrl+C para salir)" << std::endl;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            // Detectar conexión de cualquier mando genérico
            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                int deviceIdx = event.cdevice.which;
                SDL_GameController* phys = SDL_GameControllerOpen(deviceIdx);
                if (phys) {
                    PVIGEM_TARGET virt = vigem_target_x360_alloc();
                    vigem_target_add(client, virt);
                    controllers.push_back({phys, virt});
                    std::cout << ">> Conectado: " << SDL_GameControllerName(phys) << " [Mando Virtual OK]" << std::endl;
                }
            }
        }

        // Procesar datos para todos los mandos conectados
        for (auto& pair : controllers) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);

            // EJES (Joysticks)
            report.sThumbLX = SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_LEFTX);
            report.sThumbLY = -SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_LEFTY);
            report.sThumbRX = SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_RIGHTX);
            report.sThumbRY = -SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_RIGHTY);

            // BOTONES (Configuración estándar, tú los cambias en el archivo de mapeo de SDL si es necesario)
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_A)) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_B)) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_X)) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_Y)) report.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_START)) report.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_GameControllerGetButton(pair.physical, SDL_CONTROLLER_BUTTON_BACK)) report.wButtons |= XUSB_GAMEPAD_BACK;
            
            vigem_target_x360_update(client, pair.virtualPad, report);
        }
        Sleep(5); // Alta respuesta, bajo consumo
    }

    // Limpieza al cerrar
    for (auto& pair : controllers) {
        vigem_target_remove(client, pair.virtualPad);
        vigem_target_free(pair.virtualPad);
        SDL_GameControllerClose(pair.physical);
    }
    vigem_disconnect(client);
    vigem_free(client);
    SDL_Quit();
    return 0;
}
