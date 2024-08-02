#pragma once
#include <stdafx.h>
#include <GameEditor.h>

class Application {
public:
	Application();
	~Application();

	void Run();

	bool Initialize();
	void Shutdown();

	void SetResize(UINT width, UINT height);
	void TriggerEvent(EngineEvent event);
private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	LPCSTR m_applicationName;

	UINT m_screenWidth, m_screenHeight;

	GameEditor* m_pEditor = nullptr;
};

extern Application* g_pApp;
Application* GlobalApp();

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);