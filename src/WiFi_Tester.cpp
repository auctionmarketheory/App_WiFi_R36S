#include <SDL.h>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

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

void DrawGlowLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, SDL_Color color) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    // Outer glow
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 40);
    SDL_RenderDrawLine(renderer, x1-2, y1-2, x2-2, y2-2);
    SDL_RenderDrawLine(renderer, x1+2, y1+2, x2+2, y2+2);
    // Inner glow
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 100);
    SDL_RenderDrawLine(renderer, x1-1, y1-1, x2-1, y2-1);
    SDL_RenderDrawLine(renderer, x1+1, y1+1, x2+1, y2+1);
    // Core
    SDL_SetRenderDrawColor(renderer, color.r + 50 > 255 ? 255 : color.r + 50, 
                                     color.g + 50 > 255 ? 255 : color.g + 50, 
                                     color.b + 50 > 255 ? 255 : color.b + 50, 255);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void DrawCyberpunkHUD(SDL_Renderer* renderer, SDL_Color cyan, SDL_Color magenta) {
    // Vẽ lưới nền mờ
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, cyan.r, cyan.g, cyan.b, 15);
    for(int i=0; i<SCREEN_WIDTH; i+=40) SDL_RenderDrawLine(renderer, i, 0, i, SCREEN_HEIGHT);
    for(int i=0; i<SCREEN_HEIGHT; i+=40) SDL_RenderDrawLine(renderer, 0, i, SCREEN_WIDTH, i);

    // Khung viền vát góc (Chamfered)
    int pad = 10;
    int chamfer = 30;
    int w = SCREEN_WIDTH - pad*2;
    int h = SCREEN_HEIGHT - pad*2;
    
    DrawGlowLine(renderer, pad + chamfer, pad, pad + w - chamfer, pad, cyan); // Top
    DrawGlowLine(renderer, pad + w - chamfer, pad, pad + w, pad + chamfer, cyan); // Top-Right
    DrawGlowLine(renderer, pad + w, pad + chamfer, pad + w, pad + h - chamfer, cyan); // Right
    DrawGlowLine(renderer, pad + w, pad + h - chamfer, pad + w - chamfer, pad + h, cyan); // Bot-Right
    DrawGlowLine(renderer, pad + w - chamfer, pad + h, pad + chamfer, pad + h, cyan); // Bottom
    DrawGlowLine(renderer, pad + chamfer, pad + h, pad, pad + h - chamfer, cyan); // Bot-Left
    DrawGlowLine(renderer, pad, pad + h - chamfer, pad, pad + chamfer, cyan); // Left
    DrawGlowLine(renderer, pad, pad + chamfer, pad + chamfer, pad, cyan); // Top-Left
    
    // Header & Divider
    DrawGlowLine(renderer, pad, pad + 35, pad + w, pad + 35, magenta);
    DrawGlowLine(renderer, pad + w - 70, pad + 35, pad + w - 70, pad + h, cyan);
}

void DrawScanlines(SDL_Renderer* renderer) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90); 
    for (int y = 0; y < SCREEN_HEIGHT; y += 3) {
        SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
}

std::string generateHex() {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (rand() % 0xFFFF);
    return ss.str();
}

