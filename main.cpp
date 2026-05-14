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

// Estructura para guardar el mapeo de botones
struct Mapeo {
    int botones[10]; // A, B, X, Y, LB, RB, Start, Back, L3, R3
};

// Estructura para manejar los mandos activos
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
    std::cout << "\n>>> MODO CALIBRACION: " << SDL_JoystickName(joy) << std::endl;
    std::cout << "Presiona los botones fisicos cuando se te pida." << std::endl;

    for (int i = 0; i < (int)nombresBotones.size(); i++) {
        std::cout << "Presiona [" << nombresBotones[i] << "]: " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN && ev.jbutton.which == SDL_JoystickInstanceID(joy)) {
                    nuevo.botones[i] = ev.jbutton.button;
                    std::cout << "OK (" << (int)ev.jbutton.button << ")" << std::endl;
                    capturado = true;
                    Sleep(400); // Evitar rebote
                }
            }
            Sleep(10);
        }
    }
    baseDeDatos[guid] = nuevo;
    guardarConfig();
    std::cout << "Configuracion guardada para este mando.\n" << std::endl;
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Error SDL: " << SDL_GetError() << std::endl;
        system("pause");
        return 1;
    }

    PVIGEM_CLIENT client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cerr << "ERROR: No se encuentra el driver ViGEmBus. Por favor instalalo." << std::endl;
        system("pause");
        return 1;
    }

    cargarConfig();
    
    int nJoy = SDL_NumJoysticks();
    if (nJoy == 0) {
        std::cout << "No hay mandos conectados. Conecta tus mandos y reinicia." << std::endl;
        system("pause");
        return 0;
    }

    std::cout << "=== XINPUT MASTER - MENU DE CONFIGURACION ===" << std::endl;
    std::vector<std::string> listaGuids;
    for (int i = 0; i < nJoy; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        char guidStr[33];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), guidStr, 33);
        listaGuids.push_back(guidStr);
        std::cout << "[" << i + 1 << "] Mando: " << SDL_JoystickName(j) << " (" << (baseDeDatos.count(guidStr) ? "Configurado" : "NUEVO") << ")" << std::endl;
    }
    
    std::cout << "[0] Iniciar emulacion directa" << std::endl;
    std::cout << "Selecciona un numero para RECONFIGURAR o [0] para jugar: ";
    
    int opcion;
    std::cin >> opcion;

    if (opcion > 0 && opcion <= nJoy) {
        SDL_Joystick* j = SDL_JoystickOpen(opcion - 1);
        calibrarMando(j, listaGuids[opcion - 1]);
    }

    // Preparar emuladores
    std::vector<ControllerPair> emuladores;
    for (int i = 0; i < nJoy; i++) {
        std::string guid = listaGuids[i];
        if (baseDeDatos.count(guid)) {
            SDL_Joystick* j = SDL_JoystickOpen(i);
            PVIGEM_TARGET vPad = vigem_target_x360_alloc();
            vigem_target_add(client, vPad);
            emuladores.push_back({j, vPad, guid});
            std::cout << "[ACTIVO] Slot " << emuladores.size() << ": " << SDL_JoystickName(j) << std::endl;
        }
    }

    if (emuladores.empty()) {
        std::cout << "No hay mandos configurados para emular. Calibra uno primero." << std::endl;
        system("pause");
        return 0;
    }

    std::cout << "\nEmulacion iniciada. Presiona Ctrl+C para salir.\n" << std::endl;

    while (true) {
        SDL_JoystickUpdate();
        for (auto& e : emuladores) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);
            Mapeo m = baseDeDatos[e.guid];

            // Botones configurados
            if (SDL_JoystickGetButton(e.physical, m.botones[0])) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_JoystickGetButton(e.physical, m.botones[1])) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_JoystickGetButton(e.physical, m.botones[2])) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_JoystickGetButton(e.physical, m.botones[3])) report.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_JoystickGetButton(e.physical, m.botones[4])) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_JoystickGetButton(e.physical, m.botones[5])) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_JoystickGetButton(e.physical, m.botones[6])) report.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_JoystickGetButton(e.physical, m.botones[7])) report.wButtons |= XUSB_GAMEPAD_BACK;
            if (SDL_JoystickGetButton(e.physical, m.botones[8])) report.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
            if (SDL_JoystickGetButton(e.physical, m.botones[9])) report.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;

            // Ejes (Standard generico)
            report.sThumbLX = SDL_JoystickGetAxis(e.physical, 0);
            report.sThumbLY = -SDL_JoystickGetAxis(e.physical, 1);
            report.sThumbRX = SDL_JoystickGetAxis(e.physical, 2);
            report.sThumbRY = -SDL_JoystickGetAxis(e.physical, 3);

            // Gatillos como botones (Comun en mandos Twin USB como el tuyo)
            if (SDL_JoystickGetButton(e.physical, 6)) report.bLeftTrigger = 255;
            if (SDL_JoystickGetButton(e.physical, 7)) report.bRightTrigger = 255;

            vigem_target_x360_update(client, e.virtualPad, report);
        }
        Sleep(10);
    }

    return 0;
}
