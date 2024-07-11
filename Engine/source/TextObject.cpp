#include <TextObject.h>

TextObject::TextObject(IDWriteFactory* pDWriteFactory, float fontSize) {
	m_ObjType = Object_TextType;

    UpdateFormat(pDWriteFactory, fontSize);
}

TextObject::~TextObject() {
    if (pTextFormat)
        pTextFormat->Release();
}

void TextObject::UpdateFormat(IDWriteFactory* pDWriteFactory, float fontSize) {
    if (pTextFormat)
        pTextFormat->Release();

    HRESULT hr;
    hr = pDWriteFactory->CreateTextFormat(
        L"Arial",
        NULL,
        DWRITE_FONT_WEIGHT_REGULAR,
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
}

void TextObject::SetText(std::string text) {
    std::wstring wtext(text.begin(), text.end());
    m_text = wtext;
}

void TextObject::SetBrush(ID2D1SolidColorBrush* pBrush) {
    m_pBrush = pBrush;
}

void TextObject::SetSize(D2D1_SIZE_U size) {
    m_renderSize = size;
}

void TextObject::SetSize(UINT width, UINT height) {
    m_renderSize = D2D1::SizeU(width, height);
}

void TextObject::Render(ID2D1RenderTarget* pD2DRenderTarget) {
    pD2DRenderTarget->DrawText(
        m_text.c_str(),
        wcslen(m_text.c_str()),
        pTextFormat,
        D2D1::RectF(m_objPos.x, m_objPos.y, m_objPos.x + m_renderSize.width, m_objPos.y + m_renderSize.height),
        m_pBrush
    );
}