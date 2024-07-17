#pragma once
#include <stdafx.h>

enum ObjectType {
	Object_NULL		= 0,
	Object_TextType = 1,

};

struct Vector2f {
	Vector2f(float _x, float _y) : x(_x), y(_y) {}
	Vector2f() : x(0), y(0) {}
	float x;
	float y;
};

struct Vector2i {
	Vector2i(int _x, int _y) : x(_x), y(_y) {}
	Vector2i() : x(0), y(0) {}
	int x;
	int y;
};

class Object {
public:
	virtual ~Object() = default;
	virtual void Render(ID2D1RenderTarget* pD2DRenderTarget) = 0;

	int GetID() { return id; }

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	float GetRotation() { return m_rotation; }
	void SetRotation(float rot) { m_rotation = rot; }

	Vector2f GetPosition() { return m_objPos; }
	void SetPosition(Vector2f pos) { m_objPos = pos; }
	void SetPosition(float x, float y) { m_objPos = {x, y}; }

	D2D1_SIZE_U GetSize() { return m_renderSize; }
	void SetSize(D2D1_SIZE_U size) { m_renderSize = size; }
	void SetSize(UINT width, UINT height) { m_renderSize = D2D1::SizeU(width, height); }

	D2D1_RECT_F GetDrawRect() { return D2D1::RectF(m_objPos.x, m_objPos.y, m_objPos.x + m_renderSize.width, m_objPos.y + m_renderSize.height); }
	Vector2f GetDrawRectMiddle() { return Vector2f(m_objPos.x + m_renderSize.width / 2, m_objPos.y + m_renderSize.height / 2); }

	ObjectType GetObjectType() { return m_ObjType; }
protected:
	Object() { static int _id = 0; id = _id++; }

	ObjectType m_ObjType = Object_NULL;
	Vector2f m_objPos = {0, 0};
	float m_rotation = 0;
	D2D1_SIZE_U m_renderSize;
	std::string m_name = "Object";
private:
	int id;
};