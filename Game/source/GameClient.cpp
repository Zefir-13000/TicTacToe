#include <GameClient.h>
#include <Game.h>

#include <AllObjectTypes.h>

GameClient* g_pGameClient = NULL;
GameClient* GlobalGameClient() { return g_pGameClient; }

GameClient::GameClient(GameEngine* pEngine) {
	if (!SteamUser()->BLoggedOn())
	{
		Alert("Fatal Error", "Steam user must be logged in to play this game (SteamUser()->BLoggedOn() returned false).\n");
		return;
	}

	m_LocalSteamID = SteamUser()->GetSteamID();
	m_eGameState = EClientGameState::ClientGameMenu;
	g_pGameClient = this;
	m_pGameEngine = pEngine;

	m_pServer = nullptr;
	m_unServerIP = 0;
	m_usServerPort = 0;
	m_hConnServer = k_HSteamNetConnection_Invalid;

	m_eConnectedStatus = k_EClientNotConnected;

	SteamNetworkingUtils()->InitRelayNetworkAccess();

	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		m_Players[i] = NULL;
		m_PlayersNames[i] = "\0";
	}

	srand((uint32)time(NULL));

	m_pP2PAuthedGame = new P2PAuthedGame(pEngine);
	m_pLobby = new GameLobby(pEngine);
}

GameClient::~GameClient()
{
	ResetClientData();
	DisconnectFromServer();

	if (m_pP2PAuthedGame)
	{
		m_pP2PAuthedGame->EndGame();
		delete m_pP2PAuthedGame;
		m_pP2PAuthedGame = NULL;
	}

	if (m_pServer)
	{
		delete m_pServer;
		m_pServer = NULL;
	}

	if (m_pLobby) {
		delete m_pLobby;
		m_pLobby = NULL;
	}

	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		if (m_Players[i])
		{
			delete m_Players[i];
			m_Players[i] = NULL;
		}
	}
}

void GameClient::ReceiveNetworkData() {
	if (!SteamNetworkingSockets())
		return;
	if (m_hConnServer == k_HSteamNetConnection_Invalid)
		return;

	SteamNetworkingMessage_t* msgs[32];
	int res = SteamNetworkingSockets()->ReceiveMessagesOnConnection(m_hConnServer, msgs, 32);
	for (int i = 0; i < res; i++)
	{
		SteamNetworkingMessage_t* message = msgs[i];
		uint32 cubMsgSize = message->GetSize();

		m_ulLastNetworkDataReceivedTime = m_pGameEngine->GetGameTickCount();

		// make sure we're connected
		if (m_eConnectedStatus == k_EClientNotConnected && m_eGameState != EClientGameState::ClientGameConnecting)
		{
			message->Release();
			continue;
		}

		if (cubMsgSize < sizeof(DWORD))
		{
			OutputDebugString("Got garbage on client socket, too short\n");
			message->Release();
			continue;
		}

		EMessage eMsg = (EMessage)(*(DWORD*)message->GetData());
		switch (eMsg)
		{
		case k_EMsgServerSendInfo:
		{
			if (cubMsgSize != sizeof(MsgServerSendInfo_t))
			{
				OutputDebugString("Bad server info msg\n");
				break;
			}
			MsgServerSendInfo_t* pMsg = (MsgServerSendInfo_t*)message->GetData();

			// pull the IP address of the user from the socket
			OnReceiveServerInfo(CSteamID(pMsg->GetSteamIDServer()), pMsg->GetSecure(), pMsg->GetServerName());
		}
		break;

		case k_EMsgServerPassAuthentication:
		{
			if (cubMsgSize != sizeof(MsgServerPassAuthentication_t))
			{
				OutputDebugString("Bad accept connection msg\n");
				break;
			}
			MsgServerPassAuthentication_t* pMsg = (MsgServerPassAuthentication_t*)message->GetData();

			// Our game client doesn't really care about whether the server is secure, or what its 
			// steamID is, but if it did we would pass them in here as they are part of the accept message
			OnReceiveServerAuthenticationResponse(true, pMsg->GetPlayerPosition());
		}
		break;

		case k_EMsgServerFailAuthentication:
		{
			OnReceiveServerAuthenticationResponse(false, 0);
		}
		break;

		case k_EMsgServerUpdateWorld:
		{
			if (cubMsgSize != sizeof(MsgServerUpdateWorld_t))
			{
				OutputDebugString("Bad server world update msg\n");
				break;
			}

			MsgServerUpdateWorld_t* pMsg = (MsgServerUpdateWorld_t*)message->GetData();
			OnReceiveServerUpdate(pMsg->AccessUpdateData());
		}
		break;

		case k_EMsgServerExiting:
		{
			if (cubMsgSize != sizeof(MsgServerExiting_t))
			{
				OutputDebugString("Bad server exiting msg\n");
			}
			OnReceiveServerExiting();
		}
		break;

		case k_EMsgP2PSendingTicket:
			// This is really bad exmaple code that just assumes the message is the right size
			// Don't ship code like this.
			m_pP2PAuthedGame->HandleP2PSendingTicket(message->GetData());
			break;

		default:
			OutputDebugString("Unhandled message from server\n");
			break;

		}

		message->Release();
	}

	if (m_pServer)
	{
		m_pServer->ReceiveNetworkData();
	}
}

void GameClient::JoinLobby(LobbyBrowserMenuItem_t lobbyInfo) {
	if (lobbyInfo.m_eStateToTransitionTo == EClientGameState::ClientJoiningLobby)
	{
		SteamAPICall_t hSteamAPICall = SteamMatchmaking()->JoinLobby(lobbyInfo.m_steamIDLobby);
		// set the function to call when this API completes
		m_SteamCallResultLobbyEntered.Set(hSteamAPICall, this, &GameClient::OnLobbyEntered);
	
		SetGameState(lobbyInfo.m_eStateToTransitionTo);
	}
}

void GameClient::OnLobbyGameCreated(LobbyGameCreated_t* pCallback)
{
	if (m_eGameState != EClientGameState::ClientInLobby)
		return;

	// join the game server specified, via whichever method we can
	if (CSteamID(pCallback->m_ulSteamIDGameServer).IsValid())
	{
		//ResetClientData();
		InitiateServerConnection(CSteamID(pCallback->m_ulSteamIDGameServer));
	}
}

