#pragma once
#include <stdafx.h>
#include <GameEngine.h>

enum ObjectType {
	Object_None			= 0,
	Object_TextType		= 1,
	Object_ButtonType	= 2,
	Object_TextureType	= 3,
};

struct Vector2f {
	Vector2f(float _x, float _y) : x(_x), y(_y) {}
	Vector2f() : x(0), y(0) {}
	float x;
	float y;

	const static Vector2f ZERO;
};

struct Vector2i {
	Vector2i(int _x, int _y) : x(_x), y(_y) {}
	Vector2i() : x(0), y(0) {}
	int x;
	int y;

	const static Vector2i ZERO;
};

class Object {
public:
	virtual ~Object() {
		if (m_pBrush && !m_bIsBrushRef)
			m_pBrush->Release();
		m_pDWriteFactory = nullptr;
		m_pBrush = nullptr;
	}
	virtual void Render() = 0;

	int GetID() const { return id; }

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	bool GetActive() { return m_bObjectActive; }
	void SetActive(bool bActive) { m_bObjectActive = bActive; }

	float GetRotation() { return m_rotation; }
	void SetRotation(float rot) { m_rotation = rot; }

	Vector2f GetPosition() { return m_objPos; }
	void SetPosition(Vector2f pos) { m_objPos = pos; }
	void SetPosition(float x, float y) { m_objPos = {x, y}; }

	Vector2i GetSize() { return m_renderSize; }
	void SetSize(Vector2i size) { m_renderSize = size; }
	void SetSize(UINT width, UINT height) { m_renderSize = Vector2i(width, height); }

	float* GetColor() { return m_objColor; }
	void SetColor(D2D1::ColorF color) {
		if (m_pBrush && !m_bIsBrushRef) {
			m_pBrush->Release();
		}

		HRESULT hr;
		hr = GetRenderTarget()->CreateSolidColorBrush(color, &m_pBrush);
		if (FAILED(hr)) {
			OutputDebugString("Failed to create ColorBrush.\n");
		}
		m_bIsBrushRef = false;
	}
	void SetColor(ID2D1SolidColorBrush* pBrush) {
		m_bIsBrushRef = true;
		m_pBrush = pBrush;
	}

	D2D1_RECT_F GetDrawRect() { return D2D1::RectF(m_objPos.x, m_objPos.y, m_objPos.x + m_renderSize.x, m_objPos.y + m_renderSize.y); }
	Vector2f GetDrawRectMiddle() { return Vector2f(m_objPos.x + m_renderSize.x / 2, m_objPos.y + m_renderSize.y / 2); }

	ObjectType GetObjectType() { return m_ObjType; }

	virtual void Save(std::ofstream& stream);
	virtual void Load(std::ifstream& stream);
protected:
	Object() { 
		m_pDWriteFactory = GetDWriteFactory();

		static int _id = 0; 
		id = _id++; 
	}

	ObjectType m_ObjType = Object_None;
	bool m_bObjectActive = true;
	Vector2f m_objPos = Vector2f::ZERO;
	Vector2i m_renderSize = Vector2i::ZERO;
	float m_rotation = 0;
	float m_objColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	std::string m_name = "Object";

	bool m_bIsBrushRef = false;
	ID2D1SolidColorBrush* m_pBrush = nullptr;
	IDWriteFactory* m_pDWriteFactory = nullptr; // ref
private:
	int id;
};