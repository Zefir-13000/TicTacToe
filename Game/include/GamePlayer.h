#pragma once
#include <stdafx.h>
#include <GameState.h>

struct ServerPlayerUpdateData_t {
public:
	void SetMove(uint32_t x, uint32_t y) { pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) { x = pos_x; y = pos_y; };
private:
	uint32_t pos_x, pos_y;
};

struct ServerGameUpdateData_t {
public:
	void SetServerGameState(EServerGameState eState) { m_eCurrentGameState = (uint32)eState; }
	EServerGameState GetServerGameState() { return (EServerGameState)m_eCurrentGameState; }

	void SetPlayerActive(uint32 iIndex, bool bIsActive) { m_PlayersActive[iIndex] = bIsActive; }
	bool GetPlayerActive(uint32 iIndex) { return m_PlayersActive[iIndex]; }

	void SetPlayerSteamID(uint32 iIndex, uint64 ulSteamID) { m_PlayerSteamIDs[iIndex] = ulSteamID; }
	uint64 GetPlayerSteamID(uint32 iIndex) { return m_PlayerSteamIDs[iIndex]; }

	ServerPlayerUpdateData_t* AccessPlayerUpdateData(uint32 iIndex) { return &m_PlayerData[iIndex]; }

private:
	uint32 m_eCurrentGameState;

	bool m_PlayersActive[MAX_PLAYERS_PER_SERVER];
	ServerPlayerUpdateData_t m_PlayerData[MAX_PLAYERS_PER_SERVER];
	uint64 m_PlayerSteamIDs[MAX_PLAYERS_PER_SERVER];
};

struct ClientGameUpdateData_t {
public:
	void SetPlayerName(const char* Name) { strncpy_s(PlayerName, Name, sizeof(PlayerName)); }
	const char* GetPlayerName() { return PlayerName; }

	void SetMove(uint32_t x, uint32_t y) { pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) const { x = pos_x; y = pos_y; };
private:
	char PlayerName[64];

	uint32_t pos_x, pos_y;
};

class GameEngine;
class GamePlayer {
public:
	GamePlayer(GameEngine* pEngine);
	~GamePlayer();

	bool GetIsLocalPlayer() const;
	void SetIsLocalPlayer(bool isLocal);

	void SetPlayerName(const char* Name) { strncpy_s(PlayerName, Name, sizeof(PlayerName)); }
	const char* GetPlayerPersonaName() const { return PlayerName; }

	void SetMove(uint32_t x, uint32_t y) { pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) const { x = pos_x; y = pos_y; };

	void OnReceiveServerUpdate(ServerPlayerUpdateData_t* pUpdateData);
	void OnReceiveClientUpdate(ClientGameUpdateData_t* pUpdateData);

	void BuildServerUpdate(ServerPlayerUpdateData_t* pUpdateData);
	bool GetClientUpdateData(ClientGameUpdateData_t* pUpdateData);

	// SERVER-SIDE
	const char* GetPlayerName();

private:
	GameEngine* m_pGameEngine = nullptr; // ref

	uint32_t pos_x, pos_y;

	bool m_isLocalPlayer;
	char PlayerName[128];

	uint64 m_ulLastClientUpdateTick;

	ClientGameUpdateData_t m_ClientGameUpdateData;
};