void GameClient::OnLobbyUpdate(LobbyChatUpdate_t* pCallback) {
	std::string username;
	if (pCallback->m_ulSteamIDUserChanged != m_LocalSteamID.ConvertToUint64() && pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeEntered) {
		m_SteamIDPlayers[!m_PlayerIndex] = pCallback->m_ulSteamIDUserChanged;
		m_PlayersNames[!m_PlayerIndex] = SteamFriends()->GetFriendPersonaName(pCallback->m_ulSteamIDUserChanged);
		m_PlayerTextures[!m_PlayerIndex] = GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(pCallback->m_ulSteamIDUserChanged));
	}

	if (pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeEntered) {
		OutputDebugStringA((username + " joined lobby\n").c_str());
	}
	else if (pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeLeft || pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeDisconnected) {
		OutputDebugStringA((username + " left lobby\n").c_str());
		if (pCallback->m_ulSteamIDUserChanged != GetLocalSteamID().ConvertToUint64()) {
			int playerIndex = GetLobbyUserIndex(CSteamID(pCallback->m_ulSteamIDUserChanged));
			RemovePlayerFromLobby(playerIndex);
			m_PlayerReady[playerIndex] = false;
			m_PlayerReady[m_PlayerIndex] = false;
		}
	}
	else if (pCallback->m_rgfChatMemberStateChange == k_EChatMemberStateChangeKicked) {
		OutputDebugStringA((username + " kicked from lobby\n").c_str());
	}
}

void GameClient::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback) {
	if (m_pLobby->GetLobbySteamID() != pCallback->m_ulSteamIDLobby)
		return;

	if (m_eGameState == EClientGameState::ClientInLobby) {
		TextObject* player1_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_ready"));
		TextObject* player2_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_ready"));

		const char* readyText = SteamMatchmaking()->GetLobbyMemberData(m_pLobby->GetLobbySteamID(), pCallback->m_ulSteamIDMember, "IsReady");
		D2D1::ColorF ready_green = D2D1::ColorF(0.0, 1.0, 0.0, 1.0);

		if (readyText && strcmp(readyText, "true") == 0) {
			m_PlayerReady[GetLobbyUserIndex(CSteamID(pCallback->m_ulSteamIDMember))] = true;
		}
		else if (readyText && strcmp(readyText, "false") == 0) {
			m_PlayerReady[GetLobbyUserIndex(CSteamID(pCallback->m_ulSteamIDMember))] = false;
		}

		if (pCallback->m_ulSteamIDMember == GetLocalSteamID().ConvertToUint64()) {
			if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				if (readyText && strcmp(readyText, "true") == 0) {
					player1_readyText->SetText("Ready");
					player1_readyText->SetColor(ready_green);
				}
			}
			else {
				if (readyText && strcmp(readyText, "true") == 0) {
					player2_readyText->SetText("Ready");
					player2_readyText->SetColor(ready_green);
				}
			}
		}
		else {
			if (!m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				if (readyText && strcmp(readyText, "true") == 0) {
					player1_readyText->SetText("Ready");
					player1_readyText->SetColor(ready_green);
				}
			}
			else {
				if (readyText && strcmp(readyText, "true") == 0) {
					player2_readyText->SetText("Ready");
					player2_readyText->SetColor(ready_green);
				}
			}
		}
	}
}


int GameClient::GetLobbyUserIndex(CSteamID steamId) {
	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
		if (steamId == m_SteamIDPlayers[i]) {
			return i;
		}
	}
	return -1;
}

void GameClient::RemovePlayerFromLobby(int PlayerIndex) {
	TextObject* player1_nameText = nullptr;
	TextObject* player2_nameText = nullptr;

	TextObject* player1_readyText = nullptr;
	TextObject* player2_readyText = nullptr;

	TextureObject* player1_avatar = nullptr;
	TextureObject* player2_avatar = nullptr;

	ButtonObject* game_start_button = nullptr;

	player1_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_name"));
	player2_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_name"));

	player1_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_ready"));
	player2_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_ready"));

	player1_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_img"));
	player2_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_img"));

	D2D1::ColorF red_color = D2D1::ColorF(1.0, 0, 0, 1.0);
	if (player2_nameText && player2_readyText) {
		player2_nameText->SetText("");
		player2_readyText->SetText("unready");
		
		player2_readyText->SetColor(red_color);
		player2_readyText->SetActive(false);
	}
	ButtonObject* player_invite_button = static_cast<ButtonObject*>(GetPGame()->GetScene()->FindObjectByName("Player_Invite"));
	if (player_invite_button)
		player_invite_button->SetActive(true);

	if (m_pLobby->CheckIsLobbyOwner(m_SteamIDPlayers[PlayerIndex]) && player2_nameText && player2_readyText) {
		player1_nameText->SetText(GetLocalPlayerName());
		player1_readyText->SetText("unready");
		player1_readyText->SetColor(red_color);
		SteamMatchmaking()->SetLobbyMemberData(GetLobbySteamID(), "isReady", "false");
		m_pLobby->SetLobbyOwnerSteamID(GetLocalSteamID());
		player1_avatar->SetTexture(GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(GetLocalSteamID())));
		m_PlayersNames[PlayerIndex] = m_PlayersNames[m_PlayerIndex];
		m_SteamIDPlayers[PlayerIndex] = m_SteamIDPlayers[m_PlayerIndex];
		game_start_button = static_cast<ButtonObject*>(GetPGame()->GetScene()->FindObjectByName("Lobby_Start"));
		game_start_button->SetActive(true);
		game_start_button->SetClickable(false);
		m_PlayerIndex = PlayerIndex;
	}

	if (player2_avatar)
		player2_avatar->LoadTextureFromFile("empty_user_picture.png");
}

void GameClient::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* pCallback)
{
	// parse out the connect 
	const char* pchServerAddress, *pchLobbyID;
	OutputDebugString("Join lobby requested");
	LobbyBrowserMenuItem_t menuItem = { pCallback->m_steamIDLobby, EClientGameState::ClientJoiningLobby };
	JoinLobby(menuItem);
}

void GameClient::MakeMove(int index) {
	int row = index / 3 + 1;
	int col = index % 3 + 1;

	OutputDebugString("Made move");
	std::string mademove = "Made move, X: " + std::to_string(col) + ", Y: " + std::to_string(row);
	OutputDebugStringA(mademove.c_str());
	m_Players[m_PlayerIndex]->SetMove(col, row);
	last_x = col;
	last_y = row;
}

