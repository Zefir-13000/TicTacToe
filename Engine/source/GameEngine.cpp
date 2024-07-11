#include <GameEngine.h>

GameEngine::GameEngine() {

}

GameEngine::~GameEngine() {
    if (m_pD2DRenderTarget)
        m_pD2DRenderTarget->Release();
    if (m_pDWriteFactory)
        m_pDWriteFactory->Release();
    if (m_pD2DFactory)
        m_pD2DFactory->Release();

    if (m_pD3DRenderTargetView)
        m_pD3DRenderTargetView->Release();
    if (m_pSwapChain)
        m_pSwapChain->Release();
    if (m_pD3DDeviceContext) 
        m_pD3DDeviceContext->Release();
    if (m_pD3DDevice) {
        m_pD3DDevice->Release();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

bool GameEngine::Initialize(HWND hWnd, UINT windowWidth, UINT windowHeight) {
	HRESULT hr;

    m_hWnd = hWnd;

    auto options = D2D1_FACTORY_OPTIONS();

#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif // _DEBUG

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &m_pD2DFactory);
    if (FAILED(hr)) {
        OutputDebugString("Failed to create Direct2D factory.\n");
        return false;
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
    if (FAILED(hr)) {
        OutputDebugString("Failed to create DirectWrite factory.\n");
        return false;
    }

    if (!CreateDeviceD3D(hWnd, windowWidth, windowHeight)) {
        OutputDebugString("Failed to create DeviceD3D.\n");
        return false;
    }

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pRenderDXT));
    if (FAILED(hr))
    {
        OutputDebugString("Failed to get buffer RenderDXT.\n");
        return false;
    }

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);
    hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(m_pRenderDXT, &props, &m_pD2DRenderTarget);
    if (FAILED(hr))
    {
        OutputDebugString("Failed to create Dxgi Surface.\n");
        return false;
    }

	return true;
}

bool GameEngine::InitializeImgui(HWND hWnd) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pD3DDevice, m_pD3DDeviceContext);

    m_bInitedImgui = true;
    return true;
}

void GameEngine::Tick() {

}

void GameEngine::BeginRender() {
    if (m_pD3DDevice) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const float clear_color[4] = { 0.45f, 0.55f, 0.60f, 1.00f };

        m_pD3DDeviceContext->OMSetRenderTargets(1, &m_pD3DRenderTargetView, nullptr);
        m_pD3DDeviceContext->ClearRenderTargetView(m_pD3DRenderTargetView, clear_color);
    }
    m_pD2DRenderTarget->BeginDraw();
}

void GameEngine::EndRender() {
    m_pD2DRenderTarget->EndDraw();
    if (m_pD3DDevice) {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // Present
        HRESULT hr = m_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        m_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
}

void GameEngine::ClearRender(D2D1::ColorF color) {
    m_pD2DRenderTarget->Clear(color);
}

void GameEngine::Render(Object* object) {
    object->Render(m_pD2DRenderTarget);
}

void GameEngine::SetOccluded(bool isOccluded) {
    m_SwapChainOccluded = isOccluded;
}

bool GameEngine::CreateDeviceD3D(HWND hWnd, UINT windowWidth, UINT windowHeight) {
    HRESULT result;
    IDXGIFactory* factory;
    IDXGIAdapter* adapter;
    IDXGIOutput* adapterOutput;
    unsigned int numModes, i, numerator = 60, denominator = 1;
    unsigned long long stringLength;
    DXGI_MODE_DESC* displayModeList;

    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(result))
    {
        return false;
    }
    // Use the factory to create an adapter for the primary graphics interface (video card).
    result = factory->EnumAdapters(0, &adapter);
    if (FAILED(result))
    {
        return false;
    }

    // Enumerate the primary adapter output (monitor).
    result = adapter->EnumOutputs(0, &adapterOutput);
    if (FAILED(result))
    {
        return false;
    }

    // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
    result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
    if (FAILED(result))
    {
        return false;
    }

    // Create a list to hold all the possible display modes for this monitor/video card combination.
    displayModeList = new DXGI_MODE_DESC[numModes];
    if (!displayModeList)
    {
        return false;
    }

    for (i = 0; i < numModes; i++)
    {
        if (displayModeList[i].Width == windowWidth)
        {
            if (displayModeList[i].Height == windowHeight)
            {
                numerator = displayModeList[i].RefreshRate.Numerator;
                denominator = displayModeList[i].RefreshRate.Denominator;
            }
        }
    }

    // Release the display mode list.
    delete[] displayModeList;
    displayModeList = 0;

    // Release the adapter output.
    adapterOutput->Release();
    adapterOutput = 0;

    // Release the adapter.
    adapter->Release();
    adapter = 0;

    // Release the factory.
    factory->Release();
    factory = 0;

    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = numerator;
    sd.BufferDesc.RefreshRate.Denominator = denominator;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG
    
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3DDevice, &featureLevel, &m_pD3DDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pD3DDevice, &featureLevel, &m_pD3DDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTargetD3D();
    return true;
}

void GameEngine::CreateRenderTargetD3D() {
    ID3D11Texture2D* pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pD3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pD3DRenderTargetView);
    pBackBuffer->Release();
}

void GameEngine::SetResize(UINT width, UINT height) {
    HRESULT hr;

    m_pD2DRenderTarget->Release();
    m_pRenderDXT->Release();

    if (m_pD3DRenderTargetView) {
        m_pD3DRenderTargetView->Release();
        m_pD3DRenderTargetView = nullptr;
    }
    m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    GameEngine::CreateRenderTargetD3D();

    hr = m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&m_pRenderDXT));
    if (FAILED(hr))
    {
        OutputDebugString("Failed to get buffer RenderDXT.\n");
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0);
    hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(m_pRenderDXT, &props, &m_pD2DRenderTarget);
    if (FAILED(hr))
    {
        OutputDebugString("Failed to create Dxgi Surface.\n");
        return;
    }
}