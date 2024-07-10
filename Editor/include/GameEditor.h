#pragma once
#include <stdafx.h>
#include <GameEngine.h>

class GameEditor {
public:
	GameEditor(HWND hWnd);
	~GameEditor();

	void Tick();
	void Render();

	bool Initialize(UINT width, UINT height);
	void Shutdown();

	void SetResize(UINT width, UINT height);
private:
	HWND m_hWnd;
	GameEngine* m_pEngine = nullptr;
};