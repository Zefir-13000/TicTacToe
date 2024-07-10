#pragma once
#include <Object.h>

class TextObject : public Object {
public:
	TextObject(IDWriteFactory* pDWriteFactory, float fontSize);
	~TextObject();

	void Render(ID2D1HwndRenderTarget* pD2DRenderTarget) override;

	void SetText(std::string text);
	void SetBrush(ID2D1SolidColorBrush* pBrush);
	void SetSize(D2D1_SIZE_U size);
	void SetSize(UINT width, UINT height);

	void UpdateFormat(IDWriteFactory* pDWriteFactory, float fontSize);
private:
	IDWriteTextFormat* pTextFormat = nullptr;
	ID2D1SolidColorBrush* m_pBrush = nullptr;
	D2D1_SIZE_U m_renderSize;

	std::wstring m_text = L"\0";
};