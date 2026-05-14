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

struct Mapeo {
    int botones[10]; // A, B, X, Y, LB, RB, Start, Back, L3, R3
};

struct ControllerPair {
    SDL_Joystick* physical;
    PVIGEM_TARGET virtualPad;
    std::string guid;
};

std::map<std::string, Mapeo> baseDeDatos;
std::vector<std::string> nombresBotones = {"A", "B", "X", "Y", "LB", "RB", "START", "BACK", "L3", "R3"};

void guardarConfig() {
    std::ofstream archivo("controles_config.dat", std::ios::binary);
    for (auto const& [guid, mapeo] : baseDeDatos) {
        archivo << guid << " ";
        archivo.write((char*)&mapeo, sizeof(Mapeo));
    }
    archivo.close();
}

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
    archivo.close();
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) return 1;
    
    PVIGEM_CLIENT client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cout << "Error: No se encontro ViGEmBus instalado." << std::endl;
        system("pause");
        return 1;
    }

    cargarConfig();
    SDL_JoystickUpdate();
    
    int nJoy = SDL_NumJoysticks();
    if (nJoy == 0) {
        std::cout << "No hay mandos. Revisa el cable y el boton ANALOG." << std::endl;
        system("pause");
        return 0;
    }

    std::vector<ControllerPair> activos;
    std::cout << "=== MANDOS DETECTADOS: " << nJoy << " ===" << std::endl;

    for (int i = 0; i < nJoy; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        char guidStr[33];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), guidStr, 33);
        
        // Si el mando es nuevo, pedimos botones de una vez
        if (baseDeDatos.find(guidStr) == baseDeDatos.end()) {
            std::cout << "\n>>> CONFIGURANDO MANDO NUEVO: " << SDL_JoystickName(j) << std::endl;
            Mapeo nuevo;
            for (int b = 0; b < 10; b++) {
                std::cout << "Presiona [" << nombresBotones[b] << "]: " << std::flush;
                bool capturado = false;
                while (!capturado) {
                    SDL_Event ev;
                    while (SDL_PollEvent(&ev)) {
                        if (ev.type == SDL_JOYBUTTONDOWN && ev.jbutton.which == SDL_JoystickInstanceID(j)) {
                            nuevo.botones[b] = (int)ev.jbutton.button;
                            std::cout << "OK (" << (int)ev.jbutton.button << ")" << std::endl;
                            capturado = true;
                            Sleep(350);
                        }
                    }
                    Sleep(1);
                }
            }
            baseDeDatos[guidStr] = nuevo;
            guardarConfig();
        }

        PVIGEM_TARGET vPad = vigem_target_x360_alloc();
        vigem_target_add(client, vPad);
        activos.push_back({j, vPad, guidStr});
        std::cout << "[LISTO] Mando " << i + 1 << " emulando como Xbox." << std::endl;
    }

    std::cout << "\nEJECUTANDO... No cierres esta ventana.\n" << std::endl;

    while (true) {
        SDL_JoystickUpdate();
        for (auto& a : activos) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            Mapeo m = baseDeDatos[a.guid];

            // Mapeo de botones grabados
            if (SDL_JoystickGetButton(a.physical, m.botones[0])) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_JoystickGetButton(a.physical, m.botones[1])) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_JoystickGetButton(a.physical, m.botones[2])) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_JoystickGetButton(a.physical, m.botones[3])) report.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_JoystickGetButton(a.physical, m.botones[4])) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_JoystickGetButton(a.physical, m.botones[5])) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_JoystickGetButton(a.physical, m.botones[6])) report.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_JoystickGetButton(a.physical, m.botones[7])) report.wButtons |= XUSB_GAMEPAD_BACK;
            if (SDL_JoystickGetButton(a.physical, m.botones[8])) report.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
            if (SDL_JoystickGetButton(a.physical, m.botones[9])) report.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;

            // Ejes (Standard Twin USB)
            report.sThumbLX = SDL_JoystickGetAxis(a.physical, 0);
            report.sThumbLY = -SDL_JoystickGetAxis(a.physical, 1);
            report.sThumbRX = SDL_JoystickGetAxis(a.physical, 2);
            report.sThumbRY = -SDL_JoystickGetAxis(a.physical, 3);

            // Gatillos L2/R2 como botones (Por si el juego los pide)
            if (SDL_JoystickGetButton(a.physical, 6)) report.bLeftTrigger = 255;
            if (SDL_JoystickGetButton(a.physical, 7)) report.bRightTrigger = 255;

            vigem_target_x360_update(client, a.virtualPad, report);
        }
        Sleep(10);
    }
    return 0;
}
