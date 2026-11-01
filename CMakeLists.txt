cmake_minimum_required(VERSION 3.20)
project(CrumbleRush VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG QUIET)
if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  set(SDL2_LINK_TARGET PkgConfig::SDL2)
else()
  set(SDL2_LINK_TARGET SDL2::SDL2)
endif()

add_executable(crumble-rush
  src/main.cpp
  src/game.cpp
  src/renderer.cpp
  src/monetization.cpp
)
target_include_directories(crumble-rush PRIVATE src)
target_link_libraries(crumble-rush PRIVATE ${SDL2_LINK_TARGET})
if(TARGET SDL2::SDL2main)
  target_link_libraries(crumble-rush PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
  target_compile_options(crumble-rush PRIVATE /W4 /permissive-)
else()
  target_compile_options(crumble-rush PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
  target_link_libraries(crumble-rush PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
