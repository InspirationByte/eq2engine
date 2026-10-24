# Commit messages

Rules derived from the repository history (`git log`). Follow them for every commit.

## Format

Every line of the message is one change, written as a list item:

```
- <scope>: <change>
```

A commit with several changes has one line per change, without a separate subject line or blank
lines:

```
- filesystem: fix OpenPackage on in addons
- filesystem: add SP_DIRECT to not apply paths when opening files directly
- filesystem: Rename, MakeDir, RemoveDir, FileRemove returns status
```

```
- tracy: get rid of separate dll
- e2Core: refactoring of profiler_core
- e2Core: integrate tracy as static lib but export stuff like dynamic lib
```

## Rules

- Start every line with `- ` (dash and space).
- `<scope>` — the module or area changed, lowercase, matching the directory or project name:
  `ds`, `math`, `core`, `framework`, `keyvalues`, `esl`, `sys`, `matsystem`, `rhi`, `nvrhi`,
  `wgpu`, `render`, `grim`, `equi`, `physics`, `dkphysics`, `studio`, `egf`, `audio`, `input`,
  `font`, `dpk`, `network`, `premake`, `deps`, `natvis`, `tests`, `docs`, and tool names
  (`egfca`, `animca`, `egfman`, `fcompress`, `shadercooker`, `texcooker`). A sub-area may follow
  the scope: `sys: esl math ...`, `core: filesystem - ...`.
  Exceptions kept from history: `e2Core`, `GpuDriven` (sample) — use the project name as is.
- `<change>` — short description starting with a lowercase verb: `fix ...`, `add ...`,
  `rename A -> B`, `remove ...`, `optimize ...`, `implement ...`, `refactor ...`. Code
  identifiers keep their original case (`fix KVSection traits for RemoveSection`).
- One line per logical change; keep lines short (about 100 characters at most).
- No trailing period.
- Omit the scope only for changes that do not belong to one module (`- fix compile error`,
  `- update readme`).
- `Co-Authored-By` trailers are allowed (e.g. for agent-assisted commits). Put them after the
  change lines, separated by one blank line:

  ```
  - ds: add BufferedArray (implement storage)

  Co-Authored-By: Name <email>
  ```

  Other trailers (`Signed-off-by`, ...) are not used.
- Merge commits keep the default git message (`Merge branch 'develop' into ...`).

## Review before committing

Agents never run `git commit` right away:

1. Check the index first: `git diff --cached --name-only`. Never unstage, reset, stash or modify
   files staged by others — they may be a deliberate partial selection.
   - **Index is empty** — stage the files of the task with `git add <files>` (never
     `git add -A` / `git add .`). Review list: `git diff --cached --stat`. Commit: `git commit`.
   - **Index has files of others that do not overlap with the task** — do not stage the task
     files. Review list: `git diff HEAD --stat -- <files>`, plus the pre-staged files listed as
     "not included". Commit: `git commit --only -- <files>`, which commits the listed paths as
     they are in the working tree and leaves the rest of the index untouched.
   - **Index overlaps with the task files** — stop and ask the user.
2. Present for review:
   - the file list from step 1, so the user can also inspect the diff;
   - the complete commit message, exactly as it will be committed, including trailers.
3. Commit only after the user approves. If the user requests changes, update the message or the
   file list (only the task's own staging) and present it again.

## Examples

```
- ds: add BufferedArray (implement storage)
- math: add vec4_undef & vec4_infinity
- nvrhi: VK - make sure VK_KHR_SYNCHRONIZATION_2 is in enabled extensions
- matsystem: rename CMaterial m_szMaterialName -> m_name
- docs: add notice about PP_SL and PPSourceLine::Empty()
```