void GameClient::ResetClientData() {
	for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
		if (m_Players[i]) {
			delete m_Players[i];
			m_Players[i] = nullptr;
		}
		if (m_PlayerTextures[i]) {
			for (Object* object : GetPGame()->GetScene()->GetSceneObjects()) {
				if (object->GetObjectType() == Object_TextureType) {
					TextureObject* textureObj = static_cast<TextureObject*>(object);
					if (textureObj->GetTexture() == m_PlayerTextures[i]) {
						textureObj->SetTexture(nullptr);
						break;
					}
				}
			}
			m_PlayerTextures[i] = nullptr;
		}
	}
}

void GameClient::OnSteamServersConnected(SteamServersConnected_t* callback)
{
	if (SteamUser()->BLoggedOn()) {
		if (m_pLobby->GetLobbySteamID().IsValid()) {
			m_eGameState = EClientGameState::ClientInLobby;
		}
		else {
			m_eGameState = EClientGameState::ClientGameMenu;
		}
	}
	else
	{
		OutputDebugString("Got SteamServersConnected_t, but not logged on?\n");
	}
}

void GameClient::OnSteamServersDisconnected(SteamServersDisconnected_t* callback)
{
	SetGameState(EClientGameState::ClientConnectingToSteam);
	OutputDebugString("Got SteamServersDisconnected_t\n");
}

void GameClient::InitiateServerConnection(CSteamID steamIDGameServer) {
	if (m_eGameState == EClientGameState::ClientInLobby && m_pLobby->GetLobbySteamID().IsValid())
	{
		SteamMatchmaking()->LeaveLobby(m_pLobby->GetLobbySteamID());
	}

	SetGameState(EClientGameState::ClientGameConnecting);

	m_steamIDGameServerFromBrowser = m_steamIDGameServer = steamIDGameServer;

	SteamNetworkingIdentity identity;
	identity.SetSteamID(steamIDGameServer);

	std::cout << "Connecting to server..." << std::endl;
	m_hConnServer = SteamNetworkingSockets()->ConnectP2P(identity, 0, 0, nullptr);
	if (m_pP2PAuthedGame)
		m_pP2PAuthedGame->m_hConnServer = m_hConnServer;

	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();
}

void GameClient::InitiateServerConnection(uint32 unServerAddress, const int32 nPort) {
	if (m_eGameState == EClientGameState::ClientInLobby && m_pLobby->GetLobbySteamID().IsValid())
	{
		SteamMatchmaking()->LeaveLobby(m_pLobby->GetLobbySteamID());
	}

	SetGameState(EClientGameState::ClientGameConnecting);

	// Update when we last retried the connection, as well as the last packet received time so we won't timeout too soon,
	// and so we will retry at appropriate intervals if packets drop
	m_ulLastNetworkDataReceivedTime = m_ulLastConnectionAttemptRetryTime = m_pGameEngine->GetGameTickCount();

	// ping the server to find out what it's steamID is
	m_unServerIP = unServerAddress;
	m_usServerPort = (uint16)nPort;
	m_GameServerPing.RetrieveSteamIDFromGameServer(this, m_unServerIP, m_usServerPort);
}

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 2)
{
	std::ostringstream out;
	out.precision(n);
	out << std::fixed << a_value;
	return std::move(out).str();
}

