#pragma once
#include <stdafx.h>
#include <GameEngine.h>
#include <GameMessages.h>

class P2PAuthPlayer {
public:
	P2PAuthPlayer(GameEngine* pGameEngine, CSteamID steamID, HSteamNetConnection hServerConn);
	~P2PAuthPlayer();
	void EndGame();
	void StartAuthPlayer();
	bool BIsAuthOk();
	void HandleP2PSendingTicket(const MsgP2PSendingTicket_t* pMsg);

	CSteamID GetSteamID();

	STEAM_CALLBACK(P2PAuthPlayer, OnBeginAuthResponse, ValidateAuthTicketResponse_t, m_CallbackBeginAuthResponse);

	const CSteamID m_steamID;
	const HSteamNetConnection m_hServerConnection;
private:
	uint64 GetGameTimeInSeconds()
	{
		return m_pGameEngine->GetGameTickCount() / 1000;
	}

	bool m_bSentTicket;
	bool m_bSubmittedHisTicket;
	bool m_bHaveAnswer;

	uint64 m_ulConnectTime;
	uint64 m_ulTicketTime;
	uint64 m_ulAnswerTime;
	uint32 m_cubTicketIGaveThisUser;
	uint8 m_rgubTicketIGaveThisUser[1024];
	uint32 m_cubTicketHeGaveMe;
	uint8 m_rgubTicketHeGaveMe[1024];
	HAuthTicket m_hAuthTicketIGaveThisUser;
	EBeginAuthSessionResult m_eBeginAuthSessionResult;
	EAuthSessionResponse m_eAuthSessionResponse;

	GameEngine* m_pGameEngine;
};

class P2PAuthedGame {
public:
	P2PAuthedGame(GameEngine* pEngine);
	void PlayerDisconnect(int iSlot);
	void EndGame();
	void StartAuthPlayer(int iSlot, CSteamID steamID);
	void RegisterPlayer(int iSlot, CSteamID steamID);
	void HandleP2PSendingTicket(const void* pMessage);
	CSteamID GetSteamID();
	void InternalInitPlayer(int iSlot, CSteamID steamID, bool bStartAuthProcess);

	P2PAuthPlayer* m_P2PAuthPlayer[MAX_PLAYERS_PER_SERVER];
	GameEngine* m_pGameEngine;
	HSteamNetConnection m_hConnServer;
};