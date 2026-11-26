# BLINK overview

BLINK (Blender link) is a live link of an engine application with Blender over the RPC layer
([network_rpc_overview.md](network_rpc_overview.md)). The engine application is the server, a
Blender addon is the client. The core has what every such link needs: the server with one client,
the handshake, the reconnect, the engine ↔ Blender coordinates, meshes, and the Blender menu with
the connection; the application adds its own protocols on top. The first user is the level editor
of the game (`docs/blender_link_design.md` of the game repository).

Status: implemented. The Python side is checked by its tests and in background Blender against a
Python stand-in of the server; the C++ server is built into the level editor, not run against
Blender since it moved here from the editor.

| File | Contents |
|---|---|
| `shared/blink/BlinkServer.*` | `CBlinkServer`: the RPC server on `127.0.0.1:<port>`, one client, `blink.Hello`, the application protocols |
| `shared/blink/BlinkMesh.*` | `BlinkMesh`: triangles with material batches, written as a KeyValues result and a payload |
| `tools/python/blink/client.py` | `blink.Client`: the connection, `blink.Hello`, waiting and reconnect, the state for the UI (no bpy) |
| `tools/python/blink/coords.py` | Engine ↔ Blender coordinates of points, object transforms and meshes (no bpy) |
| `tools/python/blink/mesh.py` | `read_mesh`: the `blink.Mesh` of a `BlinkMesh` reply, in Blender coordinates (numpy) |
| `tools/python/blink/rpc.py` | The `eqrpc` import: a top-level package here, a sibling package inside an addon |
| `tools/python/blink/blender/` | The Blender part (bpy): `Addon` (preferences, Connect / Disconnect, the menu and the panel, the timer, auto-connect), `mesh.build_mesh`, `mesh.box_mesh` |
| `tools/python/blink/tests/` | unittest tests: the coordinates, the client against a Python server |

The C++ files are `blinkLib` (uses `networkLib`).

## Layers

```
application protocols (the level editor: session, defs, objects, ...)
        │
CBlinkServer ── blink.Client ── blink.blender.Addon (menu, preferences, auto-connect)
        │
RPCEndpoint / RPCProtocol (eqrpc in Python)
        │
CNetTransportTCP on 127.0.0.1
```

## Server

```cpp
CBlinkServer server{ "LevelEditor", "1" };	// the app name and version of the Hello reply
server.AddProtocol(session);				// before Start, the protocols live while it runs
server.SetClientCallback([](bool connected) { ... });
server.Start(BLINK_DEFAULT_PORT);			// 127.0.0.1:27100, false when the port is busy
server.Update();							// main thread, every frame: methods and callbacks run here
server.Stop();
```

- One client: a second connection is closed after the endpoint update.
- `GetState`: `BLINK_SERVER_STOPPED`, `BLINK_SERVER_FAILED` (the port could not be listened on),
  `BLINK_SERVER_LISTENING`, `BLINK_SERVER_CONNECTED`. `GetClient` is the connection for
  `RemoteCall` / `RemoteNotify` of the application protocols.
- The client callback runs from `Update` when the client connects and disconnects, and from `Stop`
  when a client was connected (the endpoint is destroyed without its callbacks). It is not called
  from the destructor.
- `Start` and `Stop` create and destroy the endpoint; the protocols are added to each new one, so a
  restart (another port) keeps them.

## Handshake: blink.Hello

| Method | Direction | Contents |
|---|---|---|
| `blink.Hello` | client → server | `client`, `clientVersion` (the addon), `host`, `hostVersion` (Blender) → `app`, `appVersion` |

The client calls it right after connect; the protocol list and versions go through `rpc.Hello` of
the endpoint. The server logs the names and keeps them (`GetClientName`, `GetHostVersion`, ...).
After the reply the client is `Connected` and the application starts its sync.

## Meshes

`BlinkMesh` holds engine coordinates: `positions`, `normals`, `texcoords`, `indices` (triangles)
and batches; `BeginBatch(material)` starts a batch at the current index count. `Write(out, payload)`
adds a `batch` section per batch (`material`, `firstIndex`, `indexCount`) to the result and writes
the four arrays to the payload with `IFileStream::WriteObj` (an int count and the items each).

`blink.read_mesh(result, payload)` reads it into a `blink.Mesh` in Blender coordinates (numpy
views, `coords.mesh_to_blender`); `blink.blender.mesh.build_mesh(bpy_mesh, mesh, on_material_created,
weld_distance, quads)` fills a Blender mesh: a material slot per material name, the UV map, the
custom normals. The engine vertices are split where the UVs or normals change; `build_mesh` welds the
vertices at one position (`WELD_DISTANCE`, 0.1 mm), keeps the UVs and normals per face corner and
drops the triangles that become degenerate, so the mesh is connected for editing and looks the same.
`quads` joins the flat triangle pairs of one material with matching UVs into quads
(`bmesh.ops.join_triangles`, the custom normals are kept). Sending a mesh back has to triangulate
and split it again.

