# Crumble Rush

**Crumble Rush** is a cozy, one-minute arcade baking game. Tap a hand-drawn cookie, keep a combo alive, catch short golden-oven windows, earn bakery upgrades, collect keepsakes, and trade a finished bakery for permanent golden-crumb bonuses. Each calendar day rotates a small festival with its own score bonus and a collectible goal.

The core loop is simple to learn and has room to master: short score-chasing runs feed a longer upgrade and collection loop. The game has no energy timer, punishment for missed days, forced ads, paid random rewards, or pay-to-win purchases.

## Market and design notes

- **Short, tactile loops have broad reach.** Sensor Tower's December 2025 worldwide download ranking placed *Block Blast!* first and *Subway Surfers* fifth. Those games point to the appeal of simple input, quick feedback, and a score or puzzle goal that is easy to understand. Crumble Rush applies that lesson to a fresh cookie-baking theme rather than copying their game rules. [Sensor Tower, December 2025 rankings](https://sensortower.com/blog/top-10-worldwide-mobile-games-by-revenue-and-downloads-in-december-2025)
- **A lightweight core needs a progression layer.** Sensor Tower reported growth in casual games in Western markets and increasing adoption of hybrid-casual monetization. This prototype pairs a one-minute arcade session with upgrades, a collection, achievements, and rebirth progression. [Sensor Tower, State of Mobile Gaming 2025](https://sensortower.com/state-of-gaming-2025)
- **A daily event is a content hook, not a retention guarantee.** AppMagic's 2026 market report found casual long-term retention declining in 2025 and growth in short hourly and daily LiveOps events. The game rotates a small daily festival, but it avoids streak loss and urgency timers. [AppMagic, Mobile Market Landscape 2026](https://appmagic.rocks/files/view/upload/Reports/EN_MobileMarkeLandscape2026.pdf)

These reports describe market-level patterns; they do not predict this game's downloads or revenue. The most useful next validation is a small playtest that measures whether new players understand the tap-and-combo loop, finish a run, and voluntarily start another.

## Features

- 60-second timed runs with mouse, touch, or keyboard input
- Combo window, score multiplier, golden-cookie feedback, screen shake, and particle bursts
- Bakery upgrades for tap strength, combo depth, and golden-oven duration
- Six milestone keepsakes with one-time cookie rewards
- A rebirth system that resets upgrades for a permanent score multiplier
- Four cookie appearances, unlocked with festival berries
- A daily pastry and a daily rotating festival; missed days do not break anything
- Local save file, written through a temporary file before replacement
- An optional rewarded-video button and store-product interface, disabled until a real provider is integrated
- No external art or font files; the visuals and small display font are drawn in SDL2

## Requirements and installation

- C++17 compiler
- CMake 3.20 or newer
- SDL2 development libraries

### Windows with vcpkg

```powershell
vcpkg install sdl2
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
.
build\Release\crumble-rush.exe
```

Replace `C:/path/to/vcpkg` with your vcpkg installation path.

### macOS

```sh
brew install cmake sdl2 pkg-config
cmake -S . -B build
cmake --build build --config Release
./build/crumble-rush
```

### Ubuntu or Debian

```sh
sudo apt install cmake g++ libsdl2-dev pkg-config
cmake -S . -B build
cmake --build build --config Release
./build/crumble-rush
```

The save file is placed in the operating system's application-preferences directory under `CrumbleCorner/CrumbleRush`.

## Controls

- **Bakery:** click **Start 60 Second Rush** or press **Enter**
- **During a run:** click/tap the cookie, or press **Space** / **Z**
- **Wardrobe:** click a cookie style; press **W** to open it from the bakery
- **Back:** press **Escape** or **Backspace** from results or wardrobe
- A run cannot be paused; it lasts one minute and does not consume lives or currency.

## Monetization integration

The project is fully playable with the default `UnavailableProvider`; no ads, purchases, network calls, or account data are active. `src/monetization.hpp` defines an adapter for rewarded videos and product purchases, and `src/monetization.cpp` safely queues provider callbacks for the game loop.

To connect a provider:

1. Implement `CommerceProvider::showRewardedVideo` and `CommerceProvider::purchaseProduct` using the platform's official SDK.
2. Call the completion callback only after the SDK confirms a completed rewarded view or verified purchase. A dismissed or failed ad must not grant its bonus.
3. Pass that provider to `Monetization` in `main.cpp`. The game loop drains commerce events on its own thread.
4. Prefer a small, optional rewarded-video offer after a completed run and clearly priced, non-random cosmetic purchases. Keep gameplay progression available without payment.
5. For iOS digital goods, use Apple's in-app purchase APIs and follow the current review rules. Apple requires in-app purchase for digital features and content sold inside an iOS app. [Apple App Review Guidelines](https://developer.apple.com/app-store/review/guidelines/)

This repository intentionally includes **no ad network or payment SDK**. Add one only after choosing a provider, defining privacy practices, and preparing a real mobile build. Revenue is not automatic: distribution, discoverability, quality, and player response need separate validation.

## iOS path

SDL2 supports iOS, so the game logic and renderer can be reused in an Xcode iOS target. This CMake setup is for desktop builds; shipping to the App Store still requires an iOS app target, signing, device testing, store metadata, age-rating answers, and privacy details. See [SDL2's iOS guide](https://wiki.libsdl.org/SDL2/README-ios) and [Apple's submission checklist](https://developer.apple.com/app-store/submitting/).

## Project structure

```text
crumble-rush/
├── CMakeLists.txt
├── README.md
└── src/
    ├── game.hpp
    ├── game.cpp
    ├── renderer.hpp
    ├── renderer.cpp
    ├── monetization.hpp
    ├── monetization.cpp
    └── main.cpp
```
