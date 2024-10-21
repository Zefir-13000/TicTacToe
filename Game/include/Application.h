#pragma once
#include <stdafx.h>
#include <Game.h>

extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText);

class Application {
public:
	Application();
	~Application();

	void Run();
	void SingleRender();

	bool Initialize();
	void Shutdown();

	void SetClosing();
	bool IsClosing() const;

	void TriggerEvent(EngineEvent event);
private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	LPCSTR m_applicationName;

	UINT m_screenWidth, m_screenHeight;

	Game* m_pGame = nullptr;

	bool m_bIsClosing = false;
};

extern Application* g_pApp;
Application* GlobalApp();

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);