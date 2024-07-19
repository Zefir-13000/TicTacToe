#pragma once
#include <stdafx.h>
#include <GameEngine.h>
#include <Scene.h>

class GameEditor {
public:
	GameEditor(HWND hWnd);
	~GameEditor();

	void Tick();
	void Render();

	bool Initialize(UINT width, UINT height);
	void Shutdown();

	void SetResize(UINT width, UINT height);
private:
	bool InitializeViewport(UINT width, UINT height);
	void RenderScene();

	HWND m_hWnd;
	GameEngine* m_pEngine = nullptr;
	Scene* m_pScene = nullptr;

	int m_selectedID = -1;
	Object* m_selectedObject = nullptr;
	bool m_bSelectedScene = false;

	ImVec2 m_viewportSize;
	ID3D11Texture2D* m_viewportTexture = nullptr;
	ID3D11ShaderResourceView* m_viewportTextureShaderView = nullptr;
	ID3D11RenderTargetView* m_viewportTextureRenderView = nullptr;
};