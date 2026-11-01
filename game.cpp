#include "game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <utility>

namespace crumble {
namespace {

constexpr float kRunLength = 60.0F;
constexpr int kFestivalGoal = 50;
constexpr int kRebirthPrice = 500;
constexpr std::array<const char*, 4> kFestivalNames{
    "BERRY PICNIC", "COCOA CARNIVAL", "HONEY HARVEST", "MOONLIGHT BAKEOFF"};
constexpr std::array<const char*, 4> kFestivalLines{
    "Berry sprinkles add a little extra to each tap.",
    "Chocolate chips are flying. Every tap gets a bonus.",
    "Golden oven windows stay open a little longer.",
    "Keep your rhythm: combos build more quickly today."};
constexpr std::array<const char*, 4> kFestivalIcons{"BERRY", "COCOA", "HONEY", "MOON"};
constexpr std::array<SDL_Color, 4> kFestivalColors{{
    {245, 126, 154, 255}, {192, 133, 96, 255}, {241, 195, 93, 255}, {159, 151, 255, 255}}};

int clampStat(int value, int low = 0, int high = 1000000000) {
    return std::clamp(value, low, high);
}

} // namespace

Game::Game(std::string savePath, Monetization& monetization)
    : savePath_(std::move(savePath)), monetization_(monetization) {
    load();
    refreshFestival();
}

int Game::dayId() const {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    return (utc.tm_year + 1900) * 400 + utc.tm_yday;
}

void Game::refreshFestival() {
    const int day = dayId();
    const int index = ((day % static_cast<int>(kFestivalNames.size())) + static_cast<int>(kFestivalNames.size())) % static_cast<int>(kFestivalNames.size());
    festival_ = {kFestivalNames[static_cast<std::size_t>(index)],
                 kFestivalLines[static_cast<std::size_t>(index)],
                 kFestivalIcons[static_cast<std::size_t>(index)],
                 kFestivalColors[static_cast<std::size_t>(index)], index + 1};
    if (profile_.festivalDay != day) {
        profile_.festivalDay = day;
        profile_.festivalTaps = 0;
        profile_.festivalClaimed = false;
        save();
    }
}

void Game::load() {
    std::ifstream file(savePath_);
    int version = 0;
    if (!(file >> version) || version != 1) {
        return;
    }
    int claimed = 0;
    file >> profile_.cookies >> profile_.eventBerries >> profile_.bestScore
         >> profile_.lifetimeScore >> profile_.runs >> profile_.prestige
         >> profile_.tapLevel >> profile_.comboLevel >> profile_.ovenLevel
         >> profile_.achievementMask >> profile_.ownedSkinsMask
         >> profile_.selectedSkin >> profile_.lastDailyDay >> profile_.festivalDay
         >> profile_.festivalTaps >> claimed;
    if (!file) {
        profile_ = Profile{};
        return;
    }
    profile_.cookies = clampStat(profile_.cookies);
    profile_.eventBerries = clampStat(profile_.eventBerries);
    profile_.bestScore = clampStat(profile_.bestScore);
    profile_.lifetimeScore = clampStat(profile_.lifetimeScore);
    profile_.runs = clampStat(profile_.runs);
    profile_.prestige = clampStat(profile_.prestige, 0, 1000000);
    profile_.tapLevel = clampStat(profile_.tapLevel, 0, 1000);
    profile_.comboLevel = clampStat(profile_.comboLevel, 0, 1000);
    profile_.ovenLevel = clampStat(profile_.ovenLevel, 0, 1000);
    profile_.achievementMask = clampStat(profile_.achievementMask, 0, 63);
    profile_.ownedSkinsMask = clampStat(profile_.ownedSkinsMask, 1, 15) | 1;
    profile_.selectedSkin = clampStat(profile_.selectedSkin, 0, 3);
    if ((profile_.ownedSkinsMask & (1 << profile_.selectedSkin)) == 0) {
        profile_.selectedSkin = 0;
    }
    profile_.festivalTaps = clampStat(profile_.festivalTaps, 0, kFestivalGoal);
    profile_.festivalClaimed = claimed != 0;
}

void Game::save() const {
    try {
        const std::filesystem::path path(savePath_);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        const std::string temporary = savePath_ + ".tmp";
        {
            std::ofstream file(temporary, std::ios::trunc);
            if (!file) return;
            file << "1\n" << profile_.cookies << ' ' << profile_.eventBerries << ' '
                 << profile_.bestScore << ' ' << profile_.lifetimeScore << ' '
                 << profile_.runs << ' ' << profile_.prestige << ' '
                 << profile_.tapLevel << ' ' << profile_.comboLevel << ' '
                 << profile_.ovenLevel << ' ' << profile_.achievementMask << ' '
                 << profile_.ownedSkinsMask << ' ' << profile_.selectedSkin << ' '
                 << profile_.lastDailyDay << ' ' << profile_.festivalDay << ' '
                 << profile_.festivalTaps << ' ' << (profile_.festivalClaimed ? 1 : 0) << '\n';
            if (!file) return;
        }
        std::error_code error;
        std::filesystem::rename(temporary, path, error);
        if (error) {
            std::filesystem::remove(path, error);
            error.clear();
            std::filesystem::rename(temporary, path, error);
        }
    } catch (...) {
        // A save failure should not interrupt a game session.
    }
}

void Game::renderMessage(std::string text) {
    message_ = std::move(text);
    messageSeconds_ = 2.8F;
}

int Game::tapPower() const {
    return 1 + profile_.tapLevel;
}

float Game::scoreMultiplier() const {
    return 1.0F + static_cast<float>(profile_.prestige) * 0.15F
         + static_cast<float>(profile_.ovenLevel) * 0.04F;
}

int Game::upgradeCost(int index) const {
    switch (index) {
        case 0: return 15 + profile_.tapLevel * 22;
        case 1: return 30 + profile_.comboLevel * 38;
        case 2: return 55 + profile_.ovenLevel * 65;
        default: return 0;
    }
}

int Game::skinCost(int index) const {
    constexpr std::array<int, 4> costs{0, 10, 25, 50};
    if (index < 0 || index >= static_cast<int>(costs.size())) return 0;
    return costs[static_cast<std::size_t>(index)];
}

bool Game::canRebirth() const {
    return profile_.cookies >= kRebirthPrice;
}

bool Game::hit(int x, int y, SDL_Rect rect) const {
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

void Game::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_FINGERDOWN) {
        SDL_Window* window = SDL_GetWindowFromID(event.tfinger.windowID);
        int windowWidth = 480;
        int windowHeight = 800;
        if (window) SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        const float scale = std::min(windowWidth / 480.0F, windowHeight / 800.0F);
        const float offsetX = (windowWidth - 480.0F * scale) * 0.5F;
        const float offsetY = (windowHeight - 800.0F * scale) * 0.5F;
        SDL_Event touchAsClick{};
        touchAsClick.type = SDL_MOUSEBUTTONDOWN;
        touchAsClick.button.button = SDL_BUTTON_LEFT;
        touchAsClick.button.x = static_cast<int>(std::lround((event.tfinger.x * windowWidth - offsetX) / scale));
        touchAsClick.button.y = static_cast<int>(std::lround((event.tfinger.y * windowHeight - offsetY) / scale));
        handleEvent(touchAsClick);
        return;
    }
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        const SDL_Keycode key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE || key == SDLK_BACKSPACE) {
            if (scene_ == Scene::Wardrobe || scene_ == Scene::Keepsakes || scene_ == Scene::Results) scene_ = Scene::Bakery;
            return;
        }
        if (scene_ == Scene::Rush && (key == SDLK_SPACE || key == SDLK_RETURN || key == SDLK_z)) {
            tap();
        } else if (scene_ == Scene::Bakery && key == SDLK_RETURN) {
            startRun();
        } else if (scene_ == Scene::Bakery && key == SDLK_w) {
            scene_ = Scene::Wardrobe;
        }
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return;
    const int x = event.button.x;
    const int y = event.button.y;

    if (scene_ == Scene::Bakery) {
        if (hit(x, y, {54, 287, 372, 76})) startRun();
        else if (hit(x, y, {54, 377, 372, 50})) claimDailyGift();
        else if (hit(x, y, {40, 484, 400, 58})) buyUpgrade(0);
        else if (hit(x, y, {40, 550, 400, 58})) buyUpgrade(1);
        else if (hit(x, y, {40, 616, 400, 58})) buyUpgrade(2);
        else if (hit(x, y, {54, 686, 372, 48})) rebirth();
        else if (hit(x, y, {54, 742, 372, 44})) scene_ = Scene::Wardrobe;
        else if (hit(x, y, {288, 157, 145, 22})) scene_ = Scene::Keepsakes;
    } else if (scene_ == Scene::Rush) {
        const int dx = x - 240;
        const int dy = y - 430;
        if (dx * dx + dy * dy <= 176 * 176) tap();
    } else if (scene_ == Scene::Results) {
        if (hit(x, y, {54, 514, 372, 66})) startRun();
        else if (hit(x, y, {54, 598, 372, 62})) requestRewardedVideo();
        else if (hit(x, y, {54, 680, 372, 54})) scene_ = Scene::Bakery;
    } else if (scene_ == Scene::Wardrobe) {
        if (hit(x, y, {54, 724, 372, 54})) scene_ = Scene::Bakery;
        else if (hit(x, y, {38, 288, 404, 96})) buySkin(1);
        else if (hit(x, y, {38, 397, 404, 96})) buySkin(2);
        else if (hit(x, y, {38, 506, 404, 96})) buySkin(3);
        else if (hit(x, y, {38, 615, 404, 84})) buySkin(0);
    } else if (scene_ == Scene::Keepsakes) {
        if (hit(x, y, {54, 724, 372, 54})) scene_ = Scene::Bakery;
    }
}

