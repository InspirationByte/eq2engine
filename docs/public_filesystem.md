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