void GameClient::Tick() {
	ReceiveNetworkData();

	if (m_eConnectedStatus != k_EClientNotConnected && m_pGameEngine->GetGameTickCount() - m_ulLastNetworkDataReceivedTime > MILLISECONDS_CONNECTION_TIMEOUT)
	{
		OutputDebugString("Game server connection failure.");
		DisconnectFromServer(); // cleanup on our side, even though server won't get our disconnect msg
		SetGameState(EClientGameState::ClientGameConnectionFailure);
	}

	SteamAPI_RunCallbacks();

	if (m_bTransitionedGameState)
	{
		m_bTransitionedGameState = false;
		OnGameStateChanged(m_eGameState);
		m_bJustTransitionedGameState = true;
	}

	// Game vars

	TextObject* lobbyText = nullptr;
	TextObject* player1_nameText = nullptr;
	TextObject* player2_nameText = nullptr;

	TextObject* player1_readyText = nullptr;
	TextObject* player2_readyText = nullptr;

	TextureObject* player1_avatar = nullptr;
	TextureObject* player2_avatar = nullptr;

	ButtonObject* player_invite_button = nullptr;
	ButtonObject* game_start_button = nullptr;

	// ---------

	if (m_eGameState == EClientGameState::ClientConnectingToSteam) {
		GetPGame()->Render("LoadScreen");
	}
	else if (m_eGameState == EClientGameState::ClientGameMenu) {
		GetPGame()->Render("MainMenu");
	}
	else if (m_eGameState == EClientGameState::ClientCreatingLobby) {
		GetPGame()->Render("LoadScreen");
	}
	else if (m_eGameState == EClientGameState::ClientInLobby) {
		// display the lobby
		//m_pLobby->RunFrame();
		GetPGame()->Render("LobbyScreen");
		lobbyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("LobbyName"));
		player1_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_name"));
		player2_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_name"));

		player1_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_ready"));
		player2_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_ready"));

		player1_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_img"));
		player2_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_img"));

		player_invite_button = static_cast<ButtonObject*>(GetPGame()->GetScene()->FindObjectByName("Player_Invite"));
		game_start_button = static_cast<ButtonObject*>(GetPGame()->GetScene()->FindObjectByName("Lobby_Start"));

		if (m_bJustTransitionedGameState) {
			m_bJustTransitionedGameState = false;

			if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				game_start_button->SetActive(true);
				player_invite_button->SetActive(true);
				player1_avatar->SetTexture(m_PlayerTextures[m_PlayerIndex]);
			}
			else {
				player2_readyText->SetActive(true);
				if (m_bSetHostReady) {
					player1_readyText->SetText("Ready");
					D2D1::ColorF ready_green = D2D1::ColorF(0.0, 1.0, 0.0, 1.0);
					player1_readyText->SetColor(ready_green);
					m_bSetHostReady = false;
				}
			}
		}

		if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
			if (m_pLobby->GetLobbyName() != lobbyText->GetText()) {
				lobbyText->SetText(m_pLobby->GetLobbyName());

				player1_nameText->SetText(m_PlayersNames[m_PlayerIndex]);
			}

			if (m_PlayerTextures[!m_PlayerIndex] != nullptr && player2_avatar->GetTexture() != m_PlayerTextures[!m_PlayerIndex]) {
				player2_nameText->SetText(m_PlayersNames[!m_PlayerIndex]);
				player2_avatar->SetTexture(m_PlayerTextures[!m_PlayerIndex]);
				player_invite_button->SetActive(false);
				player2_readyText->SetActive(true);
			}

			bool ready_flag = true;
			for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
				if (!m_PlayerReady[i]) {
					ready_flag = false;
					break;
				}
			}

			if (ready_flag) {
				if (!game_start_button->GetClickable())
					game_start_button->SetClickable(true);
			}
			else if (game_start_button->GetClickable()) {
				game_start_button->SetClickable(false);
			}

			if (m_pServer)
				goto exit_state_check;


			//SteamMatchmaking()->SetLobbyData(m_steamIDLobby, "game_starting", "1");

			// start a local game server
			//m_pServer = new GameServer(m_pGameEngine);
			// we'll have to wait until the game server connects to the Steam server back-end 
			// before telling all the lobby members to join (so that the NAT traversal code has a path to contact the game server)
			//OutputDebugString("Game server being created; game will start soon.\n");
		}
		else {
			if (!m_pLobby->GetIsSetLobbyName()) {
				std::string lobbyName = SteamMatchmaking()->GetLobbyData(m_pLobby->GetLobbySteamID(), "name");
				lobbyText->SetText(lobbyName);
				m_pLobby->SetLobbyName(lobbyName);

				if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
					player1_nameText->SetText(m_PlayersNames[m_PlayerIndex]);
					if (m_PlayerTextures[m_PlayerIndex] != nullptr && player1_avatar->GetTexture() != m_PlayerTextures[m_PlayerIndex])
						player1_avatar->SetTexture(m_PlayerTextures[m_PlayerIndex]);

					player2_nameText->SetText(m_PlayersNames[!m_PlayerIndex]);
					if (m_PlayerTextures[!m_PlayerIndex] != nullptr && player2_avatar->GetTexture() != m_PlayerTextures[!m_PlayerIndex])
						player2_avatar->SetTexture(m_PlayerTextures[!m_PlayerIndex]);
				}
				else {
					player2_nameText->SetText(m_PlayersNames[m_PlayerIndex]);
					if (m_PlayerTextures[m_PlayerIndex] != nullptr && player2_avatar->GetTexture() != m_PlayerTextures[m_PlayerIndex])
						player2_avatar->SetTexture(m_PlayerTextures[m_PlayerIndex]);

					player1_nameText->SetText(m_PlayersNames[!m_PlayerIndex]);
					if (m_PlayerTextures[!m_PlayerIndex] != nullptr && player1_avatar->GetTexture() != m_PlayerTextures[!m_PlayerIndex])
						player1_avatar->SetTexture(m_PlayerTextures[!m_PlayerIndex]);
				}
			}
		}
	}
	else if (m_eGameState == EClientGameState::ClientJoiningLobby) {
		GetPGame()->Render("LoadScreen");
		if (m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT)
		{
			OutputDebugString("Timed out connecting to lobby.");
			SetGameState(EClientGameState::ClientGameConnectionFailure);
		}
	}
	else if (m_eGameState == EClientGameState::ClientGameConnectionFailure) {
		SetGameState(EClientGameState::ClientGameMenu);
	}

	else if (m_eGameState == EClientGameState::ClientGameConnecting) {
		// Draw text telling the user a connection attempt is in progress
		//DrawConnectionAttemptText();
		GetPGame()->Render("LoadScreen");
		// Check if we've waited too long and should time out the connection
		if (m_pGameEngine->GetGameTickCount() - m_ulStateTransitionTime > MILLISECONDS_CONNECTION_TIMEOUT)
		{
			DisconnectFromServer();
			m_GameServerPing.CancelPing();
			std::cout << "Timed out connecting to game server" << std::endl;
			OutputDebugString("Timed out connecting to game server");
			SetGameState(EClientGameState::ClientGameConnectionFailure);
		}

	}
	else if (m_eGameState == EClientGameState::ClientGameStartServer) {
		GetPGame()->Render("LoadScreen");

		if (m_pServer && m_pServer->IsConnectedToSteam())
		{
			// server is up; tell everyone else to connect
			SteamMatchmaking()->SetLobbyGameServer(GetLobbySteamID(), 0, 0, m_pServer->GetSteamID());
			//ResetClientData();
			// start connecting ourself via localhost (this will automatically leave the lobby)
			InitiateServerConnection(m_pServer->GetSteamID());
		}

		if (m_pServer)
			goto exit_state_check;

		m_pServer = new GameServer(m_pGameEngine);
	}
	else if (m_eGameState == EClientGameState::ClientGameActive) {
		GetPGame()->Render("GameScreen");
		if (m_bJustTransitionedGameState) {
			m_bJustTransitionedGameState = false;
			for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
				m_PlayerTextures[i] = nullptr;
				m_PlayersNames[i] = SteamFriends()->GetFriendPersonaName(m_SteamIDPlayers[i]);
			}
			m_DeltaTimeTicks = m_pGameEngine->GetGameTickCount();
			memset(m_GameData.m_TicTacTable, 0, sizeof(m_GameData.m_TicTacTable));
		}

		player1_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_name"));
		player2_nameText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_name"));

		player1_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_img"));
		player2_avatar = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_img"));

		if (player1_nameText && player2_nameText) {
			if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				player1_nameText->SetText(m_PlayersNames[m_PlayerIndex]);
				player2_nameText->SetText(m_PlayersNames[!m_PlayerIndex]);
			}
			else {
				player1_nameText->SetText(m_PlayersNames[!m_PlayerIndex]);
				player2_nameText->SetText(m_PlayersNames[m_PlayerIndex]);
			}
		}
		if (player1_avatar && player2_avatar) {
			for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
				if (!m_PlayerTextures[i]) {
					m_PlayerTextures[i] = GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(m_SteamIDPlayers[i]));
				}
			}

			if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				if (m_PlayerTextures[m_PlayerIndex])
					player1_avatar->SetTexture(m_PlayerTextures[m_PlayerIndex]);
				if (m_PlayerTextures[!m_PlayerIndex])
					player2_avatar->SetTexture(m_PlayerTextures[!m_PlayerIndex]);
			}
			else {
				if (m_PlayerTextures[!m_PlayerIndex])
					player1_avatar->SetTexture(m_PlayerTextures[!m_PlayerIndex]);
				if (m_PlayerTextures[m_PlayerIndex])
					player2_avatar->SetTexture(m_PlayerTextures[m_PlayerIndex]);
			}
		}

		std::unordered_map<std::string, ButtonObject*> TableCells;
		for (Object* obj : GetPGame()->GetScene()->GetSceneObjects()) {
			std::string obj_name = obj->GetName();
			if (obj_name.starts_with("Object_")) {
				TableCells.insert({ obj_name, static_cast<ButtonObject*>(obj) });
				if (m_GameData.m_PlayerTurnIndex == m_PlayerIndex && !m_GameData.m_RoundEnded) {
					if (!static_cast<ButtonObject*>(obj)->GetClickable()) {
						static_cast<ButtonObject*>(obj)->SetClickable(true);
					}
				}
				else {
					if (static_cast<ButtonObject*>(obj)->GetClickable()) {
						static_cast<ButtonObject*>(obj)->SetClickable(false);
					}
				}
			}
		}

		for (int i = 0; i < 9; ++i) {
			std::string objAction = convertToTwoDigitIndex(i);
			std::string objName = "Object_" + objAction;
			TableCells[objName]->GetTextObject()->SetText(convertToTicTacUnit(m_GameData.m_TicTacTable[i]));
		}

		TextObject* GameTimer = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("GameTimer"));
		if (GameTimer) {
			GameTimer->SetText(to_string_with_precision(roundf(m_LocalGameTimer * 100) / 100));
			if (m_LocalGameTimer < 5) {
				D2D1::ColorF red(1,0,0,1);
				GameTimer->SetColor(red);
			}
			else {
				D2D1::ColorF black(0, 0, 0, 1);
				GameTimer->SetColor(black);
			}
		}

		m_LocalGameTimer -= (m_pGameEngine->GetGameTickCount() - m_DeltaTimeTicks) / 1000.0f;
		m_DeltaTimeTicks = m_pGameEngine->GetGameTickCount();
		if (m_LocalGameTimer <= 0.0f) {
			m_LocalGameTimer = 0.0f;
		}
		if (abs(m_LocalGameTimer - m_GameData.m_GameTimer) > 1.5f) {
			m_LocalGameTimer = m_GameData.m_GameTimer;
		}

		TextObject* RoundText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("RoundText"));
		if (RoundText) {
			if (m_GameData.m_RoundEnded) {
				if (m_GameData.m_RoundEnded != m_PrevGameData.m_RoundEnded)
					m_LocalGameTimer = 5.0f;
				if (m_GameData.m_WinPlayerIndex >= MAX_PLAYERS_PER_SERVER) {
					RoundText->SetText("Draw!");
				}
				else
					RoundText->SetText(m_PlayersNames[m_GameData.m_WinPlayerIndex] + " Won!");
			}
			else {
				RoundText->SetText("");
			}
		}

		TextObject* Player1_unit = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_unit"));
		TextObject* Player2_unit = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_unit"));
		if (Player1_unit && Player2_unit) {
			if (m_GameData.m_HostIsCrosses) {
				Player1_unit->SetText("X");
				Player2_unit->SetText("O");
			}
			else {
				Player1_unit->SetText("O");
				Player2_unit->SetText("X");
			}
		}

		TextureObject* Player1_turn = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_turn"));
		TextureObject* Player2_turn = static_cast<TextureObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_turn"));
		if (Player1_turn && Player2_turn) {
			if (m_pLobby->CheckIsLobbyOwner(GetLocalSteamID())) {
				if (m_GameData.m_PlayerTurnIndex == m_PlayerIndex) {
					Player1_turn->SetActive(true);
					Player2_turn->SetActive(false);
				}
				else {
					Player1_turn->SetActive(false);
					Player2_turn->SetActive(true);
				}
			}
			else {
				if (m_GameData.m_PlayerTurnIndex == m_PlayerIndex) {
					Player1_turn->SetActive(false);
					Player2_turn->SetActive(true);
				}
				else {
					Player1_turn->SetActive(true);
					Player2_turn->SetActive(false);
				}
			}
		}

		if (m_GameData.m_GameTimer > m_PrevGameData.m_GameTimer) {
			m_LocalGameTimer = m_GameData.m_GameTimer;
		}

		memcpy(&m_PrevGameData, &m_GameData, sizeof(GameStateUpdateData_t));
		for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
		{
			// TODO: game loop
			/*if (m_Players[i])
				m_Players[i]->Tick();*/
		}
	}
	else if (m_eGameState == EClientGameState::ClientGameExiting) {
		DisconnectFromServer();
		return;
	}
	else {
		OutputDebugString("Unhandled game state in GameClient::Tick\n");
	}