void Game::update(float deltaSeconds) {
    refreshFestival();
    if (messageSeconds_ > 0.0F) messageSeconds_ = std::max(0.0F, messageSeconds_ - deltaSeconds);
    if (rebirthArmSeconds_ > 0.0F) {
        rebirthArmSeconds_ = std::max(0.0F, rebirthArmSeconds_ - deltaSeconds);
        if (rebirthArmSeconds_ == 0.0F) rebirthArmed_ = false;
    }
    if (scene_ == Scene::Rush) {
        elapsed_ += deltaSeconds;
        timeLeft_ = std::max(0.0F, timeLeft_ - deltaSeconds);
        comboWindow_ = std::max(0.0F, comboWindow_ - deltaSeconds);
        const float phase = std::fmod(elapsed_, 14.0F);
        const float duration = 2.5F + static_cast<float>(profile_.ovenLevel) * 0.25F;
        goldenActive_ = phase < duration;
        goldenSeconds_ = goldenActive_ ? duration - phase : 0.0F;
        if (comboWindow_ == 0.0F) combo_ = 0;
        if (timeLeft_ == 0.0F) finishRun();
    } else {
        goldenActive_ = false;
        goldenSeconds_ = 0.0F;
    }

    for (const CommerceEvent& event : monetization_.drainEvents()) {
        if (event.kind == CommerceKind::RewardedVideo) {
            const bool wasPending = adPending_;
            adPending_ = false;
            if (event.succeeded && wasPending && scene_ == Scene::Results && !adClaimed_) {
                profile_.cookies += reward_;
                reward_ *= 2;
                adClaimed_ = true;
                renderMessage("Thanks for watching. Your run reward was doubled.");
                save();
            } else if (!event.succeeded) {
                renderMessage("The video reward did not complete.");
            }
        } else if (event.kind == CommerceKind::ProductPurchase) {
            renderMessage(event.succeeded ? "Purchase confirmed by the store." : "Purchase was not completed.");
        }
    }
}

