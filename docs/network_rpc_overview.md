# RPC overview

Remote method calls between applications: named protocols with methods, calls with replies,
notifications and binary payload streams, over any transport of
[network_transport_overview.md](network_transport_overview.md). The first user is the level editor ↔
Blender link of the game (`docs/blender_link_design.md` of the game repository); later the game uses
it for its servers (rendez-vous, lobby, statistics) next to the multiplayer data channels.

Status: the C++ endpoint and protocols are implemented and tested; the Python side
(`tools/python/eqrpc/`) is designed, not implemented yet.

| File | Contents |
|---|---|
| `shared/network/rpc/RPCEndpoint.*` | `RPCEndpoint`: connections, call ids, futures, payload streams, the built-in `rpc` protocol |
| `shared/network/rpc/RPCProtocol.*` | `RPCProtocol`, `RPCCallContext`, `RPCDeferredReply`, `RPCResponse`, `RPCMessage`, `ERPCError` |
| `framework/utils/KeyValues.cpp` | binary KeyValues (`WriteBinary` / `ParseBinary`), the wire format of the messages |
| `tests/network/rpc_tests.cpp` | endpoint tests over the loopback and TCP transports |
| `tools/python/eqrpc/` | planned: Python client and server of the same protocol (standard library only), used by the Blender addon of the game |

The files are part of `networkLib`.

## Layers

```
RPCProtocol ── named methods: Method, RemoteCall, RemoteNotify
     │
RPCEndpoint ── connections, call ids, Future, payload streams, binary KeyValues messages, "rpc" protocol
     │  two reliable lanes: calls, payload
INetTransport ── TCP, loopback, GNS (later)
```

## Endpoint

```cpp
struct RPCLanes
{
    int     calls{ 0 };     // call, reply and notify messages
    int     payload{ 1 };   // payload stream chunks
};

class RPCEndpoint : public WeakRefObject<RPCEndpoint>
{
public:
    enum ERole { CLIENT, SERVER };
    using ConnectionCb = EqFunction<void(NetConnectionId conn, bool connected)>;

    // owned transport: Update polls it
    RPCEndpoint(ERole role, INetTransportPtr transport, RPCLanes lanes = {});

    // shared transport: the owner polls it and passes the events of the RPC lanes to HandleEvent
    RPCEndpoint(ERole role, INetTransport& sharedTransport, RPCLanes lanes);
    void    AttachConnection(NetConnectionId conn);
    void    HandleEvent(const NetTransportEvent& event);

    bool    Listen(const char* address);    // SERVER, owned transport
    bool    Connect(const char* address);   // CLIENT, owned transport
    void    Disconnect(NetConnectionId conn = NET_CONNECTION_NONE);

    void    AddProtocol(RPCProtocol& protocol);
    void    RemoveProtocol(RPCProtocol& protocol);

    void    SetConnectionCallback(ConnectionCb callback);
    void    SetMaxPayloadSize(int64 size);  // 256 MB by default

    bool            IsConnected() const;    // client: the connection is established
    NetConnectionId GetConnectionId() const;  // client
    int             GetPeerProtocolVersion(NetConnectionId conn, const char* protocol) const;

    // receives, runs the methods, resolves the futures, sends the payload chunks
    void    Update();
};
```

- **Independent endpoints**: an endpoint owns its connections, call id counter, pending futures and
  payload streams; there are no global tables. An application may have any number of endpoints,
  each connected to its own server, each with its own protocols.
- **Owned or shared transport**: an endpoint either owns its transport (a tool link, a lobby
  connection) or uses two lanes of a connection the multiplayer code owns. With an owned transport
  the endpoint configures the lanes (0 calls with priority 0, 1 payload with priority 1). With a
  shared one the owner configures them, polls the transport, calls `AttachConnection` for an
  established connection and passes the events of the RPC lanes and the disconnect of the
  connection to `HandleEvent`; `Disconnect` then only forgets the connection.
- **Role**: a client has one connection (calls without a connection go to it); a server has several:
  `RemoteCall` needs a connection, `RemoteNotify` without one goes to all of them. `Connect` returns
  at once; calls made before the connection is established are queued by the transport.
- **Connection callback**: `SetConnectionCallback` is called when a connection is established and
  when an established one closes (by the peer, an error or `Disconnect`).
