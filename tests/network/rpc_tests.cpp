#include <gtest/gtest.h>

#include "core/core_common.h"
#include "network/transport/NetTransportLoopback.h"
#include "network/transport/NetTransportTCP.h"
#include "network/rpc/RPCEndpoint.h"

static constexpr int RPC_TEST_TIMEOUT_MS = 5000;

// a server and a client endpoint over a loopback pair
struct RPCTestLink
{
	RPCTestLink()
	{
		CNetTransportLoopbackPtr serverTransport;
		CNetTransportLoopbackPtr clientTransport;
		CNetTransportLoopback::CreatePair(serverTransport, clientTransport);
		server = PPNew RPCEndpoint(RPCEndpoint::SERVER, INetTransportPtr(serverTransport.Ptr()));
		client = PPNew RPCEndpoint(RPCEndpoint::CLIENT, INetTransportPtr(clientTransport.Ptr()));
	}

	~RPCTestLink()
	{
		delete client;
		delete server;
	}

	void Start()
	{
		server->Listen("");
		client->Connect("");
		Update();
	}

	void Update(int times = 1)
	{
		for (int i = 0; i < times; ++i)
		{
			server->Update();
			client->Update();
		}
	}

	template<typename CONDITION>
	bool UpdateUntil(CONDITION condition)
	{
		for (int i = 0; i < RPC_TEST_TIMEOUT_MS; ++i)
		{
			Update();
			if (condition())
				return true;
			Platform_Sleep(1);
		}
		return false;
	}

	RPCEndpoint*	server{ nullptr };
	RPCEndpoint*	client{ nullptr };
};

static CRefPtr<CMemoryStream> MakeTestPayload(int size, int seed)
{
	CRefPtr<CMemoryStream> payload = CRefPtr_new(CMemoryStream);
	payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, size);
	for (int i = 0; i < size; ++i)
	{
		const ubyte value = static_cast<ubyte>(i * 13 + seed);
		payload->Write(&value, 1, 1);
	}
	return payload;
}

static bool CheckTestPayload(IFileStream* payload, int size, int seed)
{
	if (!payload || payload->GetSize() != size)
		return false;

	const ubyte* data = static_cast<CMemoryStream*>(payload)->GetBasePointer();
	for (int i = 0; i < size; ++i)
	{
		if (data[i] != static_cast<ubyte>(i * 13 + seed))
			return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------

TEST(RPC_TESTS, CallAndReply)
{
	RPCTestLink link;

	RPCProtocol serverMath("math");
	serverMath.Method("Add", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		EXPECT_FALSE(ctx.IsNotify());
		EXPECT_EQ(payload, nullptr);

		int a = 0;
		int b = 0;
		const char* name = "";
		params.Get("a").GetValues(a);
		params.Get("b").GetValues(b);
		params.Get("name").GetValues(name);

		KVSection result;
		result.SetKey("sum", a + b);
		result.SetKey("name", name);
		ctx.Reply(result);
	});
	link.server->AddProtocol(serverMath);

	RPCProtocol clientMath("math");
	link.client->AddProtocol(clientMath);

	bool connected = false;
	link.server->SetConnectionCallback([&](NetConnectionId conn, bool isConnected) { connected = isConnected; });

	link.Start();
	EXPECT_TRUE(connected);
	EXPECT_TRUE(link.client->IsConnected());

	KVSection params;
	params.SetKey("a", 40);
	params.SetKey("b", 2);
	params.SetKey("name", "with spaces and \"quotes\"");

	int sum = 0;
	EqString name;
	Future<RPCResponse> future = clientMath.RemoteCall("Add", params);
	future.AddCallback([&](const FutureResult<RPCResponse>& result) {
		ASSERT_FALSE(result.IsError());
		result->GetResult().Get("sum").GetValues(sum);
		result->GetResult().Get("name").GetValues(name);
	});

	link.Update(2);
	ASSERT_TRUE(future.HasResult());
	EXPECT_FALSE(future.HasErrors());
	EXPECT_EQ(sum, 42);
	EXPECT_EQ(name, "with spaces and \"quotes\"");

	// Hello told the protocols of the peer
	const NetConnectionId serverConn = link.client->GetConnectionId();
	EXPECT_EQ(link.client->GetPeerProtocolVersion(serverConn, "math"), 1);
	EXPECT_EQ(link.client->GetPeerProtocolVersion(serverConn, "unknown"), 0);
}

TEST(RPC_TESTS, NotifyAndBroadcast)
{
	// a notify of the server without a connection goes to all its connections
	RPCTestLink link;

	RPCProtocol serverEvents("events");
	link.server->AddProtocol(serverEvents);

	int received = 0;
	int lastValue = 0;
	RPCProtocol clientEvents("events");
	clientEvents.Method("Changed", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		EXPECT_TRUE(ctx.IsNotify());
		++received;
		params.Get("value").GetValues(lastValue);
	});
	link.client->AddProtocol(clientEvents);
	link.Start();

	KVSection params;
	params.SetKey("value", 7);
	serverEvents.RemoteNotify("Changed", params);

	// a notify of an unknown method is only logged
	serverEvents.RemoteNotify("Unknown", params);

	link.Update(2);
	EXPECT_EQ(received, 1);
	EXPECT_EQ(lastValue, 7);
}

