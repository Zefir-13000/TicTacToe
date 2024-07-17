#pragma once
#include <Object.h>

class TextObject : public Object {
public:
	TextObject(IDWriteFactory* pDWriteFactory, float fontSize);
	~TextObject();

	void Render(ID2D1RenderTarget* pD2DRenderTarget) override;

	float GetFontSize() { return pTextFormat->GetFontSize(); }
	void SetFontSize(float fontSize) { UpdateFormat(fontSize); }

	void SetText(std::string text);
	void SetBrush(ID2D1SolidColorBrush* pBrush);

	void UpdateFormat(float fontSize);
private:
	IDWriteTextFormat* pTextFormat = nullptr;
	ID2D1SolidColorBrush* m_pBrush = nullptr;

	std::wstring m_text = L"\0";

	IDWriteFactory* m_pDWriteFactory; // ref
};