#include <Scene.h>
#include <AllObjectTypes.h>

Scene* g_pScene = nullptr;
Scene* GetScene() { return g_pScene; }

Scene::Scene(GameEngine* pEngine) {
	g_pScene = this;
	m_pEngine = pEngine;
	m_pEngine->SetScene(this);

	HRESULT hr;
	hr = GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBlackBrush);
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
		TextObject* text1 = new TextObject();
		text1->SetColor(GetBlackBrush());
		text1->SetSize(150, 50);

		AddObject(text1);
		return text1;
	}
	else if (objType == Object_ButtonType) {
		ButtonObject* button = new ButtonObject();
		button->SetColor(GetBlackBrush());
		button->GetTextObject()->SetColor(GetBlackBrush());
		button->GetTextObject()->SetSize(150, 50);
		button->SetSize(150, 50);

		AddObject(button);
		return button;
	}
	else if (objType == Object_TextureType) {
		TextureObject* texture = new TextureObject();
		texture->SetSize(64, 64);

		AddObject(texture);
		return texture;
	}
	return nullptr;
}

Object* Scene::CreateObjectByTypeName(std::string typeName) {
	if (typeName == "TextObject") {
		TextObject* text = new TextObject();
		text->SetColor(GetBlackBrush());
		text->SetSize(150, 50);

		AddObject(text);
		return text;
	}
	else if (typeName == "ButtonObject") {
		ButtonObject* button = new ButtonObject();
		button->SetColor(GetBlackBrush());
		button->GetTextObject()->SetColor(GetBlackBrush());
		button->GetTextObject()->SetSize(150, 50);
		button->SetSize(150, 50);

		AddObject(button);
		return button;
	}
	else if (typeName == "TextureObject") {
		TextureObject* texture = new TextureObject();
		texture->SetSize(64, 64);

		AddObject(texture);
		return texture;
	}
	return nullptr;
}

void Scene::Render() {
	for (Object* obj : m_objects) {
		obj->Render();
	}
}

void Scene::EventOnClick() {
	for (Object* obj : m_objects) {
		if (obj->GetObjectType() == Object_ButtonType) {
			ButtonObject* button = static_cast<ButtonObject*>(obj);
			button->OnClick();
		}
	}
}

void Scene::EventMouseMove() {
	for (Object* obj : m_objects) {
		if (obj->GetObjectType() == Object_ButtonType) {
			ButtonObject* button = static_cast<ButtonObject*>(obj);
			button->OnHover();
		}
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

	// Save actions data
	for (EngineAction* action : m_actions) {
		stream << 1;
		action->Save(stream);
		stream << std::endl;
	}
	stream << 0 << std::endl;

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

bool Scene::Load(std::string& filename) {
	if (!filename.ends_with(".scene")) {
		filename += ".scene";
	}
	if (!m_pEngine->IsEditor()) {
		static std::string game_scene_path = "data/scenes/";
		filename = game_scene_path + filename;
	}

	m_saveFilename = filename;
	std::ifstream stream(filename);
	if (!stream.good()) {
		MessageBox(GetHWND(), "Scene file not found.", "Error", MB_OK);
		return false;
	}

	// Load scene data
	size_t name_size = 0;
	stream.read((char*)&name_size, sizeof(size_t));
	m_name.resize(name_size);
	stream.read(&m_name[0], name_size);

	stream.read((char*)&m_clearColor[0], sizeof(float) * 4);

	// Load actions data
	for (EngineAction* action : m_actions) {
		delete action;
	}
	m_actions.clear();

	int placeholder = 0;
	while (stream >> placeholder) {
		if (placeholder == 0)
			break;
		EngineAction* action = CreateAction();
		if (action) {
			action->Load(stream);
		}
	}

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
			obj->Load(stream);
		}
	}

	SetSceneState(Scene_Saved);
	m_bIsChanged = false;
	stream.close();
	return true;
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

EngineAction* Scene::FindActionById(int id) {
	for (EngineAction* action : m_actions) {
		if (action->GetID() == id)
			return action;
	}
	return nullptr;
}

EngineAction* Scene::FindAction(std::string name) {
	for (EngineAction* action : m_actions) {
		if (action->GetName() == name)
			return action;
	}
	return nullptr;
}

void Scene::AddAction(EngineAction* action) {
	m_actions.push_back(action);
}

void Scene::RemoveAction(EngineAction* action) {
	int id_to_delete = action->GetID();
	std::erase_if(m_actions, [&id_to_delete](EngineAction* ele) {
		return ele->GetID() == id_to_delete;
	});
}

EngineAction* Scene::CreateAction() {
	static int _action_id = 0;
	std::string action_name;
loop:
	action_name = "Action" + std::to_string(_action_id);
	if (!FindAction(action_name)) {
		EngineAction* action = new EngineAction(action_name);
		AddAction(action);
		return action;
	}
	else {
		++_action_id;
		goto loop;
	}

	return nullptr;
}

void Scene::SetupActionCallback(std::string name, std::function<void()> callback) {
	m_actionMap[name] = callback;
}

std::function<void()> Scene::FindActionCallback(EngineAction* action) {
	std::string action_name = action->GetName();
	if (m_actionMap.contains(action_name)) {
		return m_actionMap[action_name];
	}
	return nullptr;
}