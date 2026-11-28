//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK server: the RPC link of an engine application with Blender
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "network/rpc/RPCEndpoint.h"

static constexpr int BLINK_DEFAULT_PORT = 27100;

enum EBlinkServerState : int
{
	BLINK_SERVER_STOPPED = 0,
	BLINK_SERVER_FAILED,		// the port could not be listened on
	BLINK_SERVER_LISTENING,
	BLINK_SERVER_CONNECTED,
};

// Listens on 127.0.0.1:<port> for one Blender client (other connections are closed) and answers
// blink.Hello; the application adds its own protocols. See docs/blink_overview.md.
class CBlinkServer
{
public:
	using ClientCb = EqFunction<void(bool connected)>;

	// the name and the version are sent to the client in the blink.Hello reply
	CBlinkServer(const char* appName, const char* appVersion);
	~CBlinkServer();

	// before Start; the protocols must live while the server runs
	void				AddProtocol(RPCProtocol& protocol);

	// the client connected or disconnected; from Update
	void				SetClientCallback(ClientCb callback) { m_clientCb = callback; }

	bool				Start(int port);
	void				Stop();

	// receives, runs the methods; from the main thread
	void				Update();

	EBlinkServerState	GetState() const;
	int					GetPort() const { return m_port; }
	NetConnectionId		GetClient() const { return m_client; }

	// of blink.Hello: the client (addon) and its host (Blender) names and versions
	const char*			GetClientName() const { return m_clientName; }
	const char*			GetClientVersion() const { return m_clientVersion; }
	const char*			GetHostName() const { return m_hostName; }
	const char*			GetHostVersion() const { return m_hostVersion; }

private:
	void				OnConnection(NetConnectionId conn, bool connected);

	RPCProtocol				m_blink{ "blink", 1 };
	Array<RPCProtocol*>		m_protocols{ PP_SL };
	RPCEndpoint*			m_endpoint{ nullptr };
	ClientCb				m_clientCb;

	EqString				m_appName;
	EqString				m_appVersion;
	EqString				m_clientName;
	EqString				m_clientVersion;
	EqString				m_hostName;
	EqString				m_hostVersion;

	NetConnectionId			m_client{ NET_CONNECTION_NONE };
	Array<NetConnectionId>	m_refusedConnections{ PP_SL };	// closed after the endpoint update
	int						m_port{ 0 };
	bool					m_failed{ false };
};
