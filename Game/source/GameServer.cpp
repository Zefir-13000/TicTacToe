#include <GameServer.h>
#include <GameEngine.h>
#include <GameClient.h>

GameServer::GameServer(GameEngine* pEngine) : m_pGameEngine(pEngine) {
	m_bConnectedToSteam = false;

	const char* pchGameDir = "tictactoe";
	uint32 unIP = INADDR_ANY;
	uint16 usMasterServerUpdaterPort = GAME_MASTER_SERVER_UPDATE_PORT;

	m_GameState = EServerGameState::ServerWaitingForPlayers;

#ifdef USE_GS_AUTH_API
	EServerMode eMode = eServerModeAuthenticationAndSecure;
#else
	// Don't let Steam do authentication
	EServerMode eMode = eServerModeNoAuthentication;
#endif

	if (!SteamGameServer_Init(unIP, GAME_SERVER_PORT, usMasterServerUpdaterPort, eMode, GAME_SERVER_VERSION))
	{
		OutputDebugString("SteamGameServer_Init call failed\n");
	}

	if (SteamGameServer())
	{
		SteamGameServer()->SetModDir(pchGameDir);

		// These fields are currently required, but will go away soon.
		// See their documentation for more info
		SteamGameServer()->SetProduct("TicTacToeGame");
		SteamGameServer()->SetGameDescription("TicTacToe Game");

		SteamGameServer()->LogOnAnonymous();

		SteamNetworkingUtils()->InitRelayNetworkAccess();

#ifdef USE_GS_AUTH_API
		SteamGameServer()->SetAdvertiseServerActive(true);
#endif
	}
	else
	{
		OutputDebugString("SteamGameServer() interface is invalid\n");
	}


	// Seed random num generator
	srand((uint32)time(NULL));

	m_hListenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
	m_hNetPollGroup = SteamGameServerNetworkingSockets()->CreatePollGroup();
}

GameServer::~GameServer() {
	SteamGameServerNetworkingSockets()->CloseListenSocket(m_hListenSocket);
	SteamGameServerNetworkingSockets()->DestroyPollGroup(m_hNetPollGroup);

	// Disconnect from the steam servers
	SteamGameServer()->LogOff();

	// release our reference to the steam client library
	SteamGameServer_Shutdown();
}

