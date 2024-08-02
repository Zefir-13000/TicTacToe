#include <Game.h>
#include <chrono>

Game::Game(HWND hWnd) {
	m_hWnd = hWnd;
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

	SetupActions();

	return true;
}

void Game::Shutdown() {
	if (m_pEngine) {
		delete m_pEngine;
	}
	if (m_pScene) {
		delete m_pScene;
	}
}

void Game::Tick() {
	m_pEngine->Tick();
}

void Game::TriggerEvent(EngineEvent event) {
	m_pEngine->TriggerEvent(event);
}

void Game::TestEvent() {
	std::string scene_Name = "Menu2";
	m_pScene->Load(scene_Name);
}

void Game::TestEvent2(std::string SceneName) {
	m_pScene->Load(SceneName);
}

void Game::SetupActions() {
	auto testEvent_cb = std::bind(&Game::TestEvent, this);
	auto testEvent2_cb = std::bind(&Game::TestEvent2, this, "Menu");
	m_pScene->SetupActionCallback("Action0", testEvent_cb);
	m_pScene->SetupActionCallback("ActionBack", testEvent2_cb);
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