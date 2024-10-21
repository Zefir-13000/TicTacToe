#pragma once

enum EClientConnectionState {
	k_EClientNotConnected,							// Initial state, not connected to a server
	k_EClientConnectedPendingAuthentication,		// We've established communication with the server, but it hasn't authed us yet
	k_EClientConnectedAndAuthenticated,				// Final phase, server has authed us, we are actually able to play on it
};

enum class EClientGameState {
	ClientGameMenu,
	ClientGameStartServer,
	ClientGameWaitingForPlayers,
	ClientGameActive,
	ClientGameExiting,
	ClientGameConnecting,
	ClientGameConnectionFailure,
	ClientCreatingLobby,
	ClientJoiningLobby,
	ClientInLobby,
	ClientConnectingToSteam,
	ClientRetrySteamConnection,
	ClientOverlayAPI
};

enum class EServerGameState {
	ServerWaitingForPlayers,
	ServerActive,
	ServerExiting
};