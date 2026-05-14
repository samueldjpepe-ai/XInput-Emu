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
    int botones[10];
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

void calibrarMando(SDL_Joystick* joy, std::string guid) {
    Mapeo nuevo;
    std::cout << "\n>>> CALIBRANDO: " << SDL_JoystickName(joy) << std::endl;
    for (int i = 0; i < (int)nombresBotones.size(); i++) {
        std::cout << "Presiona [" << nombresBotones[i] << "]: " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN) {
                    nuevo.botones[i] = ev.jbutton.button;
                    std::cout << "OK (" << (int)ev.jbutton.button << ")" << std::endl;
                    capturado = true;
                    Sleep(350);
                }
            }
            Sleep(10);
        }
    }
    baseDeDatos[guid] = nuevo;
    guardarConfig();
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    // Volvemos a la inicializacion simple de la V2
    SDL_Init(SDL_INIT_JOYSTICK);
    
    PVIGEM_CLIENT client = vigem_alloc();
    vigem_connect(client);

    cargarConfig();
    
    // "Despertador" para mandos Twin USB
    SDL_JoystickUpdate();
    int nJoy = SDL_NumJoysticks();

    if (nJoy == 0) {
        std::cout << "ERROR: No veo mandos. REINTENTANDO..." << std::endl;
        Sleep(1000);
        SDL_JoystickUpdate();
        nJoy = SDL_NumJoysticks();
    }

    if (nJoy == 0) {
        std::cout << "No se detectaron mandos. Presiona ANALOG en el control." << std::endl;
        system("pause");
        return 0;
    }

    std::cout << "=== MANDOS ENCONTRADOS: " << nJoy << " ===" << std::endl;
    std::vector<ControllerPair> activos;
    
    for (int i = 0; i < nJoy; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        char guidStr[33];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), guidStr, 33);
        
        std::cout << "[" << i + 1 << "] " << SDL_JoystickName(j);
        if (baseDeDatos.count(guidStr)) std::cout << " (Listo)";
        else std::cout << " (Nuevo)";
        std::cout << std::endl;

        // Si es nuevo o queremos reconfigurar, lanzamos calibracion
        if (!baseDeDatos.count(guidStr)) {
            calibrarMando(j, guidStr);
        }

        PVIGEM_TARGET vPad = vigem_target_x360_alloc();
        vigem_target_add(client, vPad);
        activos.push_back({j, vPad, guidStr});
    }

    std::cout << "\nEMULACION ACTIVA. Pulsa Ctrl+C para cerrar.\n" << std::endl;

    while (true) {
        SDL_JoystickUpdate();
        for (auto& a : activos) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            Mapeo m = baseDeDatos[a.guid];

            // Botones
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

            // Ejes (Standard)
            report.sThumbLX = SDL_JoystickGetAxis(a.physical, 0);
            report.sThumbLY = -SDL_JoystickGetAxis(a.physical, 1);
            report.sThumbRX = SDL_JoystickGetAxis(a.physical, 2);
            report.sThumbRY = -SDL_JoystickGetAxis(a.physical, 3);

            // Gatillos (L2/R2 suelen ser botones 6 y 7)
            if (SDL_JoystickGetButton(a.physical, 6)) report.bLeftTrigger = 255;
            if (SDL_JoystickGetButton(a.physical, 7)) report.bRightTrigger = 255;

            vigem_target_x360_update(client, a.virtualPad, report);
        }
        Sleep(10);
    }
    return 0;
}
