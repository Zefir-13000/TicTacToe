#pragma once
#include <stdafx.h>
#include <GameEngine.h>
#include <GameMessages.h>

struct Lobby_t
{
	CSteamID m_steamIDLobby;
	char m_rgchName[256];
};

struct LobbyBrowserMenuItem_t
{
	CSteamID m_steamIDLobby;
	EClientGameState m_eStateToTransitionTo;
};

class GameLobby {
public:
	GameLobby(GameEngine* pEngine);
	~GameLobby();

	bool GetIsSetLobbyName() const;
	std::string GetLobbyName() const;

	void SetLobbyName(std::string lobbyName);

	CSteamID GetLobbySteamID() const;
	void SetLobbySteamID(const CSteamID& steamIDLobby);

	CSteamID GetLobbyOwnerSteamID() const;
	void SetLobbyOwnerSteamID(const CSteamID& steamIDOwner);
	bool CheckIsLobbyOwner(const CSteamID& steamID);

	void Tick();
private:
	CSteamID m_steamIDLobby;
	CSteamID m_steamIDOwner;

	GameEngine* m_pGameEngine;

	bool m_bSetLobbyName = false;
	std::string m_LobbyName = "LobbyName";

	STEAM_CALLBACK(GameLobby, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange);

	// lobby state change handler
	STEAM_CALLBACK(GameLobby, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate);
	STEAM_CALLBACK(GameLobby, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackChatDataUpdate);
};