exit_state_check:

	if (m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_Players[m_PlayerIndex])
	{
		MsgClientSendLocalUpdate_t msg;
		//msg.SetShipPosition(m_PlayerIndex);

		// Send update as unreliable message.  This means that if network packets drop,
		// the networking system will not attempt retransmission, and our message may not arrive.
		// That's OK, because we would rather just send a new, update message, instead of
		// retransmitting the old one.
		if (m_Players[m_PlayerIndex]->GetClientUpdateData(msg.AccessUpdateData())) {
			msg.AccessUpdateData()->SetValid();
			msg.AccessUpdateData()->SetMove(last_x, last_y);
			SendServerData(&msg, sizeof(msg), k_nSteamNetworkingSend_Reliable);
		}
		else {
			SendServerData(&msg, sizeof(msg), k_nSteamNetworkingSend_Unreliable);
		}
	}

	if (m_pP2PAuthedGame)
	{
		if (m_pServer)
		{
			for (int i = 1; i < MAX_PLAYERS_PER_SERVER; i++)
			{

				if (m_pP2PAuthedGame->m_P2PAuthPlayer[i] && !m_pP2PAuthedGame->m_P2PAuthPlayer[i]->BIsAuthOk())
				{
					m_pServer->KickPlayerOffServer(m_pP2PAuthedGame->m_P2PAuthPlayer[i]->m_steamID);
				}
			}
		}
		else {
			if (m_pP2PAuthedGame->m_P2PAuthPlayer[0])
			{
				if (!m_pP2PAuthedGame->m_P2PAuthPlayer[0]->BIsAuthOk())
				{
					// leave the game
					SetGameState(EClientGameState::ClientGameMenu);
				}
			}
		}
	}

	if (m_pServer)
	{
		m_pServer->Tick();
	}
}

