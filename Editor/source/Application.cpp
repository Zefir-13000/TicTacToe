// Editor Application

#include <Application.h>

Application* g_pApp = nullptr;
Application* GlobalApp() { return g_pApp; };

Application::Application() {
	g_pApp = this;
}

Application::~Application() {

}

void Application::SetResize(UINT width, UINT height) {
	m_pEditor->SetResize(width, height);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, umessage, wparam, lparam))
		return true;

	switch (umessage)
	{
	case WM_SIZE:
		if (wparam == SIZE_MINIMIZED)
			return 0;

		GlobalApp()->SetResize((UINT)LOWORD(lparam), (UINT)HIWORD(lparam));
		return 0;
	case WM_DESTROY:
		PostMessage(hwnd, WM_QUIT, 0, 0);
		return 0;
	}
	return DefWindowProc(hwnd, umessage, wparam, lparam);
}

bool Application::Initialize() {

	// Init window
	WNDCLASSEX wc{};

	m_hInstance = GetModuleHandle(NULL);

	m_applicationName = "TicTacToe Editor";

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	m_screenWidth = 800;
	m_screenHeight = 600;

	// Place the window in the middle of the screen.
	int posX = (GetSystemMetrics(SM_CXSCREEN) - m_screenWidth) / 2;
	int posY = (GetSystemMetrics(SM_CYSCREEN) - m_screenHeight) / 2;

	m_hWnd = CreateWindowEx(0, m_applicationName, m_applicationName, WS_OVERLAPPEDWINDOW,
		posX, posY, m_screenWidth, m_screenHeight, NULL, NULL, m_hInstance, NULL);

	// Init engine
	bool result;

	m_pEditor = new GameEditor(m_hWnd);
	result = m_pEditor->Initialize(m_screenWidth, m_screenHeight);
	if (!result) {
		OutputDebugString("Failed to init game editor.\n");
		return false;
	}

	ShowWindow(m_hWnd, SW_SHOW);
	SetForegroundWindow(m_hWnd);
	SetFocus(m_hWnd);

	return true;
}

void Application::Shutdown() {
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hInstance);
	m_hInstance = NULL;

	if (m_pEditor) {
		m_pEditor->Shutdown();
		delete m_pEditor;
	}
}

void Application::Run() {
	MSG msg;
	while (true) {
		m_pEditor->Tick();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			m_pEditor->Render();
		}
	}
}