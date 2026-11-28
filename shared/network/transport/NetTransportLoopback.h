//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: In-process message transport: two transports joined in memory
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "NetTransport.h"

class CNetLoopbackLink;

// Both transports of a pair are used from one thread. Connect of one side connects to the other
// side if it listens; the address is ignored.
class CNetTransportLoopback : public INetTransport
{
	friend class CNetLoopbackLink;
public:
	static void		CreatePair(CRefPtr<CNetTransportLoopback>& first, CRefPtr<CNetTransportLoopback>& second);

	~CNetTransportLoopback();

	int				GetCaps() const override { return NET_TRANSPORT_LANE_PRIORITY; }
	int				GetMaxMessageSize() const override { return m_maxMessageSize; }

	bool			Listen(const char* address) override;
	NetConnectionId	Connect(const char* address) override;
	void			Close(NetConnectionId conn, int reason = 0) override;

	bool			ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes) override;
	bool			Send(NetConnectionId conn, int lane, const void* data, int size, int flags) override;
	int				GetQueuedBytes(NetConnectionId conn, int lane) const override { return 0; }

	void			Poll(NetTransportEventList& events) override;

	void			SetMaxMessageSize(int size) { m_maxMessageSize = size; }

private:
	CNetTransportLoopback(CNetLoopbackLink* link, int side);

	CRefPtr<CNetLoopbackLink>	m_link;
	int				m_side{ 0 };
	int				m_maxMessageSize{ 1024 * 1024 };
};

using CNetTransportLoopbackPtr = CRefPtr<CNetTransportLoopback>;
