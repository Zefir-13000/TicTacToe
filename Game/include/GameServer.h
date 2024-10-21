#pragma once
#include <stdafx.h>
#include <GameMessages.h>
#include <GamePlayer.h>

struct ClientConnectionData_t
{
	bool m_bActive;					// Is this slot in use? Or is it available for new connections?
	CSteamID m_SteamIDUser;			// What is the steamid of the player?
	uint64 m_ulTickCountLastData;	// What was the last time we got data from the player?
	HSteamNetConnection m_hConn;	// The handle for the connection to the player

	ClientConnectionData_t() {
		m_bActive = false;
		m_ulTickCountLastData = 0;
		m_hConn = 0;
	}
};

class GameEngine;
class GameServer {
public:
	GameServer(GameEngine* pEngine);
	~GameServer();

	void Tick();

	void ReceiveNetworkData();

	void SetGameState(EServerGameState state);

	void KickPlayerOffServer(CSteamID steamID);

	void SendUpdateDataToAllClients();

	CSteamID GetSteamID();
	bool IsConnectedToSteam() { return m_bConnectedToSteam; }
private:
	STEAM_GAMESERVER_CALLBACK(GameServer, OnSteamServersConnected, SteamServersConnected_t);
	STEAM_GAMESERVER_CALLBACK(GameServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t);
	STEAM_GAMESERVER_CALLBACK(GameServer, OnSteamServersDisconnected, SteamServersDisconnected_t);

	STEAM_GAMESERVER_CALLBACK(GameServer, OnPolicyResponse, GSPolicyResponse_t);

	STEAM_GAMESERVER_CALLBACK(GameServer, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);
	STEAM_GAMESERVER_CALLBACK(GameServer, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t);

	void SendUpdatedServerDetailsToSteam();
	
	bool SendDataToClient(uint32 PlayerIndex, char* pData, uint32 nSizeOfData);

	void OnClientBeginAuthentication(CSteamID steamIDClient, HSteamNetConnection connectionID, void* pToken, uint32 uTokenLen);
	void OnAuthCompleted(bool bAuthSuccess, uint32 iPendingAuthIndex);

	void OnReceiveClientUpdateData(uint32 PlayerIndex, ClientGameUpdateData_t* pUpdateData);

	void AddPlayer(uint32 PlayerPosition);
	void RemovePlayerFromServer(uint32 PlayerArrPosition, EDisconnectReason reason);

	// Internal
	bool m_bConnectedToSteam = false;

	HSteamListenSocket m_hListenSocket; // Socket to listen for new connections on
	HSteamNetPollGroup m_hNetPollGroup; // Poll group used to receive messages from all clients at once
	
	GameEngine* m_pGameEngine = nullptr; // ref

	// Server Config
	std::string m_ServerName;

	// Game Data
	EServerGameState m_GameState;

	uint32 m_PlayerCount = 0;
	GamePlayer* m_Players[MAX_PLAYERS_PER_SERVER];

	// Vector to keep track of client connections
	ClientConnectionData_t m_ClientData[MAX_PLAYERS_PER_SERVER];
	// Vector to keep track of client connections which are pending auth
	ClientConnectionData_t m_PendingClientData[MAX_PLAYERS_PER_SERVER];

	uint64 m_ulStateTransitionTime;
	uint64 m_ulLastServerUpdateTick;

};