# Tools overview — `tools/`

Content tools of the engine. The C++ tools are built when `ENABLE_TOOLS` is on (Windows and Linux;
`egfman` only on Windows with `ENABLE_GUI_TOOLS`) and are not built in `Profile` / `Retail`
configurations (see [architecture.md](architecture.md)).

All C++ tools initialize `e2Core` like a regular application: they read `E2.CONFIG` and use the
engine file system, so run them from the game root directory. Their configuration files
(`*.CONFIG`) are KeyValues files in that directory (see [keyvalues.md](keyvalues.md)). Console
commands and ConVars can be passed with `+name value` on the command line.

| Tool | Kind | Input | Output |
|---|---|---|---|
| `egfca` | console | `.esc` model compile scripts + source models | `.egf` models, `.pod` physics object data |
| `animca` | console | `.asc` animation compile scripts + source animations | `.mop` motion packages |
| `egfman` | GUI (wxWidgets, Windows) | `.egf`, `.mop`, `.pod` | — (viewer) |
| `fcompress` | console | `PackageCooker.CONFIG` | `.epk` packages |
| `shadercooker` | console | `ShaderCooker.CONFIG` + Slang shader sources | Shader packages for RHI backends (`.shd`, DPK format) |
| `texcooker` | console | `TextureCooker.CONFIG` + materials + source images + `.atl` atlas descriptions | Batch-converted textures (`.dds`) via external converters, texture atlases (`.atlas` + material) |
| `localization_tool` | GUI (C#, WinForms) | `resources/text_<language>/*.txt` | Edited localization files |

## egfCA — EGF compiler (`tools/egfca`)

Compiles models into EGF (Entity Geometry Format, see [public_egf_overview.md](public_egf_overview.md))
and POD (Physics Object Data).

```
egfca <script1.esc> [<script2.esc> ...]
```

- Each `.esc` script (KeyValues) describes the model: source geometry (ESM, FBX, OBJ), LODs,
  body groups, materials and skins, bones, attachments, IK chains, motion packages and physics
  objects.
- Built on `egfLib` (`shared/egf`: `CEGFGenerator`, `CEGFPhysicsGenerator`, source loaders).
- On error the tool waits for a key press before exiting.

## animCA — motion package compiler (`tools/animca`)

Compiles animations into motion packages (`.mop`).

```
animca <script1.asc> [<script2.asc> ...]
```

- Each `.asc` script (KeyValues) lists source animations (ESA, FBX), sequences, events and pose
  controllers for a model.
- Built on `egfLib` (`CMotionPackageGenerator`) and `studioLib`.

## EGFMan — model viewer (`tools/egfman`)

wxWidgets application for inspecting EGF models: geometry, LODs and body groups, materials and
skins, skeleton, animations and sequences, physics objects. Built on `fontLib`, `physicsLib`,
`dkPhysicsLib`, `renderUtilLib`, `animatingLib`.

## FCompress — package builder (`tools/fcompress`)

Builds `.epk` data packages (DPK format, `shared/dpk`).

```
fcompress [-set <PARAMETER> <VALUE>] -target <target name>
```

- Targets are sections of `Packages` in `PackageCooker.CONFIG`:
  - `output` — output package file name;
  - `mountPath` — path the files are mounted at in the file system (required);
  - `compression` — compression level (`0` — none);
  - `encryption` — encryption key (optional);
  - `add "<file or wildcard>" ["<target path>"]` — files or directories to include.
- `-set <PARAMETER> <VALUE>` defines a `%PARAMETER%` variable substituted in the config strings.
- KeyValues text files (`.res`, `.def`, ...) are converted to binary KeyValues inside the package.

## ShaderCooker — shader compiler (`tools/shadercooker`)

Offline batch compiler of Slang shaders for all RHI backends. The output is shader packages with
the `.shd` extension — DPK files (same package format as `.epk`, `shared/dpk`) that contain the
compiled shader blobs and their `ShaderInfo` description; the renderers open them with
`g_fileSystem->OpenPackage`.

```
shadercooker [-set <PARAMETER> <VALUE>] [-filter <shader name>] -target <target name>
```

- `ShaderCooker.CONFIG` defines `Targets` (`SourcePath`, `SourceExt`, `includePath`, `blobs`,
  `output`) and the shader list (`shader`, `shaderExt`, `package` entries). Each shader describes
  its source (`SourceFile` / `SourceText`), `SourceKind` entry points, `VertexLayouts`, `FileList`,
  `DebugInfo`, `SkipOptimize`.
- `-filter` compiles only matching shaders; unchanged shaders are skipped.
- `-set` defines config variables, like in `fcompress`.
- Compilation runs in parallel on a job manager.

## TexCooker — batch texture converter and atlas builder (`tools/texcooker`)

TexCooker is only a batch-conversion utility: it does not compress textures itself, it runs
external converter applications on every texture referenced by materials. It also builds texture
atlases.

```
texcooker -target <target name>
```

### Batch conversion

- `TextureCooker.CONFIG` defines compressions (converter `application` and `arguments` with
  `%ARGS%`, `%INPUT_FILENAME%`, `%OUTPUT_FILEPATH%`), *usages* with their source path, extension and
  extra arguments, and `Targets` (`compression`, `output` folder).
- The tool scans source materials and copies them to the target folder; a texture's usage comes
  from the material value: `basetexture "path" "usage:<UsageName>"`. Textures with conflicting
  usages are skipped with an error.
- Animated texture arrays (`path[N]`) are combined into one array texture.
- Unchanged textures are skipped (CRC check).

### Texture atlases

Atlas description files (`.atl`) in the source materials folder are packed before conversion:

- `material "<name>"` — output material name (required);
- `size <width> <height>` — atlas image size;
- `padding <pixels> <clamp|repeat|mirror>` — padding between images;
- `shader <name> { ... }` — shader and parameters of the generated material (default `Base`);
- `image "<name>" ...` — source images; each entry can combine layers with blend modes
  (`EBlendMode [image] [transparency] [R G B]`).

The tool writes the packed source image, a material (`.mat`) and an `.atlas` file with the image
rectangles (`atlasgroup`); the packed image is then converted like any other texture. Materials
with an `.atlas` file expose the rectangles through `IMaterial::GetAtlas()`.

## Localization tool (`tools/localization_tool`)

C# WinForms application (`EqLocalizationTool.sln`, built separately from premake) for editing
localization files. Open the game directory: the tool lists categories from
`resources/text_english/*.txt` and languages from `resources/text_*` folders, and edits tokens
side by side for each language. File format: see [public_localization.md](public_localization.md).

## Other

- `tools/imp-exp blender/io_equilibrium_esm_esa.py` — Blender add-on for importing / exporting
  ESM (models) and ESA (animations) source files used by `egfca` / `animca`.
- `tools/egfinfo`, `tools/matsys_wxapp_template` — not part of the build (no premake project).
