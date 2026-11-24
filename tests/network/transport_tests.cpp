#include <gtest/gtest.h>

#include "core/core_common.h"
#include "network/transport/NetTransportLoopback.h"
#include "network/transport/NetTransportTCP.h"

static constexpr int TRANSPORT_TEST_TIMEOUT_MS = 5000;

// polls the transports until the condition holds or the time is out
template<typename CONDITION>
static bool PollTransportsUntil(ArrayCRef<INetTransport*> transports, NetTransportEventList& events, CONDITION condition)
{
	for (int elapsed = 0; elapsed < TRANSPORT_TEST_TIMEOUT_MS; ++elapsed)
	{
		for (INetTransport* transport : transports)
			transport->Poll(events);

		if (condition())
			return true;

		Platform_Sleep(1);
	}
	return false;
}

static const NetTransportEvent* FindTransportEvent(const NetTransportEventList& events, ENetTransportEventType type, NetConnectionId conn = NET_CONNECTION_NONE)
{
	for (const NetTransportEvent& event : events)
	{
		if (event.type == type && (conn == NET_CONNECTION_NONE || event.conn == conn))
			return &event;
	}
	return nullptr;
}

static int CountTransportEvents(const NetTransportEventList& events, ENetTransportEventType type)
{
	int count = 0;
	for (const NetTransportEvent& event : events)
		count += event.type == type ? 1 : 0;
	return count;
}

//-------------------------------------------------------------------------------------------------

TEST(NET_TRANSPORT_TESTS, LoopbackMessages)
{
	CNetTransportLoopbackPtr server;
	CNetTransportLoopbackPtr client;
	CNetTransportLoopback::CreatePair(server, client);

	ASSERT_TRUE(server->Listen(""));
	const NetConnectionId clientConn = client->Connect("");
	ASSERT_NE(clientConn, NET_CONNECTION_NONE);

	NetTransportEventList serverEvents{ PP_SL };
	NetTransportEventList clientEvents{ PP_SL };
	server->Poll(serverEvents);
	client->Poll(clientEvents);

	const NetTransportEvent* accepted = FindTransportEvent(serverEvents, NET_EVENT_CONNECTED);
	ASSERT_NE(accepted, nullptr);
	ASSERT_NE(FindTransportEvent(clientEvents, NET_EVENT_CONNECTED, clientConn), nullptr);
	const NetConnectionId serverConn = accepted->conn;

	const NetLaneConfig lanes[] = { { 0, 1 }, { 1, 1 } };
	EXPECT_TRUE(client->ConfigureLanes(clientConn, lanes));

	const char text[] = "hello";
	EXPECT_TRUE(client->Send(clientConn, 1, text, sizeof(text), NET_SEND_RELIABLE));
	EXPECT_FALSE(client->Send(clientConn, 2, text, sizeof(text), NET_SEND_RELIABLE));

	serverEvents.clear();
	server->Poll(serverEvents);

	const NetTransportEvent* message = FindTransportEvent(serverEvents, NET_EVENT_MESSAGE);
	ASSERT_NE(message, nullptr);
	EXPECT_EQ(message->lane, 1);
	ASSERT_EQ(message->message->GetSize(), static_cast<VSSize>(sizeof(text)));
	EXPECT_STREQ(reinterpret_cast<const char*>(message->message->GetBasePointer()), text);

	// a local close gives no event, the peer gets a disconnect
	server->Close(serverConn);
	serverEvents.clear();
	clientEvents.clear();
	server->Poll(serverEvents);
	client->Poll(clientEvents);

	EXPECT_EQ(serverEvents.numElem(), 0);
	const NetTransportEvent* disconnected = FindTransportEvent(clientEvents, NET_EVENT_DISCONNECTED, clientConn);
	ASSERT_NE(disconnected, nullptr);
	EXPECT_EQ(disconnected->reason, NET_DISCONNECT_REMOTE);
	EXPECT_FALSE(client->Send(clientConn, 0, text, sizeof(text), NET_SEND_RELIABLE));
}

