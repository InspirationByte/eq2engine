# Network transport overview

The transport layer gives the engine one interface for message connections, with interchangeable
backends: TCP (local tool links such as the level editor ↔ Blender link) and GameNetworkingSockets
(multiplayer, later; it replaces the custom reliable UDP protocol `CEqRDPSocket` of
`shared/network`). The RPC layer ([network_rpc_overview.md](network_rpc_overview.md)) and the
multiplayer data channels are separate users of it.

Status: the interface, the TCP and the loopback backends are implemented and tested; the
GameNetworkingSockets backend is designed, not implemented yet.

| File | Contents |
|---|---|
| `shared/network/transport/NetTransport.h` | `INetTransport`, connection ids, events, send flags, capabilities, disconnect reasons |
| `shared/network/transport/NetTransportTCP.*` | `CNetTransportTCP`: TCP backend |
| `shared/network/transport/NetTransportLoopback.*` | `CNetTransportLoopback`: in-process backend for tests, two transports joined in memory |
| `shared/network/transport/NetTransportGNS.*` | planned: GameNetworkingSockets backend |
| `tests/network/transport_tests.cpp` | loopback and TCP tests |

The files are part of `networkLib`; the legacy files of `shared/network` (`CEqRDPSocket`,
`CNetworkThread`, `INetworkInterface`) stay until the GNS backend replaces them. On Windows the
executables using the TCP backend link `wsock32`.

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
    NET_TRANSPORT_LANE_PRIORITY = (1 << 1),    // lanes never wait for each other
};

enum ENetDisconnectReason : int
{
    NET_DISCONNECT_REMOTE = 1,          // the peer closed the connection
    NET_DISCONNECT_CONNECT_FAILED,      // the connection was never established
    NET_DISCONNECT_ERROR,               // socket or backend error
    NET_DISCONNECT_PROTOCOL,            // the peer sent broken data (bad frame, message too large)
};

struct NetLaneConfig
{
    int     priority{ 0 };      // lower first
    int     weight{ 1 };        // share among the lanes of one priority
};

struct NetTransportEvent
{
    ENetTransportEventType  type;               // NET_EVENT_CONNECTED, _DISCONNECTED, _MESSAGE
    NetConnectionId         conn;
    int                     lane{ 0 };
    int                     reason{ 0 };        // ENetDisconnectReason
    CRefPtr<CMemoryStream>  message;            // NET_EVENT_MESSAGE
};

class INetTransport : public RefCountedObject<INetTransport>
{
public:
    virtual int     GetCaps() const = 0;
    virtual int     GetMaxMessageSize() const = 0;

    virtual bool    Listen(const char* address) = 0;
    virtual NetConnectionId Connect(const char* address) = 0;
    virtual void    Close(NetConnectionId conn, int reason = 0) = 0;

    virtual bool    ConfigureLanes(NetConnectionId conn, ArrayCRef<NetLaneConfig> lanes) = 0;
    virtual bool    Send(NetConnectionId conn, int lane, const void* data, int size, int flags) = 0;
    virtual int     GetQueuedBytes(NetConnectionId conn, int lane) const = 0;