Texture* GameClient::GetPlayerAvatar(int iImage) {
	uint32_t width, height;
	bool bIsValid = SteamUtils()->GetImageSize(iImage, &width, &height);
	if (bIsValid) {
		uint8_t* ImageData = new uint8_t[width * height * 4];
		bIsValid = SteamUtils()->GetImageRGBA(iImage, ImageData, width * height * 4);
		
		if (bIsValid) {
			Texture* result = new Texture(ImageData, width, height);
			return result;
		}
	}
	return nullptr;
}

void GameClient::SetGameState(EClientGameState eState) {
	if (m_eGameState == eState)
		return;

	m_bTransitionedGameState = true;
	m_ulStateTransitionTime = m_pGameEngine->GetGameTickCount();
	m_eGameState = eState;

	UpdateRichPresenceConnectionInfo();
}

void GameClient::OnReceiveServerInfo(CSteamID steamIDGameServer, bool bVACSecure, const char* pchServerName) {
	m_eConnectedStatus = k_EClientConnectedPendingAuthentication;
	m_steamIDGameServer = steamIDGameServer;

	SteamNetConnectionInfo_t info;
	SteamNetworkingSockets()->GetConnectionInfo(m_hConnServer, &info);
	m_unServerIP = info.m_addrRemote.GetIPv4();
	m_usServerPort = info.m_addrRemote.m_port;

	// set how to connect to the game server, using the Rich Presence API
	// this lets our friends connect to this game via their friends list
	UpdateRichPresenceConnectionInfo();

	MsgClientBeginAuthentication_t msg;
#ifdef USE_GS_AUTH_API
	SteamNetworkingIdentity snid;
	// if the server Steam ID was aquired from another source ( m_steamIDGameServerFromBrowser )
	// then use it as the identity
	// if it only came from the server itself, then use the IP address
	if (m_steamIDGameServer == m_steamIDGameServerFromBrowser)
		snid.SetSteamID(m_steamIDGameServer);
	else
		snid.SetIPv4Addr(m_unServerIP, m_usServerPort);
	char rgchToken[1024];
	uint32 unTokenLen = 0;
	m_hAuthTicket = SteamUser()->GetAuthSessionTicket(rgchToken, sizeof(rgchToken), &unTokenLen, &snid);
	msg.SetToken(rgchToken, unTokenLen);

#else
	// When you aren't using Steam auth you can still call AdvertiseGame() so you can communicate presence data to the friends
	// system. Make sure to pass k_steamIDNonSteamGS
	uint32 unTokenLen = SteamUser()->AdvertiseGame(k_steamIDNonSteamGS, m_unServerIP, m_usServerPort);
	msg.SetSteamID(SteamUser()->GetSteamID().ConvertToUint64());
#endif

	if (msg.GetTokenLen() < 1)
		OutputDebugString("Warning: Looks like GetAuthSessionTicket didn't give us a good ticket\n");

	SendServerData(&msg, sizeof(msg), k_nSteamNetworkingSend_Reliable);
}

void GameClient::OnReceiveServerAuthenticationResponse(bool bSuccess, uint32 uPlayerPosition) {
	if (!bSuccess)
	{
		OutputDebugString("Connection failure.\nMultiplayer authentication failed\n");
		SetGameState(EClientGameState::ClientGameConnectionFailure);
		DisconnectFromServer();
	}
	else
	{
		// Is this a duplicate message? If so ignore it...
		if (m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_PlayerIndex == uPlayerPosition)
			return;

		m_PlayerIndex = uPlayerPosition;
		m_eConnectedStatus = k_EClientConnectedAndAuthenticated;
		OutputDebugString("Connected to server");

		// set information so our friends can join the lobby
		UpdateRichPresenceConnectionInfo();

		// tell steam china duration control system that we are in a match and not to be interrupted
		//SteamUser()->BSetDurationControlOnlineState(k_EDurationControlOnlineState_OnlineHighPri);
	}
}

void GameClient::OnReceiveServerExiting()
{
	if (m_pP2PAuthedGame)
		m_pP2PAuthedGame->EndGame();

#ifdef USE_GS_AUTH_API
	if (m_hAuthTicket != k_HAuthTicketInvalid)
	{
		SteamUser()->CancelAuthTicket(m_hAuthTicket);
	}
	m_hAuthTicket = k_HAuthTicketInvalid;
#else
	SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
#endif

	if (m_eGameState != EClientGameState::ClientGameActive)
		return;
	m_eConnectedStatus = k_EClientNotConnected;

	OutputDebugString("Game server has exited.");
	SetGameState(EClientGameState::ClientGameConnectionFailure);
}