TEST(NET_TRANSPORT_TESTS, LoopbackConnectFailed)
{
	CNetTransportLoopbackPtr first;
	CNetTransportLoopbackPtr second;
	CNetTransportLoopback::CreatePair(first, second);

	// the other side does not listen
	const NetConnectionId conn = first->Connect("");

	NetTransportEventList events{ PP_SL };
	first->Poll(events);

	const NetTransportEvent* disconnected = FindTransportEvent(events, NET_EVENT_DISCONNECTED, conn);
	ASSERT_NE(disconnected, nullptr);
	EXPECT_EQ(disconnected->reason, NET_DISCONNECT_CONNECT_FAILED);
}

TEST(NET_TRANSPORT_TESTS, LoopbackPeerDestroyed)
{
	CNetTransportLoopbackPtr server;
	CNetTransportLoopbackPtr client;
	CNetTransportLoopback::CreatePair(server, client);

	server->Listen("");
	const NetConnectionId conn = client->Connect("");
	server = nullptr;

	NetTransportEventList events{ PP_SL };
	client->Poll(events);

	EXPECT_NE(FindTransportEvent(events, NET_EVENT_DISCONNECTED, conn), nullptr);
}

//-------------------------------------------------------------------------------------------------

TEST(NET_TRANSPORT_TESTS, TCPMessages)
{
	CRefPtr<CNetTransportTCP> server = CRefPtr_new(CNetTransportTCP);
	CRefPtr<CNetTransportTCP> client = CRefPtr_new(CNetTransportTCP);
	INetTransport* transports[] = { server.Ptr(), client.Ptr() };

	ASSERT_TRUE(server->Listen("127.0.0.1:0"));
	const int port = server->GetListenPort();
	ASSERT_NE(port, 0);

	const NetConnectionId clientConn = client->Connect(EqString::Format("127.0.0.1:%d", port));
	ASSERT_NE(clientConn, NET_CONNECTION_NONE);

	const NetLaneConfig lanes[] = { { 0, 1 }, { 1, 1 } };
	EXPECT_TRUE(client->ConfigureLanes(clientConn, lanes));

	// messages sent before the connection is established are queued
	Array<ubyte> largeMessage(PP_SL);
	largeMessage.setNum(700 * 1024);
	for (int i = 0; i < largeMessage.numElem(); ++i)
		largeMessage[i] = static_cast<ubyte>(i * 7);

	const char text[] = "small";
	EXPECT_TRUE(client->Send(clientConn, 1, largeMessage.ptr(), largeMessage.numElem(), NET_SEND_RELIABLE));
	EXPECT_TRUE(client->Send(clientConn, 0, text, sizeof(text), NET_SEND_RELIABLE));
	EXPECT_TRUE(client->Send(clientConn, 1, nullptr, 0, NET_SEND_RELIABLE));
	EXPECT_GT(client->GetQueuedBytes(clientConn, 1), largeMessage.numElem());

	// a lane that is not configured is refused
	EXPECT_FALSE(client->Send(clientConn, 2, text, sizeof(text), NET_SEND_RELIABLE));

	NetTransportEventList events{ PP_SL };
	ASSERT_TRUE(PollTransportsUntil(transports, events, [&]() { return CountTransportEvents(events, NET_EVENT_MESSAGE) == 3; }));

	ASSERT_NE(FindTransportEvent(events, NET_EVENT_CONNECTED, clientConn), nullptr);

	// the higher priority lane went first
	Array<const NetTransportEvent*> messages(PP_SL);
	for (const NetTransportEvent& event : events)
	{
		if (event.type == NET_EVENT_MESSAGE)
			messages.append(&event);
	}

	EXPECT_EQ(messages[0]->lane, 0);
	EXPECT_STREQ(reinterpret_cast<const char*>(messages[0]->message->GetBasePointer()), text);

	EXPECT_EQ(messages[1]->lane, 1);
	ASSERT_EQ(messages[1]->message->GetSize(), static_cast<VSSize>(largeMessage.numElem()));
	EXPECT_EQ(memcmp(messages[1]->message->GetBasePointer(), largeMessage.ptr(), largeMessage.numElem()), 0);

	EXPECT_EQ(messages[2]->lane, 1);
	EXPECT_EQ(messages[2]->message->GetSize(), 0);

	EXPECT_EQ(client->GetQueuedBytes(clientConn, 1), 0);

	// the server replies on its connection, then closes after sending it
	const NetConnectionId serverConn = messages[0]->conn;
	EXPECT_TRUE(server->Send(serverConn, 0, text, sizeof(text), NET_SEND_RELIABLE));
	server->Close(serverConn);

	events.clear();
	ASSERT_TRUE(PollTransportsUntil(transports, events, [&]() { return FindTransportEvent(events, NET_EVENT_DISCONNECTED) != nullptr; }));

	const NetTransportEvent* reply = FindTransportEvent(events, NET_EVENT_MESSAGE, clientConn);
	ASSERT_NE(reply, nullptr);
	EXPECT_STREQ(reinterpret_cast<const char*>(reply->message->GetBasePointer()), text);

	const NetTransportEvent* disconnected = FindTransportEvent(events, NET_EVENT_DISCONNECTED, clientConn);
	ASSERT_NE(disconnected, nullptr);
	EXPECT_EQ(disconnected->reason, NET_DISCONNECT_REMOTE);
}