    virtual void    Poll(NetTransportEventList& events) = 0;
};
```

- **Connections**: a server accepts connections after `Listen`, each comes as `NET_EVENT_CONNECTED`.
  `Connect` returns the connection id at once (`NET_CONNECTION_NONE` only for a bad address);
  `NET_EVENT_CONNECTED` or `NET_EVENT_DISCONNECTED` with `NET_DISCONNECT_CONNECT_FAILED` follows in
  `Poll`. Messages sent before the connection is established are queued.
- **Close**: `Close` sends the queued messages and closes; no event comes for a connection closed
  locally, the peer gets `NET_EVENT_DISCONNECTED` with `NET_DISCONNECT_REMOTE`. The reason is for
  backends that pass it to the peer (GNS).
- **Messages**: a message is delivered whole or not at all; its size is at most
  `GetMaxMessageSize()`. `Send` copies the data; it returns false when the connection is closed, the
  lane is not configured or the message is too large. Larger data (RPC payloads) is split by the
  user of the transport.
- **Lanes**: messages of one lane keep their order. The lane count and priorities are set per
  connection with `ConfigureLanes` before the first `Send` of the connection (one lane by default);
  both sides configure the same lanes, the protocol on top agrees on them. Received messages carry
  their lane whatever the receiver configured.
- **Queued bytes**: `GetQueuedBytes` tells how much of a lane is not sent yet, so a user streams
  large data while the lane drains instead of queueing all of it (the RPC payload streams do).
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

`CNetTransportTCP`: non-blocking stream sockets over the engine socket definitions
(`network/net_defs.h`, winsock on Windows, BSD sockets elsewhere), one or more clients on a
listening server. Meant for local tool links (the editor ↔ Blender link listens on `127.0.0.1`), so
it has no encryption and no authentication. `GetCaps()` is 0.

- **Addresses**: `host:port`; the host is an IPv4 address or a name; an empty host is any address
  for `Listen` and the loopback address for `Connect`. Port 0 listens on a free port,
  `GetListenPort()` returns it.
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
  priority that has one (by the sent bytes over the weight among equal priorities). Users keep
  messages small (RPC payload chunks of 64 KB), so a high priority lane waits for at most one chunk.
- **Polling**: `Poll` accepts connections, completes the pending connects, reads what the sockets
  have (up to 1 MB per connection per `Poll`), splits the frames into message events and writes
  the queued frames while the sockets take them. No thread of its own: a tool link polls every
  frame.
- **Delay**: `TCP_NODELAY` is always set, frames are whole messages already; `NET_SEND_NO_DELAY` has
  nothing more to do.
- **Max message size**: 1 MB by default (constructor argument); a frame over the limit closes the
  connection with `NET_DISCONNECT_PROTOCOL`.
- **Disconnect**: a closed socket, a read or write error or a bad frame give `NET_EVENT_DISCONNECTED`
  with the reason; the queued frames of the connection are dropped.

## Loopback backend

`CNetTransportLoopback::CreatePair(first, second)` creates two transports that exchange messages in
memory. `Connect` of one side connects to the other side if it listens (the address is ignored);
the events come in the next `Poll` of each side. All messages are delivered in order, the lanes
never wait (`NET_TRANSPORT_LANE_PRIORITY`). Both transports are used from one thread. Destroying one
side disconnects the other. Used by the tests of the RPC layer and for in-process links.

## GameNetworkingSockets backend (later)

`CNetTransportGNS` over `ISteamNetworkingSockets`:

- connections are `HSteamNetConnection`, lanes are the connection lanes (`ConfigureConnectionLanes`
  with the priorities and weights of `NetLaneConfig`), so the caps are
  `NET_TRANSPORT_UNRELIABLE | NET_TRANSPORT_LANE_PRIORITY`;
- `NET_SEND_RELIABLE` maps to `k_nSteamNetworkingSend_Reliable`, otherwise unreliable;
  `NET_SEND_NO_DELAY` to `k_nSteamNetworkingSend_NoNagle`;
- `GetMaxMessageSize()` is the GNS limit (`k_cbMaxSteamNetworkingSocketsMessageSizeSend`, 512 KB,
  verify); GNS splits messages into packets itself;
- `GetQueuedBytes` is the pending reliable bytes of the lane (`GetConnectionRealTimeStatus`);
- `Poll` runs the callbacks (`RunCallbacks`) for the connection state changes and receives the
  messages of its connections (`ReceiveMessagesOnPollGroup`).

### Security and signaling

- **Encryption**: GNS connections are encrypted (key exchange and AES-GCM in the library), so the
  backend servers (rendez-vous, lobby) are reached over GNS only; the TCP backend has no encryption
  and stays for local tool links.
- **Server identity**: without Steam the game runs its own certificate authority: the servers get
  certificates signed by its key, the clients trust only that key, so a client knows it talks to the
  real server. Who the client is (accounts, tickets) is checked above the transport, by the RPC
  login of the game ([network_rpc_overview.md](network_rpc_overview.md#authorization-planned)).
- **Signaling**: peer-to-peer GNS connections exchange their ICE signaling through an outside
  service (`ISteamNetworkingConnectionSignaling`, custom signaling). The rendez-vous server is that
  service: a game protocol over RPC relays the signals between the logged-in peers (a notify such
  as `Relay { peer N; blob }`), then the peers connect directly or through a relay.

## Multiplayer data channels

The multiplayer protocols (object states, events, voice) are not RPC. They are channels on lanes of
the game connection, with their own message formats, reliable or unreliable by their data. They are
designed with the GNS backend; the transport interface gives them what they need: unreliable
messages, lane priorities and the RPC lanes of the same connection beside them.

## Tests

`network_tests` (`tests/network/transport_tests.cpp`): loopback messages, lanes, close events,
a refused connect and a destroyed peer; TCP messages queued before the connection, the lane
priority order, a 700 KB message, an empty message, the queued bytes, a close after the reply, a
refused connect and a frame over the receiver limit.

## Open questions

- Whether the TCP backend needs a thread of its own for links that are not polled every frame
  (a dedicated server tool).
