#include <TextObject.h>

TextObject::TextObject(IDWriteFactory* pDWriteFactory, float fontSize) {
    m_pDWriteFactory = pDWriteFactory;

	m_ObjType = Object_TextType;
    m_name = "TextObject";

    UpdateFormat(fontSize);
}

TextObject::~TextObject() {
    if (pTextFormat)
        pTextFormat->Release();
}

void TextObject::UpdateFormat(float fontSize) {
    if (pTextFormat)
        pTextFormat->Release();

    HRESULT hr;
    hr = m_pDWriteFactory->CreateTextFormat(
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
    pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
}

void TextObject::SetText(std::string text) {
    std::wstring wtext(text.begin(), text.end());
    m_text = wtext;
}

void TextObject::SetBrush(ID2D1SolidColorBrush* pBrush) {
    m_pBrush = pBrush;
}

void TextObject::Render(ID2D1RenderTarget* pD2DRenderTarget) {
    if (m_rotation != 0) {
        Vector2f middle_pos = GetDrawRectMiddle();
        pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(m_rotation, D2D1::Point2F(middle_pos.x, middle_pos.y)));
    }
    //pD2DRenderTarget->DrawTextLayout();
    pD2DRenderTarget->DrawText(
        m_text.c_str(),
        wcslen(m_text.c_str()),
        pTextFormat,
        GetDrawRect(),
        m_pBrush
    );

    pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}