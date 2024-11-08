#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>
#include <unordered_map>
#include <Windows.h>

#include <d2d1.h>
#include <dwrite.h>

#include <d3d11.h>
#include <d3d11_1.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <steam/isteamnetworking.h>

#include <IconsFontAwesome5.h>

#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#define USE_GS_AUTH_API

#define GAME_SERVER_PORT 25565
#define GAME_MASTER_SERVER_UPDATE_PORT 25566
#define GAME_SERVER_VERSION "1.0.0"

// Time to timeout a connection attempt in
#define MILLISECONDS_CONNECTION_TIMEOUT 15000

#define SERVER_TIMEOUT_MILLISECONDS 5000

#define MILLISECONDS_BETWEEN_ROUNDS 3000

// How many times a second does the server send world updates to clients
#define SERVER_UPDATE_SEND_RATE 60

// How many times a second do we send our updated client state to the server
#define CLIENT_UPDATE_SEND_RATE 30

// How fast does the server internally run at?
#define MAX_CLIENT_AND_SERVER_FPS 60

#define MAX_PLAYERS_PER_SERVER 2

#define SERVER_GAME_TIMER_TURN 10