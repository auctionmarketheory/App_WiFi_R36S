#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <stdlib.h>
#include "def.h"
#include "sdlutils.h"

// Globals inherited from 351Files boilerplate
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
SDL_Joystick* g_joystick = NULL;
TTF_Font *g_font = NULL;
TTF_Font *g_fontMono = NULL;
int g_charW = 0;

// Dummy implementation for 351Files dependencies
class IWindow;
std::vector<IWindow *> g_windows;

std::atomic<bool> is_pinging(false);
std::atomic<bool> thread_running(false);

// Background Ping Thread (prevent UI freeze)
void ping_thread() {
    thread_running = true;
    while(is_pinging) {
        system("ping -c 1 -W 1 8.8.8.8 >> /tmp/wifi_log.txt 2>&1");
        SDL_Delay(1000);
    }
    thread_running = false;
}

// Read bottom N lines of file
std::vector<std::string> tail_log(const std::string& path, int lines) {
    std::vector<std::string> out;
    std::ifstream file(path.c_str());
    if(!file.is_open()) return out;
    std::string line;
    std::vector<std::string> all_lines;
    while(std::getline(file, line)) {
        all_lines.push_back(line);
    }
    int start = all_lines.size() > (size_t)lines ? all_lines.size() - lines : 0;
    for(size_t i = start; i < all_lines.size(); ++i) {
        out.push_back(all_lines[i]);
    }
    return out;
}

int main(int argc, char* args[]) {
    // 1. Init SDL Framework from 351Files
    if (!SDLUtils::init()) return 1;

    // Load Fonts from /res/
    g_font = SDLUtils::loadFont(std::string(RES_PATH) + "/" + FONT_NAME, FONT_SIZE);
    g_fontMono = SDLUtils::loadFont(std::string(RES_PATH) + "/" + FONT_NAME_MONO, FONT_SIZE);
    if (g_font == NULL) return 1;

    // Cyberpunk Colors
    SDL_Color neonGreen = {0, 255, 0, 255};
    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color bgGray = {45, 45, 45, 255};

    bool quit = false;
    bool show_menu = false;
    int menu_selection = 0; // 0: Mute, 1: Quit
    bool mute = false;

    // Init log file
    system("echo '[RETROHACK OS v1.33t]' > /tmp/wifi_log.txt");
    system("echo 'SYS://TERMINAL> LOADING... | STATUS: ONLINE' >> /tmp/wifi_log.txt");

    // 2. Main Game Loop
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
            
            // 3. User Input (Mapped from def.h for RG351/R36S)
            if (BUTTON_PRESSED_MENU_CONTEXT) { // X Button
                show_menu = !show_menu;
                menu_selection = 0;
            }
            else if (show_menu) {
                if (BUTTON_PRESSED_UP) menu_selection = (menu_selection == 1) ? 0 : 1;
                if (BUTTON_PRESSED_DOWN) menu_selection = (menu_selection == 0) ? 1 : 0;
                if (BUTTON_PRESSED_BACK) show_menu = false; // B Button: Back
                if (BUTTON_PRESSED_VALIDATE) { // A Button: Confirm
                    if (menu_selection == 1) quit = true; // Actually Quit
                    if (menu_selection == 0) mute = !mute;
                    show_menu = false;
                }
            }
            else {
                if (BUTTON_PRESSED_VALIDATE) { // A Button (Auto Ping)
                    is_pinging = !is_pinging;
                    if (is_pinging && !thread_running) {
                        std::thread t(ping_thread);
                        t.detach(); // Run independently
                    }
                }
            }
        }

        // --- RENDER UI ---
        // Clear background
        SDL_SetRenderDrawColor(g_renderer, 10, 15, 10, 255);
        SDL_RenderClear(g_renderer);

        // Matrix Frame
        SDL_SetRenderDrawColor(g_renderer, 0, 200, 0, 255);
        SDL_Rect rect = { 10, 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50 };
        SDL_RenderDrawRect(g_renderer, &rect);

        // Print scrolling logs
        std::vector<std::string> logs = tail_log("/tmp/wifi_log.txt", (SCREEN_HEIGHT - 80)/LINE_HEIGHT);
        int y = 20;
        for (size_t i = 0; i < logs.size(); ++i) {
            SDLUtils::renderText(logs[i], g_fontMono, 15, y, neonGreen, black);
            y += LINE_HEIGHT;
        }

        // Draw Bottom Bar
        std::string status = is_pinging ? "[AUTO PING: ON]" : "[AUTO PING: OFF]";
        SDL_SetRenderDrawColor(g_renderer, 0, 200, 0, 255);
        SDL_Rect bottomRect = { 10, SCREEN_HEIGHT - 35, SCREEN_WIDTH - 20, 25 };
        SDL_RenderDrawRect(g_renderer, &bottomRect);
        SDLUtils::renderText(status + " | [A] TOGGLE | [X] MENU", g_font, 20, SCREEN_HEIGHT - 35, neonGreen, black);

        // Draw Popup Menu (If X is pressed)
        if (show_menu) {
            SDL_Rect menuRect = { SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 200, 120 };
            SDL_SetRenderDrawColor(g_renderer, bgGray.r, bgGray.g, bgGray.b, 255);
            SDL_RenderFillRect(g_renderer, &menuRect);
            SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(g_renderer, &menuRect);

            SDLUtils::renderText("OPTIONS", g_font, SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2 - 50, white, bgGray);

            SDL_Color color1 = (menu_selection == 0) ? neonGreen : white;
            SDL_Color color2 = (menu_selection == 1) ? neonGreen : white;
            
            std::string mute_text = mute ? "1. Unmute Beep" : "1. Mute Beep";
            SDLUtils::renderText(mute_text, g_font, SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, color1, bgGray);
            SDLUtils::renderText("2. Quit App", g_font, SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 20, color2, bgGray);
        }

        SDL_RenderPresent(g_renderer);
        SDL_Delay(MS_PER_FRAME);
    }

    is_pinging = false;
    SDLUtils::close();
    return 0;
}
