# File system — `public/core/IFileSystem.h`

`g_fileSystem` (implemented in `core/e2core/FileSystem.cpp`) is the only way engine code should
access files: it resolves paths through layered search locations and mounted packages.

```cpp
#include "core/IFileSystem.h"
```

## Search locations

The file system searches files through layered locations. Search flags (`ESearchPath`):

| Flag | Location |
|---|---|
| `SP_MOD` | Game directory (`DefaultGameDir`, e.g. `GameData`), additional search paths and mounted packages |
| `SP_DATA` | Engine data directory (`EngineDataDir`, e.g. `E2Base`) |
| `SP_ROOT` | Relative to the base path (working directory) |
| `SP_DIRECT` | Path used as is, base path ignored |
| `SP_IGNORE_PACKAGE` | Do not look into packages |

`searchFlags = -1` (default) means `SP_MOD | SP_DATA | SP_ROOT`, searched in this order; absolute
paths are always opened directly.

## Files

```cpp
IFileStreamPtr file = g_fileSystem->Open("scripts/sounds.def", FS_OPEN_READ, SP_MOD);
if (!file)
	return false;

IFileStreamPtr out = g_fileSystem->Open("cfg/config.cfg", FS_OPEN_WRITE, SP_ROOT);
out->Print("bind %s %s\n", key, command);
```

- `Open(fileName, openFlags, searchFlags)` returns a ref-counted `IFileStreamPtr` (`FS_OPEN_READ`,
  `FS_OPEN_WRITE`, `FS_OPEN_APPEND`); `nullptr` if not found. The stream is closed when the last
  reference is released. See `IFileStream` in [framework_ds.md](framework_ds.md).
- `FileExist`, `FindFilePath` (full path of the first match), `FileRemove`, `FileCopy`, `Rename`.
- Directories: `DirExist`, `MakeDir`, `RemoveDir`.
- Paths: `GetBasePath`, `SetBasePath`, `GetCurrentGameDirectory`, `GetCurrentDataDirectory`,
  `GetAbsolutePath(search, name)`.
- `AddSearchPath(pathId, dir)` / `RemoveSearchPath(pathId)` — extra `SP_MOD` directories identified
  by an id (e.g. the localization path `$LOCALIZE$`).
- Deprecated: `GetFileBuffer` (returns a buffer to release with `PPFree`), `GetFileSize`,
  `GetFileCRC32` — use `Open` and the stream API in new code.

## Packages

- `AddPackage(name, searchPath, mountPath)` / `RemovePackage(name)` — mount a package (`.epk`,
  zip) as another layer of the given search path.
- `SetAccessKey(key)` — key for encrypted packages.
- `OpenPackage(name, searchFlags)` — open a package for direct reading without mounting it;
  returns `IPackFileReaderPtr` with `GetFileCount`, `FileExist`, `FindFileIndex`,
  `Open(name or index, flags)`.

## Searching

```cpp
CFileSystemFind fsFind("materials/*.mat", SP_MOD);
while (fsFind.Next())
{
	if (fsFind.IsDirectory())
		continue;
	Msg("%s\n", fsFind.GetPath());
}
```

`CFileSystemFind(wildcard, searchPaths, dirIndex)` iterates matching files and directories;
`GetPath()` returns the name of the current entry, `GetDirIndex()` the search path index it came
from.


## Source control

In tools and the editor the file system can keep written files under source control. Only
Perforce (`p4`) is supported; it runs the `p4` command line client found in `PATH`, in the
Perforce installation directory (`P4INSTROOT`, Windows) or set by `Executable`. Profile and Retail
builds do not create the provider.
The implementation is in `core/e2core/SourceControl.cpp` and `core/e2core/SourceControlP4.cpp`.

Configuration in `E2.CONFIG`:

```
FileSystem
{
	...
	UseSourceControl 	p4;					// provider name, selects the section below
}

SourceControl p4
{
	Applications "LevelEditor" "egfMan" "texcooker";	// optional, all applications when missing
	DefaultChangelistDesc	"+auto $APP_NAME$";	// pending changelist for written files
	AddNewFiles 1;							// optional, 'p4 add' new files (default 1)
	Executable "C:/Program Files/Perforce/p4.exe";	// optional, path to p4
}
```

- The provider is created in `Init` (`ISourceControl::Create`) when `UseSourceControl` is set and the application name is in
  `Applications`; `-nosourcecontrol` on the command line disables it. The `editorMode` argument of
  `Init` is not used for this.
- `p4` runs in the base directory, so the client, port and user come from `P4CONFIG`, `p4 set` or
  the environment. The connection is checked on the first write; when the server is not reachable
  or the user is not logged in, a warning is printed and source control is off until restart.
- Only files under the client root are handled. Files outside the client view or matched by
  `P4IGNORE` are written as usual.

| Operation | Perforce action |
|---|---|
| `Open` with `FS_OPEN_WRITE` / `FS_OPEN_APPEND`, file in depot | `p4 edit` before the file is opened |
| `Open` for writing creates a file | `p4 add` (or `p4 edit` if the file is in depot) |
| `FileCopy` | goes through `Open` |
| `FileRemove`, file in depot | `p4 delete`; a file opened for add is reverted and removed |
| `Rename`, file in depot | `p4 edit` + `p4 move`; other files are renamed and added, directories are renamed without source control |
| `CheckoutSourceControlFiles`, file | `p4 edit`, for writers that do not use `Open` (`CDPKFileWriter::Begin`) |
| `CheckoutSourceControlFiles`, directory | `p4 fstat`, `p4 edit` and `p4 reopen` of `<dir>/...`: all depot files of the directory |
| `AddSourceControlFile` | `p4 add` of a file created without `Open` (`p4 edit` if it is in depot) |

`IsSourceControlEnabled` connects on the first call and tells whether written files are handled;
the level editor uses it to skip its project backup.

A directory checked out with `CheckoutSourceControlFiles` keeps the states of all its depot files
in the cache, so writing files there does not query the server per file; a file missing in its
listing is treated as not in depot. Checking out the directory again refreshes the listing.

Files are opened in a pending changelist with the `DefaultChangelistDesc` description
(`$APP_NAME$` is replaced with the application name). An existing pending changelist of the client
and user with the same description is reused, otherwise it is created on the first file.

`SourceControlScope` sends the files of the calling thread to another changelist while it exists.
Scopes nest; a file already opened in another changelist is moved to the scope changelist with
`p4 reopen`:

```cpp
SourceControlScope scScope("+export %s", levelName.ToCString());
SaveLevelFiles(levelName);	// files go to the '+export <level>' changelist
```

New files written inside a scope are added by one `p4 -x - add` when the scope ends; outside a
scope each file is added at once. Checked out files written inside a scope are passed to one
`p4 -x - revert -a` when the scope ends, so files written without changes leave the changelist;
outside a scope they stay opened. A changelist left without files is not deleted. Work split across frames or jobs opens the scope again in each
part; the changelist is found by its description.

File states are cached. `fs_sc_status` prints the provider state, `fs_sc_reset` drops the cache
after changes made in P4V or the command line (a read-only file is always queried again).
