#pragma once
#include <stdafx.h>
#include <TextObject.h>

class GameEngine {
public:
	GameEngine();
	~GameEngine();

	void Tick();

	IDWriteFactory* GetDWriteFactory() { return m_pDWriteFactory; };
	ID2D1Factory* GetD2DFactory() { return m_pD2DFactory; };
	ID2D1RenderTarget* GetRenderTarget() { return m_pD2DRenderTarget; }
	IDXGISwapChain* GetSwapChainD3D() { return m_pSwapChain; }
	ID3D11Device* GetDeviceD3D() { return m_pD3DDevice; };
	ID3D11DeviceContext* GetDeviceContextD3D() { return m_pD3DDeviceContext; };

	void BeginRender2D();
	void BeginRender3D();
	void EndRender2D();
	void EndRender3D();
	void ClearRender(D2D1::ColorF color);

	void Render(Object* object);

	bool Initialize(HWND hWnd, UINT windowWidth, UINT windowHeight);
	bool InitializeDXGISurface1(IDXGISurface1* pRenderDXT);
	bool InitializeImgui(HWND hWnd);

	bool GetOccluded() { return m_SwapChainOccluded; };

	void SetResizeSwapchain3D(UINT width, UINT height);
	void SetOccluded(bool isOccluded);
private:
	bool CreateDeviceD3D(HWND hWnd, UINT windowWidth, UINT windowHeight);
	void CreateRenderTargetD3D();

	HWND m_hWnd = nullptr;

	ID2D1Factory* m_pD2DFactory = nullptr;
	IDWriteFactory* m_pDWriteFactory = nullptr;
	ID2D1RenderTarget* m_pD2DRenderTarget = nullptr;
	IDXGISurface1* m_pRenderDXT = nullptr;

	ID3D11Device* m_pD3DDevice = nullptr;
	ID3D11DeviceContext* m_pD3DDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pD3DRenderTargetView = nullptr;

	bool m_SwapChainOccluded = false;
	bool m_bInitedImgui = false;
};