TEST(RPC_TESTS, UnknownMethod)
{
	RPCTestLink link;

	RPCProtocol clientProtocol("missing");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Future<RPCResponse> future = clientProtocol.RemoteCall("Anything", KVSection());
	link.Update(2);

	ASSERT_TRUE(future.HasResult());
	ASSERT_TRUE(future.HasErrors());
	EXPECT_EQ(future.GetErrorCode(), RPC_ERROR_UNKNOWN_METHOD);
}

TEST(RPC_TESTS, ErrorAndDefaultReply)
{
	RPCTestLink link;

	RPCProtocol serverProtocol("test");
	serverProtocol
		.Method("Fail", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			ctx.ReplyError(5, "failed on purpose");
		})
		.Method("NoReply", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("test");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Future<RPCResponse> failed = clientProtocol.RemoteCall("Fail", KVSection());
	Future<RPCResponse> noReply = clientProtocol.RemoteCall("NoReply", KVSection());
	link.Update(2);

	ASSERT_TRUE(failed.HasErrors());
	EXPECT_EQ(failed.GetErrorCode(), 5);
	EXPECT_EQ(failed.GetErrorMessage(), "failed on purpose");

	ASSERT_TRUE(noReply.HasResult());
	EXPECT_FALSE(noReply.HasErrors());
}

TEST(RPC_TESTS, DeferredReply)
{
	RPCTestLink link;

	RPCDeferredReply deferred;
	RPCProtocol serverProtocol("test");
	serverProtocol.Method("Later", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		deferred = ctx.Defer();
	});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("test");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Future<RPCResponse> future = clientProtocol.RemoteCall("Later", KVSection());
	link.Update(3);

	ASSERT_TRUE(deferred.IsValid());
	EXPECT_FALSE(future.HasResult());

	KVSection result;
	result.SetKey("done", true);
	deferred.Reply(result);
	EXPECT_FALSE(deferred.IsValid());

	// a second reply is dropped
	deferred.Reply(result);

	link.Update(2);
	ASSERT_TRUE(future.HasResult());
	bool done = false;
	future.GetResult().GetResult().Get("done").GetValues(done);
	EXPECT_TRUE(done);
}

TEST(RPC_TESTS, PayloadStreams)
{
	RPCTestLink link;

	// several chunks each way, the reply payload is a different stream
	const int callPayloadSize = RPC_PAYLOAD_CHUNK_SIZE * 5 + 123;
	const int replyPayloadSize = RPC_PAYLOAD_CHUNK_SIZE * 2 + 7;

	RPCProtocol serverProtocol("data");
	serverProtocol.Method("Echo", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		EXPECT_TRUE(CheckTestPayload(payload, callPayloadSize, 1));
		ctx.Reply(KVSection(), IFileStreamPtr(MakeTestPayload(replyPayloadSize, 2).Ptr()));
	})
	.Method("Empty", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		ASSERT_NE(payload, nullptr);
		EXPECT_EQ(payload->GetSize(), 0);
	});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("data");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Future<RPCResponse> echo = clientProtocol.RemoteCall("Echo", KVSection(), IFileStreamPtr(MakeTestPayload(callPayloadSize, 1).Ptr()));
	Future<RPCResponse> empty = clientProtocol.RemoteCall("Empty", KVSection(), IFileStreamPtr(MakeTestPayload(0, 0).Ptr()));

	ASSERT_TRUE(link.UpdateUntil([&]() { return echo.HasResult() && empty.HasResult(); }));
	ASSERT_FALSE(echo.HasErrors());
	EXPECT_TRUE(CheckTestPayload(echo.GetResult().GetPayload(), replyPayloadSize, 2));
	EXPECT_FALSE(empty.HasErrors());
}

