#include <EngineEvents.h>
#include <Scene.h>

EngineAction::EngineAction(std::string name) {
	static int _action_id = 0;
	id = _action_id++;

	m_name = name;
}

void EngineAction::Save(std::ofstream& stream) {
	size_t name_size = m_name.size();
	stream.write((const char*)&name_size, sizeof(size_t));
	stream.write(&m_name[0], name_size);
}
void EngineAction::Load(std::ifstream& stream) {
	size_t name_size = 0;
	stream.read((char*)&name_size, sizeof(size_t));
	m_name.resize(name_size);
	stream.read(&m_name[0], name_size);
}

void EngineAction::Call() {
	std::function<void()> callback = GetScene()->FindActionCallback(this);
	if (callback) {
		callback();
	}
}