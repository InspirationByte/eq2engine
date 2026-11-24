# Network transport overview — design draft

Status: **design, not implemented.** The transport layer gives the engine one interface for message
connections, with interchangeable backends: TCP (local tool links such as the level editor ↔ Blender
link) and GameNetworkingSockets (multiplayer, later; it replaces the custom reliable UDP protocol
`CEqRDPSocket` of `shared/network`). The RPC layer ([network_rpc_overview.md](network_rpc_overview.md))
and the multiplayer data channels are separate users of it. Facts marked *verify* were not checked
yet.

| File (planned) | Contents |
|---|---|
| `shared/network/transport/NetTransport.h` | `INetTransport`, connection ids, events, send flags, capabilities |
| `shared/network/transport/NetTransportTCP.*` | TCP backend |
| `shared/network/transport/NetTransportLoopback.*` | In-process backend for tests: two transports joined in memory |
| `shared/network/transport/NetTransportGNS.*` | GameNetworkingSockets backend (later) |

The files go to `networkLib`; the legacy files of `shared/network` (`CEqRDPSocket`,
`CNetworkThread`, `INetworkInterface`) stay until the GNS backend replaces them.

## Layers

```
 RPC endpoints (shared/network/rpc)        multiplayer data channels (game protocols)
            │  reliable lanes                      │  reliable and unreliable lanes
            └──────────────┬───────────────────────┘
                     INetTransport ── connections, lanes, messages
                 ┌─────────┼──────────────┐
               TCP     Loopback          GNS (later)
```

- A transport moves whole messages over connections. It knows nothing about RPC, KeyValues or game
  protocols.
- The users of a transport are independent: an RPC endpoint owns its own transport (the editor link),
  or uses some lanes of a connection that the multiplayer code owns (RPC to the game server next to
  the data channels).

## Interface

```cpp
using NetConnectionId = uint;
static constexpr NetConnectionId NET_CONNECTION_NONE = 0;

enum ENetSendFlags : int
{
    NET_SEND_RELIABLE   = (1 << 0),    // delivered once, in order within its lane
    NET_SEND_NO_DELAY   = (1 << 1),    // send now, do not wait to merge with other messages
};

enum ENetTransportCaps : int
{
    NET_TRANSPORT_UNRELIABLE    = (1 << 0),    // unreliable messages are supported
    NET_TRANSPORT_LANE_PRIORITY = (1 << 1),    // lane priorities are kept by the backend itself
};

struct NetLaneConfig
{
    int     priority{ 0 };      // lower first
    int     weight{ 1 };        // share among the lanes of one priority
};

enum ENetTransportEventType : int
{
    NET_EVENT_CONNECTED,
    NET_EVENT_DISCONNECTED,
    NET_EVENT_MESSAGE,
};

struct NetTransportEvent
{
    ENetTransportEventType  type;
    NetConnectionId         conn;
    int                     lane{ 0 };
    int                     reason{ 0 };        // disconnect reason
    CRefPtr<CMemoryStream>  message;            // NET_EVENT_MESSAGE
};

class INetTransport : public RefCountedObject<INetTransport>
{
public:
    virtual int     GetCaps() const = 0;
    virtual int     GetMaxMessageSize() const = 0;

    // server: accepts connections on the address; client: connects to it
    virtual bool    Listen(const char* address) = 0;
    virtual NetConnectionId Connect(const char* address) = 0;
    virtual void    Close(NetConnectionId conn, int reason = 0) = 0;

    virtual bool    ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes) = 0;

    // copies the data; false when the connection is closed or the message is too large
    virtual bool    Send(NetConnectionId conn, int lane, const void* data, int size, int flags) = 0;

    // runs the backend: accepts, receives, sends queued data; appends the events since the last call
    virtual void    Poll(Array<NetTransportEvent>& events) = 0;
};
```

- **Messages**: a message is delivered whole or not at all; its size is at most
  `GetMaxMessageSize()`. Larger data (RPC payloads) is split by the user of the transport.
- **Lanes**: messages of one lane keep their order; lanes do not wait for each other where the
  backend allows it. The lane count and priorities are set per connection with `ConfigureLanes`
  before the first `Send` of the connection; both sides must configure the same lanes (the protocol
  on top agrees on them).