`blink.blender.mesh.material(name, on_created)` is the Blender material of an engine material,
found by the full name in the `eq_material` property: Blender names are limited to 63 bytes and get
`.001` when taken, so a lookup by name could mix two engine materials or copy one. `on_created`
fills a new one, `fill_material(material, texture_path)` makes a Principled BSDF with
the texture (`images.load(check_existing=True)`: one image per file; the image alpha mode is None,
the engine textures keep other data in the alpha). `show_object_colors()` turns
the Solid shading of the 3D views to the Object color type, for applications that color objects by
`obj.color`.

## Coordinates

The protocols use engine coordinates; only the Blender side converts (`blink.coords`). The engine
is Y up and left handed, Blender is Z up and right handed, 1 unit is 1 m: Blender `(x, y, z)` is
engine `(-x, z, -y)`. The conversion is a reflection, so the triangle winding is reversed and the
texture V is `1 - v`. An object rotation is sent as the images of the engine unit axes (`axisX`,
`axisY`, `axisZ`), so there is no Euler order to agree on; `object_to_blender` and
`object_to_engine` convert a transform (with an optional placement offset) to the rows of the
Blender world matrix and back, dropping the scale.

## Python client

```python
class AppLink(blink.Client):
    def create_protocols(self):     # new protocols for each connection attempt
        return [Protocol("session")]
    def on_ready(self): ...         # blink.Hello is answered
    def on_closed(self): ...        # the endpoint is closed (disconnect, reconnect)
    def on_update(self): ...        # from update while connected and not waiting

link = AppLink("My addon", "0.1", "Blender", bpy.app.version_string, server_title="the editor")
link.connect("127.0.0.1:27100", reconnect_interval=5.0)
link.update()                       # main thread
```

- States: `Disconnected`, `Connecting`, `Waiting` (for the server), `Connected`, `Failed`.
  `status_text` gives the shown text ("Waiting for the editor"); an application shows its own
  sync states by overriding it. `ui_state` is what the UI shows; the timer redraws when it
  changes.
- When `blink.Hello` fails because nothing listens, or the server closes the link (it quit or was
  restarted), the client is `Waiting` and opens a new endpoint every `reconnect_interval` seconds
  until `disconnect`. The waiting endpoint is closed by the next attempt, not in the endpoint
  callback that noticed the loss.
- `report_error(what, error)` logs and shows a failed call, except the failures of a lost link;
  `fail` also sets `Failed`.

## Blender part

`blink.blender.Addon(package, client, ns, title="BLINK", draw_status=None, draw_commands=None,
props=None, classes=())` makes the bpy classes of one addon (`props`: the preferences of the
application, `{name: bpy property}`, shown after the BLINK ones in the preferences and the menu); the addon calls its `register` / `unregister`.
`ns` is the lowercase operator prefix, unique per addon (`leveleditor_blink`), so several BLINK
addons can be enabled together.

- Preferences (of `package`): Server host, Server port (`BLINK_DEFAULT_PORT` by default),
  Auto-connect (on), Reconnect interval (s) (5).
- Operators `<ns>.connect` (from the menu it asks for the host and the port) and `<ns>.disconnect`.
- The top bar menu after Help, titled `<title>: <state>`, and the View3D sidebar panel `<title>`:
  the state, the server address, the lines of `draw_status`, the last error, Connect... or
  Disconnect, the commands of `draw_commands`, then Auto-connect and Reconnect interval.
- A timer (30 ms) calls `client.update` and redraws the TOPBAR and VIEW_3D areas when `ui_state`
  changes; it stops when the client is not active.
- Auto-connect connects after register, after a file is opened (`load_post`) and when it is
  switched on; `load_pre` disconnects (what the link made is gone with the old file). Disconnect
  stops the link until the next start, file or Connect. Background Blender (`-b`) never connects
  by itself.

## Packaging

A Blender extension cannot import packages outside itself, so an addon build copies
`tools/python/eqrpc` and `tools/python/blink` (without `tests`) into the addon package; `blink.rpc`
imports eqrpc relatively there and as a top-level package in this repository. The level editor
addon of the game does it in its `build_addon.py`.

## Tests

```
cd tools/python
python -m unittest discover -s blink/tests -t .
```

The client tests run a Python server with `blink.Hello` on 127.0.0.1: the handshake, waiting for
a server that is not running yet, the reconnect after the server restarts. The mesh coordinate
test needs numpy and is skipped without it. CI runs them after the eqrpc tests.
