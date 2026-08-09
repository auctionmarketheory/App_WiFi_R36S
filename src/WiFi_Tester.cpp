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

void DrawScanlines(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 70); // Mờ mờ (70/255)
    for (int y = 0; y < SCREEN_HEIGHT; y += 2) {
        SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
}

void DrawHUD(SDL_Renderer* renderer, SDL_Color neonColor) {
    SDL_SetRenderDrawColor(renderer, neonColor.r, neonColor.g, neonColor.b, 255);
    
    // Outer border
    SDL_Rect outer = { 5, 5, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 10 };
    SDL_RenderDrawRect(renderer, &outer);
    
    // Inner border
    SDL_Rect inner = { 10, 35, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 70 };
    SDL_RenderDrawRect(renderer, &inner);

    // Decorative corner brackets
    int len = 20;
    // Top-Left inner
    SDL_RenderDrawLine(renderer, 8, 33, 8+len, 33);
    SDL_RenderDrawLine(renderer, 8, 33, 8, 33+len);
    // Bottom-Right inner
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH-8, SCREEN_HEIGHT-33, SCREEN_WIDTH-8-len, SCREEN_HEIGHT-33);
    SDL_RenderDrawLine(renderer, SCREEN_WIDTH-8, SCREEN_HEIGHT-33, SCREEN_WIDTH-8, SCREEN_HEIGHT-33-len);
    
    // Header Line
    SDL_RenderDrawLine(renderer, 10, 30, SCREEN_WIDTH - 10, 30);
    
    // Footer Line
    SDL_RenderDrawLine(renderer, 10, SCREEN_HEIGHT - 30, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 30);
}

int main(int argc, char* args[]) {
    if (!init_sdl()) return 1;

    CustomFont font, fontMono;
    if (!font.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME, FONT_SIZE + 4)) return 1;
    if (!fontMono.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME_MONO, FONT_SIZE)) return 1;

    SDL_Color neonGreen = {0, 255, 100, 255};
    SDL_Color neonBlue = {0, 200, 255, 255};
    SDL_Color redError = {255, 50, 50, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color bgGray = {20, 20, 20, 255};
    SDL_Color menuBg = {0, 0, 0, 220};

    bool quit = false;
    bool show_menu = false;
    int menu_selection = 0; 
    bool mute = false;

    system("echo '> SYSTEM STARTUP [RETROHACK OS v2.0]' > /tmp/wifi_log.txt");
    system("echo '> LOADING NETWORK MODULE... OK' >> /tmp/wifi_log.txt");
    system("echo '> STANDBY.' >> /tmp/wifi_log.txt");

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

        // Đổ nền đen
        SDL_SetRenderDrawColor(g_renderer, 5, 10, 5, 255);
        SDL_RenderClear(g_renderer);

        // Vẽ HUD
        DrawHUD(g_renderer, neonGreen);

        // Header Text
        font.renderText(g_renderer, 15, 22, "[ W I F I   N E T W O R K   A N A L Y S T ]", neonGreen);
        font.renderText(g_renderer, SCREEN_WIDTH - 120, 22, "SYS: ACTIVE", neonGreen);

        // Log Text
        std::vector<std::string> logs = tail_log("/tmp/wifi_log.txt", (SCREEN_HEIGHT - 100)/LINE_HEIGHT);
        int y = 40 + LINE_HEIGHT;
        for (size_t i = 0; i < logs.size(); ++i) {
            SDL_Color logColor = neonGreen;
            if (logs[i].find("timeout") != std::string::npos || logs[i].find("Unreachable") != std::string::npos) logColor = redError;
            fontMono.renderText(g_renderer, 15, y, logs[i], logColor);
            
            // Vẽ Blinking Cursor ở dòng cuối
            if (i == logs.size() - 1 && is_pinging) {
                if ((SDL_GetTicks() / 500) % 2 == 0) {
                    fontMono.renderText(g_renderer, 15 + logs[i].length() * 10, y, "_", neonGreen);
                }
            }
            y += LINE_HEIGHT;
        }

        if (!is_pinging) {
            if ((SDL_GetTicks() / 500) % 2 == 0) {
                 fontMono.renderText(g_renderer, 15, y, "_", neonGreen);
            }
        }

        // Footer Text
        std::string status = is_pinging ? "TX/RX: TRANSMITTING" : "TX/RX: STANDBY";
        font.renderText(g_renderer, 15, SCREEN_HEIGHT - 12, status, neonBlue);
        font.renderText(g_renderer, SCREEN_WIDTH/2 - 70, SCREEN_HEIGHT - 12, "[A] TOGGLE PING", white);
        font.renderText(g_renderer, SCREEN_WIDTH - 140, SCREEN_HEIGHT - 12, "[X] SYS MENU", white);

        // Menu Overlay
        if (show_menu) {
            SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(g_renderer, NULL); // Làm tối toàn màn hình

            SDL_Rect menuRect = { SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 80, 240, 160 };
            SDL_SetRenderDrawColor(g_renderer, bgGray.r, bgGray.g, bgGray.b, 255);
            SDL_RenderFillRect(g_renderer, &menuRect);
            SDL_SetRenderDrawColor(g_renderer, neonGreen.r, neonGreen.g, neonGreen.b, 255);
            SDL_RenderDrawRect(g_renderer, &menuRect);
            
            // Viền kép cho menu
            SDL_Rect menuRectInner = { menuRect.x+4, menuRect.y+4, menuRect.w-8, menuRect.h-8 };
            SDL_RenderDrawRect(g_renderer, &menuRectInner);

            font.renderText(g_renderer, SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2 - 60 + 20, "SYSTEM MENU", white);
            SDL_RenderDrawLine(g_renderer, menuRect.x + 10, SCREEN_HEIGHT/2 - 35, menuRect.x + menuRect.w - 10, SCREEN_HEIGHT/2 - 35);

            for (int i=0; i<2; i++) {
                int item_y = SCREEN_HEIGHT/2 - 10 + i * 40;
                std::string text = (i == 0) ? (mute ? "> UNMUTE BEEP" : "> MUTE BEEP") : "> SHUTDOWN SYS";
                
                if (menu_selection == i) {
                    SDL_Rect hl = { menuRect.x + 10, item_y, menuRect.w - 20, 30 };
                    SDL_SetRenderDrawColor(g_renderer, neonGreen.r, neonGreen.g, neonGreen.b, 255);
                    SDL_RenderFillRect(g_renderer, &hl);
                    font.renderText(g_renderer, SCREEN_WIDTH/2 - 80, item_y + 20, text, bgGray); // Chữ đen trên nền xanh
                } else {
                    font.renderText(g_renderer, SCREEN_WIDTH/2 - 80, item_y + 20, text, neonGreen);
                }
            }
        }

        // Draw CRT Scanlines over everything
        DrawScanlines(g_renderer);

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
