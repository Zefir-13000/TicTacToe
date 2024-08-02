#pragma once
#include <stdafx.h>
#include <Game.h>

class Application {
public:
	Application();
	~Application();

	void Run();

	bool Initialize();
	void Shutdown();

	void TriggerEvent(EngineEvent event);
private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	LPCSTR m_applicationName;

	UINT m_screenWidth, m_screenHeight;

	Game* m_pGame = nullptr;
};

extern Application* g_pApp;
Application* GlobalApp();

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);