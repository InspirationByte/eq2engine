# KeyValues

`framework/utils/KeyValues.h` implements the engine's lightweight JSON-like key-value format. It is
used for configuration (`E2.CONFIG`), UI layouts (`.res`), localization, shader package info and
most other text data. The file is part of `frameworkLib` and is not included by `core_common.h`:

```cpp
#include "utils/KeyValues.h"
```

## Data model

Everything is a `KVSection`:

- `name` — key name;
- `values` — zero or more typed values (`KVPairValue`: string, int, float, bool, or a nested section);
- `keys` — nested child sections (when the key has a `{ ... }` block).

A parsed file is a root `KVSection` whose `keys` are the top-level keys of the file. Key names are
matched case-insensitively by hash.

## Text format (V2, default)

```
// DO NOT DELETE

ApplicationDebug
{
	// FullCrashDumps;
	PrintLeaksOnExit;					// key without values
	ForceLogApplications				// key with several values (array)
		"DrvSyn"
		"Editor"
		"egfMan";
}

FileSystem
{
	EngineDataDir		"E2Base";		// key with a single value
	DefaultGameDir	 	"GameData";
	UseSourceControl 	p4;				// unquoted value
}

SourceControl p4						// key with a value and a nested section
{
	AutoCheckoutPrompt 0;
	ChangelistDesc	"+auto $APP_NAME$";
}
```

- A key is followed by values separated by whitespace; the entry ends with `;` or with a `{ ... }`
  section.
- Values may be unquoted (no spaces or special characters) or quoted; quoted strings support
  escapes such as `\n`, `\t`, `\"`.
- `//` and `/* ... */` comments.
- Values are stored as strings in V2; they are converted when read as `int`/`float`/`bool`.

## Text format (V3, `$schema_kv3`) — experimental

V3 is experimental and not widely used; prefer V2 for new files unless V3 features are needed.

A file starting with `$schema_kv3` is parsed with the V3 rules, which add typed values, arrays and
section values:

```
$schema_kv3

panel
{
	position 		int: [0, 0];					// typed array
	size 			int: [640, 480];

	child [file "resources/ui_partial/menu_fonts.res"];	// array of values

	child [image "background"]						// key with values and a section
	{
		path			"ui/ui_menu_bg1";
		align			[vcenter, hcenter];
		scaling			aspecth;
		textWeight		float: 0.05;				// typed single value
	}

	%script											// '%' key: {} content is a raw string
	{
		function step(dt)
			ui.fade.transform.position = vec2(1,2)
		end
	}
}
```

- `type: value` or `type: [v1, v2, ...]` — typed values; types are `int`, `float`, `bool`
  (strings are the default).
- `[a, b, c]` — array of values; elements are separated by `,` or whitespace.
- The same key name may repeat (`child` above); iterate with `Keys("child")`.
- `%` at the start of a key name (`%script`) means the content inside the following `{ }` is not
  parsed and is stored as a raw string value (used for embedded scripts). The key name is stored
  without `%`.

## Loading and saving

```cpp
KVSection root;

// from a file stream
IFileStreamPtr file = g_fileSystem->Open("resources/ui_title.res", FS_OPEN_READ, SP_MOD);
if (!file || !KeyValues::Parse(file, root))
	return false;

// from a memory buffer
KeyValues::ParseText(buffer, bufferSize, root, fileName);

// writing
KeyValues::WriteText(outStream, root);
KeyValues::WriteBinary(outStream, root);	// binary form, read back with ParseBinary
```

`KV_LoadFromFile` is deprecated; open the stream through `g_fileSystem` and call `KeyValues::Parse`.
Parse errors are reported to the log with the file name and line.

## Reading values

### Finding keys

```cpp
// Get: returns an empty default section when the key is missing, never null
const KVSection& fsSec = root.Get("FileSystem");

// FindSection: returns nullptr when the key is missing
const KVSection* regionalCfg = root.FindSection("RegionalSettings", KV_FLAG_SECTION);
if (!regionalCfg)
	return;
```

Search flags (`EKVSearchFlags`, also accepted by `Keys()`): `KV_FLAG_SECTION` — key must have
nested keys, `KV_FLAG_NOVALUE` — key must have no values, `KV_FLAG_ARRAY` — key must have more than
one value.

### Typed getters

`GetValues(out...)` reads consecutive values into the given variables and returns how many were
read. Variables keep their previous values when the key has fewer values, so initialize them with
defaults first.

```cpp
EqString language = "english";
regionalCfg->Get("DefaultLanguage").GetValues(language);

int maxLines = 10;
bool enable = true;
consoleSec.Get("MaxLines").GetValues(maxLines);
consoleSec.Get("Enable").GetValues(enable);

IVector2D position;							// vector types consume 2/3/4 values
panelSec.Get("position").GetValues(position);

EqString type, name;
childSec.GetValues(type, name);				// child [image "background"] -> "image", "background"
```

- `GetValuesAt(index, out...)` starts reading at a value index.
- `TryGetValues(out...)` / `TryGetValuesAt(...)` return a `KVValues` object that writes the results
  to the variables when destroyed; call `Cancel()` on it to discard them. Convert it to `int` for
  the number of values read.
- Supported types: `const char*`, `EqString`, `EqStringRef`, `int`, `uint`, `int8`…`uint16`,
  `float`, `bool`, `TVec2D`/`TVec3D`/`TVec4D` (`Vector2D`, `IVector2D`, ...), `MColor`.
- `KV_GetValueString`, `KV_GetValueInt`, `KV_GetVector3D` and other `KV_Get*` functions are
  deprecated for direct use; prefer `GetValues`.

### Iterating

