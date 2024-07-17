#pragma once
#include <GameEngine.h>
#include <Object.h>
#include <vector>

class Scene {
public:
	Scene(GameEngine* pEngine);
	~Scene();

	Object* FindObjectById(int id);
	void AddObject(Object* object);
	void RemoveObject(Object* object);
	void RemoveObject(int objectID);

	Object* CreateObjectByTypeName(std::string typeName);

	void Render();

	ID2D1SolidColorBrush* GetBlackBrush() { return m_pBlackBrush; }

	const std::vector<Object*>& GetSceneObjects() const { return m_objects; }
private:
	std::vector<Object*> m_objects;
	ID2D1SolidColorBrush* m_pBlackBrush = nullptr;

	GameEngine* m_pEngine; // ref to Engine
};