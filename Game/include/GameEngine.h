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
	ID2D1HwndRenderTarget* GetRenderTarget() { return m_pD2DRenderTarget; }

	void BeginRender();
	void EndRender();
	void ClearRender(D2D1::ColorF color);

	void Render(Object* object);

	bool Initialize(HWND hWnd);
private:
	HWND m_hWnd = nullptr;

	ID2D1Factory* m_pD2DFactory = nullptr;
	IDWriteFactory* m_pDWriteFactory = nullptr;
	ID2D1HwndRenderTarget* m_pD2DRenderTarget = nullptr;
};