#define SDL_MAIN_HANDLED
#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <string>
#include <SDL.h>
#include "ViGEm/Client.h"

#pragma comment(lib, "setupapi.lib")

// Estructura para guardar el mapeo
struct Mapeo {
    int botones[10]; // A, B, X, Y, LB, RB, Start, Back, L3, R3
};

struct ControllerPair {
    SDL_GameController* physical;
    PVIGEM_TARGET virtualPad;
    std::string guid;
};

std::map<std::string, Mapeo> baseDeDatos;
std::vector<std::string> nombresBotones = {"A", "B", "X", "Y", "LB", "RB", "START", "BACK", "L3", "R3"};

void cargarConfig() {
    std::ifstream archivo("controles_config.dat", std::ios::binary);
    if (!archivo) return;
    std::string guid;
    while (archivo >> guid) {
        archivo.ignore();
        Mapeo m;
        archivo.read((char*)&m, sizeof(Mapeo));
        baseDeDatos[guid] = m;
    }
}

void guardarConfig() {
    std::ofstream archivo("controles_config.dat", std::ios::binary);
    for (auto const& [guid, mapeo] : baseDeDatos) {
        archivo << guid << " " ;
        archivo.write((char*)&mapeo, sizeof(Mapeo));
    }
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) return 1;

    PVIGEM_CLIENT client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cerr << "ERROR: Driver ViGEmBus no encontrado." << std::endl;
        return 1;
    }

    cargarConfig();
    std::vector<ControllerPair> controllers;
    std::cout << "=== XInput Master (Original Fix) ===" << std::endl;
    std::cout << "Esperando mandos..." << std::endl;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                int deviceIdx = event.cdevice.which;
                SDL_GameController* phys = SDL_GameControllerOpen(deviceIdx);
                if (phys) {
                    SDL_Joystick* joy = SDL_GameControllerGetJoystick(phys);
                    char guidStr[33];
                    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guidStr, 33);

                    // Si el mando es nuevo, pedimos botones rápido
                    if (baseDeDatos.find(guidStr) == baseDeDatos.end()) {
                        std::cout << "\n>>> NUEVO MANDO: " << SDL_GameControllerName(phys) << std::endl;
                        Mapeo nuevo;
                        for (int i = 0; i < 10; i++) {
                            std::cout << "Presiona [" << nombresBotones[i] << "]: " << std::flush;
                            bool capturado = false;
                            while (!capturado) {
                                SDL_Event ev;
                                while (SDL_PollEvent(&ev)) {
                                    if (ev.type == SDL_JOYBUTTONDOWN) {
                                        nuevo.botones[i] = ev.jbutton.button;
                                        std::cout << "OK (" << (int)ev.jbutton.button << ")" << std::endl;
                                        capturado = true;
                                        Sleep(300);
                                    }
                                }
                                Sleep(10);
                            }
                        }
                        baseDeDatos[guidStr] = nuevo;
                        guardarConfig();
                    }

                    PVIGEM_TARGET virt = vigem_target_x360_alloc();
                    vigem_target_add(client, virt);
                    controllers.push_back({phys, virt, guidStr});
                    std::cout << ">> Conectado y listo." << std::endl;
                }
            }
        }

        for (auto& pair : controllers) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            Mapeo m = baseDeDatos[pair.guid];

            // Ejes (Tu código original)
            report.sThumbLX = SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_LEFTX);
            report.sThumbLY = -SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_LEFTY);
            report.sThumbRX = SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_RIGHTX);
            report.sThumbRY = -SDL_GameControllerGetAxis(pair.physical, SDL_CONTROLLER_AXIS_RIGHTY);

            // Botones personalizados
            SDL_Joystick* j = SDL_GameControllerGetJoystick(pair.physical);
            if (SDL_JoystickGetButton(j, m.botones[0])) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_JoystickGetButton(j, m.botones[1])) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_JoystickGetButton(j, m.botones[2])) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_JoystickGetButton(j, m.botones[3])) report.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_JoystickGetButton(j, m.botones[4])) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_JoystickGetButton(j, m.botones[5])) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_JoystickGetButton(j, m.botones[6])) report.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_JoystickGetButton(j, m.botones[7])) report.wButtons |= XUSB_GAMEPAD_BACK;
            if (SDL_JoystickGetButton(j, m.botones[8])) report.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
            if (SDL_JoystickGetButton(j, m.botones[9])) report.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;

            vigem_target_x360_update(client, pair.virtualPad, report);
        }
        Sleep(10);
    }

    vigem_disconnect(client);
    SDL_Quit();
    return 0;
}
