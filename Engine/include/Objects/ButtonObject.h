#pragma once
#include <UIObject.h>

class Scene;
class TextObject;
class ButtonObject : public UIObject {
public:
	ButtonObject();
	~ButtonObject();
	void OnClick() override;
	void OnHover() override;

	void Render() override;

	void Focus();
	void UnFocus();

	EngineAction* GetCallback() const;
	void SetCallback(EngineAction* callback);

	void UpdateChild();
	TextObject* GetTextObject() const;
	Vector2f GetRadius() const;
	void SetRadius(Vector2f radius);

	virtual void Save(std::ofstream& stream) override;
	virtual void Load(std::ifstream& stream) override;
private:
	bool CheckClicked();

	Vector2f m_radius = { 1.f, 1.f };

	TextObject* m_buttonText = nullptr;
	EngineAction* m_callback = nullptr;

	bool m_bFocused = false;
};