void GameServer::ReceiveNetworkData() {
	SteamNetworkingMessage_t* msgs[128];
	int numMessages = SteamGameServerNetworkingSockets()->ReceiveMessagesOnPollGroup(m_hNetPollGroup, msgs, 128);

	for (int idxMsg = 0; idxMsg < numMessages; idxMsg++)
	{
		SteamNetworkingMessage_t* message = msgs[idxMsg];
		CSteamID steamIDRemote = message->m_identityPeer.GetSteamID();
		HSteamNetConnection connection = message->m_conn;

		if (message->GetSize() < sizeof(DWORD))
		{
			OutputDebugString("Got garbage on server socket, too short\n");
			message->Release();
			message = nullptr;
			continue;
		}

		EMessage eMsg = (EMessage)(*(DWORD*)message->GetData());

		switch (eMsg) {
			case k_EMsgClientBeginAuthentication:
			{
				if (message->GetSize() != sizeof(MsgClientBeginAuthentication_t))
				{
					OutputDebugString("Bad connection attempt msg\n");
					message->Release();
					message = nullptr;
					continue;
				}
				MsgClientBeginAuthentication_t* pMsg = (MsgClientBeginAuthentication_t*)message->GetData();
			#ifdef USE_GS_AUTH_API
				OnClientBeginAuthentication(steamIDRemote, connection, (void*)pMsg->GetTokenPtr(), pMsg->GetTokenLen());
			#else
				OnClientBeginAuthentication(connection, 0);
			#endif
			}
			break;

			case k_EMsgClientSendLocalUpdate:
			{
				if (message->GetSize() != sizeof(MsgClientSendLocalUpdate_t))
				{
					OutputDebugString("Bad client update msg\n");
					message->Release();
					message = nullptr;
					continue;
				}

				// Find the connection that should exist for this users address
				bool bFound = false;
				for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
				{
					if (m_ClientData[i].m_hConn == connection)
					{
						bFound = true;
						MsgClientSendLocalUpdate_t* pMsg = (MsgClientSendLocalUpdate_t*)message->GetData();
						if (pMsg->AccessUpdateData()->isValid() && CheckValidMove(pMsg->AccessUpdateData())) {
							ChangePlayerTurn();
						}
						OnReceiveClientUpdateData(i, pMsg->AccessUpdateData());
						if (pMsg->AccessUpdateData()->isValid()) {
							uint32_t win = CheckWin();
							if (win == 3) {
								m_GameData.m_RoundEnded = true;
								m_GameData.m_GameTimer = 5;
								m_GameDataTimerServer = 5;
								m_GameData.m_WinPlayerIndex = 2;
								m_GameData.m_HostIsCrosses = !m_GameData.m_HostIsCrosses;
							}
							else if (win != 0) {
								m_GameData.m_RoundEnded = true;
								m_GameData.m_GameTimer = 5;
								m_GameDataTimerServer = 5;
								if (m_GameData.m_HostIsCrosses) {
									m_GameData.m_WinPlayerIndex = win - 1;
								}
								else {
									m_GameData.m_WinPlayerIndex = !(win - 1);
								}
								m_GameData.m_HostIsCrosses = !m_GameData.m_HostIsCrosses;
							}
						}
						break;
					}
				}
				if (!bFound)
					OutputDebugString("Got a client data update, but couldn't find a matching client\n");
			}
			break;

			case k_EMsgP2PSendingTicket:
			{
				// Received a P2P auth ticket, forward it to the intended recipient
				MsgP2PSendingTicket_t msgP2PSendingTicket;
				memcpy(&msgP2PSendingTicket, message->GetData(), sizeof(MsgP2PSendingTicket_t));
				CSteamID toSteamID = msgP2PSendingTicket.GetSteamID();

				HSteamNetConnection toHConn = 0;
				for (int j = 0; j < MAX_PLAYERS_PER_SERVER; j++)
				{
					if (toSteamID == m_ClientData[j].m_SteamIDUser)
					{
						// Mutate the message, replacing the destination SteamID with the sender's SteamID
						msgP2PSendingTicket.SetSteamID(message->m_identityPeer.GetSteamID64());
						toHConn = m_ClientData[j].m_hConn;
						SteamNetworkingSockets()->SendMessageToConnection(m_ClientData[j].m_hConn, &msgP2PSendingTicket, sizeof(msgP2PSendingTicket), k_nSteamNetworkingSend_Reliable, nullptr);
						break;
					}
				}

				if (toHConn == 0)
				{
					OutputDebugString("msgP2PSendingTicket received with no valid target to send to.");
				}
			}
			break;

			default:
				char rgch[128];
				sprintf_s(rgch, "Invalid message %x\n", eMsg);
				rgch[sizeof(rgch) - 1] = 0;
				OutputDebugString(rgch);
		}

		message->Release();
		message = nullptr;
	}
}

void GameServer::ChangePlayerTurn() {
	++m_GameData.m_PlayerTurnIndex;
	if (m_GameData.m_PlayerTurnIndex + 1 > MAX_PLAYERS_PER_SERVER) {
		m_GameData.m_PlayerTurnIndex = 0;
	}
	m_GameData.m_GameTimer = SERVER_GAME_TIMER_TURN;
	m_GameDataTimerServer = SERVER_GAME_TIMER_TURN;
}

