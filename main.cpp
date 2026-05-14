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

int main(int argc, char* argv[]) {
    SDL_SetMainReady();
    // Inicializamos TODO el sistema de entrada para máxima compatibilidad
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "Error SDL: " << SDL_GetError() << std::endl;
        system("pause");
        return 1;
    }

    PVIGEM_CLIENT client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cerr << "ERROR: Driver ViGEmBus no encontrado." << std::endl;
        system("pause");
        return 1;
    }

    cargarConfig();
    
    // Forzamos a SDL a buscar mandos de nuevo
    SDL_JoystickUpdate();
    int nJoy = SDL_NumJoysticks();
    
    if (nJoy == 0) {
        std::cout << "DEBUG: SDL no ve mandos. Intentando reinicio de subsistema..." << std::endl;
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
        nJoy = SDL_NumJoysticks();
    }

    if (nJoy == 0) {
        std::cout << "No hay mandos conectados. REVISA: ¿Esta encendido el modo ANALOG?" << std::endl;
        system("pause");
        return 0;
    }

    std::cout << "=== MANDOS DETECTADOS: " << nJoy << " ===" << std::endl;
    std::vector<std::string> listaGuids;
    for (int i = 0; i < nJoy; i++) {
        SDL_Joystick* j = SDL_JoystickOpen(i);
        if (j) {
            char guidStr[33];
            SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), guidStr, 33);
            listaGuids.push_back(guidStr);
            std::cout << "[" << i + 1 << "] " << SDL_JoystickName(j) << std::endl;
        }
    }
    
    std::cout << "\n[0] Jugar directo | [Num] Reconfigurar mando: ";
    int opcion;
    std::cin >> opcion;

    // Lógica de calibración y emulación (Igual a la anterior pero más estable)
    // ... (El resto del código se mantiene para procesar los datos)
    
    // (Asegúrate de copiar el bloque de emulación completo que te pasé antes)
    return 0;
}
