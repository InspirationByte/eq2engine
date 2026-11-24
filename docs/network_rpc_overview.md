# RPC overview — design draft

Status: **design, not implemented.** Remote method calls between applications: named protocols with
methods, calls with replies, notifications and binary payload streams, over any transport of
[network_transport_overview.md](network_transport_overview.md). The first user is the level editor ↔
Blender link of the game (`docs/blender_link_design.md` of the game repository); later the game uses
it for its servers (lobby, statistics) next to the multiplayer data channels. Facts marked *verify*
were not checked yet.

| File (planned) | Contents |
|---|---|
| `shared/network/rpc/RPCEndpoint.*` | `RPCEndpoint`: connections, call ids, futures, payload streams, the built-in `rpc` protocol |
| `shared/network/rpc/RPCProtocol.*` | `RPCProtocol`, `RPCCallContext`, `RPCDeferredReply`, `RPCResponse` |
| `framework/utils/KeyValues.cpp` | existing binary KeyValues (`WriteBinary` / `ParseBinary`), the wire format of the messages |
| `tests/network/` | Tests: binary KeyValues round trips, endpoints over the loopback transport |
| `tools/python/eqrpc/` | Python client and server of the same protocol (standard library only), used by the Blender addon of the game |
| `tools/python/eqrpc/kv.py` | Python `KVSection` and the binary KeyValues reader and writer, the JSON view |

## Layers

```
RPCProtocol ── named methods: Method, RemoteCall, RemoteNotify
     │
RPCEndpoint ── connections, call ids, Future, payload streams, binary KeyValues messages, "rpc" protocol
     │  two reliable lanes: calls, payload
INetTransport ── TCP, loopback, GNS
```

## Endpoint

```cpp
struct RPCLanes
{
    int     calls{ 0 };     // call, reply and notify messages
    int     payload{ 1 };   // payload stream chunks
};

class RPCEndpoint
{
public:
    enum ERole { CLIENT, SERVER };

    // owned transport: Update polls it
    RPCEndpoint(ERole role, CRefPtr<INetTransport> transport, RPCLanes lanes = {});

    // shared transport: the owner polls it and passes the events of the RPC lanes to HandleEvent
    RPCEndpoint(ERole role, INetTransport& sharedTransport, RPCLanes lanes);
    void    AttachConnection(NetConnectionId conn);
    void    HandleEvent(const NetTransportEvent& event);

    bool    Listen(const char* address);    // SERVER, owned transport
    bool    Connect(const char* address);   // CLIENT, owned transport

    void    AddProtocol(RPCProtocol& protocol);
    void    RemoveProtocol(RPCProtocol& protocol);

    // receives, runs the methods, resolves the futures, sends the queued chunks
    void    Update();
};
```

- **Independent endpoints**: an endpoint owns its connections, call id counter, pending futures and
  payload streams; there are no global tables. An application may have any number of endpoints,
  each connected to its own server, each with its own protocols.
- **Owned or shared transport**: an endpoint either owns its transport (a tool link, a lobby
  connection) or uses two lanes of a connection the multiplayer code owns; then the owner polls the
  transport, configures the lanes and passes the events of the RPC lanes and of the connection state
  to `HandleEvent`.
- **Role**: a client has one connection (calls without a connection go to it); a server has several:
  `RemoteCall` needs a connection, `RemoteNotify` without one goes to all of them.
- **Threads**: `Update`, the methods and the future callbacks run in the thread of the owner (the
  transport events come in `Poll` of that thread, see the transport threads contract).
- **Built-in `rpc` protocol** of every endpoint:

  | Method | Meaning |
  |---|---|
  | `Hello` | both sides send their protocols with versions after connecting; calls of other protocols wait for it; an endpoint may close an incompatible peer |
  | `Ping` | keep-alive and latency |
  | `CancelStream` | the receiver drops a payload stream it no longer needs |

- **Errors**: a call of an unknown protocol or method gets `RPC_ERROR_UNKNOWN_METHOD`, a notify of one
  is only logged; a method may reply with its own error code. A disconnect fails the pending futures
  of the connection (`RPC_ERROR_DISCONNECTED`) and drops its streams.

## Protocols