- **Reliability**: `NET_SEND_RELIABLE` is required on a backend without `NET_TRANSPORT_UNRELIABLE`
  (TCP); there an unreliable send is refused, so a user of unreliable data checks the caps.
- **Threads**: `Poll` and `Send` are called from one thread, the owner's. A backend may receive on
  its own thread and hand the messages over in `Poll`; the events never come from another thread.
- **Ownership of messages**: the received message is a `CRefPtr<CMemoryStream>` positioned at its
  start; the user may keep it.
- **Connection ids** are unique within the transport for its lifetime, never reused.
- **Addresses** are strings in the backend format (`127.0.0.1:27100` for TCP, a GNS address or
  identity for GNS).

## TCP backend

`CNetTransportTCP`: plain stream sockets, one or more clients on a listening server. Meant for local
tool links (the editor ↔ Blender link listens on `127.0.0.1`), so it has no encryption and no
authentication. The socket library is not chosen yet (see [Open questions](#open-questions)).

- **Frames**: the stream is a sequence of frames, a frame is one message of one lane:

  ```cpp
  struct NetTCPFrameHeader      // little-endian
  {
      uint32  size;             // message bytes after the header, up to GetMaxMessageSize()
      uint16  lane;
      uint16  flags;            // reserved, 0
  };
  ```

- **Lanes**: one TCP stream carries all lanes, so a frame being sent delays the frames behind it.
  The backend keeps a send queue per lane and takes the next frame from the lane of the highest
  priority that has one (by weight among equal priorities). Users keep messages small (RPC payload
  chunks of about 64 KB), so a high priority lane waits for at most one chunk.
- **Polling**: sockets are non-blocking; `Poll` accepts connections, reads what the sockets have,
  splits the frames into message events and writes the queued frames while the sockets take them.
  No thread of its own is needed: a tool link polls every frame.
- **Max message size**: 1 MB by default; a frame over the limit closes the connection (a broken or
  foreign peer).
- **Disconnect**: a closed socket, a read or write error or a bad frame give `NET_EVENT_DISCONNECTED`
  with the reason; the queued frames of the connection are dropped.

## Loopback backend

`CNetTransportLoopback`: two transports created as a pair exchange messages in memory, all lanes
reliable, with optional unreliable drops and delays for tests. Used by the tests of the RPC layer
(`tests/network/`) and for in-process links.

## GameNetworkingSockets backend (later)

`CNetTransportGNS` over `ISteamNetworkingSockets`:

- connections are `HSteamNetConnection`, lanes are the connection lanes (`ConfigureConnectionLanes`
  with the priorities and weights of `NetLaneConfig`), so the caps are
  `NET_TRANSPORT_UNRELIABLE | NET_TRANSPORT_LANE_PRIORITY`;
- `NET_SEND_RELIABLE` maps to `k_nSteamNetworkingSend_Reliable`, otherwise unreliable;
  `NET_SEND_NO_DELAY` to `k_nSteamNetworkingSend_NoNagle`;
- `GetMaxMessageSize()` is the GNS limit (`k_cbMaxSteamNetworkingSocketsMessageSizeSend`, 512 KB,
  verify); GNS splits messages into packets itself;
- `Poll` runs the callbacks (`RunCallbacks`) for the connection state changes and receives the
  messages of its connections (`ReceiveMessagesOnPollGroup`).

## Multiplayer data channels

The multiplayer protocols (object states, events, voice) are not RPC. They are channels on lanes of
the game connection, with their own message formats, reliable or unreliable by their data. They are
designed with the GNS backend; the transport interface gives them what they need: unreliable
messages, lane priorities and the RPC lanes of the same connection beside them.

## Open questions

- The TCP socket library: SDL2_net (SDL2 is already a dependency), standalone Asio, or a small own
  wrapper of BSD sockets / winsock. ZeroMQ or nng would replace the frame format with their own
  messages, but their Python sides (`pyzmq`, `pynng`) must then be bundled with the Blender addon.
- Whether the TCP backend needs a thread of its own for links that are not polled every frame
  (a dedicated server tool).
