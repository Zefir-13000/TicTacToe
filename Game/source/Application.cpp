// Game Application

#include <Application.h>

Application* g_pApp = nullptr;
Application* GlobalApp() { return g_pApp; };

extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText) {
	OutputDebugStringA(pchDebugText);
}

Application::Application() {
	g_pApp = this;
}

Application::~Application() {

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
	// Game only
	case WM_LBUTTONDOWN:
		GlobalApp()->TriggerEvent(EngineEvent_OnClick);
		return 0;
	case WM_MOUSEMOVE:
		GlobalApp()->TriggerEvent(EngineEvent_MouseMove);
		return 0;
	case WM_MOVE:
		GlobalApp()->SingleRender();
		return 0;
	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		GlobalApp()->SetClosing();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, umessage, wparam, lparam);
}

void Application::SingleRender() {
	if (m_pGame != nullptr) {
		m_pGame->Tick();
	}
}

bool Application::Initialize() {

	// Init steam
	bool result;

	result = SteamAPI_RestartAppIfNecessary(k_uAppIdInvalid);
	if (result) {
		return false;
	}

	result = SteamAPI_Init();
	if (!result)
	{
		Alert("Fatal Error", "Steam must be running to play this game (SteamAPI_Init() failed).\n");
		return false;
	}

#ifdef _DEBUG
	SteamClient()->SetWarningMessageHook(&SteamAPIDebugTextHook);
#endif // _DEBUG

	// Init window
	WNDCLASSEX wc{};

	m_hInstance = GetModuleHandle(NULL);

	m_applicationName = "TicTacToe";

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

	m_hWnd = CreateWindowEx(0, m_applicationName, m_applicationName, (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),
		posX, posY, m_screenWidth, m_screenHeight, NULL, NULL, m_hInstance, NULL);

	ShowWindow(m_hWnd, SW_SHOW);
	SetForegroundWindow(m_hWnd);
	SetFocus(m_hWnd);

	// Init engine
	m_pGame = new Game(m_hWnd);
	std::string StartSceneName = "MainMenu";

	result = m_pGame->Initialize(StartSceneName);
	if (!result) {
		OutputDebugString("Failed to init game.\n");
		return false;
	}

	return true;
}

bool Application::IsClosing() const {
	return m_bIsClosing;
}

void Application::SetClosing() {
	m_bIsClosing = true;
}

void Application::Shutdown() {
	if (m_pGame) {
		m_pGame->Shutdown();
		delete m_pGame;
	}

	DestroyWindow(m_hWnd);
	m_hWnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hInstance);
	m_hInstance = NULL;

	SteamAPI_Shutdown();
}

void Application::Run() {
	MSG msg;

	while (true) {
		m_pGame->Tick();
		
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void Application::TriggerEvent(EngineEvent event) {
	m_pGame->TriggerEvent(event);
}