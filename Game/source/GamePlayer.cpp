#include <GamePlayer.h>
#include <GameEngine.h>

GamePlayer::GamePlayer(GameEngine* pEngine) : m_pGameEngine(pEngine) {

}

GamePlayer::~GamePlayer() {

}

void GamePlayer::SetIsLocalPlayer(bool isLocal) {
	m_isLocalPlayer = isLocal;
}

bool GamePlayer::GetIsLocalPlayer() const {
	return m_isLocalPlayer;
}

void GamePlayer::OnReceiveClientUpdate(ClientGameUpdateData_t* pUpdateData) {
	memcpy(&m_ClientGameUpdateData, pUpdateData, sizeof(m_ClientGameUpdateData));
	pUpdateData->GetMove(pos_x, pos_y);
}

void GamePlayer::BuildServerUpdate(ServerPlayerUpdateData_t* pUpdateData) {
	pUpdateData->SetMove(this->pos_x, this->pos_y);
}

void GamePlayer::OnReceiveServerUpdate(ServerPlayerUpdateData_t* pUpdateData) {
	pUpdateData->GetMove(pos_x, pos_y);
}

bool GamePlayer::GetClientUpdateData(ClientGameUpdateData_t* pUpdateData) {
	if (m_pGameEngine->GetGameTickCount() - m_ulLastClientUpdateTick < 1000.0f / CLIENT_UPDATE_SEND_RATE)
		return false;

	m_ulLastClientUpdateTick = m_pGameEngine->GetGameTickCount();

	if (m_isLocalPlayer)
	{
		m_ClientGameUpdateData.SetPlayerName(SteamFriends()->GetPersonaName());
		m_ClientGameUpdateData.SetMove(pos_x, pos_y);
	}

	memcpy(pUpdateData, &m_ClientGameUpdateData, sizeof(m_ClientGameUpdateData));
	memset(&m_ClientGameUpdateData, 0, sizeof(m_ClientGameUpdateData));

	return true;
}

// --------- SERVER-SIDE -----------
const char* GamePlayer::GetPlayerName() {
	return m_ClientGameUpdateData.GetPlayerName();
}