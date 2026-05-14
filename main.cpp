#define SDL_MAIN_HANDLED
#include <windows.h>
#include <iostream>
#include <SDL.h>
#include "ViGEm/Client.h"

#pragma comment(lib, "setupapi.lib")

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) return 1;

    auto client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cerr << "Error: Instala ViGEmBus" << std::endl;
        return 1;
    }

    auto pad = vigem_target_x360_alloc();
    vigem_target_add(client, pad);

    std::cout << "EMULADOR ACTIVO: Twin USB -> Xbox 360" << std::endl;

    SDL_GameController* controller = nullptr;
    XUSB_REPORT report;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                if (!controller) {
                    controller = SDL_GameControllerOpen(event.cdevice.which);
                    std::cout << "Mando conectado: " << SDL_GameControllerName(controller) << std::endl;
                }
            }
        }

        if (controller) {
            XUSB_REPORT_INIT(&report);

            // Sticks (Palancas)
            report.sThumbLX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            report.sThumbLY = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            report.sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
            report.sThumbRY = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

            // Botones principales
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B)) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X)) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y)) report.wButtons |= XUSB_GAMEPAD_Y;
            
            // Gatillos y Flechas (DPAD)
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP)) report.wButtons |= XUSB_GAMEPAD_DPAD_UP;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;

            vigem_target_x360_update(client, pad, report);
        }
        Sleep(10);
    }

    vigem_target_remove(client, pad);
    vigem_target_free(pad);
    vigem_disconnect(client);
    SDL_Quit();
    return 0;
}