TEST(RPC_TESTS, PayloadLayout)
{
	RPCTestLink link;

	// the layout of the payload is the contract of the method: WriteObj / ReadObj
	Array<Vector3D> received(PP_SL);
	Array<int> receivedIndices(PP_SL);

	RPCProtocol serverProtocol("mesh");
	serverProtocol.Method("SetMesh", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		payload->ReadObj(received);
		payload->ReadObj(receivedIndices);
	});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("mesh");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Array<Vector3D> positions(PP_SL);
	Array<int> indices(PP_SL);
	for (int i = 0; i < 1000; ++i)
	{
		positions.append(Vector3D(i, i * 2, i * 3));
		indices.append(i);
	}

	CRefPtr<CMemoryStream> payload = CRefPtr_new(CMemoryStream);
	payload->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
	payload->WriteObj(positions);
	payload->WriteObj(indices);

	Future<RPCResponse> future = clientProtocol.RemoteCall("SetMesh", KVSection(), IFileStreamPtr(payload.Ptr()));
	ASSERT_TRUE(link.UpdateUntil([&]() { return future.HasResult(); }));

	ASSERT_EQ(received.numElem(), positions.numElem());
	ASSERT_EQ(receivedIndices.numElem(), indices.numElem());
	EXPECT_FLOAT_EQ(received[999].z, 2997.0f);
	EXPECT_EQ(receivedIndices[500], 500);
}

TEST(RPC_TESTS, PayloadTooLarge)
{
	RPCTestLink link;
	link.server->SetMaxPayloadSize(1000);

	bool methodRan = false;
	RPCProtocol serverProtocol("data");
	serverProtocol.Method("Put", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		methodRan = true;
	});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("data");
	link.client->AddProtocol(clientProtocol);
	link.Start();

	Future<RPCResponse> future = clientProtocol.RemoteCall("Put", KVSection(), IFileStreamPtr(MakeTestPayload(RPC_PAYLOAD_CHUNK_SIZE * 3, 0).Ptr()));
	ASSERT_TRUE(link.UpdateUntil([&]() { return future.HasResult(); }));
	link.Update(4);

	EXPECT_FALSE(methodRan);
	ASSERT_TRUE(future.HasErrors());
	EXPECT_TRUE(future.GetErrorCode() == RPC_ERROR_PAYLOAD_TOO_LARGE || future.GetErrorCode() == RPC_ERROR_CANCELLED);

	// the connection still works
	Future<RPCResponse> next = clientProtocol.RemoteCall("Put", KVSection(), IFileStreamPtr(MakeTestPayload(100, 0).Ptr()));
	ASSERT_TRUE(link.UpdateUntil([&]() { return next.HasResult(); }));
	EXPECT_FALSE(next.HasErrors());
	EXPECT_TRUE(methodRan);
}

TEST(RPC_TESTS, DisconnectFailsCalls)
{
	RPCTestLink link;

	RPCProtocol serverProtocol("test");
	serverProtocol.Method("Never", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		ctx.Defer();
	});
	link.server->AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("test");
	link.client->AddProtocol(clientProtocol);

	bool disconnected = false;
	link.client->SetConnectionCallback([&](NetConnectionId conn, bool connected) { disconnected = !connected; });

	NetConnectionId serverConn = NET_CONNECTION_NONE;
	link.server->SetConnectionCallback([&](NetConnectionId conn, bool connected) { serverConn = conn; });
	link.Start();

	Future<RPCResponse> future = clientProtocol.RemoteCall("Never", KVSection());
	link.Update(2);
	EXPECT_FALSE(future.HasResult());

	// the server drops the client
	ASSERT_NE(serverConn, NET_CONNECTION_NONE);
	link.server->Disconnect(serverConn);
	link.Update(2);

	ASSERT_TRUE(future.HasErrors());
	EXPECT_EQ(future.GetErrorCode(), RPC_ERROR_DISCONNECTED);
	EXPECT_TRUE(disconnected);
	EXPECT_FALSE(link.client->IsConnected());

	Future<RPCResponse> afterClose = clientProtocol.RemoteCall("Never", KVSection());
	ASSERT_TRUE(afterClose.HasErrors());
	EXPECT_EQ(afterClose.GetErrorCode(), RPC_ERROR_NOT_CONNECTED);
}