void GameClient::OnReceiveServerUpdate(ServerGameUpdateData_t* pUpdateData) {
	switch (pUpdateData->GetServerGameState())
	{
	case EServerGameState::ServerWaitingForPlayers:
		if (m_eGameState == EClientGameState::ClientGameMenu)
			break;
		else if (m_eGameState == EClientGameState::ClientGameExiting)
			break;

		SetGameState(EClientGameState::ClientGameWaitingForPlayers);
		break;
	case EServerGameState::ServerActive:
		if (m_eGameState == EClientGameState::ClientGameMenu)
			break;
		else if (m_eGameState == EClientGameState::ClientGameExiting)
			break;

		if (m_eGameState != EClientGameState::ClientGameActive) {
			for (int i = 0; i < MAX_PLAYERS_PER_SERVER; ++i) {
				m_SteamIDPlayers[i] = CSteamID();
			}
		}
		SetGameState(EClientGameState::ClientGameActive);
		break;
	case EServerGameState::ServerExiting:
		if (m_eGameState == EClientGameState::ClientGameExiting)
			break;

		SetGameState(EClientGameState::ClientGameMenu);
		break;
	}

	if (m_pP2PAuthedGame)
	{
		// has the player list changed?
		if (m_pServer)
		{
			for (uint32 i = 1; i < MAX_PLAYERS_PER_SERVER; ++i) {
				CSteamID steamIDNew(pUpdateData->GetPlayerSteamID(i));
				if (steamIDNew == SteamUser()->GetSteamID())
				{
					OutputDebugString("Server player slot 0 is not server owner.\n");
				}
				else if (steamIDNew != m_SteamIDPlayers[i])
				{
					if (m_SteamIDPlayers[i].IsValid())
					{
						m_pP2PAuthedGame->PlayerDisconnect(i);
					}
					if (steamIDNew.IsValid())
					{
						m_pP2PAuthedGame->RegisterPlayer(i, steamIDNew);
					}
				}
			}
		}
		else {
			CSteamID steamIDNew(pUpdateData->GetPlayerSteamID(0));
			if (steamIDNew == SteamUser()->GetSteamID())
			{
				OutputDebugString("Server player slot 0 is not server owner.\n");
			}
			else if (steamIDNew != m_SteamIDPlayers[0])
			{
				if (m_SteamIDPlayers[0].IsValid())
				{
					OutputDebugString("Server player slot 0 has disconnected - but thats the server owner.\n");
					m_pP2PAuthedGame->PlayerDisconnect(0);
				}
				if (steamIDNew.IsValid())
				{
					m_pP2PAuthedGame->StartAuthPlayer(0, steamIDNew);
				}
			}
		}
	}

	pUpdateData->GetGameUpdateData(&m_GameData);
	for (uint32 i = 0; i < MAX_PLAYERS_PER_SERVER; ++i)
	{
		m_SteamIDPlayers[i].SetFromUint64(pUpdateData->GetPlayerSteamID(i));

		if (pUpdateData->GetPlayerActive(i)) {
			if (!m_Players[i]) {
				ServerPlayerUpdateData_t* pPlayerData = pUpdateData->AccessPlayerUpdateData(i);
				m_Players[i] = new GamePlayer(m_pGameEngine);
				//pPlayerData->GetMove(x, y);
				//m_Players[i]->SetMove(x, y);
				if (i == m_PlayerIndex) {
					// Its local player
				}
			}
			
			if (i == m_PlayerIndex)
				m_Players[i]->SetIsLocalPlayer(true);
			else
				m_Players[i]->SetIsLocalPlayer(false);

			m_Players[i]->OnReceiveServerUpdate(pUpdateData->AccessPlayerUpdateData(i));
			if (m_PlayersNames[i] == "\0") {
				m_PlayersNames[i] = SteamFriends()->GetFriendPersonaName(m_SteamIDPlayers[i]);
				m_Players[i]->SetPlayerName(m_PlayersNames[i].c_str());
			}
		}
		else {
			if (m_Players[i])
			{
				if (m_PlayersNames[i] != "\0") {
					m_PlayersNames[i] = "\0";
				}
				delete m_Players[i];
				m_Players[i] = NULL;
			}
		}
	}
}

void GameClient::UpdateRichPresenceConnectionInfo() {
	char rgchConnectString[128];
	rgchConnectString[0] = 0;

	if (m_eConnectedStatus == k_EClientConnectedAndAuthenticated && m_unServerIP && m_usServerPort)
	{
		// game server connection method
		sprintf_s(rgchConnectString, "+connect %d:%d", m_unServerIP, m_usServerPort);
	}
	else if (m_pLobby->GetLobbySteamID().IsValid())
	{
		// lobby connection method
		sprintf_s(rgchConnectString, "+connect_lobby %llu", m_pLobby->GetLobbySteamID().ConvertToUint64());
	}

	SteamFriends()->SetRichPresence("connect", rgchConnectString);
}

void GameClient::OnGameStateChanged(EClientGameState eGameStateNew) {
	const char* pchSteamRichPresenceDisplay = "AtMainMenu";
	bool bDisplayScoreInRichPresence = false;

	if (m_eGameState == EClientGameState::ClientCreatingLobby)
	{
		// start creating the lobby
		if (!m_SteamCallResultLobbyCreated.IsActive())
		{
			// ask steam to create a lobby
			SteamAPICall_t hSteamAPICall = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic /* public lobby, anyone can find it */, MAX_PLAYERS_PER_SERVER);
			// set the function to call when this completes
			m_SteamCallResultLobbyCreated.Set(hSteamAPICall, this, &GameClient::OnLobbyCreated);
		}
		SteamFriends()->SetRichPresence("status", "Creating a lobby");
		pchSteamRichPresenceDisplay = "WaitingForMatch";
	}
	else if (m_eGameState == EClientGameState::ClientInLobby)
	{
		pchSteamRichPresenceDisplay = "WaitingForMatch";
	}
	else if (m_eGameState == EClientGameState::ClientGameMenu)
	{
		// we've switched out to the main menu

		// Tell the server we have left if we are connected
		DisconnectFromServer();

		// shut down any server we were running
		if (m_pServer)
		{
			delete m_pServer;
			m_pServer = NULL;
		}

		SteamFriends()->SetRichPresence("status", "Main menu");

		// Refresh inventory
		//SpaceWarLocalInventory()->RefreshFromServer();
	}

	if (pchSteamRichPresenceDisplay != NULL)
	{
		SteamFriends()->SetRichPresence("steam_display", bDisplayScoreInRichPresence ? "#StatusWithScore" : "#StatusWithoutScore");
		SteamFriends()->SetRichPresence("gamestatus", pchSteamRichPresenceDisplay);
	}

	// steam_player_group defines who the user is playing with.  Set it to the steam ID
	// of the server if we are connected, otherwise blank.
	if (m_steamIDGameServer.IsValid())
	{
		char rgchBuffer[32];
		sprintf_s(rgchBuffer, "%llu", m_steamIDGameServer.ConvertToUint64());
		SteamFriends()->SetRichPresence("steam_player_group", rgchBuffer);
	}
	else
	{
		SteamFriends()->SetRichPresence("steam_player_group", "");
	}
}

