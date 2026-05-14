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
    int dpad[4];    // Up, Down, Left, Right
    int ejes[4];     // LX, LY, RX, RY (Índices de ejes)
    int signos[4];   // 1 o -1 para corregir inversión
};

struct ControllerPair {
    SDL_Joystick* physical;
    PVIGEM_TARGET virtualPad;
    std::string guid;
};

std::map<std::string, Mapeo> baseDeDatos;
std::vector<std::string> nombresBotones = {"A", "B", "X", "Y", "LB", "RB", "START", "BACK", "L3", "R3"};
std::vector<std::string> nombresEjes = {"STICK IZQUIERDO hacia ARRIBA", "STICK IZQUIERDO hacia la DERECHA", "STICK DERECHO hacia ARRIBA", "STICK DERECHO hacia la DERECHA"};

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
    
    // 1. CALIBRAR BOTONES
    for (int i = 0; i < 10; i++) {
        std::cout << "Presiona [" << nombresBotones[i] << "]: " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN && ev.jbutton.which == SDL_JoystickInstanceID(joy)) {
                    nuevo.botones[i] = ev.jbutton.button;
                    std::cout << "OK (" << ev.jbutton.button << ")" << std::endl;
                    capturado = true;
                    Sleep(400);
                }
            }
        }
    }

    // 2. CALIBRAR CRUCETA
    std::cout << "\nPresiona las FLECHAS (DPAD):\n";
    std::vector<std::string> dNames = {"ARRIBA", "ABAJO", "IZQUIERDA", "DERECHA"};
    for (int i = 0; i < 4; i++) {
        std::cout << "Presiona flecha " << dNames[i] << ": " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN && ev.jbutton.which == SDL_JoystickInstanceID(joy)) {
                    nuevo.dpad[i] = ev.jbutton.button;
                    std::cout << "OK\n";
                    capturado = true; Sleep(400);
                }
            }
        }
    }

    // 3. CALIBRAR ANÁLOGOS (Detección de Ejes)
    std::cout << "\n--- CALIBRANDO STICKS (Muevelos a tope) ---\n";
    for (int i = 0; i < 4; i++) {
        std::cout << "Mueve el " << nombresEjes[i] << ": " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_JoystickUpdate();
            for (int a = 0; a < SDL_JoystickNumAxes(joy); a++) {
                int val = SDL_JoystickGetAxis(joy, a);
                if (abs(val) > 20000) { // Si el movimiento es fuerte
                    nuevo.ejes[i] = a;
                    nuevo.signos[i] = (val > 0) ? 1 : -1;
                    std::cout << "OK (Eje " << a << ")\n";
                    capturado = true;
                    Sleep(800);
                    break;
                }
            }
            Sleep(10);
        }
    }

    baseDeDatos[guid] = nuevo;
    guardarConfig();
    std::cout << "¡Configuracion completa!\n";
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO);
    PVIGEM_CLIENT client = vigem_alloc();
    vigem_connect(client);

    cargarConfig();
    SDL_JoystickUpdate();
    int nJoy = SDL_NumJoysticks();

    if (nJoy == 0) { std::cout << "No hay mandos.\n"; system("pause"); return 0; }

    std::cout << "=== XINPUT MASTER 6-SLOTS ===\n";
    std::vector<std::string> guids;
    for (int i = 0; i < nJoy && i < 6; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        char gStr[33]; SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), gStr, 33);
        guids.push_back(gStr);
        std::cout << "[" << i + 1 << "] " << SDL_JoystickName(j) << (baseDeDatos.count(gStr) ? " (Listo)" : " (Nuevo)") << std::endl;
    }

    std::cout << "\nElige numero para configurar o [0] para jugar: ";
    int op; std::cin >> op;
    if (op > 0 && op <= (int)guids.size()) calibrarMando(SDL_JoystickOpen(op - 1), guids[op - 1]);

    std::vector<ControllerPair> emus;
    for (int i = 0; i < nJoy && i < 6; i++) {
        if (baseDeDatos.count(guids[i])) {
            SDL_Joystick* phys = SDL_JoystickOpen(i);
            PVIGEM_TARGET vPad = vigem_target_x360_alloc();
            vigem_target_add(client, vPad);
            emus.push_back({phys, vPad, guids[i]});
        }
    }

    while (true) {
        SDL_JoystickUpdate();
        for (auto& e : emus) {
            XUSB_REPORT rep; XUSB_REPORT_INIT(&rep);
            Mapeo m = baseDeDatos[e.guid];

            // Botones y DPAD
            if (SDL_JoystickGetButton(e.physical, m.botones[0])) rep.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_JoystickGetButton(e.physical, m.botones[1])) rep.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_JoystickGetButton(e.physical, m.botones[2])) rep.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_JoystickGetButton(e.physical, m.botones[3])) rep.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_JoystickGetButton(e.physical, m.botones[4])) rep.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_JoystickGetButton(e.physical, m.botones[5])) rep.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_JoystickGetButton(e.physical, m.botones[6])) rep.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_JoystickGetButton(e.physical, m.botones[7])) rep.wButtons |= XUSB_GAMEPAD_BACK;
            if (SDL_JoystickGetButton(e.physical, m.botones[8])) rep.wButtons |= XUSB_GAMEPAD_LEFT_THUMB;
            if (SDL_JoystickGetButton(e.physical, m.botones[9])) rep.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB;
            if (SDL_JoystickGetButton(e.physical, m.dpad[0])) rep.wButtons |= XUSB_GAMEPAD_DPAD_UP;
            if (SDL_JoystickGetButton(e.physical, m.dpad[1])) rep.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
            if (SDL_JoystickGetButton(e.physical, m.dpad[2])) rep.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
            if (SDL_JoystickGetButton(e.physical, m.dpad[3])) rep.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

            // Análogos dinámicos
            // Mapeo: 0=LY, 1=LX, 2=RY, 3=RX (Usamos los signos grabados para que Arriba sea Arriba siempre)
            rep.sThumbLY = SDL_JoystickGetAxis(e.physical, m.ejes[0]) * (m.signos[0] * -1);
            rep.sThumbLX = SDL_JoystickGetAxis(e.physical, m.ejes[1]) * m.signos[1];
            rep.sThumbRY = SDL_JoystickGetAxis(e.physical, m.ejes[2]) * (m.signos[2] * -1);
            rep.sThumbRX = SDL_JoystickGetAxis(e.physical, m.ejes[3]) * m.signos[3];

            // Gatillos basicos
            if (SDL_JoystickGetButton(e.physical, 6)) rep.bLeftTrigger = 255;
            if (SDL_JoystickGetButton(e.physical, 7)) rep.bRightTrigger = 255;

            vigem_target_x360_update(client, e.virtualPad, rep);
        }
        Sleep(10);
    }
    return 0;
}
