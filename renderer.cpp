#include "renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace crumble {
namespace {

constexpr SDL_Color kInk{51, 29, 34, 255};
constexpr SDL_Color kCream{255, 242, 215, 255};
constexpr SDL_Color kMuted{202, 174, 155, 255};
constexpr SDL_Color kPanel{77, 43, 46, 245};
constexpr SDL_Color kLine{143, 87, 73, 255};
constexpr SDL_Color kGold{255, 202, 103, 255};
constexpr SDL_Color kMint{130, 221, 184, 255};
constexpr SDL_Color kCoral{248, 126, 133, 255};

void color(SDL_Renderer* renderer, SDL_Color value) {
    SDL_SetRenderDrawColor(renderer, value.r, value.g, value.b, value.a);
}

void fillCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color value) {
    if (radius <= 0) return;
    color(renderer, value);
    for (int y = -radius; y <= radius; ++y) {
        const int x = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(renderer, cx - x, cy + y, cx + x, cy + y);
    }
}

void outlineCircle(SDL_Renderer* renderer, int cx, int cy, int radius, SDL_Color value, int thickness = 2) {
    color(renderer, value);
    constexpr float tau = 6.28318530718F;
    const int steps = std::max(48, radius * 8);
    for (int ring = 0; ring < thickness; ++ring) {
        const float r = static_cast<float>(radius - ring);
        for (int i = 0; i < steps; ++i) {
            const float angle = tau * static_cast<float>(i) / static_cast<float>(steps);
            SDL_RenderDrawPoint(renderer, cx + static_cast<int>(std::cos(angle) * r),
                                cy + static_cast<int>(std::sin(angle) * r));
        }
    }
}

void fillRound(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color value) {
    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    color(renderer, value);
    SDL_Rect middle{rect.x + radius, rect.y, rect.w - radius * 2, rect.h};
    SDL_RenderFillRect(renderer, &middle);
    SDL_Rect sides{rect.x, rect.y + radius, rect.w, rect.h - radius * 2};
    SDL_RenderFillRect(renderer, &sides);
    fillCircle(renderer, rect.x + radius, rect.y + radius, radius, value);
    fillCircle(renderer, rect.x + rect.w - radius - 1, rect.y + radius, radius, value);
    fillCircle(renderer, rect.x + radius, rect.y + rect.h - radius - 1, radius, value);
    fillCircle(renderer, rect.x + rect.w - radius - 1, rect.y + rect.h - radius - 1, radius, value);
}