TEST(NET_TRANSPORT_TESTS, TCPConnectFailed)
{
	// take a free port and close it
	int port = 0;
	{
		CRefPtr<CNetTransportTCP> server = CRefPtr_new(CNetTransportTCP);
		ASSERT_TRUE(server->Listen("127.0.0.1:0"));
		port = server->GetListenPort();
	}

	CRefPtr<CNetTransportTCP> client = CRefPtr_new(CNetTransportTCP);
	INetTransport* transports[] = { client.Ptr() };

	const NetConnectionId conn = client->Connect(EqString::Format("127.0.0.1:%d", port));
	ASSERT_NE(conn, NET_CONNECTION_NONE);

	NetTransportEventList events{ PP_SL };
	ASSERT_TRUE(PollTransportsUntil(transports, events, [&]() { return FindTransportEvent(events, NET_EVENT_DISCONNECTED, conn) != nullptr; }));
	EXPECT_EQ(FindTransportEvent(events, NET_EVENT_DISCONNECTED, conn)->reason, NET_DISCONNECT_CONNECT_FAILED);
	EXPECT_EQ(FindTransportEvent(events, NET_EVENT_CONNECTED), nullptr);
}

TEST(NET_TRANSPORT_TESTS, TCPMessageTooLarge)
{
	// the receiver closes a connection whose frame is over its limit
	CRefPtr<CNetTransportTCP> server = CRefPtr_new(CNetTransportTCP, 1024);
	CRefPtr<CNetTransportTCP> client = CRefPtr_new(CNetTransportTCP);
	INetTransport* transports[] = { server.Ptr(), client.Ptr() };

	ASSERT_TRUE(server->Listen("127.0.0.1:0"));
	const NetConnectionId conn = client->Connect(EqString::Format("127.0.0.1:%d", server->GetListenPort()));

	Array<ubyte> message(PP_SL);
	message.setNum(4096);
	memset(message.ptr(), 0, message.numElem());
	EXPECT_TRUE(client->Send(conn, 0, message.ptr(), message.numElem(), NET_SEND_RELIABLE));

	NetTransportEventList events{ PP_SL };
	ASSERT_TRUE(PollTransportsUntil(transports, events, [&]() { return CountTransportEvents(events, NET_EVENT_DISCONNECTED) == 2; }));

	// the server side closes with the protocol reason, the client sees the remote close
	bool protocolClose = false;
	for (const NetTransportEvent& event : events)
		protocolClose = protocolClose || (event.type == NET_EVENT_DISCONNECTED && event.reason == NET_DISCONNECT_PROTOCOL);
	EXPECT_TRUE(protocolClose);
	EXPECT_EQ(CountTransportEvents(events, NET_EVENT_MESSAGE), 0);
}
