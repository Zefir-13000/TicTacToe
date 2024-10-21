#include <Object.h>

const Vector2i Vector2i::ZERO;
const Vector2f Vector2f::ZERO;

void Object::Save(std::ofstream& stream) {
	size_t name_size = m_name.size();
	stream.write((const char*)&name_size, sizeof(size_t));
	stream.write(&m_name[0], name_size);

	stream.write((const char*)&m_bObjectActive, sizeof(bool));

	stream.write((const char*)&m_objColor[0], sizeof(float) * 4);

	stream.write((const char*)&m_objPos, sizeof(Vector2f));
	stream.write((const char*)&m_renderSize, sizeof(Vector2i));
	stream.write((const char*)&m_rotation, sizeof(float));
}

void Object::Load(std::ifstream& stream) {
	size_t name_size = 0;
	stream.read((char*)&name_size, sizeof(size_t));
	m_name.resize(name_size);
	stream.read(&m_name[0], name_size);

	stream.read((char*)&m_bObjectActive, sizeof(bool));

	stream.read((char*)&m_objColor[0], sizeof(float) * 4);

	stream.read((char*)&m_objPos, sizeof(Vector2f));
	stream.read((char*)&m_renderSize, sizeof(Vector2i));
	stream.read((char*)&m_rotation, sizeof(float));

	D2D1::ColorF color(m_objColor[0], m_objColor[1], m_objColor[2], m_objColor[3]);
	SetColor(color);
}