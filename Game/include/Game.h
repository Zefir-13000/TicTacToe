#pragma once
#include <stdafx.h>
#include <GameEngine.h>
#include <Scene.h>

class Game {
public:
	Game(HWND hWnd);
	~Game();

	void Tick();
	void Render();

	bool Initialize(std::string& StartSceneName);
	void Shutdown();

	void TestEvent();
	void TestEvent2(std::string SceneName);

	void SetupActions();
	void TriggerEvent(EngineEvent event);
private:
	void RenderScene();

	HWND m_hWnd;
	GameEngine* m_pEngine = nullptr;
	Scene* m_pScene = nullptr;
};