A protocol is a named set of methods bound to one endpoint, in the style of the RMC services of
[AlcatrazServer](https://github.com/REHamster/AlcatrazServer).

```cpp
struct RPCResponse
{
    KVSection               result;     // verify: KVSection must be movable into a Future result,
    CRefPtr<CMemoryStream>  payload;    //         else the response keeps a CRefPtr to it
};

class RPCCallContext
{
public:
    RPCEndpoint&        GetEndpoint() const;
    NetConnectionId     GetConnection() const;
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
        RPCDeferredReply reply = ctx.Defer();
        RunLater([this, reply, payload, defId = KV_GetValueInt(params.FindSection("def"))]() {
            ApplyDefMesh(defId, payload);
            reply.Reply(KVSection());
        });
    });

m_endpoint.AddProtocol(m_defsProtocol);

m_defsProtocol.RemoteNotify("Changed", params);
m_defsProtocol.RemoteCall("GetSelection", KVSection()).AddCallback([](const FutureResult<RPCResponse>& r) {
    if (r.IsError())
        return;
    ...
});
```

- `RemoteCall` returns the engine `Future` (`framework/ds/future.h`, see
  [framework_ds.md](framework_ds.md)); the endpoint resolves its `Promise` in `Update`, so the
  callbacks run in the thread of the owner. Remote errors and disconnects come as `SetError`.
  `Future::Wait` must not be called in the thread that runs `Update` of the endpoint: nothing would
  resolve the future while it waits. Use `AddCallback` there; `Wait` is for other threads (a job
  waiting for a reply that the owner's `Update` resolves).
- `RemoteNotify` sends an event without a call id; the receiver handles it with the same method,
  `ctx.Reply` does nothing there.
- A protocol is bound to one endpoint at a time. The same protocol on two endpoints is two instances
  whose methods are set up by one function (`SetupChatProtocol(RPCProtocol&)`); protocol names are
  unique per endpoint only.
- A protocol is a member of its owner; `RemoveProtocol` or its destructor unbinds it and fails its
  pending futures.
- A deferred reply sent after its connection closed is dropped.
- Optional typed parameters on top of the same API: `Method<SetMeshParams>("SetMesh", ...)` parses the
  parameters with `KeyValues::ParseDesc` into a structure with `DEFINE_KEYVALUES_DESC`.

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
  `payload`. Call ids are per connection and direction.
- `params` and `result` are given to the methods and to `RPCResponse` as they are: no conversion
  between the wire and the method.
- Names are sent instead of numeric ids, for debugging; they can become hashes (`StringToHash`)
  later without changing the API.
- A message must fit `GetMaxMessageSize()` of the transport; data larger than parameters goes into the
  payload.

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
  allocates the stream at once and can show progress. The chunks (about 64 KB, at most the transport
  message size) follow; the method runs when the chunk flagged `RPC_PAYLOAD_LAST` arrives. A call
  without `payload` runs at once. The calls lane goes before the payload lane, so small calls are not
  held back by large payloads.
- The sender may cancel a stream with a chunk flagged `RPC_PAYLOAD_CANCEL`, the receiver with
  `rpc.CancelStream`; a cancelled call never runs its method, a cancelled reply fails its future
  (`RPC_ERROR_CANCELLED`).
- The sender passes any `IFileStreamPtr`: a `CMemoryStream` filled for the call or a file of the file
  system (`IFileSystem::Open`), sent without loading it whole. The endpoint reads it chunk by chunk
  from the start by `GetSize()`, the stream belongs to the endpoint after the call.
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
  coordinates and bulk data as payload.
- A section has at most 65535 values and a name of at most 65535 bytes; longer arrays are payload.
- Implementation fixes in `KeyValues.cpp`:
  - `WriteValueBinary` does not clear the value header, so a `BOOL` value writes 3 bytes of garbage;
    it must be cleared like the section header, so equal sections give equal bytes;
  - `ParseBinary` trusts the counts and lengths (strings go to `stackalloc`); a message comes from
    another process, so the parser must check them against the remaining size and fail instead.

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
- Python `int` becomes `INT`, `float` becomes `FLOAT` (verify that `KVPairValue::GetFloat` of an
  `INT` value returns it as a float, so a float parameter given as `1` is read correctly).
- The view loses only the interleaving of different repeated keys (`a, b, a`); messages built from
  `kv.Section` directly keep it.
- The engine prints messages as KeyValues text in its logs; a C++ JSON printer is added only if a
  tool needs it.

## Python side

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
- A protocol documents its parameters in KeyValues text.
- The payload is a `memoryview`; a reader reads the engine layout: `read_array(np.float32, 3)` reads
  the `int32` count and makes a `np.frombuffer` view without a copy.

## Tests

`tests/network/` (GoogleTest, the `auto_tests` workspace):

- binary KeyValues: round trips of engine KeyValues files, equal bytes for equal sections, truncated
  and corrupt input (the parser must stop at the end of the message instead of reading past it);
- the Python `kv` module against bytes written by the engine, and every row of the JSON view;
- endpoints over the loopback transport: calls and replies, notifies, deferred replies, unknown
  methods, payload streams with several chunks, cancels, disconnects failing the futures, two
  endpoints in one process connected to different servers.

## Open questions

- The `Hello` contract when the protocol versions differ: refuse the peer, or let each protocol
  decide.
- Whether methods need a call timeout in the endpoint, or the caller handles it on the future.