TEST(RPC_TESTS, RemoveProtocolFailsCalls)
{
	RPCTestLink link;

	RPCProtocol serverProtocol("test");
	serverProtocol.Method("Never", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		ctx.Defer();
	});
	link.server->AddProtocol(serverProtocol);

	Future<RPCResponse> future;
	{
		RPCProtocol clientProtocol("test");
		link.client->AddProtocol(clientProtocol);
		link.Start();

		future = clientProtocol.RemoteCall("Never", KVSection());
	}

	ASSERT_TRUE(future.HasErrors());
	EXPECT_EQ(future.GetErrorCode(), RPC_ERROR_CANCELLED);
}

TEST(RPC_TESTS, IndependentEndpoints)
{
	// one application with two client endpoints, each connected to its own server
	RPCTestLink linkA;
	RPCTestLink linkB;

	RPCProtocol serverA("name");
	serverA.Method("Get", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		KVSection result;
		result.SetKey("name", "A");
		ctx.Reply(result);
	});
	RPCProtocol serverB("name");
	serverB.Method("Get", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		KVSection result;
		result.SetKey("name", "B");
		ctx.Reply(result);
	});
	linkA.server->AddProtocol(serverA);
	linkB.server->AddProtocol(serverB);

	RPCProtocol clientA("name");
	RPCProtocol clientB("name");
	linkA.client->AddProtocol(clientA);
	linkB.client->AddProtocol(clientB);

	linkA.Start();
	linkB.Start();

	Future<RPCResponse> fromA = clientA.RemoteCall("Get", KVSection());
	Future<RPCResponse> fromB = clientB.RemoteCall("Get", KVSection());
	linkA.Update(2);
	linkB.Update(2);

	ASSERT_TRUE(fromA.HasResult() && fromB.HasResult());
	EqString nameA;
	EqString nameB;
	fromA.GetResult().GetResult().Get("name").GetValues(nameA);
	fromB.GetResult().GetResult().Get("name").GetValues(nameB);
	EXPECT_EQ(nameA, "A");
	EXPECT_EQ(nameB, "B");
}

TEST(RPC_TESTS, OverTCP)
{
	CRefPtr<CNetTransportTCP> serverTransport = CRefPtr_new(CNetTransportTCP);
	RPCEndpoint server(RPCEndpoint::SERVER, INetTransportPtr(serverTransport.Ptr()));
	RPCEndpoint client(RPCEndpoint::CLIENT, INetTransportPtr(CRefPtr_new(CNetTransportTCP).Ptr()));

	const int payloadSize = RPC_PAYLOAD_CHUNK_SIZE * 8 + 1;

	RPCProtocol serverProtocol("data");
	serverProtocol.Method("Size", [&](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
		EXPECT_TRUE(CheckTestPayload(payload, payloadSize, 3));
		KVSection result;
		result.SetKey("size", static_cast<int>(payload->GetSize()));
		ctx.Reply(result);
	});
	server.AddProtocol(serverProtocol);

	RPCProtocol clientProtocol("data");
	client.AddProtocol(clientProtocol);

	ASSERT_TRUE(server.Listen("127.0.0.1:0"));
	ASSERT_TRUE(client.Connect(EqString::Format("127.0.0.1:%d", serverTransport->GetListenPort())));

	// sent before the connection is established
	Future<RPCResponse> future = clientProtocol.RemoteCall("Size", KVSection(), IFileStreamPtr(MakeTestPayload(payloadSize, 3).Ptr()));

	for (int i = 0; i < RPC_TEST_TIMEOUT_MS && !future.HasResult(); ++i)
	{
		server.Update();
		client.Update();
		Platform_Sleep(1);
	}

	ASSERT_TRUE(future.HasResult());
	ASSERT_FALSE(future.HasErrors());
	int size = 0;
	future.GetResult().GetResult().Get("size").GetValues(size);
	EXPECT_EQ(size, payloadSize);
	EXPECT_TRUE(client.IsConnected());
}

//-------------------------------------------------------------------------------------------------
// The engine side of the Python eqrpc interop tests (tools/python/eqrpc/tests/test_engine.py),
// started by them: listens on a free port of 127.0.0.1, prints "EQRPC_PORT <port>" and serves the
// "interop" protocol until the Quit notify or a timeout.