```cpp
// all keys of a section
for (const KVSection& key : root.Keys())
	Msg("%s\n", key.GetName());

// keys with a given name only
for (const KVSection& childSec : panelSec.Keys("child"))
	...

// values of a key as a given type
for (EqStringRef appName : debugSec.Get("ForceLogApplications").Values<EqStringRef>())
	...

for (Vector2D point : sec.Get("points").Values<Vector2D>())	// steps by 2 values
	...
```

Low-level access: `KeyCount()`, `KeyAt(i)`, `ValueCount()`, `ValueAt(i)` (`KVPairValue` with
`GetString`, `GetInt`, `GetFloat`, `GetBool`), `IsSection()`, `IsArray()`, `GetType()`.

## Building and modifying

```cpp
KVSection root;
KVSection& sec = root.CreateSection("Console");
sec.SetKey("MaxLines", 10);		// creates or replaces the key
sec.SetKey("Enable", true);
sec.AddKey("Font", "Roboto");		// always adds a new key, even if the name exists

KVSection& langs = root.CreateSection("Languages");
langs.AddValue("English");
langs.AddValue("Russian");
```

- `SetKey` / `AddKey` accept strings, numbers, bools and vectors and return the section itself, so
  calls can be chained: `section.SetKey("model", path).SetKey("castShadow", false);`
- `AddValue` / `SetValue(value, idx)` edit the values of a key.
- `RemoveSectionByName(name, removeAll)`, `RemoveSection(sec)`, `Clear()`, `ClearValues()`.
- `Clone()` (new section outside the pool), `CopyTo(dest)`, `MergeFrom(base, recursive)`.

## Description-based parsing (`KeyValuesDesc.h`)

Plain structures can be filled from a section with field descriptions. Field names in the
description are the key names in the file; fields whose keys are missing keep their default values.
Reference usage: `tests/ds/keyvalues_tests.cpp`.

```cpp
enum ETestFlags : int
{
	TEST_FLAGS_NONE,
	TEST_FLAG1 = (1 << 0),
	TEST_FLAG2 = (1 << 1),
	TEST_FLAG3 = (1 << 2),
};

BEGIN_KEYVALUES_FLAGS_DESC(ETestFlags)
	KV_FLAG_DESC(ETestFlags::TEST_FLAG1, "flag1")
	KV_FLAG_DESC(ETestFlags::TEST_FLAG2, "flag2")
	KV_FLAG_DESC(ETestFlags::TEST_FLAG3, "flag3")
END_KEYVALUES_FLAGS_DESC

struct EmbeddedDef
{
	DEFINE_KEYVALUES_DESC();
	int embeddedFlags{ TEST_FLAGS_NONE };
};

BEGIN_KEYVALUES_DESC(EmbeddedDef)
	KV_DESC_FLAGS(embeddedFlags, ETestFlags)
END_KEYVALUES_DESC

struct GeomInstanceDef
{
	DEFINE_KEYVALUES_DESC();
	Array<EqString>		bodyGroups{ PP_SL };
	Array<EmbeddedDef>	arrayOfEmbedded{ PP_SL };
	EqString			model;
	EmbeddedDef			embedded;
	bool				staticShadowmap{ false };
	bool				castShadow{ true };
};

BEGIN_KEYVALUES_DESC(GeomInstanceDef)
	KV_DESC_FIELD(model)
	KV_DESC_FIELD(staticShadowmap)
	KV_DESC_FIELD(castShadow)
	KV_DESC_EMBEDDED(embedded)
	KV_DESC_ARRAY_FIELD(bodyGroups)
	KV_DESC_ARRAY_EMBEDDED(arrayOfEmbedded)
END_KEYVALUES_DESC

// usage
GeomInstanceDef def;
KeyValues::ParseDesc(def, section);
```

Matching file contents:

```
model				"weapons/w_har77.egf";
castShadow			0;
staticShadowmap		1;
embedded
{
	embeddedFlags	flag1 flag3;
}
bodyGroups			body1 hat case;
arrayOfEmbedded
{
	embed1 { embeddedFlags flag1 flag3; }
	embed2 { embeddedFlags flag2; }
}
```

- `DEFINE_KEYVALUES_DESC()` goes inside the structure declaration; `BEGIN_KEYVALUES_DESC` /
  `END_KEYVALUES_DESC` and `BEGIN_KEYVALUES_FLAGS_DESC` / `END_KEYVALUES_FLAGS_DESC` go in a `.cpp`.
- For `KV_DESC_ARRAY_EMBEDDED` the names of the nested keys (`embed1`, `embed2`) are not used; each
  nested key becomes one array element in order.

| Macro | Field kind |
|---|---|
| `KV_DESC_FIELD(name)` | Value(s) of key `name` read with `GetValues` |
| `KV_DESC_EMBEDDED(name)` | Nested structure with its own `Desc`, read from section `name` |
| `KV_DESC_ARRAY_FIELD(name)` | `Array<T>` or C array filled from the values of key `name` |
| `KV_DESC_ARRAY_EMBEDDED(name)` | `Array<T>` or C array of structures, one per nested key of `name` |
| `KV_DESC_FLAGS(name, enumType)` | Bit flags combined from value names, matched case-insensitively |

`DEFINE_KEYVALUES_DESC_TYPE(descName)` / `BEGIN_KEYVALUES_DESC_TYPE(classname, descName)` define a
description outside the structure.

## Custom tokenizing

`KeyValues::Tokenizer(buffer, size, fileName, startLine, callback)` runs only the lexer and calls
the callback for each token (`t` text token, `b` break, `s+`/`s-` section depth, ...). Return
`KV_PARSE_RESUME` to continue. It is used for formats that only borrow the KeyValues syntax, such as
localization files (`core/e2core/Localize.cpp`).
