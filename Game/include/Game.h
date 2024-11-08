#pragma once
#include <stdafx.h>
#include <GameEngine.h>
#include <Scene.h>
#include <GameClient.h>

void Alert(const char* lpCaption, const char* lpText);

class Game {
public:
	Game(HWND hWnd);
	~Game();

	void Tick();
	void Render();
	void Render(std::string SceneName);

	void LoadScene(std::string SceneName);

	bool Initialize(std::string& StartSceneName);
	void Shutdown();

	Scene* GetScene() const;

	void SetupActions();
	void TriggerEvent(EngineEvent event);
private:
	void RenderScene();

	void Action_StartGame();
	void Action_ExitGame();
	void Action_LobbyBack();
	void Action_LobbyReady();
	void Action_LobbyStart();
	void Action_LobbyInvite();

	void Action_GameCellClick(int index);

	HWND m_hWnd;
	GameEngine* m_pEngine = nullptr;
	Scene* m_pScene = nullptr;
	GameClient* m_pGameClient = nullptr;
};

extern Game* g_pGame;
extern Game* GetPGame();