int main(int argc, char* args[]) {
    if (!init_sdl()) return 1;

    CustomFont font, fontMono, fontSmall;
    if (!font.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME, FONT_SIZE + 4)) return 1;
    if (!fontMono.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME_MONO, FONT_SIZE)) return 1;
    if (!fontSmall.load(g_renderer, std::string(RES_PATH) + "/" + FONT_NAME_MONO, FONT_SIZE - 4)) return 1;

    SDL_Color neonCyan = {0, 255, 255, 255};
    SDL_Color neonMagenta = {255, 0, 255, 255};
    SDL_Color neonYellow = {255, 255, 0, 255};
    SDL_Color redError = {255, 50, 50, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color bgGray = {10, 10, 15, 255};

    bool quit = false;
    bool show_menu = false;
    int menu_selection = 0; 
    bool mute = false;
    
    std::vector<std::string> telemetry;
    for(int i=0; i<20; i++) telemetry.push_back(generateHex());

    system("echo '> SYSTEM STARTUP [CYBER-OS v5.0]' > /tmp/wifi_log.txt");
    system("echo '> UPLINK MODULE... ESTABLISHED' >> /tmp/wifi_log.txt");
    system("echo '> AWAITING COMMAND...' >> /tmp/wifi_log.txt");

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
            bool pressed_up = false;
            bool pressed_down = false;
            bool pressed_menu = false;
            bool pressed_validate = false;
            bool pressed_back = false;

            if (event.type == SDL_JOYBUTTONDOWN) {
                int b = event.jbutton.button;
                if (b == 0 || b == 1) pressed_validate = true; // Chấp nhận cả A (0) và B (1) làm Validate
                if (b == 1 || b == 0) pressed_back = true;     // Chấp nhận cả B (1) và A (0) làm Back
                if (b == 2 || b == 3) pressed_menu = true;     // Chấp nhận cả X (2) và Y (3) làm Menu
                if (b == 6 || b == 13) pressed_up = true;      // Hỗ trợ L2 (6) hoặc DPad Up dạng Button (13)
                if (b == 7 || b == 14) pressed_down = true;    // Hỗ trợ R2 (7) hoặc DPad Down dạng Button (14)
            } else if (event.type == SDL_JOYHATMOTION) {
                if (event.jhat.value & SDL_HAT_UP) pressed_up = true;
                if (event.jhat.value & SDL_HAT_DOWN) pressed_down = true;
            } else if (event.type == SDL_JOYAXISMOTION) {
                if (event.jaxis.axis == 1) { // Left Analog Y axis
                    if (event.jaxis.value < -16000) pressed_up = true;
                    if (event.jaxis.value > 16000) pressed_down = true;
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) pressed_up = true;
                if (event.key.keysym.sym == SDLK_DOWN) pressed_down = true;
                if (event.key.keysym.sym == SDLK_RETURN) pressed_validate = true;
                if (event.key.keysym.sym == SDLK_ESCAPE) pressed_back = true;
                if (event.key.keysym.sym == SDLK_m || event.key.keysym.sym == SDLK_x) pressed_menu = true;
            }

            if (pressed_menu) {
                show_menu = !show_menu;
                menu_selection = 0;
            }
            else if (show_menu) {
                // Sửa lỗi debounce đơn giản, chỉ nhận 1 lần khi phím thả ra (KEYDOWN / BUTTONDOWN là nhạy, ta có thể để nguyên hoặc thêm delay nhỏ)
                // Tuy nhiên SDL_PollEvent chạy liên tục nên ta chỉ lấy sự kiện DOWN
                if (pressed_up) menu_selection = (menu_selection == 1) ? 0 : 1;
                if (pressed_down) menu_selection = (menu_selection == 0) ? 1 : 0;
                
                // Ở đây ta có 1 trick: Nếu show_menu đang bật, nhấn Back để tắt.
                // Nhưng A/B đều nhận là Validate/Back, vậy ta quy ước: nếu ấn Y/X (Menu) thì tắt menu.
                // Nếu ấn A hoặc B thì nó sẽ chọn chức năng. Ta sẽ chỉ lấy Validate để kích hoạt.
                if (pressed_validate) { 
                    if (menu_selection == 1) quit = true; 
                    if (menu_selection == 0) mute = !mute;
                    show_menu = false;
                }
            }
            else {
                if (pressed_validate) { 
                    is_pinging = !is_pinging;
                    if (is_pinging && !thread_running) {
                        std::thread t(ping_thread);
                        t.detach();
                    }
                }
            }
        }

        if(rand() % 4 == 0) {
            telemetry.erase(telemetry.begin());
            telemetry.push_back(generateHex());
        }

        SDL_SetRenderDrawColor(g_renderer, bgGray.r, bgGray.g, bgGray.b, 255);
        SDL_RenderClear(g_renderer);

        DrawCyberpunkHUD(g_renderer, neonCyan, neonMagenta);

        font.renderText(g_renderer, 30, 32, ">>> NETWORK DIAGNOSTIC CORE", neonCyan);
        font.renderText(g_renderer, SCREEN_WIDTH - 200, 32, "SYS: ACTIVE", neonMagenta);

        std::vector<std::string> logs = tail_log("/tmp/wifi_log.txt", (SCREEN_HEIGHT - 130)/LINE_HEIGHT);
        int y = 60;
        for (size_t i = 0; i < logs.size(); ++i) {
            SDL_Color logColor = neonCyan;
            if (logs[i].find("timeout") != std::string::npos || logs[i].find("Unreachable") != std::string::npos) logColor = redError;
            if (logs[i].find("statistics") != std::string::npos) logColor = neonYellow;
            fontMono.renderText(g_renderer, 25, y, logs[i], logColor);
            
            if (i == logs.size() - 1 && is_pinging) {
                if ((SDL_GetTicks() / 300) % 2 == 0) {
                    fontMono.renderText(g_renderer, 25 + logs[i].length() * 10, y, "\xDB", neonMagenta); 
                }
            }
            y += LINE_HEIGHT;
        }

        if (!is_pinging && logs.size() > 0) {
            if ((SDL_GetTicks() / 300) % 2 == 0) {
                 fontMono.renderText(g_renderer, 25, y, "\xDB", neonMagenta);
            }
        }

        int tel_y = 60;
        for(size_t i=0; i<telemetry.size(); i++) {
            SDL_Color tColor = neonCyan;
            tColor.a = 100 + (i * 155 / telemetry.size()); 
            fontSmall.renderText(g_renderer, SCREEN_WIDTH - 70 + 10, tel_y, telemetry[i], tColor);
            tel_y += 18;
        }

        if (!show_menu) {
            std::string status = is_pinging ? "[TX/RX: TRANSMITTING]" : "[TX/RX: STANDBY]";
            font.renderText(g_renderer, 30, SCREEN_HEIGHT - 22, status, neonMagenta);
            font.renderText(g_renderer, SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT - 22, "[A] TOGGLE PING", white);
            font.renderText(g_renderer, SCREEN_WIDTH - 160, SCREEN_HEIGHT - 22, "[X] SYS MENU", white);
        } else {
            SDL_Rect menuRect = { 20, SCREEN_HEIGHT - 70, SCREEN_WIDTH - 100, 50 };
            SDL_SetRenderDrawColor(g_renderer, bgGray.r, bgGray.g, bgGray.b, 240);
            SDL_RenderFillRect(g_renderer, &menuRect);
            DrawGlowLine(g_renderer, menuRect.x, menuRect.y, menuRect.x+menuRect.w, menuRect.y, neonMagenta);

            font.renderText(g_renderer, 30, SCREEN_HEIGHT - 35, "OPTIONS:", neonYellow);
            
            for (int i=0; i<2; i++) {
                int item_x = 150 + i * 180;
                std::string text = (i == 0) ? (mute ? "[1] UNMUTE" : "[1] MUTE") : "[2] SHUTDOWN";
                
                if (menu_selection == i) {
                    SDL_Rect hl = { item_x - 5, SCREEN_HEIGHT - 55, 130, 30 };
                    SDL_SetRenderDrawColor(g_renderer, neonMagenta.r, neonMagenta.g, neonMagenta.b, 255);
                    SDL_RenderFillRect(g_renderer, &hl);
                    font.renderText(g_renderer, item_x, SCREEN_HEIGHT - 35, text, bgGray); 
                } else {
                    font.renderText(g_renderer, item_x, SCREEN_HEIGHT - 35, text, neonCyan);
                }
            }
        }

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
