#include <GameEditor.h>
#include <AllObjectTypes.h>

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

void GameEditor::TriggerEvent(EngineEvent event) {
	m_pEngine->TriggerEvent(event);
}

void GameEditor::RenderScene() {
	float* clearColor = m_pScene->GetClearColor();
	
	m_pEngine->BeginRender2D();
	m_pEngine->ClearRender(D2D1::ColorF(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));

	m_pScene->Render();

	// Editor object helper
	if (m_selectedObject) {
		if (m_selectedObject->GetRotation() != 0) {
			Vector2f middle_pos = m_selectedObject->GetDrawRectMiddle();
			GetRenderTarget()->SetTransform(D2D1::Matrix3x2F::Rotation(m_selectedObject->GetRotation(), D2D1::Point2F(middle_pos.x, middle_pos.y)));
		}
		GetRenderTarget()->DrawRectangle(m_selectedObject->GetDrawRect(), m_pScene->GetBlackBrush());
		GetRenderTarget()->SetTransform(D2D1::Matrix3x2F::Identity());
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
	// Check scene changed
	{
		SceneState sceneState = m_pScene->GetSceneState();
		if (sceneState == Scene_Unititled) {
			std::string formatText = "TicTacToe - Untitled*";
			SetWindowText(m_hWnd, formatText.c_str());
		}
		else if (sceneState == Scene_Saved && m_pScene->GetIsChanged()) {
			std::string formatText = "TicTacToe - ";
			formatText += m_pScene->GetSceneName();
			formatText += "*";
			SetWindowText(m_hWnd, formatText.c_str());
		}
		else {
			std::string formatText = "TicTacToe - ";
			formatText += m_pScene->GetSceneName();
			SetWindowText(m_hWnd, formatText.c_str());
		}
	}

	// Render 2D Scene
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
				std::string filename;
				if (ImGui::MenuItem(ICON_FA_PLUS " Create")) {
					int res = m_pScene->HandleSaveNewScene(m_hWnd);
					if (res == IDNO || res == IDYES) {
						delete m_pScene;
						m_pScene = new Scene(m_pEngine);
						Unselect();
					}
				}
				if (ImGui::MenuItem(ICON_FA_FOLDER " Open", "Ctrl+O")) {
					m_pScene->HandleOpenNewScene(m_hWnd);
					Unselect();
				}
				if (ImGui::MenuItem(ICON_FA_SAVE " Save", "Ctrl+S")) {
					if (m_pScene->GetSceneState() == Scene_Unititled)
						filename = m_pScene->ShowSaveDialog(m_hWnd);
					else
						filename = m_pScene->GetSceneSaveFilename();
					// Save and Create new scene
					if (filename.size() > 0) {
						m_pScene->SetSceneSaveFilename(filename);
						m_pScene->Save(filename);
					}
				}
				if (ImGui::MenuItem(ICON_FA_SAVE " Save as..")) {
					filename = m_pScene->ShowSaveDialog(m_hWnd);
					// Save and Create new scene
					if (filename.size() > 0) {
						m_pScene->SetSceneSaveFilename(filename);
						m_pScene->Save(filename);
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		// Check for shortcuts
		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl), false) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RightCtrl), false)) {
			if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_O), false)) {
				m_pScene->HandleOpenNewScene(m_hWnd);
				Unselect();
			}
			if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S), false)) {
				std::string filename;
				if (m_pScene->GetSceneState() == Scene_Unititled)
					filename = m_pScene->ShowSaveDialog(m_hWnd);
				else
					filename = m_pScene->GetSceneSaveFilename();
				// Save and Create new scene
				if (filename.size() > 0) {
					m_pScene->SetSceneSaveFilename(filename);
					m_pScene->Save(filename);
				}
				else {
					// Cancel creating new scene because of an error
					MessageBox(m_hWnd, "Error: Save file", "Error", MB_ICONERROR | MB_OK);
				}
			}
			ImGui::GetIO().KeyCtrl = false;
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

		// Viewport tab on the center
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImGui::PopStyleVar(3);

		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		if (width != m_viewportSize.x || height != m_viewportSize.y) {
			//InitializeViewport(width, height);
		}

		ImGui::Image((ImTextureID)m_viewportTextureShaderView, m_viewportSize);
		ImGui::End();

		// Hierarchy tab on the left
		ImGui::Begin("Hierarchy");
		if (ImGui::Button(ICON_FA_PLUS "Create")) {
			ImGui::OpenPopup("CreateObjectPopup");
		}

		if (ImGui::BeginPopup("CreateObjectPopup")) {
			// List of object types
			static const char* objectTypes[] = { "Action", "TextObject", "ButtonObject"};
			static const int objectCount = sizeof(objectTypes) / sizeof(objectTypes[0]);

			// Iterate over object types
			for (int i = 0; i < objectCount; ++i) {
				// If an object type is selected
				if (ImGui::Selectable(objectTypes[i])) {
					// Check action
					if (strcmp(objectTypes[i], "Action") == 0) {
						// Create action
						m_pScene->CreateAction();
					}
					else {
						// Create object
						m_pScene->CreateObjectByTypeName(objectTypes[i]);
					}
					m_pScene->SetChanged();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		ImGui::SeparatorText("");
		ImGuiTreeNodeFlags scene_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (ImGui::TreeNodeEx(m_pScene->GetSceneName().c_str(), scene_flags)) {
			// Check scene object clicked
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				SelectScene();
			}

			// Draw scene objects
			std::vector<Object*> scene_objects = m_pScene->GetSceneObjects();
			for (Object* obj : scene_objects) {
				if (!obj)
					continue;

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
				if (m_selectedObject && obj->GetID() == m_selectedID) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool treeOpen;
				if (obj->GetObjectType() == Object_ButtonType) {
					flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
					flags &= ~ImGuiTreeNodeFlags_Leaf;
					treeOpen = ImGui::TreeNodeEx(obj->GetName().c_str(), flags);
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						SelectObject(obj);
					}

					if (treeOpen) {
						flags = ImGuiTreeNodeFlags_Leaf;
						
						TextObject* text_obj = static_cast<ButtonObject*>(obj)->GetTextObject();
						if (m_selectedObject && text_obj->GetID() == m_selectedID) {
							flags |= ImGuiTreeNodeFlags_Selected;
						}
						bool treeOpenText = ImGui::TreeNodeEx(text_obj->GetName().c_str(), flags);

						if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
							SelectObject(text_obj);
						}

						if (treeOpenText)
							ImGui::TreePop();
					}
				}
				else {
					treeOpen = ImGui::TreeNodeEx(obj->GetName().c_str(), flags);
					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						SelectObject(obj);
					}
				}

				char menupopupid[256];
				sprintf_s(menupopupid, 256, "%s%d", "MenuObjectPopup", obj->GetID());
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup(menupopupid);
				}

				if (ImGui::BeginPopup(menupopupid)) {
					static const char* objectMenu[] = { "Delete" };
					static const int objectMenuCount = sizeof(objectMenu) / sizeof(objectMenu[0]);

					for (int i = 0; i < objectMenuCount; ++i) {
						// If an object type is selected
						if (ImGui::Selectable(objectMenu[i])) {
							if (objectMenu[i] == "Delete") {
								if (m_selectedID == obj->GetID()) {
									Unselect();
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

			ImGui::TreePop();
		}
		ImGui::SeparatorText("");
		if (ImGui::TreeNodeEx("Actions", scene_flags)) {
			// Check scene object clicked
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				Unselect();
			}

			std::vector<EngineAction*> scene_actions = m_pScene->GetSceneActions();
			for (EngineAction* action : scene_actions) {
				if (!action)
					continue;

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
				if (m_selectedAction && action->GetID() == m_selectedID) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool treeOpen = ImGui::TreeNodeEx(action->GetName().c_str(), flags);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					SelectAction(action);
				}

				char menupopupid[256];
				sprintf_s(menupopupid, 256, "%s%d", "MenuActionPopup", action->GetID());
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup(menupopupid);
				}

				if (ImGui::BeginPopup(menupopupid)) {
					static const char* objectMenu[] = { "Delete" };
					static const int objectMenuCount = sizeof(objectMenu) / sizeof(objectMenu[0]);

					for (int i = 0; i < objectMenuCount; ++i) {
						// If an object type is selected
						if (ImGui::Selectable(objectMenu[i])) {
							if (objectMenu[i] == "Delete") {
								if (m_selectedID == action->GetID()) {
									Unselect();
								}
								m_pScene->RemoveAction(action);
							}
							ImGui::CloseCurrentPopup();
						}
					}

					ImGui::EndPopup();
				}

				if (treeOpen)
					ImGui::TreePop();
			}
			
			ImGui::TreePop();
		}

		ImGui::End();

		// Inspector tab on the right
		ImGui::Begin("Inspector");
		if (m_bSelectedScene) {
			std::string sceneName = m_pScene->GetSceneName();
			ImGui::Text("Name:"); ImGui::SameLine();
			if (ImGui::InputText("##SceneName", &sceneName, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (sceneName.size() > 0) {
					m_pScene->SetSceneName(sceneName);
					m_pScene->SetChanged();
				}
			}

			float* sceneClearColor = m_pScene->GetClearColor();
			ImGui::Text("BgColor:"); ImGui::SameLine();
			if (ImGui::ColorEdit4("##SceneColor", &sceneClearColor[0])) {
				m_pScene->SetChanged();
			}
		}
		else if (m_selectedAction) {
			std::string actionName = m_selectedAction->GetName();
			ImGui::Text("Name:"); ImGui::SameLine();
			if (ImGui::InputText("##ActionName", &actionName, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (actionName.size() > 0) {
					EngineAction* _action = m_pScene->FindAction(actionName);
					if (!_action || _action == m_selectedAction) {
						m_selectedAction->SetName(actionName);
						m_pScene->SetChanged();
					}
					else {
						MessageBox(m_hWnd, "Action with that name already exists.", "Failure", MB_OK);
					}
				}
			}
		}
		else if (m_selectedObject) {
			Vector2f pos = m_selectedObject->GetPosition();
			Vector2i size = m_selectedObject->GetSize();
			float rotation = m_selectedObject->GetRotation();
			std::string objName = m_selectedObject->GetName();

			ImGui::Text("Name:"); ImGui::SameLine();
			if (ImGui::InputText("##ObjName", &objName, ImGuiInputTextFlags_EnterReturnsTrue)) {
				if (objName.size() > 0) {
					m_selectedObject->SetName(objName);
					m_pScene->SetChanged();
				}
			}

			ImGui::Text("Pos:"); ImGui::SameLine();
			if (ImGui::DragFloat2("##ObjPos", &pos.x)) {
				m_selectedObject->SetPosition(pos);
				m_pScene->SetChanged();

				if (m_selectedObject->GetObjectType() == Object_ButtonType)
					static_cast<ButtonObject*>(m_selectedObject)->UpdateChild();
			}

			ImGui::Text("Rot:"); ImGui::SameLine();
			if (ImGui::DragFloat("##ObjRot", &rotation)) {
				m_selectedObject->SetRotation(rotation);
				m_pScene->SetChanged();

				if (m_selectedObject->GetObjectType() == Object_ButtonType)
					static_cast<ButtonObject*>(m_selectedObject)->UpdateChild();
			}

			ImGui::Text("Size:"); ImGui::SameLine();
			if (ImGui::DragInt2("##ObjSize", &size.x, 1.0f, 0.01f, 100000.f)) {
				if (size.x < 0) {
					size.x = 0.01f;
				}
				if (size.y < 0) {
					size.y = 0.01f;
				}
				m_selectedObject->SetSize(size);
				m_pScene->SetChanged();

				if (m_selectedObject->GetObjectType() == Object_ButtonType)
					static_cast<ButtonObject*>(m_selectedObject)->UpdateChild();
			}

			float* ObjColor = m_selectedObject->GetColor();
			ImGui::Text("Color:"); ImGui::SameLine();
			if (ImGui::ColorEdit4("##ObjColor", &ObjColor[0])) {
				m_pScene->SetChanged();

				D2D1::ColorF color(ObjColor[0], ObjColor[1], ObjColor[2], ObjColor[3]);
				m_selectedObject->SetColor(color);
			}

			if (m_selectedObject->GetObjectType() == Object_TextType) {
				TextObject* textObj = static_cast<TextObject*>(m_selectedObject);
				float fontSize = textObj->GetFontSize();
				ImGui::Text("FontSize:"); ImGui::SameLine();
				if (ImGui::DragFloat("##ObjFont", &fontSize, 1.0f, 1.f, 1000.f)) {
					if (fontSize < 1.0f)
						fontSize = 1.0f;
					textObj->SetFontSize(fontSize);
					m_pScene->SetChanged();
				}

				static const char* fontWeights[] = { "Light", "Regular", "Medium", "Bold", "Black" };
				static int fontWeightsInt[] = { 300, 400, 500, 700, 900 };
				static const int fontWeightCount = sizeof(fontWeights) / sizeof(fontWeights[0]);
				int selectedFontWeight = 1;
				float fontWeight = textObj->GetFontWeight();

				for (int i = 0; i < fontWeightCount; ++i) {
					if (fontWeightsInt[i] == fontWeight) {
						selectedFontWeight = i;
						break;
					}
				}

				ImGui::Text("FontWeight:"); ImGui::SameLine();
				if (ImGui::BeginCombo("##ObjFontCombo", fontWeights[selectedFontWeight])) {
					for (int n = 0; n < fontWeightCount; n++) {
						bool isSelected = (selectedFontWeight == n);
						if (ImGui::Selectable(fontWeights[n], isSelected)) {
							selectedFontWeight = n;
							textObj->SetFontWeight((DWRITE_FONT_WEIGHT)fontWeightsInt[n]);
							m_pScene->SetChanged();
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				std::string obj_text = textObj->GetText();
				ImGui::Text("Text:"); ImGui::SameLine();
				if (ImGui::InputTextMultiline("##ObjTextInput", &obj_text)) {
					textObj->SetText(obj_text);
					m_pScene->SetChanged();
				}

				DWRITE_TEXT_ALIGNMENT text_align = textObj->GetTextAlign();
				ImGui::Text("Text Align:"); ImGui::SameLine();
				ImGui::BeginGroup(); 
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(20, 20))) {
					textObj->SetTextAlign(DWRITE_TEXT_ALIGNMENT_LEADING);
					m_pScene->SetChanged();
				}
				ImGui::SameLine();
				if (ImGui::Button("|", ImVec2(20, 20))) {
					textObj->SetTextAlign(DWRITE_TEXT_ALIGNMENT_CENTER);
					m_pScene->SetChanged();
				}
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_ARROW_RIGHT, ImVec2(20, 20))) {
					textObj->SetTextAlign(DWRITE_TEXT_ALIGNMENT_TRAILING);
					m_pScene->SetChanged();
				}

				ImGui::EndGroup();
			}
			else if (m_selectedObject->GetObjectType() == Object_ButtonType) {
				ButtonObject* buttonObj = static_cast<ButtonObject*>(m_selectedObject);
				Vector2f radius = buttonObj->GetRadius();
				ImGui::Text("Radius:"); ImGui::SameLine();
				if (ImGui::DragFloat2("##ObjButtonRadius", &radius.x)) {
					buttonObj->SetRadius(radius);
					m_pScene->SetChanged();
				}

				std::vector<EngineAction*> scene_actions = m_pScene->GetSceneActions();
				EngineAction* action = buttonObj->GetCallback();
				std::string actionName = "None";
				if (action) {
					actionName = action->GetName();
				}

				ImGui::Text("Action:"); ImGui::SameLine();
				if (ImGui::BeginCombo("##ObjButtonActionCombo", actionName.c_str())) {
					for (EngineAction* action_ : scene_actions) {
						bool isSelected = (action_ == action);
						if (ImGui::Selectable(action_->GetName().c_str(), isSelected)) {
							buttonObj->SetCallback(action_);
							m_pScene->SetChanged();
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
		}
		else {
			ImGui::Text("Please, select object.");
		}
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

void GameEditor::SelectObject(Object* obj) {
	Unselect();
	m_selectedObject = obj;
	m_selectedID = obj->GetID();
}

void GameEditor::SelectAction(EngineAction* action) {
	Unselect();
	m_selectedAction = action;
	m_selectedID = action->GetID();
}

void GameEditor::SelectScene() {
	Unselect();
	m_bSelectedScene = true;
}

void GameEditor::Unselect() {
	m_selectedID = -1;
	m_selectedObject = nullptr;
	m_selectedAction = nullptr;
	m_bSelectedScene = false;
}