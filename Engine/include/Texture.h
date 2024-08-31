#pragma once
#include <stdafx.h>
#include <Object.h>

#include <wincodec.h>

inline extern const std::string TEXTURE_DEFAULT_FILENAME = "untitled";
class Texture {
public:
	Texture(uint8_t* srcData, uint32_t width, uint32_t height);
	~Texture();

	ID2D1Bitmap* GetBitmap() const;

	void UpdateData(uint8_t* srcData);
	void UpdateSize(uint32_t width, uint32_t height);

	bool LoadFromFile(std::string filename);
	std::string ShowOpenDialog(HWND hWnd);

	std::string GetFilename() const;
	Vector2i GetSize() const;
	D2D1_RECT_F GetDrawRect(Vector2f pos, Vector2i size);

	static Texture* GetEmptyTexture(uint32_t width, uint32_t height);
private:
	bool InitBitmap(uint8_t* srcData);

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	std::string m_filename = TEXTURE_DEFAULT_FILENAME;
	ID2D1Bitmap* m_bitmap = nullptr;
};