#pragma once
#include <GameEngine.h>
#include <Object.h>
#include <vector>

enum SceneState {
	Scene_Unititled = 0,
	Scene_Saved = 1,
};

inline extern const std::string SCENE_DEFAULT_NAME = "untitled.scene";
class Scene {
public:
	Scene(GameEngine* pEngine);
	~Scene();

	Object* FindObjectById(int id);
	void AddObject(Object* object);
	void RemoveObject(Object* object);
	void RemoveObject(int objectID);

	EngineAction* FindActionById(int id);
	EngineAction* FindAction(std::string name);
	void AddAction(EngineAction* action);
	void RemoveAction(EngineAction* action);

	EngineAction* CreateAction();
	Object* CreateObject(ObjectType objType);
	Object* CreateObjectByTypeName(std::string typeName);

	void Render();

	void EventOnClick();
	void EventMouseMove();

	std::string GetSceneName() { return m_name; }
	void SetSceneName(std::string name) { m_name = name; }

	std::string GetSceneSaveFilename() { return m_saveFilename; }
	void SetSceneSaveFilename(std::string filename) { m_saveFilename = filename; }

	SceneState GetSceneState() { return m_sceneState; }
	void SetSceneState(SceneState sceneState) { m_sceneState = sceneState; }

	bool GetIsChanged() { return m_bIsChanged; }
	void SetChanged() { m_bIsChanged = true; }

	float* GetClearColor() { return m_clearColor; }

	ID2D1SolidColorBrush* GetBlackBrush() { return m_pBlackBrush; }

	const std::vector<Object*>& GetSceneObjects() const { return m_objects; }
	const std::vector<EngineAction*>& GetSceneActions() const { return m_actions; }

	void SetupActionCallback(std::string name, std::function<void()> callback);
	std::function<void()> FindActionCallback(EngineAction* action);

	void Save(std::string& filename);
	bool Load(std::string& filename);

	std::string ShowSaveDialog(HWND hWnd);
	std::string ShowOpenDialog(HWND hWnd);

	int HandleSaveNewScene(HWND hWnd);
	void HandleOpenNewScene(HWND hWnd);
private:
	std::vector<Object*> m_objects;
	std::vector<EngineAction*> m_actions;
	std::unordered_map<std::string, std::function<void()>> m_actionMap;

	ID2D1SolidColorBrush* m_pBlackBrush = nullptr;

	bool m_bIsChanged = false;

	float m_clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	std::string m_name = "Scene";
	std::string m_saveFilename = SCENE_DEFAULT_NAME;
	SceneState m_sceneState = Scene_Unititled;

	GameEngine* m_pEngine; // ref
};

extern Scene* g_pScene;
extern Scene* GetScene();