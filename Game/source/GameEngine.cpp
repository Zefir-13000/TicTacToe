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
}

bool GameEngine::Initialize(HWND hWnd) {
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

    RECT rect;
    GetClientRect(m_hWnd, &rect);

    hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(m_hWnd, D2D1::SizeU(rect.right, rect.bottom)), &m_pD2DRenderTarget);
    if (FAILED(hr)) {
        OutputDebugString("Failed to create HwndRenderTarget.\n");
        return false;
    }

	return true;
}

void GameEngine::Tick() {

}

void GameEngine::BeginRender() {
    m_pD2DRenderTarget->BeginDraw();
}

void GameEngine::EndRender() {
    m_pD2DRenderTarget->EndDraw();
}

void GameEngine::ClearRender(D2D1::ColorF color) {
    m_pD2DRenderTarget->Clear(color);
}

void GameEngine::Render(Object* object) {
    object->Render(m_pD2DRenderTarget);
}