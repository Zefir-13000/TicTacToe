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

Object* Scene::CreateObject(ObjectType objType) {
	if (objType == Object_TextType) {
		TextObject* text1 = new TextObject(m_pEngine->GetDWriteFactory(), 32.f);
		text1->SetColor(GetBlackBrush());
		text1->SetPosition(0, 0);
		text1->SetSize(150, 50);

		AddObject(text1);
		return text1;
	}
	return nullptr;
}

Object* Scene::CreateObjectByTypeName(std::string typeName) {
	if (typeName == "TextObject") {
		TextObject* text1 = new TextObject(m_pEngine->GetDWriteFactory(), 32.f);
		text1->SetColor(GetBlackBrush());
		text1->SetPosition(0, 0);
		text1->SetSize(150, 50);

		AddObject(text1);
		return text1;
	}
	return nullptr;
}

void Scene::Render() {
	ID2D1RenderTarget* pRenderTarget = m_pEngine->GetRenderTarget();
	for (Object* obj : m_objects) {
		obj->Render(pRenderTarget);
	}
}

void Scene::Save(std::string& filename) {
	m_saveFilename = filename;
	std::ofstream stream(filename);
	// Save scene data
	size_t name_size = m_name.size();
	stream.write((const char*)&name_size, sizeof(size_t));
	stream.write(&m_name[0], name_size);

	stream.write((const char*)&m_clearColor[0], sizeof(float) * 4);
	stream << std::endl;

	// Save objects data
	for (Object* obj : m_objects) {
		stream << obj->GetObjectType();
		obj->Save(stream);
		stream << std::endl;
	}
	SetSceneState(Scene_Saved);
	m_bIsChanged = false;
	stream.close();
}

void Scene::Load(std::string& filename) {
	m_saveFilename = filename;
	std::ifstream stream(filename);
	// Load scene data
	size_t name_size = 0;
	stream.read((char*)&name_size, sizeof(size_t));
	m_name.resize(name_size);
	stream.read(&m_name[0], name_size);

	stream.read((char*)&m_clearColor[0], sizeof(float) * 4);

	// Load objects data
	for (Object* obj : m_objects) {
		delete obj;
	}
	m_objects.clear();

	int objTypeInt = 0;
	while (stream >> objTypeInt) {
		ObjectType ObjType = static_cast<ObjectType>(objTypeInt);
		Object* obj = CreateObject(ObjType);
		if (obj) {
			obj->Load(m_pEngine->GetRenderTarget(), stream);
		}
	}
	SetSceneState(Scene_Saved);
	m_bIsChanged = false;
	stream.close();
}

std::string Scene::ShowSaveDialog(HWND hWnd) {
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";
	strcpy(szFileName, m_saveFilename.c_str());

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "scene";

	if (GetSaveFileName(&ofn)) {
		return std::string(szFileName);
	}
	return "\0";
}

std::string Scene::ShowOpenDialog(HWND hWnd) {
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";
	strcpy(szFileName, m_saveFilename.c_str());

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Scene Files (*.scene)\0*.scene\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "scene";

	if (GetOpenFileName(&ofn)) {
		return std::string(szFileName);
	}
	return "\0";
}

int Scene::HandleSaveNewScene(HWND hWnd) {
	std::string filename;
	SceneState sceneState = GetSceneState();
	if (sceneState == Scene_Unititled && GetIsChanged()) {
		const int result = MessageBox(hWnd, "All unsaved work will be lost", "Save changes?", MB_YESNOCANCEL);
		switch (result)
		{
		case IDYES:
			filename = ShowSaveDialog(hWnd);
			// Save and Create new scene
			if (filename.size() > 0) {
				SetSceneSaveFilename(filename);
				Save(filename);
			}
			return IDYES;
			break;
		case IDNO:
			// Create new scene
			return IDNO;
			break;
		case IDCANCEL:
			// Do nothing
			return IDCANCEL;
			break;
		}
	}
	else if (sceneState == Scene_Saved && GetIsChanged()) {
		const int result = MessageBox(hWnd, "All unsaved work will be lost", "Save changes?", MB_YESNOCANCEL);
		switch (result)
		{
		case IDYES:
			// Save and Create new scene
			Save(m_saveFilename);
			return IDYES;
			break;
		case IDNO:
			// Create new scene
			return IDNO;
			break;
		case IDCANCEL:
			// Do nothing
			return IDCANCEL;
			break;
		}
	}
	else if (sceneState == Scene_Saved) {
		return IDYES;
	}
	return 0;
}

void Scene::HandleOpenNewScene(HWND hWnd) {
	std::string filename;
	SceneState sceneState = GetSceneState();
	if (sceneState == Scene_Unititled && GetIsChanged()) {
		const int result = MessageBox(hWnd, "All unsaved work will be lost", "Save changes?", MB_YESNOCANCEL);
		switch (result)
		{
		case IDYES:
			// Save and Open new scene
			filename = ShowSaveDialog(hWnd);
			if (filename.size() > 0) {
				SetSceneSaveFilename(filename);
				Save(filename);
			}
			filename = ShowOpenDialog(hWnd);
			if (filename.size() > 0) {
				Load(filename);
			}
			break;
		case IDNO:
			// Open new scene
			filename = ShowOpenDialog(hWnd);
			if (filename.size() > 0) {
				Load(filename);
			}
			break;
		case IDCANCEL:
			// Do nothing
			break;
		}
	}
	else if (sceneState == Scene_Saved && GetIsChanged()) {
		const int result = MessageBox(hWnd, "All unsaved work will be lost", "Save changes?", MB_YESNOCANCEL);
		switch (result)
		{
		case IDYES:
			Save(m_saveFilename);
			filename = ShowOpenDialog(hWnd);
			if (filename.size() > 0) {
				Load(filename);
			}
			break;
		case IDNO:
			filename = ShowOpenDialog(hWnd);
			if (filename.size() > 0) {
				Load(filename);
			}
			break;
		case IDCANCEL:
			// Do nothing
			break;
		}
	}
	else if ((sceneState == Scene_Saved || sceneState == Scene_Unititled) && !GetIsChanged()) {
		filename = ShowOpenDialog(hWnd);
		if (filename.size() > 0) {
			Load(filename);
		}
	}
}