#include <windows.h>
#include <iostream>
#include <SDL.h>
#include <ViGEm/Client.h>
#include <string>

// Vincular librerías de Windows
#pragma comment(lib, "setupapi.lib")

// Función para leer el archivo config.ini
int GetConfigInt(const char* section, const char* key, int defaultValue) {
    return GetPrivateProfileIntA(section, key, defaultValue, ".\\config.ini");
}

int main(int argc, char* argv[]) {
    // 1. Inicializar SDL (Lectura de controles)
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cout << "Error inicializando SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 2. Conectar al Bus de ViGEm (Mando Virtual)
    auto client = vigem_alloc();
    if (!VIGEM_SUCCESS(vigem_connect(client))) {
        std::cout << "Error: ViGEmBus no encontrado. Instala el driver." << std::endl;
        return -1;
    }

    auto xbox = vigem_target_x360_alloc();
    vigem_target_add(client, xbox);

    std::cout << "========================================" << std::endl;
    std::cout << "   XINPUT EMULATOR PORTABLE (ACTIVO)    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Ocultando mando fisico y emulando Xbox 360..." << std::endl;

    SDL_GameController* controller = nullptr;
    XUSB_REPORT report;
    bool running = true;

    // Bucle Principal
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_CONTROLLERDEVICEADDED && !controller) {
                controller = SDL_GameControllerOpen(event.cdevice.which);
                std::cout << "-> Mando conectado: " << SDL_GameControllerName(controller) << std::endl;
            }
        }

        if (controller) {
            memset(&report, 0, sizeof(XUSB_REPORT));

            // LEER CONFIGURACION DESDE CONFIG.INI
            // Mapeamos los botones físicos a los de Xbox
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "A", 0))) report.wButtons |= XUSB_GAMEPAD_A;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "B", 1))) report.wButtons |= XUSB_GAMEPAD_B;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "X", 2))) report.wButtons |= XUSB_GAMEPAD_X;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "Y", 3))) report.wButtons |= XUSB_GAMEPAD_Y;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "LB", 4))) report.wButtons |= XUSB_GAMEPAD_LEFT_SHOULDER;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "RB", 5))) report.wButtons |= XUSB_GAMEPAD_RIGHT_SHOULDER;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "START", 6))) report.wButtons |= XUSB_GAMEPAD_START;
            if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)GetConfigInt("Buttons", "BACK", 7))) report.wButtons |= XUSB_GAMEPAD_BACK;

            // STICKS (Palancas)
            report.sThumbLX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            report.sThumbLY = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            report.sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
            report.sThumbRY = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

            // GATILLOS
            report.bLeftTrigger = (BYTE)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 8);
            report.bRightTrigger = (BYTE)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 8);

            // Enviar datos al mando virtual
            vigem_target_x360_update(client, xbox, report);
        }

        Sleep(1); // Latencia ultra baja
    }

    // Limpieza al cerrar
    vigem_target_remove(client, xbox);
    vigem_target_free(xbox);
    vigem_disconnect(client);
    vigem_free(client);
    SDL_Quit();

    return 0;
}
