#include <ButtonObject.h>
#include <TextObject.h>
#include <GameEngine.h>
#include <Scene.h>

ButtonObject::ButtonObject() {
	m_ObjType = Object_ButtonType;
	m_name = "ButtonObject";

	m_buttonText = new TextObject();
	m_buttonText->SetTextAlign(DWRITE_TEXT_ALIGNMENT_CENTER);
}

ButtonObject::~ButtonObject() {
	if (m_buttonText)
		delete m_buttonText;
}

EngineAction* ButtonObject::GetCallback() const {
	return m_callback;
}

void ButtonObject::SetCallback(EngineAction* callback) {
	m_callback = callback;
}

void ButtonObject::UpdateChild() {
	m_buttonText->SetPosition(m_objPos);
	m_buttonText->SetRotation(m_rotation);
	m_buttonText->SetSize(m_renderSize);
}

bool ButtonObject::CheckClicked() {
	HWND hWnd = GetHWND();

	if (!hWnd) {
		return false;
	}

	POINT mouse_pos;
	if (GetCursorPos(&mouse_pos) && ScreenToClient(hWnd, &mouse_pos)) {
		// Check in rect
		if (mouse_pos.x > m_objPos.x && mouse_pos.x < m_objPos.x + m_renderSize.x &&
			mouse_pos.y > m_objPos.y && mouse_pos.y < m_objPos.y + m_renderSize.y) {
			return true;
		}
	}

	return false;
}

void ButtonObject::Render() {
	ID2D1RenderTarget* pD2DRenderTarget = GetRenderTarget();

	D2D1_ROUNDED_RECT rect;
	rect.rect.left = m_objPos.x;
	rect.rect.top = m_objPos.y;
	rect.rect.right = m_objPos.x + m_renderSize.x;
	rect.rect.bottom = m_objPos.y + m_renderSize.y;

	rect.radiusX = m_radius.x;
	rect.radiusY = m_radius.y;

	if (m_rotation != 0) {
		Vector2f middle_pos = GetDrawRectMiddle();
		pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Rotation(m_rotation, D2D1::Point2F(middle_pos.x, middle_pos.y)));
	}
	
	if (m_bFocused) {
		pD2DRenderTarget->FillRoundedRectangle(rect, m_pBrush);
	}
	else {
		pD2DRenderTarget->DrawRoundedRectangle(rect, m_pBrush, 3.f);
	}
	pD2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	m_buttonText->Render();
}

void ButtonObject::OnClick() {
	if (m_callback && CheckClicked()) {
		m_callback->Call();
	}
}

void ButtonObject::OnHover() {
	if (CheckClicked()) {
		Focus();
	}
	else {
		UnFocus();
	}
}

void ButtonObject::Focus() {
	if (!m_bFocused && m_bClickable) {
		m_bFocused = true;

		float* text_color = m_buttonText->GetColor();
		text_color[0] = 1;
		text_color[1] = 1;
		text_color[2] = 1;
		text_color[3] = 1;

		D2D1::ColorF new_color(text_color[0], text_color[1], text_color[2], text_color[3]);
		m_buttonText->SetColor(new_color);
	}
}

void ButtonObject::UnFocus() {
	if (m_bFocused && m_bClickable) {
		m_bFocused = false;

		float* text_color = m_buttonText->GetColor();
		text_color[0] = 0;
		text_color[1] = 0;
		text_color[2] = 0;
		text_color[3] = 1;

		D2D1::ColorF new_color(text_color[0], text_color[1], text_color[2], text_color[3]);
		m_buttonText->SetColor(new_color);
	}
}

bool ButtonObject::GetClickable() const {
	return m_bClickable;
}

void ButtonObject::SetClickable(bool bClickable) {
	m_bClickable = bClickable;
	if (bClickable) {
		UnFocus();
		float* text_color = m_buttonText->GetColor();
		text_color[0] = 0;
		text_color[1] = 0;
		text_color[2] = 0;
		text_color[3] = 1;

		D2D1::ColorF new_color(text_color[0], text_color[1], text_color[2], text_color[3]);
		m_buttonText->SetColor(new_color);
		ButtonObject::SetColor(new_color);
	}
	else {
		UnFocus();

		float* text_color = m_buttonText->GetColor();
		text_color[0] = 0;
		text_color[1] = 0;
		text_color[2] = 0;
		text_color[3] = 0.2;

		D2D1::ColorF new_color(text_color[0], text_color[1], text_color[2], text_color[3]);
		m_buttonText->SetColor(new_color);
		ButtonObject::SetColor(new_color);
	}
}

TextObject* ButtonObject::GetTextObject() const {
	return m_buttonText;
}

Vector2f ButtonObject::GetRadius() const {
	return m_radius;
}

void ButtonObject::SetRadius(Vector2f radius) {
	m_radius = radius;
}

void ButtonObject::Save(std::ofstream& stream) {
	Object::Save(stream);

	stream.write((const char*)&m_radius, sizeof(Vector2f));
	stream.write((const char*)&m_bClickable, sizeof(bool));

	// Action
	std::string action_name = m_callback->GetName();
	size_t name_size = action_name.size();
	stream.write((const char*)&name_size, sizeof(size_t));
	stream.write(&action_name[0], name_size);

	m_buttonText->Save(stream);
}

void ButtonObject::Load(std::ifstream& stream) {
	Object::Load(stream);

	stream.read((char*)&m_radius, sizeof(Vector2f));
	stream.read((char*)&m_bClickable, sizeof(bool));

	size_t name_size = 0;
	std::string action_name = "";
	stream.read((char*)&name_size, sizeof(size_t));
	action_name.resize(name_size);
	stream.read(&action_name[0], name_size);

	std::vector<EngineAction*> scene_actions = GetScene()->GetSceneActions();
	for (EngineAction* action : scene_actions) {
		if (action->GetName() == action_name) {
			SetCallback(action);
			break;
		}
	}

	m_buttonText->Load(stream);
}