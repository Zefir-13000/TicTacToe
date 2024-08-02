#include <TextObject.h>

TextObject::TextObject(float fontSize) {
	m_ObjType = Object_TextType;
    m_name = "TextObject";
    m_text = "Text";
    pTextFormat = nullptr;
    m_fontSize = fontSize;

    UpdateFormat(m_fontSize, m_fontWeight);
}

TextObject::TextObject() {
    m_ObjType = Object_TextType;
    m_name = "TextObject";
    m_text = "Text";
    pTextFormat = nullptr;
    m_fontSize = 32;

    UpdateFormat(m_fontSize, m_fontWeight);
}

TextObject::~TextObject() {
    if (pTextFormat) {
        pTextFormat->Release();
        pTextFormat = nullptr;
    }
}

void TextObject::UpdateFormat(float fontSize, DWRITE_FONT_WEIGHT fontWeight) {
    if (!m_pDWriteFactory) {
        OutputDebugString("TextObject: Failed to get m_pDWriteFactory.\n");
        return;
    }
    if (pTextFormat) {
        pTextFormat->Release();
        pTextFormat = nullptr;
    }

    HRESULT hr;
    hr = m_pDWriteFactory->CreateTextFormat(
        L"Arial",
        NULL,
        fontWeight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &pTextFormat
    );
    if (FAILED(hr)) {
        OutputDebugString("TextObject: Failed to create text format.\n");
        return;
    }
    pTextFormat->SetTextAlignment(m_textAlign);
}

std::string TextObject::GetText() {
    return m_text; 
}

void TextObject::SetText(std::string text) {
    m_text = text;
}

float TextObject::GetFontSize() { 
    return m_fontSize; 
}

void TextObject::SetFontSize(float fontSize) {
    m_fontSize = fontSize;
    UpdateFormat(m_fontSize, m_fontWeight);
}

DWRITE_FONT_WEIGHT TextObject::GetFontWeight() {
    return pTextFormat->GetFontWeight(); 
}

void TextObject::SetFontWeight(DWRITE_FONT_WEIGHT fontWeight) {
    m_fontWeight = fontWeight;
    UpdateFormat(m_fontSize, m_fontWeight);
}

DWRITE_TEXT_ALIGNMENT TextObject::GetTextAlign() { 
    return m_textAlign;
}

void TextObject::SetTextAlign(DWRITE_TEXT_ALIGNMENT textAlign) {
    m_textAlign = textAlign;
    pTextFormat->SetTextAlignment(m_textAlign);
}

void TextObject::Render() {
    ID2D1RenderTarget* pD2DRenderTarget = GetRenderTarget();

    if (!pTextFormat) {
        OutputDebugString("TextObject - pTextFormat was null\n");
        return;
    }
    if (!m_pBrush) {
        OutputDebugString("TextObject - m_pBrush was null\n");
        return;
    }

    if (m_rotation != 0) {
        Vector2f middle_pos = GetDrawRectMiddle();
        pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(m_rotation, D2D1::Point2F(middle_pos.x, middle_pos.y)));
    }

    std::wstring wtext(m_text.begin(), m_text.end());
    pD2DRenderTarget->DrawText(
        wtext.c_str(),
        wcslen(wtext.c_str()),
        pTextFormat,
        GetDrawRect(),
        m_pBrush
    );

    pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void TextObject::Save(std::ofstream& stream) {
    Object::Save(stream);

    stream.write((const char*)&m_fontSize, sizeof(float));
    stream.write((const char*)&m_fontWeight, sizeof(int));
    stream.write((const char*)&m_textAlign, sizeof(int));
    
    size_t text_size = m_text.size();
    stream.write((const char*)&text_size, sizeof(size_t));
    stream.write(&m_text[0], text_size);
}

void TextObject::Load(std::ifstream& stream) {
    Object::Load(stream);

    stream.read((char*)&m_fontSize, sizeof(float));

    int fontWeightInt = 0;
    stream.read((char*)&fontWeightInt, sizeof(int));
    m_fontWeight = static_cast<DWRITE_FONT_WEIGHT>(fontWeightInt);

    int textAlignInt = 0;
    stream.read((char*)&textAlignInt, sizeof(int));
    m_textAlign = static_cast<DWRITE_TEXT_ALIGNMENT>(textAlignInt);

    size_t text_size = 0;
    stream.read((char*)&text_size, sizeof(size_t));
    m_text.resize(text_size);
    stream.read(&m_text[0], text_size);
   
    UpdateFormat(m_fontSize, m_fontWeight);
}