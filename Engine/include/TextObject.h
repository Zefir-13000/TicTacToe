#pragma once
#include <Object.h>

class TextObject : public Object {
public:
	TextObject(IDWriteFactory* pDWriteFactory, float fontSize);
	~TextObject();

	void Render(ID2D1RenderTarget* pD2DRenderTarget) override;

	float GetFontSize();
	void SetFontSize(float fontSize);

	DWRITE_FONT_WEIGHT GetFontWeight();
	void SetFontWeight(DWRITE_FONT_WEIGHT fontWeight);

	DWRITE_TEXT_ALIGNMENT GetTextAlign();
	void SetTextAlign(DWRITE_TEXT_ALIGNMENT textAlign);

	std::string GetText() { return std::string(m_text.begin(), m_text.end()); };
	void SetText(std::string text);
	void SetBrush(ID2D1SolidColorBrush* pBrush);

	void UpdateFormat(float fontSize, DWRITE_FONT_WEIGHT fontWeight);
private:
	float m_fontSize = 32;
	DWRITE_FONT_WEIGHT m_fontWeight = DWRITE_FONT_WEIGHT_NORMAL;

	IDWriteTextFormat* pTextFormat = nullptr;
	ID2D1SolidColorBrush* m_pBrush = nullptr;

	std::wstring m_text = L"\0";
	DWRITE_TEXT_ALIGNMENT m_textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;

	IDWriteFactory* m_pDWriteFactory = nullptr; // ref
};