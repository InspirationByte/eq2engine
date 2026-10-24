# Localization — `public/core/ILocalize.h`

`g_localizer` (implemented in `core/e2core/Localize.cpp`) maps text tokens to localized wide
strings.

```cpp
#include "core/ILocalize.h"
```

## Language and files

- The current language is taken from the `language` ConVar (archived); if it is empty,
  `RegionalSettings.DefaultLanguage` from `E2.CONFIG` is used (fallback `english`). The name is
  lower-cased.
- The language also adds a `$LOCALIZE$` search path `<game dir>_<language>` to the file system, so
  localized assets can override regular ones.
- Token files are `resources/text_<language>/<prefix>.txt`, loaded with `AddTokensFile(prefix)`
  and unloaded with `RemoveTokensFile(prefix)`. `core` is loaded at initialization; subsystems
  and games add their own prefixes.
- Changing `language` at runtime reloads all loaded token files (`ReloadLanguageFiles()`).

## File format

Token files use the KeyValues syntax (parsed with `KeyValues::Tokenizer`, see
[keyvalues.md](keyvalues.md)): a token name followed by one quoted text, terminated by `;`.

```
NFS3_TACHOMETER_SETTINGS	"NFS 3 Tachometer Settings";
NFS3_TACHOMETER_SPEEDO		"Speedometer < %s >";
NFS3_TACHOMETER_KPH			"KM/H";
```

- Files must be UTF-8 (a UTF-8 BOM is skipped; UTF-16 files are rejected).
- `\n` and `\t` inside the text become a new line and a tab.
- `%s` in texts is converted to `%ls`, so localized strings can be used directly as wide format
  strings with wide string arguments.
- Token names are case-insensitive (hashed). A token defined twice prints a warning; the first
  definition wins.

## API

- `GetTokenString(token, defaultText)` — wide text of a token, or `defaultText` if missing
  (a `DEVMSG_LOCALE` developer message is printed).
- `GetToken(token)` — `ILocToken*` (`GetToken()` name, `GetText()` text) or `nullptr`.
- `AddToken(token, text)` — add a token at runtime (UTF-8 or wide text); such tokens survive
  language reloads.
- `SetLanguageName(name)`, `GetLanguageName()`, `ReloadLanguageFiles()`.

## Helpers

```cpp
// strings starting with '#' are tokens; other strings are returned as is (converted to wide)
const wchar_t* title = LocalizedString("#MENU_TITLE_PRESS_ENTER");

// token with a default text
const wchar_t* text = DKLOC("MENU_QUIT", "Quit");
```

- `LocalizedString(str)` — for strings coming from data (UI files, scripts): `#TOKEN` is
  looked up, plain strings are shown untranslated (with a `DEVMSG_LOCALE` message). It returns a
  static buffer: copy the result if it must be kept.
- `DKLOC(token, default)` — for tokens referenced from code; the default is a narrow literal
  turned into a wide literal by the macro.
- Built-in token `INSCOPYRIGHT` holds the copyright string.
