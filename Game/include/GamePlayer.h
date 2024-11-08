#pragma once
#include <stdafx.h>
#include <GameState.h>

struct ServerPlayerUpdateData_t {
public:
	void SetMove(uint32_t x, uint32_t y) { pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) const { x = pos_x; y = pos_y; };

	bool GetMoved() const { return has_moved; };
	void SetMoved(bool bMoved) { has_moved = bMoved; }
private:

	uint32_t pos_x = 0, pos_y = 0;
	bool has_moved = false;
};

struct GameStateUpdateData_t {
	float m_GameTimer;
	bool m_RoundEnded = false;
	bool m_HostIsCrosses = true;
	uint32_t m_PlayerTurnIndex = 0;
	uint32_t m_WinPlayerIndex = 0;
	uint32_t m_TicTacTable[9] = {0};
};

struct ServerGameUpdateData_t {
public:
	void SetServerGameState(EServerGameState eState) { m_eCurrentGameState = (uint32)eState; }
	EServerGameState GetServerGameState() { return (EServerGameState)m_eCurrentGameState; }

	void SetPlayerActive(uint32 iIndex, bool bIsActive) { m_PlayersActive[iIndex] = bIsActive; }
	bool GetPlayerActive(uint32 iIndex) { return m_PlayersActive[iIndex]; }

	void SetPlayerSteamID(uint32 iIndex, uint64 ulSteamID) { m_PlayerSteamIDs[iIndex] = ulSteamID; }
	uint64 GetPlayerSteamID(uint32 iIndex) { return m_PlayerSteamIDs[iIndex]; }

	void GetGameUpdateData(GameStateUpdateData_t* pData) {
		pData->m_GameTimer = m_GameData.m_GameTimer;
		pData->m_HostIsCrosses = m_GameData.m_HostIsCrosses;
		pData->m_PlayerTurnIndex = m_GameData.m_PlayerTurnIndex;
		pData->m_RoundEnded = m_GameData.m_RoundEnded;
		pData->m_WinPlayerIndex = m_GameData.m_WinPlayerIndex;
		memcpy(pData->m_TicTacTable, m_GameData.m_TicTacTable, sizeof(m_GameData.m_TicTacTable));
	}
	void SetGameUpdateData(GameStateUpdateData_t* pData) {
		m_GameData.m_GameTimer = pData->m_GameTimer;
		m_GameData.m_HostIsCrosses = pData->m_HostIsCrosses;
		m_GameData.m_PlayerTurnIndex = pData->m_PlayerTurnIndex;
		m_GameData.m_RoundEnded = pData->m_RoundEnded;
		m_GameData.m_WinPlayerIndex = pData->m_WinPlayerIndex;

		memcpy(m_GameData.m_TicTacTable, pData->m_TicTacTable, sizeof(m_GameData.m_TicTacTable));
	}

	ServerPlayerUpdateData_t* AccessPlayerUpdateData(uint32 iIndex) { return &m_PlayerData[iIndex]; }
	GameStateUpdateData_t* AccessGameUpdateDate() { return &m_GameData; }
private:
	uint32 m_eCurrentGameState;

	GameStateUpdateData_t m_GameData;

	bool m_PlayersActive[MAX_PLAYERS_PER_SERVER];
	ServerPlayerUpdateData_t m_PlayerData[MAX_PLAYERS_PER_SERVER];
	uint64 m_PlayerSteamIDs[MAX_PLAYERS_PER_SERVER];
};

struct ClientGameUpdateData_t {
public:
	void SetPlayerName(const char* Name) { strncpy_s(PlayerName, Name, sizeof(PlayerName)); }
	const char* GetPlayerName() { return PlayerName; }

	bool isValid() const { return m_bValid; }
	void SetValid() { m_bValid = true; }

	void SetMove(uint32_t x, uint32_t y) { pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) const { x = pos_x; y = pos_y; };
private:
	char PlayerName[64];

	uint32_t pos_x = 0, pos_y = 0;
	uint32_t m_bValid = false;
};

class GameEngine;
class GamePlayer {
public:
	GamePlayer(GameEngine* pEngine);
	~GamePlayer();

	bool GetIsHost() const;
	void SetIsHost(bool isHost);

	bool GetIsLocalPlayer() const;
	void SetIsLocalPlayer(bool isLocal);

	void SetPlayerName(const char* Name) { strncpy_s(PlayerName, Name, sizeof(PlayerName)); }
	const char* GetPlayerPersonaName() const { return PlayerName; }

	void SetMove(uint32_t x, uint32_t y) { m_hasUpdate = true; pos_x = x; pos_y = y; };
	void GetMove(uint32_t& x, uint32_t& y) const { x = pos_x; y = pos_y; };

	void OnReceiveServerUpdate(ServerPlayerUpdateData_t* pUpdateData);
	void OnReceiveClientUpdate(ClientGameUpdateData_t* pUpdateData);

	void BuildServerUpdate(ServerPlayerUpdateData_t* pUpdateData);
	bool GetClientUpdateData(ClientGameUpdateData_t* pUpdateData);

	// SERVER-SIDE
	const char* GetPlayerName();

private:
	GameEngine* m_pGameEngine = nullptr; // ref

	uint32_t pos_x = 0, pos_y = 0;

	bool m_isLocalPlayer;
	bool m_isHost;
	char PlayerName[128];

	bool m_hasUpdate = false;

	uint64 m_ulLastClientUpdateTick;

	ClientGameUpdateData_t m_ClientGameUpdateData;
};