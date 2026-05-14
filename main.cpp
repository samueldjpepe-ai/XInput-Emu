#define SDL_MAIN_HANDLED
#include <windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <SDL.h>
#include "ViGEm/Client.h"

#pragma comment(lib, "setupapi.lib")

struct Mapeo {
    int botones[8]; // A, B, X, Y, LB, RB, Start, Back
};

std::map<std::string, Mapeo> baseDeDatos;
std::vector<std::string> nombresBotones = {"A", "B", "X", "Y", "LB", "RB", "START", "BACK"};

// Función para guardar en archivo
void guardarConfig() {
    std::ofstream archivo("controles_config.dat", std::ios::binary);
    for (auto const& [guid, mapeo] : baseDeDatos) {
        archivo << guid << " ";
        archivo.write((char*)&mapeo, sizeof(Mapeo));
    }
}

// Función para cargar desde archivo
void cargarConfig() {
    std::ifstream archivo("controles_config.dat", std::ios::binary);
    if (!archivo) return;
    std::string guid;
    Mapeo m;
    while (archivo >> guid) {
        archivo.ignore();
        archivo.read((char*)&m, sizeof(Mapeo));
        baseDeDatos[guid] = m;
    }
}

void calibrarMando(SDL_Joystick* joy, std::string guid) {
    Mapeo nuevoMapeo;
    std::cout << "\n--- Calibrando mando: " << guid << " ---" << std::endl;
    for (int i = 0; i < nombresBotones.size(); i++) {
        std::cout << "Presiona el boton para " << nombresBotones[i] << ": " << std::flush;
        bool pulsado = false;
        while (!pulsado) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_JOYBUTTONDOWN) {
                    nuevoMapeo.botones[i] = ev.jbutton.button;
                    std::cout << "OK (" << ev.jbutton.button << ")" << std::endl;
                    pulsado = true;
                    Sleep(400);
                }
            }
        }
    }
    baseDeDatos[guid] = nuevoMapeo;
    guardarConfig();
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
    cargarConfig();
    
    auto client = vigem_alloc();
    vigem_connect(client);

    std::cout << "=== EMULADOR XINPUT PRO ===" << std::endl;
    std::cout << "Si quieres recalibrar los mandos, presiona 'R' ahora..." << std::endl;
    
    // Aquí el programa revisa los mandos conectados
    int nJoy = SDL_NumJoysticks();
    for (int i = 0; i < nJoy; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        char guidStr[33];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), guidStr, 33);
        
        if (baseDeDatos.find(guidStr) == baseDeDatos.end()) {
            std::cout << "Mando nuevo detectado!" << std::endl;
            calibrarMando(j, guidStr);
        } else {
            std::cout << "Mando reconocido: " << guidStr << " [Configuracion cargada]" << std::endl;
        }
    }

    // Bucle de emulación (usando baseDeDatos[guidStr])
    // ... (aquí iría el código de envío de señales que ya tenemos)
    
    return 0;
}
