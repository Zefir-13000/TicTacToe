#pragma once
#include <UIObject.h>
#include <Texture.h>

class Scene;
class TextureObject : public Object {
public:
	TextureObject();
	~TextureObject();

	void Render() override;

	Texture* GetTexture() const;
	void SetTexture(Texture* texture);

	Vector2i GetTextureSize() const;
	void SetTextureSize(Vector2i size);

	void LoadTextureFromFile(std::string filename);

	void UpdateSize(uint32_t width, uint32_t height);

	virtual void Save(std::ofstream& stream);
	virtual void Load(std::ifstream& stream);
private:
	Vector2i m_textureSize = Vector2i::ZERO;
	Texture* m_texture;
};