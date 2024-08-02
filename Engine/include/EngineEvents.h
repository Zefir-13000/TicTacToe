#pragma once
#include <stdafx.h>

enum EngineEvent {
	EngineEvent_OnClick = 0,
	EngineEvent_MouseMove = 1,
};

class EngineAction {
public:
	EngineAction(std::string name);

	int GetID() const { return id; }

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	void Call();

	void Save(std::ofstream& stream);
	void Load(std::ifstream& stream);
private:
	std::string m_name;
	int id;
};