#include <Scene.h>

Scene::Scene(GameEngine* pEngine) {
	m_pEngine = pEngine;

	HRESULT hr;
	hr = m_pEngine->GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBlackBrush);
	if (FAILED(hr)) {
		OutputDebugString("Failed to create Black SolidBrush.\n");
	}
}

Scene::~Scene() {
	if (m_pBlackBrush)
		m_pBlackBrush->Release();

	for (Object* obj : m_objects) {
		delete obj;
	}
}

void Scene::AddObject(Object* object) {
	m_objects.push_back(object);
}

void Scene::RemoveObject(Object* object)
{
	int id_to_delete = object->GetID();
	std::erase_if(m_objects, [&id_to_delete](Object* ele) {
		return ele->GetID() == id_to_delete;
	});
}

void Scene::RemoveObject(int objectID) {
	std::erase_if(m_objects, [&objectID](Object* ele) {
		return ele->GetID() == objectID;
	});
}

Object* Scene::FindObjectById(int id) {
	for (Object* obj : m_objects) {
		if (obj->GetID() == id)
			return obj;
	}
	return nullptr;
}

Object* Scene::CreateObjectByTypeName(std::string typeName) {
	Object* result = nullptr;
	if (typeName == "TextObject") {
		TextObject* text1 = new TextObject(m_pEngine->GetDWriteFactory(), 32.f);
		text1->SetText("TestText");
		text1->SetBrush(GetBlackBrush());
		text1->SetPosition(0, 0);
		text1->SetSize(150, 50);

		AddObject(text1);
		result = text1;
	}

	return result;
}

void Scene::Render() {
	ID2D1RenderTarget* pRenderTarget = m_pEngine->GetRenderTarget();
	for (Object* obj : m_objects) {
		obj->Render(pRenderTarget);
	}
}