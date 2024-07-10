#include <GameEditor.h>

GameEditor::GameEditor(HWND hWnd) {
	m_hWnd = hWnd;
}

GameEditor::~GameEditor() {

}

bool GameEditor::Initialize(UINT width, UINT height) {
	bool result;

	m_pEngine = new GameEngine();
	result = m_pEngine->Initialize(m_hWnd);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	result = m_pEngine->InitializeImgui(m_hWnd, width, height);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	return true;
}

void GameEditor::Shutdown() {
	if (m_pEngine) {
		delete m_pEngine;
	}
}

void GameEditor::Tick() {
	m_pEngine->Tick();
}

void GameEditor::Render() {
	if (m_pEngine->GetOccluded() && m_pEngine->GetSwapChainD3D()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
	{
		::Sleep(10);
		return;
	}

	m_pEngine->SetOccluded(false);

	m_pEngine->BeginRender();
	//m_pEngine->ClearRender(D2D1::ColorF(D2D1::ColorF::White));


	bool show_demo_window = true;
	ImGui::ShowDemoWindow(&show_demo_window);

	m_pEngine->EndRender();
}

void GameEditor::SetResize(UINT width, UINT height) {
	m_pEngine->SetResize(width, height);
}