void GameServer::NextRound() {
	m_GameData.m_GameTimer = SERVER_GAME_TIMER_TURN;
	m_GameDataTimerServer = SERVER_GAME_TIMER_TURN;
	m_GameData.m_RoundEnded = false;
	memset(m_GameData.m_TicTacTable, 0, sizeof(m_GameData.m_TicTacTable));
}

bool GameServer::CheckValidMove(ClientGameUpdateData_t* gameData) {
	uint32_t x, y;
	gameData->GetMove(x, y);
	if (m_GameData.m_TicTacTable[convertToIndex(x, y)] == 0) {
		return true;
	}
	return false;
}

uint32_t GameServer::CheckWin() {
	// check vert
	for (uint32_t x = 1; x < 4; ++x) {
		bool flag = true;
		uint32_t prev_unit = 0;
		for (uint32_t y = 1; y < 4; ++y) {
			uint32_t cell_unit = m_GameData.m_TicTacTable[convertToIndex(x, y)];
			if (cell_unit == 0) {
				flag = false;
				break;
			}
			if (prev_unit == 0) {
				prev_unit = cell_unit;
				continue;
			}
			if (prev_unit != cell_unit) {
				flag = false;
				break;
			}
		}

		if (flag) {
			return prev_unit;
		}
	}
	// check horz
	for (uint32_t y = 1; y < 4; ++y) {
		bool flag = true;
		uint32_t prev_unit = 0;
		for (uint32_t x = 1; x < 4; ++x) {
			uint32_t cell_unit = m_GameData.m_TicTacTable[convertToIndex(x, y)];
			if (cell_unit == 0) {
				flag = false;
				break;
			}
			if (prev_unit == 0) {
				prev_unit = cell_unit;
				continue;
			}
			if (prev_unit != cell_unit) {
				flag = false;
				break;
			}
		}

		if (flag) {
			return prev_unit;
		}
	}

	// check major diagonal
	bool flag_diag = true;
	uint32_t prev_unit_diag = 0;
	for (uint32_t xy = 1; xy < 4; ++xy) {
		uint32_t cell_unit = m_GameData.m_TicTacTable[convertToIndex(xy, xy)];
		if (cell_unit == 0) {
			flag_diag = false;
			break;
		}
		if (prev_unit_diag == 0) {
			prev_unit_diag = cell_unit;
			continue;
		}
		if (prev_unit_diag != cell_unit) {
			flag_diag = false;
			break;
		}
	}
	if (flag_diag) {
		return prev_unit_diag;
	}

	// check minor diagonal
	flag_diag = true;
	prev_unit_diag = 0;
	for (uint32_t xy = 1; xy < 4; ++xy) {
		uint32_t cell_unit = m_GameData.m_TicTacTable[convertToIndex(4 - xy, xy)];
		if (cell_unit == 0) {
			flag_diag = false;
			break;
		}
		if (prev_unit_diag == 0) {
			prev_unit_diag = cell_unit;
			continue;
		}
		if (prev_unit_diag != cell_unit) {
			flag_diag = false;
			break;
		}
	}
	if (flag_diag) {
		return prev_unit_diag;
	}

	bool flag_draw = true;
	for (int i = 0; i < 9; ++i) {
		uint32_t cell_unit = m_GameData.m_TicTacTable[i];
		if (cell_unit == 0) {
			flag_draw = false;
			break;
		}
	}
	if (flag_draw)
		return 3;

	return 0;
}

