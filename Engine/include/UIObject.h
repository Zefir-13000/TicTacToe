#pragma once
#include <Object.h>

class UIObject : public Object {
public:
	virtual ~UIObject() = default;
	virtual void OnClick() = 0;
	virtual void OnHover() = 0;

protected:
	UIObject() = default;
};