void GameClient::OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure)
{
	if (m_eGameState != EClientGameState::ClientCreatingLobby)
		return;

	// record which lobby we're in
	if (pCallback->m_eResult == k_EResultOK)
	{
		// success
		m_pLobby->SetLobbySteamID(pCallback->m_ulSteamIDLobby);
		m_pLobby->SetLobbyOwnerSteamID(GetLocalSteamID());

		// set the name of the lobby if it's ours
		char rgchLobbyName[256];
		sprintf_s(rgchLobbyName, "%s's lobby", SteamFriends()->GetPersonaName());

		m_pLobby->SetLobbyName(rgchLobbyName);
		SteamMatchmaking()->SetLobbyData(pCallback->m_ulSteamIDLobby, "name", rgchLobbyName);
		std::cout << "Created lobby - " << rgchLobbyName << std::endl;
		m_SteamIDPlayers[0] = GetLocalSteamID();
		m_PlayerTextures[0] = GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(GetLocalSteamID()));
		m_PlayersNames[0] = SteamFriends()->GetPersonaName();
		m_PlayerIndex = 0;

		// mark that we're in the lobby
		SetGameState(EClientGameState::ClientInLobby);
	}
	else
	{
		// failed, show error
		OutputDebugString("Failed to create lobby (lost connection to Steam back-end servers.");
		SetGameState(EClientGameState::ClientGameConnectionFailure);
	}
}

void GameClient::OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure)
{
	if (m_eGameState != EClientGameState::ClientJoiningLobby)
		return;

	if (pCallback->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess)
	{
		// failed, show error
		OutputDebugString("Failed to enter lobby");
		SetGameState(EClientGameState::ClientGameConnectionFailure);
		return;
	}

	TextObject* player1_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player1_ready"));
	TextObject* player2_readyText = static_cast<TextObject*>(GetPGame()->GetScene()->FindObjectByName("Player2_ready"));

	// success
	m_pLobby->SetLobbyOwnerSteamID(SteamMatchmaking()->GetLobbyOwner(pCallback->m_ulSteamIDLobby));
	m_pLobby->SetLobbySteamID(pCallback->m_ulSteamIDLobby);

	std::vector<CSteamID> players_steamId = EnumLobbyUsers();
	if (players_steamId.size() > MAX_PLAYERS_PER_SERVER) {
		OutputDebugString("lobby is full");
		SetGameState(EClientGameState::ClientGameMenu);
		return;
	}

	for (int i = 0; i < players_steamId.size(); ++i) {
		if (players_steamId[i] == GetLocalSteamID()) {
			m_PlayerIndex = i;
			const char* playerName = SteamFriends()->GetPersonaName();
			m_PlayersNames[m_PlayerIndex] = playerName;
			m_PlayerTextures[m_PlayerIndex] = GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(GetLocalSteamID()));
		}
		else {
			const char* playerName = SteamFriends()->GetFriendPersonaName(players_steamId[i]);
			m_PlayersNames[i] = playerName;
			m_PlayerTextures[i] = GetPlayerAvatar(SteamFriends()->GetLargeFriendAvatar(players_steamId[i]));
			const char* readyText = SteamMatchmaking()->GetLobbyMemberData(m_pLobby->GetLobbySteamID(), players_steamId[i], "IsReady");
			D2D1::ColorF ready_green = D2D1::ColorF(0.0, 1.0, 0.0, 1.0);

			if (!m_pLobby->CheckIsLobbyOwner(GetLocalSteamID()) && readyText && strcmp(readyText, "true") == 0) {
				m_PlayerReady[i] = true;
				m_bSetHostReady = true;
			}
		}
		m_SteamIDPlayers[i] = players_steamId[i];
	}
	
	SetGameState(EClientGameState::ClientInLobby);
}

std::vector<CSteamID> GameClient::EnumLobbyUsers() const {
	std::vector<CSteamID> results = {};
	int n_player = SteamMatchmaking()->GetNumLobbyMembers(m_pLobby->GetLobbySteamID());
	for (int i = 0; i < n_player; ++i) {
		CSteamID player_steamId = SteamMatchmaking()->GetLobbyMemberByIndex(m_pLobby->GetLobbySteamID(), i);
		results.push_back(player_steamId);
	}
	return results;
}

bool GameClient::SendServerData(const void* pData, uint32 nSizeOfData, int nSendFlags) {
	EResult res = SteamNetworkingSockets()->SendMessageToConnection(m_hConnServer, pData, nSizeOfData, nSendFlags, nullptr);
	switch (res)
	{
	case k_EResultOK:
	case k_EResultIgnored:
		break;

	case k_EResultInvalidParam:
		OutputDebugString("Failed sending data to server: Invalid connection handle, or the individual message is too big\n");
		return false;
	case k_EResultInvalidState:
		OutputDebugString("Failed sending data to server: Connection is in an invalid state\n");
		return false;
	case k_EResultNoConnection:
		OutputDebugString("Failed sending data to server: Connection has ended\n");
		return false;
	case k_EResultLimitExceeded:
		OutputDebugString("Failed sending data to server: There was already too much data queued to be sent\n");
		return false;
	default:
	{
		char msg[256];
		sprintf(msg, "SendMessageToConnection returned %d\n", res);
		OutputDebugString(msg);
		return false;
	}
	}
	return true;
}

void GameClient::DisconnectFromServer()
{
	if (m_eConnectedStatus != k_EClientNotConnected)
	{
#ifdef USE_GS_AUTH_API
		if (m_hAuthTicket != k_HAuthTicketInvalid)
			SteamUser()->CancelAuthTicket(m_hAuthTicket);
		m_hAuthTicket = k_HAuthTicketInvalid;
#else
		SteamUser()->AdvertiseGame(k_steamIDNil, 0, 0);
#endif

		// tell steam china duration control system that we are no longer in a match
		SteamUser()->BSetDurationControlOnlineState(k_EDurationControlOnlineState_Offline);

		m_eConnectedStatus = k_EClientNotConnected;
	}
	if (m_pP2PAuthedGame)
	{
		m_pP2PAuthedGame->EndGame();
	}

	if (m_hConnServer != k_HSteamNetConnection_Invalid)
		SteamNetworkingSockets()->CloseConnection(m_hConnServer, static_cast<int>(EDisconnectReason::ClientDisconnect), nullptr, false);
	m_steamIDGameServer = CSteamID();
	m_steamIDGameServerFromBrowser = CSteamID();
	m_hConnServer = k_HSteamNetConnection_Invalid;
}