#include <GameEditor.h>

GameEditor::GameEditor(HWND hWnd) {
	m_hWnd = hWnd;
}

GameEditor::~GameEditor() {

}

bool GameEditor::InitializeViewport(UINT width, UINT height) {
	return true;

	/*D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));

	texDesc.Width = Data.Width;
	texDesc.Height = Data.Height;
	texDesc.Format = R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.CPUAccessFlags = 0;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = 0;
	texDesc.MipLevels = 1;*/
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

	result = InitializeViewport(width, height);
	if (!result) {
		OutputDebugString("Failed to init game viewport.\n");
		return false;
	}

	return true;
}

void GameEditor::Shutdown() {
	if (m_pEngine) {
		delete m_pEngine;
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

	// TODO: Render to game preview to texture

	// Begin Render 2D/3D
	m_pEngine->BeginRender();
	
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
				if (ImGui::MenuItem("Create")) {
				}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
				}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {
				}
				if (ImGui::MenuItem("Save as..")) {
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

			//ImGuiID left_id, right_id, right_2_id;

			//auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, &left_id, &right_id);
			//auto dock_id_right = ImGui::DockBuilderSplitNode(right_id, ImGuiDir_Right, 0.2f, &right_2_id, &right_id);
			//auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
			///ImGui::DockBuilderDockWindow("Left", left_id);
			//ImGui::DockBuilderDockWindow("Down", right_id);
			//ImGui::DockBuilderDockWindow("Right", dock_id_right);

			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGui::End();

		ImGui::Begin("Left");
		ImGui::Text("Hello, left!");
		ImGui::End();

		ImGui::Begin("Down");
		ImGui::Text("Hello, down!");
		ImGui::End();

		ImGui::Begin("Right");
		ImGui::Text("Hello, right!");
		ImGui::End();

		bool show_demo_window = true;
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	/*ID2D1SolidColorBrush* m_pBlackBrush = nullptr;
	HRESULT hr;
	hr = m_pEngine->GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pBlackBrush);
	if (FAILED(hr)) {
		OutputDebugString("Failed to create Black SolidBrush.\n");
	}*/

	//TextObject* text1 = new TextObject(m_pEngine->GetDWriteFactory(), 32.f);
	//text1->SetText("TestText");
	//text1->SetBrush(m_pBlackBrush);
	//text1->SetPosition(50, 50);
	//text1->SetSize(150, 50);
	//m_pEngine->Render(text1);

	//m_pBlackBrush->Release();
	//delete text1;

	// End Draw call 2D/3D
	m_pEngine->EndRender();
}

void GameEditor::SetResize(UINT width, UINT height) {
	m_pEngine->SetResize(width, height);
}