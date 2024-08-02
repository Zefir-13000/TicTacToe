#pragma once
#include <Object.h>

class TextObject : public Object {
public:
	TextObject(float fontSize);
	TextObject();
	~TextObject();

	void Render() override;

	float GetFontSize();
	void SetFontSize(float fontSize);

	DWRITE_FONT_WEIGHT GetFontWeight();
	void SetFontWeight(DWRITE_FONT_WEIGHT fontWeight);

	DWRITE_TEXT_ALIGNMENT GetTextAlign();
	void SetTextAlign(DWRITE_TEXT_ALIGNMENT textAlign);

	std::string GetText();
	void SetText(std::string text);

	void UpdateFormat(float fontSize, DWRITE_FONT_WEIGHT fontWeight);
	
	virtual void Save(std::ofstream& stream);
	virtual void Load(std::ifstream& stream);
private:
	float m_fontSize = 32.f;
	std::string m_text;
	DWRITE_FONT_WEIGHT m_fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
	IDWriteTextFormat* pTextFormat = nullptr;
	DWRITE_TEXT_ALIGNMENT m_textAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
};