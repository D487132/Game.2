#pragma once

#include "monetization.hpp"

#include <SDL.h>

#include <array>
#include <string>

namespace crumble {

enum class Scene { Bakery, Rush, Results, Wardrobe, Keepsakes };

struct Profile {
    int cookies = 0;
    int eventBerries = 0;
    int bestScore = 0;
    int lifetimeScore = 0;
    int runs = 0;
    int prestige = 0;
    int tapLevel = 0;
    int comboLevel = 0;
    int ovenLevel = 0;
    int achievementMask = 0;
    int ownedSkinsMask = 1;
    int selectedSkin = 0;
    int lastDailyDay = -1;
    int festivalDay = -1;
    int festivalTaps = 0;
    bool festivalClaimed = false;
};

struct Festival {
    std::string name;
    std::string line;
    std::string icon;
    SDL_Color accent{};
    int dailyBonus = 0;
};

class Game {
public:
    Game(std::string savePath, Monetization& monetization);
    void handleEvent(const SDL_Event& event);
    void update(float deltaSeconds);

    Scene scene() const { return scene_; }
    const Profile& profile() const { return profile_; }
    const Festival& festival() const { return festival_; }
    int score() const { return score_; }
    int bestCombo() const { return bestCombo_; }
    int combo() const { return combo_; }
    int reward() const { return reward_; }
    int feedbackId() const { return feedbackId_; }
    float timeLeft() const { return timeLeft_; }
    float comboWindow() const { return comboWindow_; }
    float goldenSeconds() const { return goldenSeconds_; }
    bool goldenActive() const { return goldenActive_; }
    bool adPending() const { return adPending_; }
    bool adClaimed() const { return adClaimed_; }
    const std::string& message() const { return message_; }
    float messageSeconds() const { return messageSeconds_; }
    int upgradeCost(int index) const;
    int skinCost(int index) const;
    int tapPower() const;
    float scoreMultiplier() const;
    bool canRebirth() const;
    void renderMessage(std::string text);

private:
    void load();
    void save() const;
    void refreshFestival();
    void startRun();
    void tap();
    void finishRun();
    void checkAchievements();
    void claimDailyGift();
    void buyUpgrade(int index);
    void rebirth();
    void buySkin(int index);
    void requestRewardedVideo();
    int dayId() const;
    bool hit(int x, int y, SDL_Rect rect) const;

    std::string savePath_;
    Monetization& monetization_;
    Profile profile_;
    Festival festival_;
    Scene scene_ = Scene::Bakery;
    int score_ = 0;
    int combo_ = 0;
    int bestCombo_ = 0;
    int reward_ = 0;
    int tapCount_ = 0;
    int feedbackId_ = 0;
    float timeLeft_ = 60.0F;
    float elapsed_ = 0.0F;
    float comboWindow_ = 0.0F;
    float goldenSeconds_ = 0.0F;
    bool goldenActive_ = false;
    bool adPending_ = false;
    bool adClaimed_ = false;
    bool rebirthArmed_ = false;
    float rebirthArmSeconds_ = 0.0F;
    std::string message_;
    float messageSeconds_ = 0.0F;
};

} // namespace crumble
