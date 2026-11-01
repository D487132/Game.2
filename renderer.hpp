#pragma once

#include "game.hpp"

#include <SDL.h>

#include <vector>

namespace crumble {

class Renderer {
public:
    explicit Renderer(SDL_Renderer* renderer) : renderer_(renderer) {}
    void draw(const Game& game);

private:
    void background();
    void panel(SDL_Rect rect, SDL_Color fill, SDL_Color border, int radius = 18);
    void button(SDL_Rect rect, const std::string& label, bool enabled, SDL_Color accent);
    void text(const std::string& value, int x, int y, int scale, SDL_Color color, bool centered = false);
    void label(const std::string& value, int x, int y, int scale, SDL_Color color, bool centered = false);
    void drawCookie(int cx, int cy, int radius, int skin, bool golden, float pulse);
    void particles(int cx, int cy, int count, SDL_Color color);
    void bakery(const Game& game);
    void rush(const Game& game);
    void results(const Game& game);
    void wardrobe(const Game& game);
    void keepsakes(const Game& game);
    struct Particle { float x, y, vx, vy, life, maxLife; int radius; SDL_Color color; };
    SDL_Renderer* renderer_;
    std::vector<Particle> particles_;
    Uint32 ticks_ = 0;
    Uint32 lastTicks_ = 0;
    int lastFeedback_ = -1;
    float shakeSeconds_ = 0.0F;
    int shakeX_ = 0;
    int shakeY_ = 0;
};

} // namespace crumble
