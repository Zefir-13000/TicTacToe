#pragma once
#include <stdafx.h>

enum ObjectType {
	Object_NULL		= 0,
	Object_TextType = 1,

};

class Object {
public:
	Object() = default;
	~Object() = default;

	virtual void Render(ID2D1RenderTarget* pD2DRenderTarget) = 0;

	POINT GetPosition() { return m_objPos; }
	void SetPosition(POINT pos) { m_objPos = pos; };
	void SetPosition(int x, int y) { m_objPos = {x, y}; };

	ObjectType GetObjectType() { return m_ObjType; };
protected:
	ObjectType m_ObjType = Object_NULL;
	POINT m_objPos = {0, 0};
};