TEST(RPC_TESTS, DISABLED_ServePython)
{
	CRefPtr<CNetTransportTCP> transport = CRefPtr_new(CNetTransportTCP);
	RPCEndpoint server(RPCEndpoint::SERVER, INetTransportPtr(transport.Ptr()));

	bool quit = false;
	RPCProtocol protocol("interop", 3);
	protocol
		.Method("Echo", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			ctx.Reply(params, payload);
		})
		.Method("Engine", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			// every kind of KeyValues data, as engine code builds it
			KVSection result;
			result.SetKey("int", 42);
			result.SetKey("float", 0.5f);
			result.SetKey("bool", true);
			result.SetKey("string", "Привет, \"world\"");
			result.SetKey("position", Vector3D(1.0f, -2.0f, 3.5f));
			result.AddKey("model", "a");
			result.AddKey("model", "b");
			result.CreateSection("empty");
			result.CreateSection("straight", "main").SetKey("lanes", 2);
			KVSection& nested = result.CreateSection("nested");
			nested.CreateSection("deeper").SetKey("depth", 2);

			KVSection inner;
			inner.SetKey("x", 1);
			result.CreateSection("value", nullptr, KVPAIR_SECTION).AddValue(inner);

			// Array<Vector3D> and Array<int>, the WriteObj layout
			Array<Vector3D> positions(PP_SL);
			Array<int> indices(PP_SL);
			positions.append(Vector3D(0.0f, 0.0f, 0.0f));
			positions.append(Vector3D(1.0f, 2.0f, 3.0f));
			indices.append(0);
			indices.append(1);
			indices.append(-1);

			CRefPtr<CMemoryStream> data = CRefPtr_new(CMemoryStream);
			data->Open(FS_OPEN_READ | FS_OPEN_WRITE, nullptr, 0);
			data->WriteObj(positions);
			data->WriteObj(indices);
			ctx.Reply(result, IFileStreamPtr(data.Ptr()));
		})
		.Method("Check", [](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			// what the engine reads from the Python messages
			int intValue = 0;
			float floatValue = 0.0f;
			bool boolValue = false;
			const char* stringValue = "";
			float mixed[3] = { 0.0f, 0.0f, 0.0f };
			const char* lanes = "";
			int depth = 0;
			params.Get("int").GetValues(intValue);
			params.Get("float").GetValues(floatValue);
			params.Get("bool").GetValues(boolValue);
			params.Get("string").GetValues(stringValue);
			params.Get("mixed").GetValues(mixed[0], mixed[1], mixed[2]);
			params.Get("straight").GetValues(lanes);
			params.Get("nested").Get("deeper").Get("depth").GetValues(depth);

			int models = 0;
			for (const KVSection* key : params.keys)
				models += CString::Compare(key->GetName(), "model") == 0 ? 1 : 0;

			KVSection result;
			result.SetKey("int", intValue);
			result.SetKey("float", floatValue);
			result.SetKey("bool", boolValue);
			result.SetKey("string", stringValue);
			result.SetKey("mixed", Vector3D(mixed[0], mixed[1], mixed[2]));
			result.SetKey("straight", lanes);
			result.SetKey("depth", depth);
			result.SetKey("models", models);
			result.SetKey("payload", payload ? static_cast<int>(payload->GetSize()) : -1);
			ctx.Reply(result);
		})
		.Method("CallBack", [&protocol](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			// calls the Python client and replies with its result
			int value = 0;
			params.Get("value").GetValues(value);

			KVSection callParams;
			callParams.SetKey("value", value);
			RPCDeferredReply reply = ctx.Defer();
			protocol.RemoteCall(ctx.GetConnectionId(), "Twice", callParams).AddCallback([reply](const FutureResult<RPCResponse>& response) {
				if (response.IsError())
				{
					reply.ReplyError(response.GetErrorCode(), response.GetErrorMessage());
					return;
				}
				reply.Reply(response->GetResult());
			});
		})
		.Method("Quit", [&quit](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
			quit = true;
		});
	server.AddProtocol(protocol);

	ASSERT_TRUE(server.Listen("127.0.0.1:0"));
	printf("EQRPC_PORT %d\n", transport->GetListenPort());
	fflush(stdout);

	for (int i = 0; i < 60000 && !quit; ++i)
	{
		server.Update();
		Platform_Sleep(1);
	}

	// sends what is queued before the endpoint is destroyed
	for (int i = 0; i < 100; ++i)
	{
		server.Update();
		Platform_Sleep(1);
	}
	EXPECT_TRUE(quit);
}