void GameServer::Tick() {
	SteamGameServer_RunCallbacks();
	SendUpdatedServerDetailsToSteam();

	uint32 uPlayerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		// If there is no ship, skip
		if (!m_ClientData[i].m_bActive)
			continue;

		uint64 ticks = m_pGameEngine->GetGameTickCount();
		if (ticks - m_ClientData[i].m_ulTickCountLastData > SERVER_TIMEOUT_MILLISECONDS)
		{
			OutputDebugString("Timing out player connection\n");
			RemovePlayerFromServer(i, EDisconnectReason::ClientKicked);
		}
		else
		{
			++uPlayerCount;
		}
	}
	m_PlayerCount = uPlayerCount;

	switch (m_GameState)
	{
	case EServerGameState::ServerWaitingForPlayers:
		// Wait a few seconds (so everyone can join if a lobby just started this server)
		if (m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime >= MILLISECONDS_BETWEEN_ROUNDS)
		{
			// Just keep waiting until at least one ship is active
			for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
			{
				if (m_ClientData[i].m_bActive)
				{
					// Transition to active
					OutputDebugString("Server going active after waiting for players\n");
					SetGameState(EServerGameState::ServerActive);
				}
			}
		}
		break;

	case EServerGameState::ServerActive:
		if (m_bJustTransitionedGameState) {
			m_bJustTransitionedGameState = false;
			m_GameData.m_GameTimer = SERVER_GAME_TIMER_TURN;
			m_GameDataTimerServer = SERVER_GAME_TIMER_TURN;
			m_DeltaTimeTicks = m_pGameEngine->GetGameTickCount();
		}

		m_GameDataTimerServer -= (m_pGameEngine->GetGameTickCount() - m_DeltaTimeTicks) / 1000.0f;
		if (m_GameDataTimerServer <= 0 && !m_GameData.m_RoundEnded) {
			ChangePlayerTurn();
			m_GameData.m_RoundEnded = true;
			m_GameData.m_GameTimer = 5;
			m_GameDataTimerServer = 5;
			if (m_GameData.m_HostIsCrosses) {
				m_GameData.m_WinPlayerIndex = m_GameData.m_PlayerTurnIndex;
				if (m_GameData.m_PlayerTurnIndex == 0) {
					m_GameData.m_HostIsCrosses = !m_GameData.m_HostIsCrosses;
				}
			}
			else {
				m_GameData.m_WinPlayerIndex = !m_GameData.m_PlayerTurnIndex;
				if (m_GameData.m_PlayerTurnIndex == 1) {
					m_GameData.m_HostIsCrosses = !m_GameData.m_HostIsCrosses;
				}
			}
			m_GameData.m_HostIsCrosses = !m_GameData.m_HostIsCrosses;
		}
		else if (m_GameDataTimerServer <= 0 && m_GameData.m_RoundEnded) {
			NextRound();
		}
		m_GameData.m_GameTimer = m_GameDataTimerServer;
		m_DeltaTimeTicks = m_pGameEngine->GetGameTickCount();


		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			//TODO: Replace with logic
			//if (m_Players[i])
			//	m_Players[i]->Tick();
		}

		// Check for collisions which could lead to a winner this round
		//CheckForCollisions();

		break;
	case EServerGameState::ServerExiting:
		break;
	default:
		OutputDebugString("Unhandled game state in GameServer::Tick\n");
	}

	// Send client updates (will internal limit itself to the tick rate desired)
	SendUpdateDataToAllClients();
}

void GameServer::SendUpdateDataToAllClients()
{
	// Limit the rate at which we update, even if our internal frame rate is higher
	if (m_pGameEngine->GetGameTickCount() - m_ulLastServerUpdateTick < 1000.0f / SERVER_UPDATE_SEND_RATE)
		return;

	m_ulLastServerUpdateTick = m_pGameEngine->GetGameTickCount();

	MsgServerUpdateWorld_t msg;

	msg.AccessUpdateData()->SetServerGameState(m_GameState);
	msg.AccessUpdateData()->SetGameUpdateData(&m_GameData);
	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		msg.AccessUpdateData()->SetPlayerActive(i, m_ClientData[i].m_bActive);
		msg.AccessUpdateData()->SetPlayerSteamID(i, m_ClientData[i].m_SteamIDUser.ConvertToUint64());

		if (m_Players[i])
		{
			m_Players[i]->BuildServerUpdate(msg.AccessUpdateData()->AccessPlayerUpdateData(i));
		}
	}

	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_ClientData[i].m_bActive)
			continue;

		SendDataToClient(i, (char*)&msg, sizeof(msg));
	}
}

