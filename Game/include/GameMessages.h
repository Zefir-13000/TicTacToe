#pragma once
#include <stdafx.h>
#include <GamePlayer.h>

enum class EDisconnectReason {
	ClientDisconnect = k_ESteamNetConnectionEnd_App_Min + 1,
	ServerClosed = k_ESteamNetConnectionEnd_App_Min + 2,
	ServerReject = k_ESteamNetConnectionEnd_App_Min + 3,
	ServerFull = k_ESteamNetConnectionEnd_App_Min + 4,
	ClientKicked = k_ESteamNetConnectionEnd_App_Min + 5
};

enum EMessage {
	k_EMsgServerBegin = 0,
	k_EMsgServerSendInfo = k_EMsgServerBegin + 1,
	k_EMsgServerFailAuthentication = k_EMsgServerBegin + 2,
	k_EMsgServerPassAuthentication = k_EMsgServerBegin + 3,
	k_EMsgServerExiting = k_EMsgServerBegin + 4,
	k_EMsgServerUpdateWorld = k_EMsgServerBegin + 5,

	k_EMsgClientBegin = 500,
	k_EMsgClientBeginAuthentication = k_EMsgClientBegin + 2,
	k_EMsgClientSendLocalUpdate = k_EMsgClientBegin + 3,

	k_EMsgP2PBegin = 600,
	k_EMsgP2PSendingTicket = k_EMsgP2PBegin + 1,

	k_EForceDWORD = 0x7fffffff,
};

// Sent on new connection
struct MsgServerSendInfo_t
{
	MsgServerSendInfo_t() : m_MessageType(k_EMsgServerSendInfo) {}
	DWORD GetMessageType() { return m_MessageType; }

	void SetSteamIDServer(uint64 SteamID) { m_SteamIDServer = SteamID; }
	uint64 GetSteamIDServer() { return m_SteamIDServer; }

	void SetSecure(bool bSecure) { m_bIsVACSecure = bSecure; }
	bool GetSecure() { return m_bIsVACSecure; }

	void SetServerName(const char* pchName) { strncpy_s(m_ServerName, pchName, sizeof(m_ServerName)); }
	const char* GetServerName() { return m_ServerName; }

private:
	const DWORD m_MessageType;
	uint64 m_SteamIDServer;
	bool m_bIsVACSecure;
	char m_ServerName[128];
};

struct MsgServerFailAuthentication_t
{
	MsgServerFailAuthentication_t() : m_MessageType(k_EMsgServerFailAuthentication) {}
	DWORD GetMessageType() { return m_MessageType; }
private:
	const DWORD m_MessageType;
};

// Msg from the server to client when accepting a pending connection
struct MsgServerPassAuthentication_t
{
	MsgServerPassAuthentication_t() : m_MessageType(k_EMsgServerPassAuthentication) {}
	DWORD GetMessageType() { return m_MessageType; }

	void SetPlayerPosition(uint32 pos) { m_PlayerPosition = pos; }
	uint32 GetPlayerPosition() { return m_PlayerPosition; }

private:
	const DWORD m_MessageType;
	uint32 m_PlayerPosition;
};

struct MsgClientBeginAuthentication_t
{
	MsgClientBeginAuthentication_t() : m_MessageType(k_EMsgClientBeginAuthentication) {}
	DWORD GetMessageType() { return m_MessageType; }

	void SetToken(const char* pchToken, uint32 unLen) { m_uTokenLen = unLen; memcpy(m_rgchToken, pchToken, MIN(unLen, sizeof(m_rgchToken))); }
	uint32 GetTokenLen() { return m_uTokenLen; }
	const char* GetTokenPtr() { return m_rgchToken; }

	void SetSteamID(uint64 ulSteamID) { m_ulSteamID = ulSteamID; }
	uint64 GetSteamID() { return m_ulSteamID; }

private:
	const DWORD m_MessageType;

	uint32 m_uTokenLen;
#ifdef USE_GS_AUTH_API
	char m_rgchToken[1024];
#endif
	uint64 m_ulSteamID;
};

struct MsgServerExiting_t
{
	MsgServerExiting_t() : m_MessageType(k_EMsgServerExiting) {}
	DWORD GetMessageType() { return m_MessageType; }

private:
	const DWORD m_MessageType;
};

struct MsgP2PSendingTicket_t
{
	MsgP2PSendingTicket_t() : m_MessageType(k_EMsgP2PSendingTicket) {}
	DWORD GetMessageType() { return m_MessageType; }

	void SetToken(const void* pToken, uint32 unLen) { m_uTokenLen = unLen; memcpy(m_rgchToken, pToken, MIN(unLen, sizeof(m_rgchToken))); }
	uint32 GetTokenLen() const { return m_uTokenLen; }
	const char* GetTokenPtr() const { return m_rgchToken; }

	// Sender or receiver (depending on context)
	void SetSteamID(uint64 ulSteamID) { m_ulSteamID = ulSteamID; }
	uint64 GetSteamID() const { return m_ulSteamID; }

private:
	DWORD m_MessageType;
	uint32 m_uTokenLen;
	char m_rgchToken[1024];
	uint64 m_ulSteamID;
};

struct MsgClientSendLocalUpdate_t
{
	MsgClientSendLocalUpdate_t() : m_MessageType(k_EMsgClientSendLocalUpdate) {}
	DWORD GetMessageType() { return m_MessageType; }

	ClientGameUpdateData_t* AccessUpdateData() { return &m_ClientUpdateData; }

private:
	const DWORD m_MessageType;

	ClientGameUpdateData_t m_ClientUpdateData;
};

// Msg from the server to clients when updating the world state
struct MsgServerUpdateWorld_t
{
	MsgServerUpdateWorld_t() : m_MessageType(k_EMsgServerUpdateWorld) {}
	DWORD GetMessageType() { return m_MessageType; }

	ServerGameUpdateData_t* AccessUpdateData() { return &m_ServerUpdateData; }

private:
	const DWORD m_MessageType;
	ServerGameUpdateData_t m_ServerUpdateData;
};