std::array<std::uint8_t, 7> glyph(char ch) {
    switch (ch) {
        case 'A': return {14,17,17,31,17,17,17}; case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {14,17,16,16,16,17,14}; case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31}; case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {14,17,16,23,17,17,15}; case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {31,4,4,4,4,4,31}; case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17}; case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17}; case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14}; case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13}; case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30}; case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14}; case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10}; case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4}; case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31}; case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2}; case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14}; case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14}; case '9': return {14,17,17,15,1,1,14};
        case ':': return {0,4,4,0,4,4,0}; case '.': return {0,0,0,0,0,12,12};
        case ',': return {0,0,0,0,4,4,8}; case '!': return {4,4,4,4,4,0,4};
        case '?': return {14,17,1,2,4,0,4}; case '-': return {0,0,0,31,0,0,0};
        case '+': return {0,4,4,31,4,4,0}; case '/': return {1,2,2,4,8,8,16};
        case '%': return {17,2,4,8,17,0,0}; case ''': return {4,4,8,0,0,0,0};
        default: return {0,0,0,0,0,0,0};
    }
}

SDL_Color skinColor(int skin) {
    switch (skin) {
        case 1: return {240, 119, 150, 255};
        case 2: return {145, 137, 231, 255};
        case 3: return {116, 204, 165, 255};
        default: return {201, 119, 70, 255};
    }
}

} // namespace

void Renderer::text(const std::string& value, int x, int y, int scale, SDL_Color ink, bool centered) {
    std::string upper = value;
    for (char& c : upper) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    const int width = upper.empty() ? 0 : static_cast<int>(upper.size()) * 6 * scale - scale;
    if (centered) x -= width / 2;
    color(renderer_, ink);
    for (std::size_t index = 0; index < upper.size(); ++index) {
        const auto rows = glyph(upper[index]);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((rows[static_cast<std::size_t>(row)] & (1U << (4 - col))) == 0) continue;
                SDL_Rect pixel{x + static_cast<int>(index) * 6 * scale + col * scale,
                               y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer_, &pixel);
            }
        }
    }
}

void Renderer::label(const std::string& value, int x, int y, int scale, SDL_Color ink, bool centered) {
    text(value, x, y, scale, ink, centered);
}

void Renderer::panel(SDL_Rect rect, SDL_Color fill, SDL_Color border, int radius) {
    SDL_Rect shadow{rect.x, rect.y + 5, rect.w, rect.h};
    fillRound(renderer_, shadow, radius, {25, 19, 27, 100});
    fillRound(renderer_, rect, radius, fill);
    color(renderer_, border);
    SDL_Rect inner{rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
    SDL_RenderDrawRect(renderer_, &inner);
    SDL_Rect shine{rect.x + radius, rect.y + 2, std::max(0, rect.w - 2 * radius), 2};
    SDL_RenderFillRect(renderer_, &shine);
}

void Renderer::button(SDL_Rect rect, const std::string& labelText, bool enabled, SDL_Color accent) {
    const SDL_Color face = enabled ? SDL_Color{103, 59, 52, 255} : SDL_Color{64, 48, 50, 255};
    panel(rect, face, enabled ? accent : SDL_Color{92, 74, 73, 255}, 15);
    if (enabled) {
        SDL_Rect stripe{rect.x + 2, rect.y + 2, rect.w - 4, 5};
        fillRound(renderer_, stripe, 3, {accent.r, accent.g, accent.b, 120});
    }
    const int scale = static_cast<int>(labelText.size()) * 12 > rect.w - 20 ? 1 : 2;
    text(labelText, rect.x + rect.w / 2, rect.y + (rect.h - 7 * scale) / 2 - 1,
         scale, enabled ? kCream : kMuted, true);
}

void Renderer::background() {
    for (int y = 0; y < 800; ++y) {
        const float t = static_cast<float>(y) / 800.0F;
        SDL_Color shade{
            static_cast<Uint8>(31 + 27 * t),
            static_cast<Uint8>(22 + 19 * t),
            static_cast<Uint8>(34 + 15 * t), 255};
        color(renderer_, shade);
        SDL_RenderDrawLine(renderer_, 0, y, 480, y);
    }
    const int drift = static_cast<int>((ticks_ / 70) % 48);
    for (int row = -1; row < 19; ++row) {
        for (int col = 0; col < 12; ++col) {
            const int x = 17 + col * 43 + (row % 2) * 20;
            const int y = 20 + row * 47 + drift;
            const Uint8 alpha = static_cast<Uint8>(16 + ((row * 13 + col * 7) % 20));
            fillCircle(renderer_, x, y, 1 + (col % 3 == 0), {244, 198, 148, alpha});
        }
    }
    fillCircle(renderer_, 420, 95, 92, {153, 85, 79, 18});
    fillCircle(renderer_, 44, 655, 123, {105, 119, 147, 14});
}

void Renderer::drawCookie(int cx, int cy, int radius, int skin, bool golden, float pulse) {
    const SDL_Color dough = skinColor(skin);
    const float wobble = 1.0F + std::sin(pulse * 7.0F) * 0.012F;
    const int r = static_cast<int>(static_cast<float>(radius) * wobble);
    if (golden) {
        outlineCircle(renderer_, cx, cy, r + 17 + static_cast<int>(std::sin(pulse * 5.0F) * 3.0F), {255, 209, 102, 120}, 4);
        outlineCircle(renderer_, cx, cy, r + 8, {255, 222, 142, 190}, 2);
    }
    fillCircle(renderer_, cx + 1, cy + 9, r + 2, {25, 18, 24, 130});
    fillCircle(renderer_, cx, cy, r, dough);
    outlineCircle(renderer_, cx, cy, r, {255, 225, 184, 215}, 4);
    fillCircle(renderer_, cx - r / 3, cy - r / 3, r / 4, {255, 230, 184, 92});

    const SDL_Color chip = skin == 2 ? SDL_Color{62, 49, 94, 255} : SDL_Color{76, 38, 36, 255};
    constexpr std::array<std::array<int, 2>, 9> chips{{
        {{-47,-42}},{{13,-55}},{{51,-16}},{{-12,-8}},{{45,31}},{{-48,40}},{{4,52}},{{-62,2}},{{25,3}}}};
    for (const auto& point : chips) {
        const int size = std::max(4, radius / 16 + (point[0] % 2 == 0 ? 1 : 0));
        fillCircle(renderer_, cx + point[0] * radius / 110, cy + point[1] * radius / 110, size, chip);
        fillCircle(renderer_, cx + point[0] * radius / 110 - 2, cy + point[1] * radius / 110 - 2, std::max(1, size / 3), {255, 215, 160, 130});
    }
}

void Renderer::particles(int cx, int cy, int count, SDL_Color accent) {
    for (int i = 0; i < count; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(count)) * 6.2831853F + (i % 2) * 0.19F;
        const float speed = 55.0F + static_cast<float>((i * 37) % 95);
        particles_.push_back({static_cast<float>(cx), static_cast<float>(cy),
                              std::cos(angle) * speed, std::sin(angle) * speed - 35.0F,
                              0.55F + static_cast<float>(i % 4) * 0.09F,
                              0.55F + static_cast<float>(i % 4) * 0.09F,
                              2 + i % 4, accent});
    }
}

void Renderer::bakery(const Game& game) {
    const Profile& p = game.profile();
    text("CRUMBLE", 240, 37, 4, kCream, true);
    text("RUSH", 240, 72, 2, kGold, true);
    text("A LITTLE BAKERY ARCADE", 240, 96, 1, kMuted, true);

    panel({42, 122, 396, 36}, {75, 47, 47, 230}, {140, 90, 75, 255}, 12);
    text("COOKIES " + std::to_string(p.cookies), 55, 136, 1, kCream);
    text("BEST " + std::to_string(p.bestScore), 263, 136, 1, kGold);
    text("GOLDEN CRUMBS " + std::to_string(p.prestige), 55, 165, 1, {255, 211, 123, 255});
    int badgeCount = 0;
    for (int bits = p.achievementMask; bits != 0; bits &= bits - 1) ++badgeCount;
    text("BADGES " + std::to_string(badgeCount) + "/6", 298, 165, 1, kMint);

    panel({38, 187, 404, 83}, {54, 39, 54, 246}, game.festival().accent, 17);
    fillCircle(renderer_, 414, 207, 5, game.festival().accent);
    text("TODAY'S FESTIVAL", 55, 199, 1, game.festival().accent);
    text(game.festival().name, 55, 214, 2, kCream);
    text(game.festival().line, 55, 234, 1, kMuted);
    text(std::to_string(p.festivalTaps) + "/50 TAPS  -  " + std::to_string(p.eventBerries) + " BERRIES", 55, 246, 1, kMuted);
    SDL_Rect barBg{55, 260, 347, 5};
    fillRound(renderer_, barBg, 3, {44, 33, 43, 255});
    SDL_Rect bar{55, 260, 347 * std::min(p.festivalTaps, 50) / 50, 5};
    if (bar.w > 0) fillRound(renderer_, bar, 3, game.festival().accent);

    button({54, 287, 372, 76}, "START 60 SECOND RUSH", true, kGold);
    const bool dailyReady = p.lastDailyDay != p.festivalDay;
    const std::string dailyText = dailyReady ? "COLLECT DAILY PASTRY  +20" : "DAILY PASTRY COLLECTED";
    button({54, 377, 372, 50}, dailyText, true, dailyReady ? kMint : kGold);

    text("BAKERY UPGRADES", 45, 456, 1, kMuted);
    const std::array<std::string, 3> names{"TAP POWER", "COMBO MASTERY", "GOLDEN OVEN"};
    const std::array<int, 3> levels{p.tapLevel, p.comboLevel, p.ovenLevel};
    for (int i = 0; i < 3; ++i) {
        const int y = 484 + i * 66;
        panel({40, y, 400, 58}, {67, 43, 46, 242}, {117, 75, 64, 255}, 13);
        const SDL_Color accent = i == 0 ? kCoral : i == 1 ? kMint : kGold;
        fillCircle(renderer_, 68, y + 29, 13, accent);
        text(std::to_string(i + 1), 68, y + 23, 1, kInk, true);
        text(names[static_cast<std::size_t>(i)] + "  LV " + std::to_string(levels[static_cast<std::size_t>(i)]), 92, y + 12, 1, kCream);
        const bool affordable = p.cookies >= game.upgradeCost(i);
        text(std::to_string(game.upgradeCost(i)) + " COOKIES", 92, y + 31, 1, affordable ? kGold : kMuted);
        text("BUY", 400, y + 23, 1, affordable ? kMint : kMuted, true);
    }
    const bool rebirthReady = game.canRebirth();
    const std::string rebirthText = rebirthReady ? ("FRESH START  +" + std::to_string(std::max(1, p.cookies / 500)) + " GOLDEN CRUMBS") : "FRESH START  AT 500 COOKIES";
    button({54, 686, 372, 48}, rebirthText, rebirthReady, {255, 213, 129, 255});
    button({54, 742, 372, 44}, "COOKIE WARDROBE", true, {172, 157, 242, 255});
    if (game.messageSeconds() > 0) {
        panel({42, 160, 396, 22}, {50, 38, 44, 244}, {203, 149, 111, 255}, 8);
        text(game.message(), 240, 168, 1, kCream, true);
    }
}

void Renderer::rush(const Game& game) {
    const float time = game.timeLeft();
    text("BAKE!", 240, 44, 3, kCream, true);
    text(std::to_string(static_cast<int>(std::ceil(time))) + " SEC", 240, 94, 3, time < 10.0F ? kCoral : kGold, true);
    panel({55, 137, 370, 14}, {61, 44, 48, 255}, {145, 96, 79, 255}, 7);
    SDL_Rect timeBar{58, 140, static_cast<int>(364.0F * time / 60.0F), 8};
    if (timeBar.w > 0) fillRound(renderer_, timeBar, 4, time < 10.0F ? kCoral : kGold);
    text("SCORE", 240, 179, 1, kMuted, true);
    text(std::to_string(game.score()), 240, 198, 4, kCream, true);
    const int combo = game.combo();
    text(combo > 1 ? ("COMBO X" + std::to_string(combo)) : "KEEP THE RHYTHM", 240, 244, 1, combo >= 10 ? kMint : kMuted, true);
    panel({90, 266, 300, 9}, {53, 41, 47, 255}, {103, 80, 73, 255}, 5);
    SDL_Rect comboBar{93, 269, static_cast<int>(294.0F * std::clamp(game.comboWindow() / 1.8F, 0.0F, 1.0F)), 3};
    if (comboBar.w > 0) fillRound(renderer_, comboBar, 2, kMint);
    const int bounce = static_cast<int>(std::sin(static_cast<float>(ticks_) / 180.0F) * 3.0F);
    drawCookie(240, 430 + bounce, 135, game.profile().selectedSkin, game.goldenActive(), static_cast<float>(ticks_) / 1000.0F);
    if (game.goldenActive()) {
        text("GOLDEN OVEN!  +BONUS", 240, 598, 1, kGold, true);
        text("WINDOW " + std::to_string(static_cast<int>(std::ceil(game.goldenSeconds()))) + " SEC", 240, 615, 1, kMuted, true);
    } else {
        text("TAP THE COOKIE OR PRESS SPACE", 240, 605, 1, kCream, true);
    }
    panel({48, 654, 384, 70}, {65, 43, 48, 244}, {120, 77, 69, 255}, 16);
    text("EACH TAP BUILDS YOUR COMBO", 240, 669, 1, kMuted, true);
    text("POWER " + std::to_string(game.tapPower()) + "   FESTIVAL +" + std::to_string(game.festival().dailyBonus), 240, 692, 1, kCream, true);
    if (game.messageSeconds() > 0.0F) {
        panel({48, 738, 384, 34}, {48, 36, 43, 242}, {184, 120, 91, 255}, 11);
        text(game.message(), 240, 751, 1, kCream, true);
    }
}

void Renderer::results(const Game& game) {
    const Profile& p = game.profile();
    text("FRESH FROM THE OVEN", 240, 82, 2, kGold, true);
    drawCookie(240, 223, 73, p.selectedSkin, false, static_cast<float>(ticks_) / 1000.0F);
    panel({48, 323, 384, 158}, {73, 45, 49, 247}, {190, 129, 91, 255}, 20);
    text("YOUR BATCH", 240, 343, 1, kMuted, true);
    text(std::to_string(game.score()), 240, 367, 4, kCream, true);
    text("BEST " + std::to_string(p.bestScore) + "   TOP COMBO " + std::to_string(game.bestCombo()), 240, 420, 1, kMint, true);
    text("REWARD  +" + std::to_string(game.reward()) + " COOKIES", 240, 452, 1, kGold, true);
    button({54, 514, 372, 66}, "BAKE ANOTHER BATCH", true, kGold);
    button({54, 598, 372, 62}, game.adClaimed() ? "REWARD VIDEO CLAIMED" : game.adPending() ? "WAITING FOR VIDEO..." : "OPTIONAL VIDEO  -  DOUBLE REWARD", !game.adClaimed() && !game.adPending(), kMint);
    button({54, 680, 372, 54}, "BACK TO BAKERY", true, {180, 157, 242, 255});
    if (game.messageSeconds() > 0.0F) {
        panel({48, 746, 384, 32}, {48, 36, 43, 242}, {184, 120, 91, 255}, 11);
        text(game.message(), 240, 758, 1, kCream, true);
    }
}

void Renderer::wardrobe(const Game& game) {
    const Profile& p = game.profile();
    text("COOKIE WARDROBE", 240, 73, 3, kCream, true);
    text("FESTIVAL BERRIES  " + std::to_string(p.eventBerries), 240, 112, 1, kGold, true);
    constexpr std::array<const char*, 4> names{"CLASSIC CHIP", "BERRY SWIRL", "MOON SPRINKLE", "MINT CRUNCH"};
    constexpr std::array<const char*, 4> prices{"FREE", "10 BERRIES", "25 BERRIES", "50 BERRIES"};
    for (int i = 0; i < 4; ++i) {
        const int y = 176 + i * 123;
        const bool owned = (p.ownedSkinsMask & (1 << i)) != 0;
        const bool selected = p.selectedSkin == i;
        const SDL_Color accent = skinColor(i);
        panel({38, y, 404, 108}, {73, 44, 49, 246}, accent, 17);
        drawCookie(97, y + 52, 33, i, false, static_cast<float>(ticks_) / 1000.0F);
        text(names[static_cast<std::size_t>(i)], 151, y + 25, 1, kCream);
        text(selected ? "EQUIPPED" : owned ? "OWNED" : prices[static_cast<std::size_t>(i)], 151, y + 53, 1, selected ? kMint : kMuted);
        text(selected ? "ON" : owned ? "WEAR" : "UNLOCK", 374, y + 45, 1, selected ? kMint : kGold, true);
    }
    button({54, 724, 372, 54}, "BACK TO THE BAKERY", true, {180, 157, 242, 255});
    if (game.messageSeconds() > 0.0F) {
        panel({48, 670, 384, 33}, {48, 36, 43, 242}, {184, 120, 91, 255}, 11);
        text(game.message(), 240, 682, 1, kCream, true);
    }
}

void Renderer::keepsakes(const Game& game) {
    constexpr std::array<const char*, 6> names{
        "FIRST BATCH", "COOKIE MOUNTAIN", "REGULAR BAKER", "COMBO STAR", "FRESH START", "COLOR COLLECTOR"};
    constexpr std::array<const char*, 6> goals{
        "BAKE 100 TOTAL POINTS", "SCORE 250 IN ONE RUSH", "FINISH 10 RUSHES",
        "REACH A COMBO OF 20", "REBIRTH YOUR BAKERY", "UNLOCK THREE STYLES"};
    text("BAKERY KEEPSAKES", 240, 74, 3, kCream, true);
    text("EACH KEEPSAKE GIVES 30 COOKIES", 240, 113, 1, kMuted, true);
    for (int i = 0; i < 6; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const SDL_Rect rect{38 + col * 205, 166 + row * 172, 199, 150};
        const bool earned = (game.profile().achievementMask & (1 << i)) != 0;
        const SDL_Color edge = earned ? kMint : SDL_Color{111, 78, 72, 255};
        panel(rect, earned ? SDL_Color{61, 68, 53, 248} : SDL_Color{67, 43, 48, 244}, edge, 16);
        fillCircle(renderer_, rect.x + rect.w / 2, rect.y + 35, 15, earned ? kGold : SDL_Color{112, 91, 86, 255});
        text(std::to_string(i + 1), rect.x + rect.w / 2, rect.y + 29, 1, kInk, true);
        text(names[static_cast<std::size_t>(i)], rect.x + rect.w / 2, rect.y + 61, 1, earned ? kCream : kMuted, true);
        text(goals[static_cast<std::size_t>(i)], rect.x + rect.w / 2, rect.y + 88, 1, earned ? kMint : kMuted, true);
        text(earned ? "COLLECTED  +30" : "NOT YET", rect.x + rect.w / 2, rect.y + 119, 1, earned ? kGold : kMuted, true);
    }
    button({54, 724, 372, 54}, "BACK TO THE BAKERY", true, {180, 157, 242, 255});
}

void Renderer::draw(const Game& game) {
    const Uint32 now = SDL_GetTicks();
    const float dt = lastTicks_ == 0 ? 0.016F : std::min(0.05F, static_cast<float>(now - lastTicks_) / 1000.0F);
    ticks_ = now;
    lastTicks_ = now;
    if (game.feedbackId() != lastFeedback_) {
        const SDL_Color spark = game.goldenActive() ? kGold : skinColor(game.profile().selectedSkin);
        particles(240, 430, 14, spark);
        if (game.goldenActive()) shakeSeconds_ = 0.11F;
        lastFeedback_ = game.feedbackId();
    }
    for (auto& p : particles_) {
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 125.0F * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& p) { return p.life <= 0.0F; }), particles_.end());
    if (shakeSeconds_ > 0.0F) {
        shakeSeconds_ = std::max(0.0F, shakeSeconds_ - dt);
        shakeX_ = (std::rand() % 9) - 4;
        shakeY_ = (std::rand() % 9) - 4;
    } else {
        shakeX_ = shakeY_ = 0;
    }

    color(renderer_, {31, 22, 34, 255});
    SDL_RenderClear(renderer_);
    SDL_Rect viewport{shakeX_, shakeY_, 480, 800};
    SDL_RenderSetViewport(renderer_, &viewport);
    background();
    switch (game.scene()) {
        case Scene::Bakery: bakery(game); break;
        case Scene::Rush: rush(game); break;
        case Scene::Results: results(game); break;
        case Scene::Wardrobe: wardrobe(game); break;
        case Scene::Keepsakes: keepsakes(game); break;
    }
    for (const auto& p : particles_) {
        const Uint8 alpha = static_cast<Uint8>(255.0F * std::clamp(p.life / p.maxLife, 0.0F, 1.0F));
        fillCircle(renderer_, static_cast<int>(p.x), static_cast<int>(p.y), p.radius, {p.color.r, p.color.g, p.color.b, alpha});
    }
    SDL_RenderSetViewport(renderer_, nullptr);
    SDL_RenderPresent(renderer_);
}

} // namespace crumble