void Game::startRun() {
    scene_ = Scene::Rush;
    score_ = 0;
    combo_ = 0;
    bestCombo_ = 0;
    reward_ = 0;
    tapCount_ = 0;
    timeLeft_ = kRunLength;
    elapsed_ = 0.0F;
    comboWindow_ = 0.0F;
    goldenSeconds_ = 2.5F;
    goldenActive_ = true;
    adPending_ = false;
    adClaimed_ = false;
    renderMessage("60 seconds. Bake your best batch!");
}

void Game::tap() {
    if (scene_ != Scene::Rush || timeLeft_ <= 0.0F) return;
    if (comboWindow_ <= 0.0F) combo_ = 0;
    ++combo_;
    ++tapCount_;
    ++feedbackId_;
    bestCombo_ = std::max(bestCombo_, combo_);
    comboWindow_ = 1.25F + static_cast<float>(profile_.comboLevel) * 0.12F;
    const float comboBoost = 1.0F + std::min(combo_, 24) * 0.025F + profile_.comboLevel * 0.04F;
    int points = std::max(1, static_cast<int>(std::lround((tapPower() + festival_.dailyBonus - 1) * comboBoost * scoreMultiplier())));
    if (goldenActive_) points += static_cast<int>(std::lround((6 + profile_.ovenLevel) * scoreMultiplier()));
    score_ += points;
    profile_.lifetimeScore += points;
    profile_.festivalTaps = std::min(kFestivalGoal, profile_.festivalTaps + 1);
    if (!profile_.festivalClaimed && profile_.festivalTaps >= kFestivalGoal) {
        profile_.festivalClaimed = true;
        profile_.eventBerries += 10;
        renderMessage("Festival goal! You earned 10 berry tokens.");
    }
    checkAchievements();
}

