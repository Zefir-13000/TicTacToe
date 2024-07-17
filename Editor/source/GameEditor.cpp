#include <GameEditor.h>

GameEditor::GameEditor(HWND hWnd) {
	m_hWnd = hWnd;
}

GameEditor::~GameEditor() {
}

bool GameEditor::InitializeViewport(UINT width, UINT height) {
	if (m_viewportTextureRenderView)
		m_viewportTextureRenderView->Release();
	if (m_viewportTextureShaderView)
		m_viewportTextureShaderView->Release();
	if (m_viewportTexture)
		m_viewportTexture->Release();

	HRESULT hr;

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));

	m_viewportSize.x = width;
	m_viewportSize.y = height;

	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.CPUAccessFlags = 0;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	texDesc.MipLevels = 1;

	hr = m_pEngine->GetDeviceD3D()->CreateTexture2D(&texDesc, nullptr, &m_viewportTexture);
	if (FAILED(hr)) {
		OutputDebugString("Failed to init main viewport texture.\n");
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	ZeroMemory(&SRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = m_pEngine->GetDeviceD3D()->CreateShaderResourceView(m_viewportTexture,
		&SRVDesc, &m_viewportTextureShaderView);
	if (FAILED(hr)) {
		OutputDebugString("Failed to init main viewport texture shader view.\n");
		return false;
	}

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc{};
	ZeroMemory(&RTVDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	RTVDesc.Format = texDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	hr = m_pEngine->GetDeviceD3D()->CreateRenderTargetView(m_viewportTexture, 
		&RTVDesc, &m_viewportTextureRenderView);
	if (FAILED(hr)) {
		OutputDebugString("Failed to init main viewport texture render view.\n");
		return false;
	}

	IDXGIResource1* SharedResource = nullptr;
	HANDLE SharedHandle = nullptr;
	IDXGISurface1* pRenderDXT = nullptr; // Surface for imgui texture

	hr = m_viewportTexture->QueryInterface(__uuidof(IDXGIResource1), (void**)&SharedResource);
	if (FAILED(hr)) {
		OutputDebugString("Failed to query IDXGIResource.\n");
		return false;
	}
	hr = SharedResource->GetSharedHandle(&SharedHandle);
	if (FAILED(hr)) {
		OutputDebugString("Failed to get SharedHandle.\n");
		return false;
	}
	hr = m_pEngine->GetDeviceD3D()->OpenSharedResource(SharedHandle, __uuidof(IDXGISurface1), (void**)&pRenderDXT);
	if (FAILED(hr)) {
		OutputDebugString("Failed to open shared resource.\n");
		return false;
	}

	bool result = m_pEngine->InitializeDXGISurface1(pRenderDXT);
	if (!result) {
		return false;
	}

	SharedResource->Release();

	return true;
}

void GameEditor::RenderScene() {
	const float clear_color[4] = { 1.0f, 0.5f, 1.0f, 1.00f };

	m_pEngine->BeginRender2D();
	m_pEngine->ClearRender(D2D1::ColorF(D2D1::ColorF::Blue));

	m_pScene->Render();

	// Editor object helper
	if (m_selectedObject) {
		m_pEngine->GetRenderTarget()->DrawRectangle(m_selectedObject->GetDrawRect(), m_pScene->GetBlackBrush());
	}

	m_pEngine->EndRender2D();
}

bool GameEditor::Initialize(UINT width, UINT height) {
	bool result;

	m_pEngine = new GameEngine();
	result = m_pEngine->Initialize(m_hWnd, width, height);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	result = m_pEngine->InitializeImgui(m_hWnd);
	if (!result) {
		OutputDebugString("Failed to init game engine.\n");
		return false;
	}

	result = InitializeViewport(800, 600);
	if (!result) {
		OutputDebugString("Failed to init game viewport.\n");
		return false;
	}

	m_pScene = new Scene(m_pEngine);

	return true;
}

void GameEditor::Shutdown() {
	if (m_viewportTextureRenderView)
		m_viewportTextureRenderView->Release();
	if (m_viewportTextureShaderView)
		m_viewportTextureShaderView->Release();
	if (m_viewportTexture)
		m_viewportTexture->Release();

	if (m_pEngine) {
		delete m_pEngine;
	}
	if (m_pScene) {
		delete m_pScene;
	}
}

void GameEditor::Tick() {
	m_pEngine->Tick();
}

ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
void GameEditor::Render() {
	// Check Minimized
	{
		if (m_pEngine->GetOccluded() && m_pEngine->GetSwapChainD3D()->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			return;
		}

		m_pEngine->SetOccluded(false);
	}

	// Render 2D Scenes
	RenderScene();

	m_pEngine->BeginRender3D();

	// Begin Render 3D
	
	// Render Imgui UI
	{
		// Main dock space
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, window_flags);
		ImGui::PopStyleVar(3);

		// Menu bar
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem(ICON_FA_PLUS " Create")) {
				}
				if (ImGui::MenuItem(ICON_FA_FOLDER " Open", "Ctrl+O")) {
				}
				if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S")) {
				}
				if (ImGui::MenuItem(ICON_FA_SAVE " Save as..")) {
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
			window_flags |= ImGuiWindowFlags_NoBackground;
		}

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		// Init windows dock
		static auto first_time = true;
		if (first_time) {
			first_time = false;
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

			ImGuiID heirarchy_id, viewport_id, inspector_id;

			ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, &inspector_id, &viewport_id);
			ImGui::DockBuilderSplitNode(viewport_id, ImGuiDir_Left, 0.25f, &heirarchy_id, &viewport_id);

			ImGui::DockBuilderDockWindow("Hierarchy", heirarchy_id);
			ImGui::DockBuilderDockWindow("Inspector", inspector_id);
			ImGui::DockBuilderDockWindow("Viewport", viewport_id);

			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		if (width != m_viewportSize.x || height != m_viewportSize.y) {
			InitializeViewport(width, height);
		}

		ImGui::Image((ImTextureID)m_viewportTextureShaderView, ImGui::GetContentRegionAvail());
		ImGui::End();

		ImGui::Begin("Hierarchy");
		if (ImGui::Button(ICON_FA_PLUS "Create")) {
			ImGui::OpenPopup("CreateObjectPopup");
		}
		ImGui::SeparatorText("Scene:");

		if (ImGui::BeginPopup("CreateObjectPopup")) {
			// List of object types
			static const char* objectTypes[] = { "TextObject" };
			static const int objectCount = sizeof(objectTypes) / sizeof(objectTypes[0]);

			// Iterate over object types
			for (int i = 0; i < objectCount; ++i) {
				// If an object type is selected
				if (ImGui::Selectable(objectTypes[i])) {
					// Create object
					m_pScene->CreateObjectByTypeName(objectTypes[i]);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		std::vector<Object*> scene_objects = m_pScene->GetSceneObjects();
		for (Object* obj : scene_objects) {
			if (!obj)
				continue;

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
			if (obj->GetID() == m_selectedID) {
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool treeOpen = ImGui::TreeNodeEx(obj->GetName().c_str(), flags);
			if (ImGui::IsItemClicked(0)) {
				m_selectedID = obj->GetID();
				m_selectedObject = obj;
			}

			char menupopupid[256];
			sprintf_s(menupopupid, 256, "%s%d", "MenuObjectPopup", obj->GetID());
			if (ImGui::IsItemClicked(1)) {
				ImGui::OpenPopup(menupopupid);
			}

			if (ImGui::BeginPopup(menupopupid)) {
				static const char* objectMenu[] = { "Delete" };
				static const int objectMenuCount = sizeof(objectMenu) / sizeof(objectMenu[0]);

				for (int i = 0; i < objectMenuCount; ++i) {
					// If an object type is selected
					if (ImGui::Selectable(objectMenu[i])) {
						// Create object
						if (objectMenu[i] == "Delete") {
							if (m_selectedID == obj->GetID()) {
								m_selectedID = -1;
								m_selectedObject = nullptr;
							}
							m_pScene->RemoveObject(obj);
						}
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::EndPopup();
			}

			if (treeOpen)
				ImGui::TreePop();
		}
		ImGui::End();

		ImGui::Begin("Inspector");
		if (m_selectedObject) {
			Vector2f pos = m_selectedObject->GetPosition();
			D2D1_SIZE_U sizeU = m_selectedObject->GetSize();
			Vector2i size = {(int)sizeU.width, (int)sizeU.height};
			float rotation = m_selectedObject->GetRotation();
			char objName[1024];
			strcpy(objName, m_selectedObject->GetName().c_str());

			ImGui::Text("Name:"); ImGui::SameLine();
			if (ImGui::InputText("##ObjName", objName, 1024))
				m_selectedObject->SetName(objName);

			ImGui::Text("Pos:"); ImGui::SameLine();
			if (ImGui::DragFloat2("##ObjPos", &pos.x))
				m_selectedObject->SetPosition(pos);

			ImGui::Text("Rot:"); ImGui::SameLine();
			if (ImGui::DragFloat("##ObjRot", &rotation))
				m_selectedObject->SetRotation(rotation);

			ImGui::Text("Size:"); ImGui::SameLine();
			if (ImGui::DragInt2("##ObjSize", &size.x, 1.0f, 0.01f, 100000.f)) {
				if (size.x < 0) {
					size.x = 0.01f;
				}
				if (size.y < 0) {
					size.y = 0.01f;
				}
				sizeU.width = (UINT)size.x;
				sizeU.height = (UINT)size.y;
				m_selectedObject->SetSize(sizeU);
			}

			if (m_selectedObject->GetObjectType() == Object_TextType) {
				TextObject* textObj = dynamic_cast<TextObject*>(m_selectedObject);
				float fontSize = textObj->GetFontSize();
				ImGui::Text("FontSize:"); ImGui::SameLine();
				if (ImGui::DragFloat("##Font", &fontSize, 1.0f, 1.f, 1000.f)) {
					if (fontSize < 1.0f)
						fontSize = 1.0f;
					textObj->SetFontSize(fontSize);
				}
			}
		}
		else
			ImGui::Text("Please, select object.");
		ImGui::End();
		
		//bool show_demo_window = true;
		//ImGui::ShowDemoWindow(&show_demo_window);
	}

	// End Draw call 3D
	m_pEngine->EndRender3D();
}

void GameEditor::SetResize(UINT width, UINT height) {
	m_pEngine->SetResizeSwapchain3D(width, height);
}