#pragma once

#define SFML_STATIC 1
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <windows.h>  
#include <iostream>
#include <fstream>
#include <array>
#include <memory>
#include "protocol.h"

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-audio-s-d.lib")
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#pragma comment (lib, "lib/freetype.lib")
#else
#pragma comment (lib, "lib/sfml-audio-s.lib")
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#pragma comment (lib, "lib/freetype.lib")
#endif
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

using namespace std;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 32;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;