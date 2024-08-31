#include <TextureObject.h>

TextureObject::TextureObject() {
	m_ObjType = Object_TextureType;
	m_name = "TextureObject";

	m_texture = Texture::GetEmptyTexture(64, 64);
	m_textureSize = m_texture->GetSize();
}

TextureObject::~TextureObject() {
	if (m_texture) {
		delete m_texture;
	}
}

Texture* TextureObject::GetTexture() const {
	return m_texture;
}

void TextureObject::SetTexture(Texture* texture) {
	if (m_texture) {
		delete m_texture;
	}

	m_texture = texture;
	m_textureSize = m_texture->GetSize();
}

Vector2i TextureObject::GetTextureSize() const {
	return m_textureSize;
}

void TextureObject::SetTextureSize(Vector2i size) {
	if (m_texture) {
		delete m_texture;
	}

	m_textureSize = size;
	m_texture = Texture::GetEmptyTexture(size.x, size.y);
}

void TextureObject::UpdateSize(uint32_t width, uint32_t height) {
	m_texture->UpdateSize(width, height);
	m_textureSize = m_texture->GetSize();
}

void TextureObject::Render() {
	ID2D1RenderTarget* pD2DRenderTarget = GetRenderTarget();

	if (m_rotation != 0) {
		Vector2f middle_pos = GetDrawRectMiddle();
		pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(m_rotation, D2D1::Point2F(middle_pos.x, middle_pos.y)));
	}

	pD2DRenderTarget->DrawBitmap(
		m_texture->GetBitmap(),
		GetDrawRect(),
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
		m_texture->GetDrawRect(Vector2f::ZERO, m_textureSize)
	);

	pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void TextureObject::LoadTextureFromFile(std::string filename) {
	m_texture->LoadFromFile(filename);
	m_textureSize = m_texture->GetSize();
}

void TextureObject::Save(std::ofstream& stream) {
	Object::Save(stream);

	std::string filename = m_texture->GetFilename();
	size_t filename_size = filename.size();
	stream.write((const char*)&filename_size, sizeof(size_t));
	stream.write(&filename[0], filename_size);
}

void TextureObject::Load(std::ifstream& stream) {
	Object::Load(stream);

	std::string filename = "";
	size_t filename_size = 0;
	stream.read((char*)&filename_size, sizeof(size_t));
	filename.resize(filename_size);
	stream.read(&filename[0], filename_size);

	if (filename != TEXTURE_DEFAULT_FILENAME) {

		char buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);

		std::string exe_path(buffer);
		std::string filename_path = exe_path.substr(0, exe_path.rfind("\\"));
		std::string filename_local = filename_path + "\\data\\images\\" + filename;
		if (m_texture->LoadFromFile(filename_local))
			m_textureSize = m_texture->GetSize();
		else
			m_texture = Texture::GetEmptyTexture(64, 64);
	}
}
