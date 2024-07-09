#pragma once
#include <stdafx.h>
#include <GameEngine.h>

class Game {
public:
	Game(HWND hWnd);
	~Game();

	void Tick();
	void Render();

	bool Initialize();
	void Shutdown();

private:
	HWND m_hWnd;
	GameEngine* m_pEngine = nullptr;

	ID2D1SolidColorBrush* m_pBlackBrush = nullptr;

	TextObject* text1 = nullptr;
};