void GameServer::SetGameState(EServerGameState eState) {
	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_bJustTransitionedGameState = true;
	this->m_GameState = eState;
}

void GameServer::OnReceiveClientUpdateData(uint32 PlayerIndex, ClientGameUpdateData_t* pUpdateData)
{
	if (m_ClientData[PlayerIndex].m_bActive && m_Players[PlayerIndex])
	{
		m_ClientData[PlayerIndex].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();
		m_Players[PlayerIndex]->OnReceiveClientUpdate(pUpdateData);
		if (pUpdateData->isValid() && CheckValidMove(pUpdateData)) {
			uint32_t x, y;
			m_Players[PlayerIndex]->GetMove(x, y);
			if (x > 0 && y > 0 && x < 4 && y < 4) {
				int index = convertToIndex(x, y);
				if (m_GameData.m_HostIsCrosses) {
					if (PlayerIndex == 0) {
						m_GameData.m_TicTacTable[index] = 1;
					}
					else {
						m_GameData.m_TicTacTable[index] = 2;
					}
				}
				else {
					if (PlayerIndex == 0) {
						m_GameData.m_TicTacTable[index] = 2;
					}
					else {
						m_GameData.m_TicTacTable[index] = 1;
					}
				}
			}
		}
	}
}

void GameServer::SendUpdatedServerDetailsToSteam() {
	char rgchServerName[128];
	if (GlobalGameClient())
	{
		// If a client is running (should always be since we don't support a dedicated server)
		// then we'll form the name based off of it
		sprintf_s(rgchServerName, "%s's game", GlobalGameClient()->GetLocalPlayerName());
	}
	else
	{
		sprintf_s(rgchServerName, "%s", "Game!");
	}
	m_ServerName = rgchServerName;

	SteamGameServer()->SetMaxPlayerCount(MAX_PLAYERS_PER_SERVER);
	SteamGameServer()->SetPasswordProtected(false);
	SteamGameServer()->SetServerName(m_ServerName.c_str());
	SteamGameServer()->SetBotPlayerCount(0); // optional, defaults to zero
	SteamGameServer()->SetMapName("TestMap");

#ifdef USE_GS_AUTH_API

	// Update all the players names/scores
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_ClientData[i].m_bActive && m_Players[i])
		{
			SteamGameServer()->BUpdateUserData(m_ClientData[i].m_SteamIDUser, m_Players[i]->GetPlayerName(), 0);
		}
	}
#endif
}

void GameServer::OnSteamServersConnected(SteamServersConnected_t* pLogonSuccess) {
	OutputDebugString("Server connected to Steam successfully\n");
	m_bConnectedToSteam = true;

	// Tell Steam about our server details
	SendUpdatedServerDetailsToSteam();
}

void GameServer::OnSteamServersDisconnected(SteamServersDisconnected_t* pLoggedOff)
{
	m_bConnectedToSteam = false;
	OutputDebugString("Server got logged out of Steam\n");
}

void GameServer::OnSteamServersConnectFailure(SteamServerConnectFailure_t* pConnectFailure)
{
	m_bConnectedToSteam = false;
	OutputDebugString("Server failed to connect to Steam\n");
}

void GameServer::OnPolicyResponse(GSPolicyResponse_t* pPolicyResponse)
{
#ifdef USE_GS_AUTH_API
	// Check if we were able to go VAC secure or not
	if (SteamGameServer()->BSecure())
	{
		OutputDebugString("Server is VAC Secure!\n");
	}
	else
	{
		OutputDebugString("Server is not VAC Secure!\n");
	}
	char rgch[128];
	sprintf_s(rgch, "Game server SteamID: %llu\n", SteamGameServer()->GetSteamID().ConvertToUint64());
	rgch[sizeof(rgch) - 1] = 0;
	OutputDebugString(rgch);
#endif
}

