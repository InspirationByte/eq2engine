//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: In-process message transport: two transports joined in memory
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "NetTransportLoopback.h"

// the state shared by the two transports of a pair; events wait here until the side polls
class CNetLoopbackLink : public RefCountedObject<CNetLoopbackLink>
{
public:
	struct Connection
	{
		int		laneCount[2]{ 1, 1 };
	};

	void	PushEvent(int side, ENetTransportEventType type, NetConnectionId conn, int reason = 0)
	{
		NetTransportEvent& event = pending[side].append();
		event.type = type;
		event.conn = conn;
		event.reason = reason;
	}

	void	DropEvents(int side, NetConnectionId conn)
	{
		for (int i = pending[side].numElem() - 1; i >= 0; --i)
		{
			if (pending[side][i].conn == conn)
				pending[side].removeIndex(i);
		}
	}

	NetTransportEventList				pending[2]{ PP_SL, PP_SL };
	Map<NetConnectionId, Connection>	connections{ PP_SL };
	NetConnectionId						idCounter{ NET_CONNECTION_NONE };
	bool								listening[2]{ false, false };
	bool								alive[2]{ true, true };
};

//-------------------------------------------------------------------------------------------------

void CNetTransportLoopback::CreatePair(CRefPtr<CNetTransportLoopback>& first, CRefPtr<CNetTransportLoopback>& second)
{
	CNetLoopbackLink* link = PPNew CNetLoopbackLink();
	first = CRefPtr<CNetTransportLoopback>(PPNew CNetTransportLoopback(link, 0));
	second = CRefPtr<CNetTransportLoopback>(PPNew CNetTransportLoopback(link, 1));
}

CNetTransportLoopback::CNetTransportLoopback(CNetLoopbackLink* link, int side)
	: m_link(link)
	, m_side(side)
{
}

CNetTransportLoopback::~CNetTransportLoopback()
{
	const int otherSide = 1 - m_side;
	for (auto it = m_link->connections.begin(); !it.atEnd(); ++it)
		m_link->PushEvent(otherSide, NET_EVENT_DISCONNECTED, it.key(), NET_DISCONNECT_REMOTE);

	m_link->connections.clear();
	m_link->pending[m_side].clear();
	m_link->alive[m_side] = false;
	m_link->listening[m_side] = false;
}

bool CNetTransportLoopback::Listen(const char* address)
{
	m_link->listening[m_side] = true;
	return true;
}

NetConnectionId CNetTransportLoopback::Connect(const char* address)
{
	const int otherSide = 1 - m_side;
	const NetConnectionId conn = ++m_link->idCounter;

	if (!m_link->alive[otherSide] || !m_link->listening[otherSide])
	{
		m_link->PushEvent(m_side, NET_EVENT_DISCONNECTED, conn, NET_DISCONNECT_CONNECT_FAILED);
		return conn;
	}

	m_link->connections.insert(conn, CNetLoopbackLink::Connection());
	m_link->PushEvent(m_side, NET_EVENT_CONNECTED, conn);
	m_link->PushEvent(otherSide, NET_EVENT_CONNECTED, conn);
	return conn;
}

void CNetTransportLoopback::Close(NetConnectionId conn, int reason)
{
	auto it = m_link->connections.find(conn);
	if (it.atEnd())
		return;

	// messages already sent to the other side are still delivered, like the queued data of TCP
	m_link->connections.remove(it);
	m_link->DropEvents(m_side, conn);
	m_link->PushEvent(1 - m_side, NET_EVENT_DISCONNECTED, conn, NET_DISCONNECT_REMOTE);
}

bool CNetTransportLoopback::ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes)
{
	auto it = m_link->connections.find(conn);
	if (it.atEnd() || lanes.numElem() == 0)
		return false;

	(*it).laneCount[m_side] = lanes.numElem();
	return true;
}

bool CNetTransportLoopback::Send(NetConnectionId conn, int lane, const void* data, int size, int flags)
{
	auto it = m_link->connections.find(conn);
	if (it.atEnd())
		return false;

	if (lane < 0 || lane >= (*it).laneCount[m_side] || size < 0 || size > m_maxMessageSize)
		return false;

	CRefPtr<CMemoryStream> message = CRefPtr_new(CMemoryStream);
	message->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, size);
	if (size)
		message->Write(data, 1, size);
	message->Seek(0, FS_SEEK_SET);

	NetTransportEvent& event = m_link->pending[1 - m_side].append();
	event.type = NET_EVENT_MESSAGE;
	event.conn = conn;
	event.lane = lane;
	event.message = std::move(message);
	return true;
}

void CNetTransportLoopback::Poll(NetTransportEventList& events)
{
	NetTransportEventList& pending = m_link->pending[m_side];
	for (NetTransportEvent& event : pending)
		events.append(std::move(event));
	pending.clear();
}
