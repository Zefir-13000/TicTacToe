#pragma once
#include <stdafx.h>
#include <GamePlayer.h>
#include <GameMessages.h>
#include <GameServer.h>
#include <P2PAuthedGame.h>
#include <GameLobby.h>
#include <TextureObject.h>

static std::string convertToTwoDigitIndex(int index) {
	int row = index / 3 + 1;
	int col = index % 3 + 1;

	return std::to_string(row) + std::to_string(col);
}

static int convertToIndex(int x, int y) {
	return (y - 1) * 3 + (x - 1);
}

class GameEngine;
class GameClient {
public:
	enum class TicTacUnit {
		CROSS = 1,
		NOUGHT = 2
	};

	GameClient(GameEngine* pEngine);
	~GameClient();

	void Tick();

	void ReceiveNetworkData();

	void InitiateServerConnection(CSteamID steamIDGameServer);
	void InitiateServerConnection(uint32 unServerAddress, const int32 nPort);

	bool SendServerData(const void* pData, uint32 nSizeOfData, int nSendFlags);

	void SetGameState(EClientGameState eState);

	void ResetClientData();

	void MakeMove(int index);

	static std::string convertToTicTacUnit(int unit) {
		if (unit == static_cast<int>(GameClient::TicTacUnit::CROSS)) {
			return "X";
		}
		else if (unit == static_cast<int>(GameClient::TicTacUnit::NOUGHT)) {
			return "O";
		}
		return "";
	}

	CSteamID GetLobbySteamID() const { return m_pLobby->GetLobbySteamID(); }
	CSteamID GetLocalSteamID() const { return m_LocalSteamID; }
	const char* GetLocalPlayerName()
	{
		return SteamFriends()->GetPersonaName();
	}
private:
	void OnReceiveServerInfo(CSteamID steamIDGameServer, bool bVACSecure, const char* pchServerName);
	void OnReceiveServerAuthenticationResponse(bool bSuccess, uint32 uPlayerPosition);
	void OnReceiveServerExiting();
	void OnReceiveServerUpdate(ServerGameUpdateData_t* pUpdateData);

	void DisconnectFromServer();

	void UpdateRichPresenceConnectionInfo();

	void OnGameStateChanged(EClientGameState eGameStateNew);

	Texture* GetPlayerAvatar(int iImage);

	// Internal
	CSteamID m_LocalSteamID;

	uint32 m_PlayerIndex;
	EClientConnectionState m_eConnectedStatus;

	GamePlayer* m_Players[MAX_PLAYERS_PER_SERVER] = { 0 };
	CSteamID m_SteamIDPlayers[MAX_PLAYERS_PER_SERVER];
	std::string m_PlayersNames[MAX_PLAYERS_PER_SERVER];
	Texture* m_PlayerTextures[MAX_PLAYERS_PER_SERVER] = { nullptr };
	bool m_PlayerReady[MAX_PLAYERS_PER_SERVER] = { 0 };

	GameServer* m_pServer = nullptr;
	GameEngine* m_pGameEngine = nullptr;

	GameStateUpdateData_t m_GameData;
	GameStateUpdateData_t m_PrevGameData;
	float m_LocalGameTimer;
	uint64 m_DeltaTimeTicks;
	uint32_t last_x, last_y;

	uint64 m_ulLastNetworkDataReceivedTime;
	uint64 m_ulLastConnectionAttemptRetryTime;

	bool m_bTransitionedGameState;
	bool m_bJustTransitionedGameState;
	bool m_bSetHostReady = false;
	uint64 m_ulStateTransitionTime;
	EClientGameState m_eGameState;

	CSteamID m_steamIDGameServer;
	CSteamID m_steamIDGameServerFromBrowser;
	uint32 m_unServerIP;
	uint16 m_usServerPort;
	HAuthTicket m_hAuthTicket;
	HSteamNetConnection m_hConnServer;

	P2PAuthedGame* m_pP2PAuthedGame;
	GameLobby* m_pLobby;

	//bool m_bIsLobbyOwner;
	//CSteamID m_steamIDLobby;
	void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);
	CCallResult<GameClient, LobbyCreated_t> m_SteamCallResultLobbyCreated;

	// callback for when we've joined a lobby
	void OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure);
	CCallResult<GameClient, LobbyEnter_t> m_SteamCallResultLobbyEntered;

	// Uses m_steamIDLobby
	void RemovePlayerFromLobby(int PlayerIndex);
	int GetLobbyUserIndex(CSteamID steamId);
	std::vector<CSteamID> EnumLobbyUsers() const;
	void JoinLobby(LobbyBrowserMenuItem_t lobbyInfo);

	STEAM_CALLBACK(GameClient, OnSteamServersConnected, SteamServersConnected_t);
	STEAM_CALLBACK(GameClient, OnSteamServersDisconnected, SteamServersDisconnected_t);

	STEAM_CALLBACK(GameClient, OnLobbyGameCreated, LobbyGameCreated_t);
	STEAM_CALLBACK(GameClient, OnLobbyUpdate, LobbyChatUpdate_t);
	STEAM_CALLBACK(GameClient, OnLobbyDataUpdate, LobbyDataUpdate_t);
	STEAM_CALLBACK(GameClient, OnGameLobbyJoinRequested, GameLobbyJoinRequested_t);

	class GameServerPing : public ISteamMatchmakingPingResponse
	{
	public:
		GameServerPing()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
			m_pGameClient = NULL;
		}

		void RetrieveSteamIDFromGameServer(GameClient* pGameClient, uint32 unIP, uint16 unPort)
		{
			m_pGameClient = pGameClient;
			m_hGameServerQuery = SteamMatchmakingServers()->PingServer(unIP, unPort, this);
		}

		void CancelPing()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

		// Server has responded successfully and has updated data
		virtual void ServerResponded(gameserveritem_t& server)
		{
			if (m_hGameServerQuery != HSERVERQUERY_INVALID && server.m_steamID.IsValid())
			{
				m_pGameClient->InitiateServerConnection(server.m_steamID);
			}

			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

		// Server failed to respond to the ping request
		virtual void ServerFailedToRespond()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

	private:
		HServerQuery m_hGameServerQuery;	// we're ping a game server, so we can convert IP:Port to a steamID
		GameClient* m_pGameClient;
	};
	GameServerPing m_GameServerPing;
};

extern GameClient* g_pGameClient;
GameClient* GlobalGameClient();