#pragma once
#include <stdafx.h>
#include <EngineEvents.h>

extern IDWriteFactory* g_pDWriteFactory;
extern IDWriteFactory* GetDWriteFactory();

extern HWND g_hWnd;
extern HWND GetHWND();

extern ID2D1RenderTarget* g_pD2DRenderTarget;
extern ID2D1RenderTarget* GetRenderTarget();

class Scene;
class Object;
class GameEngine {
public:
	GameEngine();
	~GameEngine();

	void Tick();

	ID2D1Factory* GetD2DFactory() { return m_pD2DFactory; }
	IDXGISwapChain* GetSwapChainD3D() { return m_pSwapChain; }
	ID3D11Device* GetDeviceD3D() { return m_pD3DDevice; }
	ID3D11DeviceContext* GetDeviceContextD3D() { return m_pD3DDeviceContext; }

	void BeginRender2D();
	void BeginRender3D();
	void EndRender2D();
	void EndRender3D();
	void ClearRender(D2D1::ColorF color);

	void Render(Object* object);

	bool Initialize(HWND hWnd, UINT windowWidth, UINT windowHeight);
	bool InitializeDXGISurface1(IDXGISurface1* pRenderDXT);
	bool InitializeSwapchainRenderTarget();
	bool InitializeImgui(HWND hWnd);

	bool GetOccluded() { return m_SwapChainOccluded; };

	void SetResizeSwapchain3D(UINT width, UINT height);
	void SetOccluded(bool isOccluded);

	void SetScene(Scene* pScene);

	void TriggerEvent(EngineEvent event);

	bool IsEditor() { return m_bInitedImgui; }

	uint64 GetGameTickCount() const;
private:
	bool CreateDeviceD3D(HWND hWnd, UINT windowWidth, UINT windowHeight);
	void CreateRenderTargetD3D();

	Scene* m_pScene = nullptr; // ref

	ID2D1Factory* m_pD2DFactory = nullptr;
	IDXGISurface1* m_pRenderDXT = nullptr;

	ID3D11Device* m_pD3DDevice = nullptr;
	ID3D11DeviceContext* m_pD3DDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pD3DRenderTargetView = nullptr;

	bool m_SwapChainOccluded = false;
	bool m_bInitedImgui = false;

	uint64 m_ulGameTickCount;
	uint64 m_ulPreviousGameTickCount;
	uint64 m_ulPerfCounterToMillisecondsDivisor;
	uint64 m_ulFirstQueryPerformanceCounterValue;
};