void GameServer::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pCallback)
{
	/// Connection handle
	HSteamNetConnection hConn = pCallback->m_hConn;

	/// Full connection info
	SteamNetConnectionInfo_t info = pCallback->m_info;

	/// Previous state.  (Current state is in m_info.m_eState)
	ESteamNetworkingConnectionState eOldState = pCallback->m_eOldState;

	if (info.m_hListenSocket &&
		eOldState == k_ESteamNetworkingConnectionState_None &&
		info.m_eState == k_ESteamNetworkingConnectionState_Connecting) {
		// Connection from a new client
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
			if (!m_ClientData[i].m_bActive && !m_PendingClientData[i].m_hConn) {
				// Found one.  "Accept" the connection.
				EResult res = SteamGameServerNetworkingSockets()->AcceptConnection(hConn);
				if (res != k_EResultOK)
				{
					char msg[256];
					sprintf(msg, "AcceptConnection returned %d", res);
					OutputDebugString(msg);
					SteamGameServerNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
					return;
				}

				m_PendingClientData[i].m_hConn = hConn;

				// add the user to the poll group
				SteamGameServerNetworkingSockets()->SetConnectionPollGroup(hConn, m_hNetPollGroup);

				// Send them the server info as a reliable message
				MsgServerSendInfo_t msg;

				msg.SetSteamIDServer(SteamGameServer()->GetSteamID().ConvertToUint64());
			#ifdef USE_GS_AUTH_API
				// You can only make use of VAC when using the Steam authentication system
				msg.SetSecure(SteamGameServer()->BSecure());
			#endif
				msg.SetServerName(m_ServerName.c_str());
				SteamGameServerNetworkingSockets()->SendMessageToConnection(hConn, &msg, sizeof(MsgServerSendInfo_t), k_nSteamNetworkingSend_Reliable, nullptr);

				return;
			}
		}

		// No empty slots.  Server full!
		OutputDebugString("Rejecting connection; server full");
		SteamGameServerNetworkingSockets()->CloseConnection(hConn, k_ESteamNetConnectionEnd_AppException_Generic, "Server full!", false);
	}
	// Check if a client has disconnected
	else if ((eOldState == k_ESteamNetworkingConnectionState_Connecting || eOldState == k_ESteamNetworkingConnectionState_Connected) &&
		info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		// Handle disconnecting a client
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			// If there is no player, skip
			if (!m_ClientData[i].m_bActive)
				continue;

			if (m_ClientData[i].m_SteamIDUser == info.m_identityRemote.GetSteamID())//pCallback->m_steamIDRemote)
			{
				OutputDebugString("Disconnected dropped user\n");
				RemovePlayerFromServer(i, EDisconnectReason::ClientDisconnect);
				break;
			}
		}
	}
}

void GameServer::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* pResponse)
{
	if (pResponse->m_eAuthSessionResponse == k_EAuthSessionResponseOK)
	{
		// This is the final approval, and means we should let the client play (find the pending auth by steamid)
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (!m_PendingClientData[i].m_bActive)
				continue;
			else if (m_PendingClientData[i].m_SteamIDUser == pResponse->m_SteamID)
			{
				OutputDebugString("Auth completed for a client\n");
				OnAuthCompleted(true, i);
				return;
			}
		}
	}
	else
	{
		// Looks like we shouldn't let this user play, kick them
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (!m_PendingClientData[i].m_bActive)
				continue;
			else if (m_PendingClientData[i].m_SteamIDUser == pResponse->m_SteamID)
			{
				OutputDebugString("Auth failed for a client\n");
				OnAuthCompleted(false, i);
				return;
			}
		}
	}
}

