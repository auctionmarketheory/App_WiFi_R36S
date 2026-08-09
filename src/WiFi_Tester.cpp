#include <SDL.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <stdlib.h>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "def.h"

SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
SDL_Joystick* g_joystick = NULL;

class CustomFont {
public:
    SDL_Texture* atlas;
    stbtt_bakedchar cdata[96];
    int tex_w, tex_h;
    float size;

    CustomFont() : atlas(NULL), tex_w(512), tex_h(512), size(20.0f) {}

    bool load(SDL_Renderer* renderer, const std::string& path, float font_size) {
        size = font_size;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if(!file.is_open()) return false;
        std::streamsize size_file = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> buffer(size_file);
        if(!file.read((char*)buffer.data(), size_file)) return false;

        std::vector<unsigned char> bitmap(tex_w * tex_h);
        stbtt_BakeFontBitmap(buffer.data(), 0, size, bitmap.data(), tex_w, tex_h, 32, 96, cdata);

        std::vector<unsigned char> rgba(tex_w * tex_h * 4, 255);
        for(int i=0; i<tex_w*tex_h; i++) {
            rgba[i*4 + 3] = bitmap[i];
        }

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(rgba.data(), tex_w, tex_h, 32, tex_w * 4,
            0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if(!surface) return false;

        atlas = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
        return atlas != NULL;
    }

    void renderText(SDL_Renderer* renderer, float x, float y, const std::string& text, SDL_Color color) {
        SDL_SetTextureColorMod(atlas, color.r, color.g, color.b);
        for (char c : text) {
            if (c >= 32 && c < 128) {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, tex_w, tex_h, c - 32, &x, &y, &q, 1);
                
                SDL_Rect src = { (int)(q.s0 * tex_w), (int)(q.t0 * tex_h), 
                                 (int)((q.s1 - q.s0) * tex_w), (int)((q.t1 - q.t0) * tex_h) };
                SDL_Rect dst = { (int)q.x0, (int)q.y0, (int)(q.x1 - q.x0), (int)(q.y1 - q.y0) };
                SDL_RenderCopy(renderer, atlas, &src, &dst);
            }
        }
    }

    ~CustomFont() {
        if(atlas) SDL_DestroyTexture(atlas);
    }
};

std::atomic<bool> is_pinging(false);
std::atomic<bool> thread_running(false);

void ping_thread() {
    thread_running = true;
    while(is_pinging) {
        system("ping -c 1 -W 1 8.8.8.8 >> /tmp/wifi_log.txt 2>&1");
        SDL_Delay(1000);
    }
    thread_running = false;
}

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

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) return false;
    
    if (SDL_NumJoysticks() >= 1) {
        g_joystick = SDL_JoystickOpen(0);
    }
    
    #if FULLSCREEN == 1
       g_window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    #else
       g_window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    #endif
    if (!g_window) return false;

    #if HARDWARE_ACCELERATION == 1
       g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    #else
       g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    #endif
    if (!g_renderer) return false;
    
    return true;
}

int main(int argc, char* args[]) {
    if (!init_sdl()) return 1;

    CustomFont font, fontMono;
    if (!font.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME, FONT_SIZE + 2)) return 1;
    if (!fontMono.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME_MONO, FONT_SIZE - 2)) return 1;

    SDL_Color neonGreen = {0, 255, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color bgGray = {45, 45, 45, 255};

    bool quit = false;
    bool show_menu = false;
    int menu_selection = 0; 
    bool mute = false;

    system("echo '[RETROHACK OS v1.33t]' > /tmp/wifi_log.txt");
    system("echo 'SYS://TERMINAL> LOADING... | STATUS: ONLINE' >> /tmp/wifi_log.txt");

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
            if (BUTTON_PRESSED_MENU_CONTEXT) {
                show_menu = !show_menu;
                menu_selection = 0;
            }
            else if (show_menu) {
                if (BUTTON_PRESSED_UP) menu_selection = (menu_selection == 1) ? 0 : 1;
                if (BUTTON_PRESSED_DOWN) menu_selection = (menu_selection == 0) ? 1 : 0;
                if (BUTTON_PRESSED_BACK) show_menu = false; 
                if (BUTTON_PRESSED_VALIDATE) { 
                    if (menu_selection == 1) quit = true; 
                    if (menu_selection == 0) mute = !mute;
                    show_menu = false;
                }
            }
            else {
                if (BUTTON_PRESSED_VALIDATE) { 
                    is_pinging = !is_pinging;
                    if (is_pinging && !thread_running) {
                        std::thread t(ping_thread);
                        t.detach();
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(g_renderer, 10, 15, 10, 255);
        SDL_RenderClear(g_renderer);
        SDL_SetRenderDrawColor(g_renderer, 0, 200, 0, 255);
        SDL_Rect rect = { 10, 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50 };
        SDL_RenderDrawRect(g_renderer, &rect);

        std::vector<std::string> logs = tail_log("/tmp/wifi_log.txt", (SCREEN_HEIGHT - 80)/LINE_HEIGHT);
        int y = 20 + LINE_HEIGHT;
        for (size_t i = 0; i < logs.size(); ++i) {
            fontMono.renderText(g_renderer, 15, y, logs[i], neonGreen);
            y += LINE_HEIGHT;
        }

        std::string status = is_pinging ? "[AUTO PING: ON]" : "[AUTO PING: OFF]";
        SDL_SetRenderDrawColor(g_renderer, 0, 200, 0, 255);
        SDL_Rect bottomRect = { 10, SCREEN_HEIGHT - 35, SCREEN_WIDTH - 20, 25 };
        SDL_RenderDrawRect(g_renderer, &bottomRect);
        font.renderText(g_renderer, 20, SCREEN_HEIGHT - 35 + 20, status + " | [A] TOGGLE | [X] MENU", neonGreen);

        if (show_menu) {
            SDL_Rect menuRect = { SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 60, 200, 120 };
            SDL_SetRenderDrawColor(g_renderer, bgGray.r, bgGray.g, bgGray.b, 255);
            SDL_RenderFillRect(g_renderer, &menuRect);
            SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(g_renderer, &menuRect);

            font.renderText(g_renderer, SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT/2 - 50 + 20, "OPTIONS", white);

            SDL_Color color1 = (menu_selection == 0) ? neonGreen : white;
            SDL_Color color2 = (menu_selection == 1) ? neonGreen : white;
            std::string mute_text = mute ? "1. Unmute Beep" : "1. Mute Beep";
            font.renderText(g_renderer, SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10 + 20, mute_text, color1);
            font.renderText(g_renderer, SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 20 + 20, "2. Quit App", color2);
        }

        SDL_RenderPresent(g_renderer);
        SDL_Delay(MS_PER_FRAME);
    }
    is_pinging = false;
    
    if (g_joystick) SDL_JoystickClose(g_joystick);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