void Game::finishRun() {
    scene_ = Scene::Results;
    ++profile_.runs;
    profile_.bestScore = std::max(profile_.bestScore, score_);
    reward_ = std::max(5, 4 + score_ / 24 + bestCombo_ / 8);
    profile_.cookies += reward_;
    checkAchievements();
    renderMessage("Fresh batch ready. Your cookies are waiting.");
    save();
}

void Game::checkAchievements() {
    const std::array<bool, 6> checks{{
        profile_.lifetimeScore >= 100,
        profile_.bestScore >= 250,
        profile_.runs >= 10,
        bestCombo_ >= 20,
        profile_.prestige >= 1,
        (profile_.ownedSkinsMask & 0x0E) == 0x0E}};
    constexpr std::array<const char*, 6> names{
        "FIRST BATCH", "COOKIE MOUNTAIN", "REGULAR BAKER", "COMBO STAR", "FRESH START", "COLOR COLLECTOR"};
    for (int i = 0; i < static_cast<int>(checks.size()); ++i) {
        const int bit = 1 << i;
        if (checks[static_cast<std::size_t>(i)] && (profile_.achievementMask & bit) == 0) {
            profile_.achievementMask |= bit;
            profile_.cookies += 30;
            renderMessage(std::string("Keepsake earned: ") + names[static_cast<std::size_t>(i)] + "! +30 cookies.");
        }
    }
}

void Game::claimDailyGift() {
    const int today = dayId();
    if (profile_.lastDailyDay == today) {
        renderMessage("Your daily pastry is already collected.");
        return;
    }
    profile_.lastDailyDay = today;
    profile_.cookies += 20;
    renderMessage("Daily pastry collected. +20 cookies!");
    checkAchievements();
    save();
}

void Game::buyUpgrade(int index) {
    const int cost = upgradeCost(index);
    if (index < 0 || index > 2 || profile_.cookies < cost) {
        renderMessage("Bake another batch to afford that upgrade.");
        return;
    }
    profile_.cookies -= cost;
    if (index == 0) ++profile_.tapLevel;
    if (index == 1) ++profile_.comboLevel;
    if (index == 2) ++profile_.ovenLevel;
    rebirthArmed_ = false;
    renderMessage("Upgrade added to your bakery.");
    save();
}

void Game::rebirth() {
    if (!canRebirth()) {
        renderMessage("Collect 500 cookies first to open a fresh bakery.");
        return;
    }
    if (!rebirthArmed_ || rebirthArmSeconds_ <= 0.0F) {
        rebirthArmed_ = true;
        rebirthArmSeconds_ = 3.0F;
        renderMessage("Tap FRESH START again within 3 seconds to confirm.");
        return;
    }
    const int earned = std::max(1, profile_.cookies / kRebirthPrice);
    profile_.prestige += earned;
    profile_.cookies = 0;
    profile_.tapLevel = 0;
    profile_.comboLevel = 0;
    profile_.ovenLevel = 0;
    rebirthArmed_ = false;
    checkAchievements();
    renderMessage("Fresh bakery! Golden crumbs make every run stronger.");
    save();
}

void Game::buySkin(int index) {
    if (index < 0 || index > 3) return;
    const int bit = 1 << index;
    if ((profile_.ownedSkinsMask & bit) != 0) {
        profile_.selectedSkin = index;
        renderMessage("Cookie style changed.");
        save();
        return;
    }
    const int cost = skinCost(index);
    if (profile_.eventBerries < cost) {
        renderMessage("Earn event berries by reaching the festival tap goal.");
        return;
    }
    profile_.eventBerries -= cost;
    profile_.ownedSkinsMask |= bit;
    profile_.selectedSkin = index;
    checkAchievements();
    renderMessage("New cookie style unlocked!");
    save();
}

void Game::requestRewardedVideo() {
    if (adClaimed_ || adPending_) return;
    adPending_ = monetization_.showRewardedVideo();
    renderMessage(adPending_ ? "Opening an optional reward video..." : "Reward videos are not connected in this build.");
}

} // namespace crumble