void GameServer::OnClientBeginAuthentication(CSteamID steamIDClient, HSteamNetConnection connectionID, void* pToken, uint32 uTokenLen)
{
	// First, check this isn't a duplicate and we already have a user logged on from the same steamid
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_ClientData[i].m_hConn == connectionID)
		{
			// We already logged them on... (should maybe tell them again incase they don't know?)
			return;
		}
	}

	// Second, do we have room?
	uint32 nPendingOrActivePlayerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_PendingClientData[i].m_bActive)
			++nPendingOrActivePlayerCount;

		if (m_ClientData[i].m_bActive)
			++nPendingOrActivePlayerCount;
	}

	// We are full (or will be if the pending players auth), deny new login
	if (nPendingOrActivePlayerCount >= MAX_PLAYERS_PER_SERVER)
	{
		SteamGameServerNetworkingSockets()->CloseConnection(connectionID, static_cast<int>(EDisconnectReason::ServerFull), "Server full", false);
	}

	// If we get here there is room, add the player as pending
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_PendingClientData[i].m_bActive)
		{
			m_PendingClientData[i].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();
#ifdef USE_GS_AUTH_API
			// authenticate the user with the Steam back-end servers
			EBeginAuthSessionResult res = SteamGameServer()->BeginAuthSession(pToken, uTokenLen, steamIDClient);
			if (res != k_EBeginAuthSessionResultOK)
			{
				SteamGameServerNetworkingSockets()->CloseConnection(connectionID, static_cast<int>(EDisconnectReason::ServerReject), "BeginAuthSession failed", false);
				break;
			}

			m_PendingClientData[i].m_SteamIDUser = steamIDClient;
			m_PendingClientData[i].m_bActive = true;
			m_PendingClientData[i].m_hConn = connectionID;
			break;
#else
			m_PendingClientData[i].m_bActive = true;
			// we need to tell the server our Steam id in the non-auth case, so we stashed it in the login message, pull it back out
			m_PendingClientData[i].m_SteamIDUser = *(CSteamID*)pToken;
			m_PendingClientData[i].m_connection = connectionID;
			// You would typically do your own authentication method here and later call OnAuthCompleted
			// In this sample we just automatically auth anyone who connects
			OnAuthCompleted(true, i);
			break;
#endif
		}
	}
}

void GameServer::OnAuthCompleted(bool bAuthSuccessful, uint32 iPendingAuthIndex)
{
	if (!m_PendingClientData[iPendingAuthIndex].m_bActive)
	{
		OutputDebugString("Got auth completed callback for client who is not pending\n");
		return;
	}

	if (!bAuthSuccessful)
	{
#ifdef USE_GS_AUTH_API
		// Tell the GS the user is leaving the server
		SteamGameServer()->EndAuthSession(m_PendingClientData[iPendingAuthIndex].m_SteamIDUser);
#endif
		// Send a deny for the client, and zero out the pending data
		MsgServerFailAuthentication_t msg;
		int64 outMessage;
		SteamGameServerNetworkingSockets()->SendMessageToConnection(m_PendingClientData[iPendingAuthIndex].m_hConn, &msg, sizeof(msg), k_nSteamNetworkingSend_Reliable, &outMessage);
		m_PendingClientData[iPendingAuthIndex] = ClientConnectionData_t();
		return;
	}

	bool bAddedOk = false;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (!m_ClientData[i].m_bActive)
		{
			// copy over the data from the pending array
			memcpy(&m_ClientData[i], &m_PendingClientData[iPendingAuthIndex], sizeof(ClientConnectionData_t));
			m_PendingClientData[iPendingAuthIndex] = ClientConnectionData_t();
			m_ClientData[i].m_ulTickCountLastData = m_pGameEngine->GetGameTickCount();

			AddPlayer(i);
			OutputDebugString("Added player!\n");

			MsgServerPassAuthentication_t msg;
			msg.SetPlayerPosition(i);
			SendDataToClient(i, (char*)&msg, sizeof(msg));

			bAddedOk = true;

			break;
		}
	}

	// If we just successfully added the player, check if they are #2 so we can restart the round
	if (bAddedOk)
	{
		uint32 uPlayers = 0;
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			if (m_ClientData[i].m_bActive)
				++uPlayers;
		}

		// If we just got the second player, immediately reset round as a draw.  This will prevent
		// the existing player getting a win, and it will cause a new round to start right off
		// so that the one player can't just float around not letting the new one get into the game.
		if (uPlayers == 2)
		{
			if (m_GameState != EServerGameState::ServerWaitingForPlayers) {
				SetGameState(EServerGameState::ServerActive);
			}
		}
	}
}

