#include <Game.h>
#include <Application.h>
#include <chrono>

Game* g_pGame = nullptr;
Game* GetPGame() { return g_pGame; }

void Alert(const char* lpCaption, const char* lpText) {
#ifndef _WIN32
	fprintf(stderr, "Message: '%s', Detail: '%s'\n", lpCaption, lpText);
#else
	MessageBox(NULL, lpText, lpCaption, MB_OK);
#endif
}

Game::Game(HWND hWnd) {
	m_hWnd = hWnd;
	g_pGame = this;
}

Game::~Game() {

}

bool Game::Initialize(std::string& StartSceneName) {
	bool result;

	m_pEngine = new GameEngine();
	result = m_pEngine->Initialize(m_hWnd, 800, 600);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	// Init 2D on main window
	if (!m_pEngine->InitializeSwapchainRenderTarget()) {
		OutputDebugString("Failed to init Swapchain RenderTarget.\n");
		return false;
	}

	m_pScene = new Scene(m_pEngine);
	if (!m_pScene->Load(StartSceneName)) {
		return false;
	}

	m_pGameClient = new GameClient(m_pEngine);

	SetupActions();

	return true;
}

void Game::Shutdown() {
	if (m_pGameClient) {
		delete m_pGameClient;
	}
	if (m_pScene) {
		delete m_pScene;
	}
	if (m_pEngine) {
		delete m_pEngine;
	}
}

void Game::Tick() {
	m_pEngine->Tick();
	m_pGameClient->Tick();
}

void Game::LoadScene(std::string SceneName) {
	m_pScene->Load(SceneName);
}

Scene* Game::GetScene() const {
	return m_pScene;
}

void Game::TriggerEvent(EngineEvent event) {
	m_pEngine->TriggerEvent(event);
}

void Game::Action_StartGame() {
	m_pGameClient->SetGameState(EClientGameState::ClientCreatingLobby);
}

void Game::Action_ExitGame() {
	PostQuitMessage(0);
}

void Game::Action_LobbyBack() {
	if (m_pGameClient->GetLobbySteamID().IsValid())
		SteamMatchmaking()->LeaveLobby(m_pGameClient->GetLobbySteamID());

	m_pGameClient->ResetClientData();
	m_pGameClient->SetGameState(EClientGameState::ClientGameMenu);
}

void Game::Action_LobbyReady() {
	SteamMatchmaking()->SetLobbyMemberData(m_pGameClient->GetLobbySteamID(), "IsReady", "true");
}

void Game::Action_LobbyStart() {
	OutputDebugString("Game starting...\n");
	SteamMatchmaking()->SetLobbyData(m_pGameClient->GetLobbySteamID(), "game_starting", "1");
	m_pGameClient->SetGameState(EClientGameState::ClientGameStartServer);
}

void Game::Action_LobbyInvite() {
	SteamFriends()->ActivateGameOverlayInviteDialog(m_pGameClient->GetLobbySteamID());
}

void Game::Action_GameCellClick(int index) {
	m_pGameClient->MakeMove(index);
}

void Game::SetupActions() {
	auto mainMenu_startGame_cb = std::bind(&Game::Action_StartGame, this);
	auto mainMenu_exitGame_cb = std::bind(&Game::Action_ExitGame, this);
	auto lobby_back_cb = std::bind(&Game::Action_LobbyBack, this);
	auto lobby_ready_cb = std::bind(&Game::Action_LobbyReady, this);
	auto lobby_start_cb = std::bind(&Game::Action_LobbyStart, this);
	auto lobby_invite_cb = std::bind(&Game::Action_LobbyInvite, this);
	
	m_pScene->SetupActionCallback("MainMenu_StartGame", mainMenu_startGame_cb);
	m_pScene->SetupActionCallback("MainMenu_ExitGame", mainMenu_exitGame_cb);
	m_pScene->SetupActionCallback("Lobby_Back", lobby_back_cb);
	m_pScene->SetupActionCallback("Lobby_Ready", lobby_ready_cb);
	m_pScene->SetupActionCallback("Lobby_Start", lobby_start_cb);
	m_pScene->SetupActionCallback("Lobby_Invite", lobby_invite_cb);

	for (int i = 0; i < 9; ++i) {
		std::string strIndex = convertToTwoDigitIndex(i);
		std::string objAction = "Object" + strIndex;
		auto game_cell_click_cb = std::bind(&Game::Action_GameCellClick, this, i);
		m_pScene->SetupActionCallback(objAction, game_cell_click_cb);
	}
}

void Game::RenderScene() {
	float* clearColor = m_pScene->GetClearColor();

	m_pEngine->BeginRender2D();
	m_pEngine->ClearRender(D2D1::ColorF(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));

	m_pScene->Render();

	m_pEngine->EndRender2D();
}

void Game::Render() {
	// Check Minimized
	{
		if (m_pEngine->GetOccluded() && m_pEngine->GetSwapChainD3D()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			return;
		}

		m_pEngine->SetOccluded(false);
	}

	// Render 2D Scenes
	RenderScene();

	// Swapchain empty render call
	m_pEngine->BeginRender3D();
	m_pEngine->EndRender3D();
}

void Game::Render(std::string SceneName) {
	if (GlobalApp()->IsClosing()) {
		return;
	}

	if (m_pScene->GetSceneName() != SceneName) {
		m_pScene->Load(SceneName);
	}

	Game::Render();
}