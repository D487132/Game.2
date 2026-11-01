#include "game.hpp"
#include "renderer.hpp"

#include <SDL.h>

#include <algorithm>
#include <iostream>
#include <string>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL could not initialize: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_Window* window = SDL_CreateWindow(
        "Crumble Rush", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        480, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, 480, 800);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    char* prefPath = SDL_GetPrefPath("CrumbleCorner", "CrumbleRush");
    const std::string savePath = prefPath ? std::string(prefPath) + "save.dat" : "crumble-rush-save.dat";
    SDL_free(prefPath);

    crumble::Monetization monetization;
    crumble::Game game(savePath, monetization);
    crumble::Renderer view(renderer);
    bool running = true;
    Uint64 previous = SDL_GetPerformanceCounter();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                if (game.scene() == crumble::Scene::Rush) {
                    continue;
                }
                if (game.scene() == crumble::Scene::Wardrobe || game.scene() == crumble::Scene::Keepsakes || game.scene() == crumble::Scene::Results) {
                    game.handleEvent(event);
                } else {
                    running = false;
                }
            } else {
                game.handleEvent(event);
            }
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
        const float delta = frequency > 0.0 ? static_cast<float>((now - previous) / frequency) : 0.0F;
        previous = now;
        game.update(std::min(delta, 0.1F));
        view.draw(game);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