bool GameServer::SendDataToClient(uint32 PlayerIndex, char* pData, uint32 nSizeOfData) {
	if (PlayerIndex >= MAX_PLAYERS_PER_SERVER)
		return false;

	int64 messageOut;
	if (!SteamGameServerNetworkingSockets()->SendMessageToConnection(m_ClientData[PlayerIndex].m_hConn, pData, nSizeOfData, k_nSteamNetworkingSend_Unreliable, &messageOut))
	{
		OutputDebugString("Failed sending data to a client\n");
		return false;
	}
	return true;
}

void GameServer::AddPlayer(uint32 PlayerPosition) {
	if (PlayerPosition >= MAX_PLAYERS_PER_SERVER)
	{
		OutputDebugString("Trying to add player at invalid positon\n");
		return;
	}

	if (m_Players[PlayerPosition])
	{
		OutputDebugString("Trying to add a player where one already exists\n");
		return;
	}

	m_Players[PlayerPosition] = new GamePlayer(m_pGameEngine); // Default middle of screen
}

void GameServer::KickPlayerOffServer(CSteamID steamID)
{
	uint32 uPlayerCount = 0;
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		// If there is no player, skip
		if (!m_ClientData[i].m_bActive)
			continue;

		if (m_ClientData[i].m_SteamIDUser == steamID)
		{
			OutputDebugString("Kicking player\n");
			RemovePlayerFromServer(i, EDisconnectReason::ClientKicked);
			// send him a kick message
			MsgServerFailAuthentication_t msg;
			int64 outMessage;
			SteamGameServerNetworkingSockets()->SendMessageToConnection(m_ClientData[i].m_hConn, &msg, sizeof(msg), k_nSteamNetworkingSend_Reliable, &outMessage);
		}
		else
		{
			++uPlayerCount;
		}
	}
	m_PlayerCount = uPlayerCount;
}

void GameServer::RemovePlayerFromServer(uint32 PlayerArrPosition, EDisconnectReason reason) {
	if (PlayerArrPosition >= MAX_PLAYERS_PER_SERVER)
	{
		OutputDebugString("Trying to remove player at invalid position\n");
		return;
	}

	if (!m_Players[PlayerArrPosition])
	{
		OutputDebugString("Trying to remove a player that does not exist\n");
		return;
	}

	OutputDebugString("Removing a player\n");
	delete m_Players[PlayerArrPosition];
	m_Players[PlayerArrPosition] = NULL;

	// close the hNet connection
	SteamGameServerNetworkingSockets()->CloseConnection(m_ClientData[PlayerArrPosition].m_hConn, static_cast<int>(reason), nullptr, false);

#ifdef USE_GS_AUTH_API
	// Tell the GS the user is leaving the server
	SteamGameServer()->EndAuthSession(m_ClientData[PlayerArrPosition].m_SteamIDUser);
#endif
	m_ClientData[PlayerArrPosition] = ClientConnectionData_t();
}

CSteamID GameServer::GetSteamID()
{
#ifdef USE_GS_AUTH_API
	return SteamGameServer()->GetSteamID();
#else
	// this is a placeholder steam id to use when not making use of Steam auth or matchmaking
	return k_steamIDNonSteamGS;
#endif
}