- **Threads**: `Update`, `HandleEvent`, the methods and the future callbacks run in the thread of the
  owner (the transport events come in `Poll` of that thread, see the transport threads contract).
- **Built-in `rpc` protocol** of every endpoint (`RPC_PROTOCOL_NAME`, `RPC_PROTOCOL_VERSION`):

  | Method | Meaning |
  |---|---|
  | `Hello` | notify sent first on every connection, with the protocols of the endpoint and their versions; the receiver keeps them (`GetPeerProtocolVersion`) and logs a version mismatch. Protocols are added before `Listen` / `Connect`, later ones are not announced |
  | `Ping` | replies with an empty result: keep-alive and latency |
  | `CancelStream` | notify `{ stream N }`: the receiver of a payload refuses it, the sender stops sending it |

- **Errors** (`ERPCError`, negative; methods reply with their own positive codes):

  | Code | When |
  |---|---|
  | `RPC_ERROR_UNKNOWN_METHOD` | the peer has no such protocol or method; a notify of one is only logged |
  | `RPC_ERROR_DISCONNECTED` | the connection closed or the endpoint was destroyed before the reply |
  | `RPC_ERROR_CANCELLED` | the payload of the call or the reply was cancelled, or the protocol was removed |
  | `RPC_ERROR_PAYLOAD_TOO_LARGE` | the peer refused the payload size |
  | `RPC_ERROR_NOT_CONNECTED` | no connection, or the protocol is not added to an endpoint |
  | `RPC_ERROR_SEND_FAILED` | the message is larger than the transport allows |

- **Broken input**: a message that does not parse, a payload chunk without its header or a stream
  longer or shorter than announced closes the connection.

## Protocols

