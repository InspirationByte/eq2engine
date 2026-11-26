//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK server: the RPC link of an engine application with Blender
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "network/transport/NetTransportTCP.h"
#include "BlinkServer.h"

CBlinkServer::CBlinkServer(const char* appName, const char* appVersion)
	: m_appName(appName), m_appVersion(appVersion)
{
	m_blink.Method("Hello", [this](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		const char* client = "";
		const char* clientVersion = "";
		const char* host = "";
		const char* hostVersion = "";
		params.Get("client").GetValues(client);
		params.Get("clientVersion").GetValues(clientVersion);
		params.Get("host").GetValues(host);
		params.Get("hostVersion").GetValues(hostVersion);

		m_clientName = client;
		m_clientVersion = clientVersion;
		m_hostName = host;
		m_hostVersion = hostVersion;
		Msg("BLINK: %s %s, %s %s\n", client, clientVersion, host, hostVersion);

		KVSection result;
		result.SetKey("app", m_appName.ToCString());
		result.SetKey("appVersion", m_appVersion.ToCString());
		ctx.Reply(result);
	});
}

CBlinkServer::~CBlinkServer()
{
	m_clientCb = ClientCb();	// the owner may be destroyed already
	Stop();
}

void CBlinkServer::AddProtocol(RPCProtocol& protocol)
{
	ASSERT_MSG(!m_endpoint, "CBlinkServer::AddProtocol must be called before Start");
	m_protocols.append(&protocol);
}

bool CBlinkServer::Start(int port)
{
	Stop();

	m_port = port;

	CRefPtr<CNetTransportTCP> transport = CRefPtr_new(CNetTransportTCP);
	m_endpoint = PPNew RPCEndpoint(RPCEndpoint::SERVER, INetTransportPtr(transport.Ptr()));
	m_endpoint->AddProtocol(m_blink);
	for (RPCProtocol* protocol : m_protocols)
		m_endpoint->AddProtocol(*protocol);
	m_endpoint->SetConnectionCallback([this](NetConnectionId conn, bool connected) {
		OnConnection(conn, connected);
	});

	const EqString address = EqString::Format("127.0.0.1:%d", m_port);
	if (!m_endpoint->Listen(address))
	{
		MsgWarning("BLINK: can't listen on %s\n", address.ToCString());
		delete m_endpoint;
		m_endpoint = nullptr;
		m_failed = true;
		return false;
	}

	Msg("BLINK: listening on %s\n", address.ToCString());
	return true;
}

void CBlinkServer::Stop()
{
	const bool hadClient = m_client != NET_CONNECTION_NONE;
	m_client = NET_CONNECTION_NONE;
	m_refusedConnections.clear();
	m_failed = false;

	// the endpoint does not call the connection callback when it is destroyed
	delete m_endpoint;
	m_endpoint = nullptr;

	if (hadClient && m_clientCb)
		m_clientCb(false);
}

void CBlinkServer::Update()
{
	if (!m_endpoint)
		return;

	m_endpoint->Update();

	for (NetConnectionId conn : m_refusedConnections)
		m_endpoint->Disconnect(conn);
	m_refusedConnections.clear();
}

EBlinkServerState CBlinkServer::GetState() const
{
	if (m_failed)
		return BLINK_SERVER_FAILED;
	if (!m_endpoint)
		return BLINK_SERVER_STOPPED;
	if (m_client != NET_CONNECTION_NONE)
		return BLINK_SERVER_CONNECTED;
	return BLINK_SERVER_LISTENING;
}

void CBlinkServer::OnConnection(NetConnectionId conn, bool connected)
{
	if (connected)
	{
		if (m_client != NET_CONNECTION_NONE)
		{
			// the endpoint is in its Update, the connection is closed after it
			MsgWarning("BLINK: another client is already connected, connection refused\n");
			m_refusedConnections.append(conn);
			return;
		}

		m_client = conn;
		m_clientName.Clear();
		m_clientVersion.Clear();
		m_hostName.Clear();
		m_hostVersion.Clear();
		Msg("BLINK: client connected\n");
		if (m_clientCb)
			m_clientCb(true);
		return;
	}

	if (conn != m_client)
		return;

	m_client = NET_CONNECTION_NONE;
	Msg("BLINK: client disconnected\n");
	if (m_clientCb)
		m_clientCb(false);
}
