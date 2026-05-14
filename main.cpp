#define SDL_MAIN_HANDLED
#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <string>
#include <conio.h>
#include <SDL.h>
#include "ViGEm/Client.h"

#pragma comment(lib, "setupapi.lib")

struct Mapeo {
    int botones[10]; 
    int dpad_tipo; 
    int dpad_ids[4]; 
    int ejes[4];     
    int signos[4];   
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

// Función para procesar el stick con zona muerta e inversión
short procesarStick(int valor, int signo, bool esEjeY) {
    if (abs(valor) < 8000) return 0; // Zona muerta para evitar que se mueva solo

    // Si es Eje Y (Arriba/Abajo), invertimos la polaridad para Xbox
    float multiplicador = esEjeY ? -1.0f : 1.0f;
    
    int resultado = (int)(valor * (signo == 1 ? 1 : -1) * multiplicador);
    
    if (resultado > 32767) return 32767;
    if (resultado < -32768) return -32768;
    return (short)resultado;
}

void calibrarMando(SDL_Joystick* joy, std::string guid) {
    Mapeo nuevo;
    std::cout << "\n>>> CALIBRANDO: " << SDL_JoystickName(joy) << std::endl;
    
    // 1. BOTONES
    for (int i = 0; i < 10; i++) {
        std::cout << "Presiona [" << nombresBotones[i] << "]: " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN && ev.jbutton.which == SDL_JoystickInstanceID(joy)) {
                    nuevo.botones[i] = ev.jbutton.button;
                    std::cout << "OK (" << ev.jbutton.button << ")" << std::endl;
                    capturado = true; Sleep(400);
                }
            }
        }
    }

    // 2. CRUCETA (DPAD)
    std::cout << "\nPresiona las FLECHAS (DPAD):\n";
    for (int i = 0; i < 4; i++) {
        std::cout << "Presiona direccion " << (i==0?"ARRIBA":i==1?"ABAJO":i==2?"IZQUIERDA":"DERECHA") << ": " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_JoystickUpdate();
            for (int b = 0; b < SDL_JoystickNumButtons(joy); b++) {
                if (SDL_JoystickGetButton(joy, b)) {
                    nuevo.dpad_tipo = 0; nuevo.dpad_ids[i] = b;
                    std::cout << "OK\n"; capturado = true; Sleep(500); break;
                }
            }
            if (capturado) break;
            for (int h = 0; h < SDL_JoystickNumHats(joy); h++) {
                Uint8 val = SDL_JoystickGetHat(joy, h);
                if (val != SDL_HAT_CENTERED) {
                    nuevo.dpad_tipo = 1; nuevo.dpad_ids[i] = val;
                    std::cout << "OK (Hat)\n"; capturado = true; Sleep(500); break;
                }
            }
            Sleep(10);
        }
    }

    // 3. ANÁLOGOS
    std::cout << "\n--- CALIBRANDO STICKS ---\n";
    for (int i = 0; i < 4; i++) {
        std::cout << "Mueve y MANTEN el " << nombresEjes[i] << ": " << std::flush;
        bool capturado = false;
        while (!capturado) {
            SDL_JoystickUpdate();
            for (int a = 0; a < SDL_JoystickNumAxes(joy); a++) {
                int val = SDL_JoystickGetAxis(joy, a);
                if (abs(val) > 25000) { 
                    nuevo.ejes[i] = a;
                    nuevo.signos[i] = (val > 0) ? 1 : -1;
                    std::cout << "OK!\n";
                    capturado = true; Sleep(800); break;
                }
            }
            Sleep(10);
        }
    }

    baseDeDatos[guid] = nuevo;
    guardarConfig();
    std::cout << "¡Configuracion lista!\n";
}

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO);
    PVIGEM_CLIENT client = vigem_alloc();
    vigem_connect(client);
    cargarConfig();

    while (true) {
        SDL_JoystickUpdate();
        int nJoy = SDL_NumJoysticks();
        system("cls");
        std::cout << "=== XINPUT MASTER (MODO MENU) ===\n";
        std::vector<std::string> guids;
        for (int i = 0; i < nJoy && i < 6; i++) {
            SDL_Joystick* j = SDL_JoystickOpen(i);
            char gStr[33]; SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), gStr, 33);
            guids.push_back(gStr);
            std::cout << "[" << i + 1 << "] " << SDL_JoystickName(j) << (baseDeDatos.count(gStr) ? " (Listo)" : " (Nuevo)") << std::endl;
        }

        std::cout << "\n[0] Jugar | [1-6] Configurar | [M] Volver aqui desde el juego\nOpcion: ";
        int op; std::cin >> op;

        if (op == 0) {
            std::vector<ControllerPair> emus;
            for (int i = 0; i < nJoy && i < 6; i++) {
                if (baseDeDatos.count(guids[i])) {
                    emus.push_back({SDL_JoystickOpen(i), vigem_target_x360_alloc(), guids[i]});
                    vigem_target_add(client, emus.back().virtualPad);
                }
            }

            std::cout << "\n>>> EMULANDO. Pulsa 'M' para volver al menu.\n";
            while (true) {
                if (_kbhit() && (tolower(_getch()) == 'm')) break;

                SDL_JoystickUpdate();
                for (auto& e : emus) {
                    XUSB_REPORT rep; XUSB_REPORT_INIT(&rep);
                    Mapeo m = baseDeDatos[e.guid];

                    // Botones
                    for(int b=0; b<10; b++) {
                        if(SDL_JoystickGetButton(e.physical, m.botones[b])) {
                            switch(b) {
                                case 0: rep.wButtons |= XUSB_GAMEPAD_A; break;
                                case 1: rep.wButtons |= XUSB_GAMEPAD_B; break;
                                case 2: rep.wButtons |= XUSB_GAMEPAD_X; break;
                                case 3: rep.wButtons |= XUSB_GAMEPAD_Y; break;
                                case 4: rep.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER; break;
                                case 5: rep.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER; break;
                                case 6: rep.wButtons |= XUSB_GAMEPAD_START; break;
                                case 7: rep.wButtons |= XUSB_GAMEPAD_BACK; break;
                                case 8: rep.wButtons |= XUSB_GAMEPAD_LEFT_THUMB; break;
                                case 9: rep.wButtons |= XUSB_GAMEPAD_RIGHT_THUMB; break;
                            }
                        }
                    }

                    // DPAD
                    if (m.dpad_tipo == 0) {
                        if (SDL_JoystickGetButton(e.physical, m.dpad_ids[0])) rep.wButtons |= XUSB_GAMEPAD_DPAD_UP;
                        if (SDL_JoystickGetButton(e.physical, m.dpad_ids[1])) rep.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
                        if (SDL_JoystickGetButton(e.physical, m.dpad_ids[2])) rep.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
                        if (SDL_JoystickGetButton(e.physical, m.dpad_ids[3])) rep.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;
                    } else {
                        Uint8 h = SDL_JoystickGetHat(e.physical, 0);
                        if (h & SDL_HAT_UP) rep.wButtons |= XUSB_GAMEPAD_DPAD_UP;
                        if (h & SDL_HAT_DOWN) rep.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
                        if (h & SDL_HAT_LEFT) rep.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
                        if (h & SDL_HAT_RIGHT) rep.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;
                    }

                    // Sticks Corregidos
                    rep.sThumbLY = procesarStick(SDL_JoystickGetAxis(e.physical, m.ejes[0]), m.signos[0], true);
                    rep.sThumbLX = procesarStick(SDL_JoystickGetAxis(e.physical, m.ejes[1]), m.signos[1], false);
                    rep.sThumbRY = procesarStick(SDL_JoystickGetAxis(e.physical, m.ejes[2]), m.signos[2], true);
                    rep.sThumbRX = procesarStick(SDL_JoystickGetAxis(e.physical, m.ejes[3]), m.signos[3], false);

                    // Gatillos
                    if (SDL_JoystickGetButton(e.physical, 6)) rep.bLeftTrigger = 255;
                    if (SDL_JoystickGetButton(e.physical, 7)) rep.bRightTrigger = 255;

                    vigem_target_x360_update(client, e.virtualPad, rep);
                }
                Sleep(8);
            }
            for(auto& e : emus) { vigem_target_remove(client, e.virtualPad); vigem_target_free(e.virtualPad); }
        } else if (op > 0 && op <= (int)guids.size()) {
            calibrarMando(SDL_JoystickOpen(op - 1), guids[op - 1]);
        }
    }
    return 0;
}