A protocol is a named set of methods bound to one endpoint, in the style of the RMC services of
[AlcatrazServer](https://github.com/REHamster/AlcatrazServer).

```cpp
class RPCMessage : public RefCountedObject<RPCMessage>    // a received message
{
public:
    KVSection               root;
    CRefPtr<CMemoryStream>  payload;
};

struct RPCResponse
{
    const KVSection&        GetResult() const;  // "result" of the reply, an empty section when none
    CRefPtr<CMemoryStream>  GetPayload() const;
    RPCMessagePtr           message;
};

class RPCCallContext
{
public:
    RPCEndpoint&        GetEndpoint() const;
    NetConnectionId     GetConnectionId() const;
    bool                IsNotify() const;       // no reply is expected

    void                Reply(const KVSection& result, IFileStreamPtr payload = nullptr);
    void                ReplyError(int code, const char* message);
    RPCDeferredReply    Defer();                // reply later, the context is not valid after the method returns
};

using RPCMethodFunc = EqFunction<void(RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload)>;

class RPCProtocol
{
public:
    RPCProtocol(const char* name, int version = 1);
    ~RPCProtocol();                             // removes itself from its endpoint

    RPCProtocol&        Method(const char* name, RPCMethodFunc func);

    Future<RPCResponse> RemoteCall(const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);
    void                RemoteNotify(const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);

    Future<RPCResponse> RemoteCall(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);
    void                RemoteNotify(NetConnectionId conn, const char* method, const KVSection& params, IFileStreamPtr payload = nullptr);
};
```

```cpp
RPCProtocol m_defsProtocol{ "defs" };

m_defsProtocol
    .Method("GetList", [this](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr) {
        KVSection result;
        WriteDefList(result);
        ctx.Reply(result);
    })
    .Method("SetMesh", [this](RPCCallContext& ctx, const KVSection& params, IFileStreamPtr payload) {
        int defId = 0;
        params.Get("def").GetValues(defId);

        RPCDeferredReply reply = ctx.Defer();
        RunLater([this, reply, payload, defId]() {
            ApplyDefMesh(defId, payload);
            reply.Reply(KVSection());
        });
    });

m_endpoint.AddProtocol(m_defsProtocol);

m_defsProtocol.RemoteNotify("Changed", params);
m_defsProtocol.RemoteCall("GetSelection", KVSection()).AddCallback([](const FutureResult<RPCResponse>& r) {
    if (r.IsError())
        return;
    const KVSection& result = r->GetResult();
    ...
});
```

- `RemoteCall` returns the engine `Future` (`framework/ds/future.h`, see
  [framework_ds.md](framework_ds.md)); the endpoint resolves its `Promise` in `Update`, so the
  callbacks run in the thread of the owner. Remote errors and disconnects come as `SetError`.
  `Future::Wait` must not be called in the thread that runs `Update` of the endpoint: nothing would
  resolve the future while it waits. Use `AddCallback` there; `Wait` is for other threads (a job
  waiting for a reply that the owner's `Update` resolves).
- `KVSection` can't be copied, while a `Future` result is copied: the response keeps the received
  message by `RPCMessagePtr`, and `GetResult` points into it.
- `RemoteNotify` sends an event without a call id; the receiver handles it with the same method,
  `ctx.Reply` does nothing there.
- A call that the method neither replies nor defers gets an empty result when the method returns.
- `RPCDeferredReply` is valid while its connection is open; it is sent once, from the thread of the
  endpoint (a job posts it back, as `RunLater` above). It holds the endpoint by a weak pointer, so a
  reply after the endpoint is destroyed is dropped.
- A protocol is bound to one endpoint at a time. The same protocol on two endpoints is two instances
  whose methods are set up by one function (`SetupChatProtocol(RPCProtocol&)`); protocol names are
  unique per endpoint only.
- A protocol is a member of its owner; `RemoveProtocol` or its destructor unbinds it and fails its
  pending futures with `RPC_ERROR_CANCELLED`.
- Optional typed parameters on top of the same API: `Method<SetMeshParams>("SetMesh", ...)` parses the
  parameters with `KeyValues::ParseDesc` into a structure with `DEFINE_KEYVALUES_DESC` (not
  implemented yet).

## Messages

A message of the calls lane is one root `KVSection` in the binary KeyValues form (see
[Binary KeyValues](#binary-keyvalues)); shown here as KeyValues text, as the logs print it:

```
id 9
protocol "defs"
method "SetMesh"
params { def 12; revision 3; }
payload { stream 12; size 212992; }
```

```
id 9
result { }
```

```
id 9
error { code 2; message "unknown def 12"; }
```

- A notify has no `id`. A reply has the `id` of its call and `result` or `error`, and may have a
  `payload`. Call ids start at 1 and are per connection and direction.
- `params` and `result` are given to the methods and to `RPCResponse` as they are: no conversion
  between the wire and the method.
- Names are sent instead of numeric ids, for debugging; they can become hashes (`StringToHash`)
  later without changing the API.
- A message must fit `GetMaxMessageSize()` of the transport, else the call fails with
  `RPC_ERROR_SEND_FAILED`; data larger than parameters goes into the payload.

### Payload streams

A message of the payload lane is a chunk of a stream:

```cpp
struct RPCPayloadChunkHeader    // little-endian, followed by the chunk data
{
    uint32  streamId;           // per connection and direction
    uint8   flags;              // RPC_PAYLOAD_LAST, RPC_PAYLOAD_CANCEL
    uint8   reserved[3];
};
```

- The call (or reply) message is sent first with the stream id and the full size, so the receiver
  allocates the stream at once and can show progress. The chunks (`RPC_PAYLOAD_CHUNK_SIZE`, 64 KB, at
  most the transport message size) follow; the method runs when the chunk flagged
  `RPC_PAYLOAD_LAST` arrives. A call without `payload` runs at once.
- An empty payload has no stream: the message has `payload { stream 0; size 0; }` and the method
  gets an empty stream at once.
- The calls lane goes before the payload lane, so small calls are not held back by large payloads.
  On a backend whose lanes do not wait for each other a chunk may come before its message; the
  receiver keeps it until the message comes.
- `Update` reads the next chunks of the outgoing streams while the payload lane has less than four
  chunks queued (`INetTransport::GetQueuedBytes`), so a large payload is not copied into the
  transport at once. Streams of one connection go one after another.
- The sender passes any `IFileStreamPtr`: a `CMemoryStream` filled for the call or a file of the file
  system (`IFileSystem::Open`), sent without loading it whole. The endpoint reads it from the start
  by `GetSize()` (each stream keeps its offset, so a notify to all connections reads one source for
  each); the stream belongs to the endpoint after the call. Payloads are smaller than 2 GB (the size
  is a KeyValues `int`).
- A payload larger than `SetMaxPayloadSize` of the receiver is refused: a call gets the
  `RPC_ERROR_PAYLOAD_TOO_LARGE` reply, a reply fails its future with that code, and the receiver
  sends `rpc.CancelStream`; its chunks still coming are dropped.
- A source that can't be read cancels the stream with a chunk flagged `RPC_PAYLOAD_CANCEL`: our call
  fails with `RPC_ERROR_CANCELLED`, the receiver of a reply fails its future with it, a cancelled
  call never runs its method.
- The receiver gets a `CRefPtr<CMemoryStream>` allocated with the announced size and positioned at
  its start; the method may read it at once or keep the reference.
- The payload layout is the contract of the method, like any engine serializer: it is written and
  read with `WriteObj` / `ReadObj` (`VSWrite` / `VSRead` of `ArrayBase` write an `int` count and the
  elements). New fields go to the end; a layout change increases the protocol version.

  ```cpp
  payload->WriteObj(positions);     // Array<Vector3D>
  payload->WriteObj(indices);       // Array<uint32>
  ...
  payload->ReadObj(positions);
  payload->ReadObj(indices);
  ```

- Engine serializers that write to `IFileStream` (`KeyValues::WriteBinary`, model and level writers)
  can be payloads between C++ applications as they are.
- `IFileStream::ReadArray` / `WriteArray` go element by element (a virtual `Read` call per element).
  If that is too slow for large meshes, trivially copyable types can be read with one `Read` call,
  enabled by an explicit type trait, so a custom `VSRead` of a POD structure is not bypassed.

## Binary KeyValues

The messages use the existing binary KeyValues of the engine (`KeyValues::WriteBinary` /
`ParseBinary`, ident `BKVS`, see [keyvalues.md](keyvalues.md)). It keeps a `KVSection` exactly:
values and keys of one section, repeated keys in their order, nested sections as values. No other
serialization library is used.

```
section:  int32  ident 'BKVS'
          int32  type           EKVPairType of the section
          int32  keyCount
          uint16 valueCount
          uint16 nameLen
          char   name[nameLen]  UTF-8, no terminator
          value  values[valueCount]
          section keys[keyCount]

value:    int32  type           STRING 0, INT 1, FLOAT 2, BOOL 3, SECTION 4
          int32 / float32 / bool (4 bytes)   string length for STRING
          char   string[length] for STRING, or a section for SECTION
```

- Little-endian, 4-byte fields, no padding between them (the header is 16 bytes, a value header 8).
- Numbers are `int32` and `float32`, the types `KVPairValue` keeps; protocols send positions in local
  coordinates and bulk data as payload. `KVPairValue::GetFloat` of an `INT` value returns it as a
  float, so a float parameter sent as `1` is read correctly.
- A section has at most 65535 values and a name of at most 65535 bytes; longer arrays are payload.
- `WriteBinary` clears the value headers, so equal sections give equal bytes.
- `ParseBinary` checks every count and length against the rest of the stream and limits the nesting
  depth (64), so broken data from another process fails the parse instead of reading past the end.

### JSON view

JSON is only a view of the messages: Python code builds simple parameters from `dict` / `list`
values and prints messages for logs and debugging with `json.dumps`. The view maps:

| KeyValues | JSON |
|---|---|
| `speed 50` (one value) | `"speed": 50` |
| `position 1 2 3` (several values) | `"position": [1, 2, 3]` |
| `INT`, `FLOAT`, `BOOL`, `STRING` | number, number, `true` / `false`, string |
| key without values and keys | `null` |
| section with keys | object |
| section with values and keys: `straight "main" { ... }` | `"straight": {"_v": ["main"], ...}` |
| a nested section as a value (`KVPAIR_SECTION`) | `{"_kv": { ... }}` in the values |
| repeated key with sections: several `straight "…" {}` | array of objects: `"straight": [{"_v": ["a"], ...}, {"_v": ["b"], ...}]` |
| repeated key with values: `model "a"`, `model "b"` | array of arrays: `"model": [["a"], ["b"]]` |

- An array of scalars (and `_kv` values) is the values of one key; an array of only objects or only
  arrays is a repeated key.
- Python `int` becomes `INT`, `float` becomes `FLOAT`.
- The view loses only the interleaving of different repeated keys (`a, b, a`); messages built from
  `kv.Section` directly keep it.
- The engine prints messages as KeyValues text in its logs; a C++ JSON printer is added only if a
  tool needs it.

## Authorization (planned)

Now every peer is trusted: the editor link listens on `127.0.0.1` only. A backend server (the
rendez-vous server of the multiplayer, the lobby) takes connections from the internet and splits the
work in two levels:

- **Transport**: the channel is encrypted and the server is the one the client expects. The GNS
  backend does both (see [security and signaling](network_transport_overview.md#security-and-signaling)); the TCP backend stays for local
  tool links.
- **RPC**: who the peer is and what it may call. The endpoint gives the mechanism, the game gives the
  policy; there is no built-in login protocol.

Login is an ordinary game protocol method, for example `account.Login { ticket "..." }`. The server
checks the ticket asynchronously (a platform web API or an account service) and replies through
`ctx.Defer()`. The endpoint gets:

- **Connection data**: `SetConnectionData(conn, CRefPtr<RefCountedObject>)` and
  `GetConnectionData(conn)`; the login method keeps the user there, the other methods read it
  through `RPCCallContext`.
- **Call filter**: `SetCallFilter(func(conn, protocol, method) -> int)` before a call is dispatched;
  a non-zero code is the reply (`RPC_ERROR_NOT_AUTHORIZED`, a new error), a filtered notify is
  dropped. Which methods are open before the login and which roles the others need is up to the
  filter.
- **Login timeout**: a connection with no connection data after a set time is closed.
- **Limits for untrusted peers**, per connection: the bytes of all incoming payload streams (now the
  max payload size is per stream and the stream count is not limited), the pending calls and streams,
  the message rate. A rendez-vous server needs payloads of a few KB at most.

The parser of the binary KeyValues already checks the input (see [Binary KeyValues](#binary-keyvalues)). The message
format does not change: tickets and tokens go in `params`.

## Python side (planned)

`tools/python/eqrpc/` implements the same protocol in Python with the standard library (`socket`,
`struct`) over the TCP frames, so the Blender addon needs no wheels:

```python
endpoint = eqrpc.Endpoint.connect("127.0.0.1", 27100)
defs = eqrpc.Protocol("defs")
endpoint.add_protocol(defs)

@defs.method("Changed")
def on_changed(ctx, params, payload): ...

defs.remote_call("SetMesh", {"def": 12}, payload=data).then(lambda r: ...)
defs.remote_notify("SelectionChanged", {"ids": [1, 2]})

endpoint.update()   # from the main thread (in Blender: bpy.app.timers)
```

- `eqrpc.kv.Section` mirrors `KVSection` (`name`, `values`, `keys`, `find`, `value`, `add`) and reads
  and writes the binary form. Methods get a `Section`; `remote_call` and `remote_notify` take a
  `Section` or a `dict` converted by the JSON view; `section.to_json()` gives the view for logs.
- A value is written with its own type and the section type is the type of its first value: the
  engine reader gives the values of a section the type of the section.
- A protocol documents its parameters in KeyValues text.
- The payload is a `memoryview`; a reader reads the engine layout: `read_array(np.float32, 3)` reads
  the `int32` count and makes a `np.frombuffer` view without a copy.

## Tests

GoogleTest, the `auto_tests` workspace:

- `ds_tests` (`tests/ds/keyvalues_tests.cpp`): binary KeyValues round trip, equal bytes for equal
  sections, every truncation of a message and corrupt counts and lengths fail the parse;
- `network_tests` (`tests/network/rpc_tests.cpp`), endpoints over the loopback transport: calls and
  replies, Hello versions, notifies, unknown methods, errors and the default reply, deferred
  replies, payload streams of several chunks both ways, empty payloads, the `WriteObj` / `ReadObj`
  layout, refused payloads, disconnects and removed protocols failing the futures, two independent
  client endpoints; one call with a payload over the TCP transport.

The Python `kv` module will be tested against bytes written by the engine and every row of the JSON
view.

## Open questions

- The `Hello` contract when the protocol versions differ: refuse the peer, or let each protocol
  decide (now it is only logged).
- Whether methods need a call timeout in the endpoint, or the caller handles it on the future.
