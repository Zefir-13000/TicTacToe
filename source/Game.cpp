#include <Game.h>
#include <chrono>

Game::Game(HWND hWnd) {
	m_hWnd = hWnd;
}

Game::~Game() {

}

bool Game::Initialize() {
	bool result;

	m_pEngine = new GameEngine();
	result = m_pEngine->Initialize(m_hWnd);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	HRESULT hr;
	hr = m_pEngine->GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBlackBrush);
	if (FAILED(hr)) {
		OutputDebugString("Failed to create Black SolidBrush.\n");
		return false;
	}

	text1 = new TextObject(m_pEngine->GetDWriteFactory(), 32.f);
	text1->SetText("TestText");
	text1->SetBrush(m_pBlackBrush);
	text1->SetPosition(50, 50);
	text1->SetSize(150, 50);

	return true;
}

void Game::Shutdown() {
	if (text1)
		delete text1;
	if (m_pEngine) {
		delete m_pEngine;
	}
}

void Game::Tick() {
	m_pEngine->Tick();
}

void Game::Render() {
	ID2D1HwndRenderTarget* m_pRenderTarget = m_pEngine->GetRenderTarget();

	m_pEngine->BeginRender();
	m_pEngine->ClearRender(D2D1::ColorF(D2D1::ColorF::White));

	text1->Render(m_pRenderTarget);

	m_pEngine->EndRender();
}