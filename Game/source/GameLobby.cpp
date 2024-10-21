#include <GameLobby.h>

GameLobby::GameLobby(GameEngine* pEngine) : m_pGameEngine(pEngine),
m_CallbackPersonaStateChange(this, &GameLobby::OnPersonaStateChange),
m_CallbackLobbyDataUpdate(this, &GameLobby::OnLobbyDataUpdate),
m_CallbackChatDataUpdate(this, &GameLobby::OnLobbyChatUpdate)
{

}

GameLobby::~GameLobby()
{

}

CSteamID GameLobby::GetLobbySteamID() const {
	return m_steamIDLobby;
}

void GameLobby::SetLobbySteamID(const CSteamID& steamIDLobby)
{
	m_steamIDLobby = steamIDLobby;
	m_bSetLobbyName = false;
}

CSteamID GameLobby::GetLobbyOwnerSteamID() const {
	return m_steamIDOwner;
}

void GameLobby::SetLobbyOwnerSteamID(const CSteamID& steamIDOwner) {
	m_steamIDOwner = steamIDOwner;
}

bool GameLobby::CheckIsLobbyOwner(const CSteamID& steamID) {
	return m_steamIDOwner == steamID;
}

bool GameLobby::GetIsSetLobbyName() const {
	return m_bSetLobbyName;
}

std::string GameLobby::GetLobbyName() const {
	return m_LobbyName;
}

void GameLobby::SetLobbyName(std::string lobbyName)
{
	if (m_bSetLobbyName)
		return;

	m_LobbyName = lobbyName;
	m_bSetLobbyName = true;
}

void GameLobby::Tick()
{
}

void GameLobby::OnPersonaStateChange(PersonaStateChange_t* pCallback)
{
	// callbacks are broadcast to all listeners, so we'll get this for every friend who changes state
	// so make sure the user is in the lobby before acting
	if (!SteamFriends()->IsUserInSource(pCallback->m_ulSteamID, m_steamIDLobby))
		return;
}

void GameLobby::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback)
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	if (m_steamIDLobby != pCallback->m_ulSteamIDLobby)
		return;

	// set the heading
	//m_pMenu->SetHeading(SteamMatchmaking()->GetLobbyData(m_steamIDLobby, "name"));
	OutputDebugString(SteamMatchmaking()->GetLobbyData(m_steamIDLobby, "name"));

	// rebuild the menu
	//m_pMenu->Rebuild(m_steamIDLobby);
}

void GameLobby::OnLobbyChatUpdate(LobbyChatUpdate_t* pCallback) {

}