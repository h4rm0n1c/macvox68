# AGENTS.md — MacVox68 (Classic Mac OS 7 TTS App) — Codex Operating Notes

MacVox68 is a **Classic Mac OS 7.x (68k)** application built with **Retro68** and tested in **BasiliskII (Mac OS 7.5.3)**.

This file exists to keep Codex from:
- inventing modern APIs,
- touching emulator-mounted disk images unsafely,
- committing junk / vendoring external caches,
- “cleaning up” in ways that break Rez / resources / classic patterns.

Keep changes small and reversible.

**If the human operator gives explicit instructions that differ from this file, follow the operator.  
Otherwise, treat this file as the default guide.**

---

## Environments

### Codex container
- Used to **edit repo files**.
- Has **reference caches** under `/opt/` for lookup.
- Might not have a working Retro68 toolchain.
- Includes Poppler utilities (`pdfinfo`, `pdftotext`, etc.) installed (poppler 24.02.0) for inspecting reference PDFs.

### User host machine (build truth)
- Builds the project and produces the `.dsk` used by BasiliskII.

### BasiliskII
- Runs Mac OS 7.5.3 and mounts the built `.dsk`.

---

## Hard safety rule

- **Never rebuild/regenerate a `.dsk` while BasiliskII has it mounted.**
  - Quit BasiliskII → rebuild → relaunch.

---

## What Codex should do

- Edit/add **Toolbox-era C/C++** code.
- Edit/add **classic Mac resources** when needed (`.r` / Rez).
- Stay within System 7 constraints:
  - cooperative event loop (`WaitNextEvent`)
  - Resource Manager + handles
  - QuickDraw update regions
  - no threads, no modern FS APIs, no UTF-8-by-default assumptions
- When unsure: consult local docs in `/opt` before guessing.

---

## What Codex must NOT do / assume

- Do not assume BasiliskII can run in this container.
- Do not assume the container has a working Retro68 toolchain.
- Do not vendor/copy/commit anything from `/opt/*`.

---

## `/opt` reference caches

Populated by the setup script. Use for reading/lookup. Do not commit contents.

Typical locations:
- `/opt/Retro68` — Retro68 repo checkout (reference).
- `/opt/Interfaces&Libraries` — Apple headers/libs (MPW era).
- `/opt/Latest Notes From Apple` — Apple dev notes.
- `/opt/MPW` — MPW tools/docs (reference).
- `/opt/MacDevDocs` — Inside Macintosh, Tech Notes, HIG.

Curated Classic-era examples (extracted from `.sea` / `.hqx` archives; reference only):
- `/opt/MacExamples` — extracted examples including `.rsrc` sidecar files (resource forks flattened).
- `/opt/MacExamples_TextOnly` — text-only subset (fast grep/search; `.c/.h/.r` etc).

Some reference PDFs and text documents in `/opt` may not have file extensions (a quirk of the extracted archives); check contents before assuming type.

### Note: extracted examples may look “split” on Unix

Some Classic Mac samples were extracted on Linux, where one original file/project can appear as multiple entries, for example:
- `name` plus `name.rsrc`
- `name.c` plus `name.c.rsrc`
- non-ASCII / odd-suffix variants like `name.µ`, `name.µ.rsrc`, `name.π.rsrc`, `name.π.rsrc.rsrc`
- sidecars for headers, e.g. `screen buffering.c`, `screen buffering.c.rsrc`, `screen buffering.h`, `screen buffering.h.rsrc`

Interpretation rule:
- Treat each group of related files as **one original Classic Mac file/project**, not as duplicates or garbage.
- The `.rsrc` and suffix companions may contain essential build/runtime data (resources, metadata, templates, etc.).
- Do **not** delete, rename, “dedupe”, or reformat these examples as a cleanup pass.
- Use them as reference for how Classic-era projects are structured (source + resources + metadata), even if they are not built with Retro68 here.

Prefer `/opt` caches over web searches for API truth and for Rez/resource idioms.

---

## Repo hygiene

- Never commit build outputs or generated artifacts.
- Builds are out-of-tree.

Minimum `.gitignore` expectations:
    build/
    *-build/
    CMakeFiles/
    CMakeCache.txt
    cmake_install.cmake
    CTestTestfile.cmake
    *.ninja
    .ninja_*
    *.dsk
    *.hda
    *.img
    *.bin
    *.ad
    *.gdb
    .finf/
    .rsrc/

---

## Build workflow (host reference)

Host build commands (run on host, from build dir):
    cmake -G Ninja ../MacVox68 -DCMAKE_TOOLCHAIN_FILE="$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake"
    ninja

Do not run these host build steps in the container unless explicitly asked by the operator.

---

## Project baseline (already true)

- App name: **MacVox68**
- Type: `'APPL'`
- Creator: `'MV68'` (stable; do not change)

UI baseline:
- The app runs with a **code-defined window UI** (Window Manager) in `window_ui.c`.
- `main.c` is a simple loop calling `ui_app_init()` then `ui_app_pump_events()`.

---

## Cleanup rules (current reality)

The repo still contains the original Retro68 sample dialog code/resources:
- `dialog.c`
- `dialog.r`

These may be unused now.

Codex cleanup expectations:
- Determine whether `dialog.c` / `dialog.r` are referenced by the build (CMake) or by any source includes.
- If unused:
    - remove them from the build inputs, and
    - either delete them, or move them into a clearly named `legacy/` or `samples/` area (keep licensing headers intact), and
    - ensure no Rez step still tries to compile `dialog.r`.
- If still used somewhere:
    - do not “fix formatting” inside Rez DITL blocks; Retro68 Rez is sensitive.
    - prefer removing the dependency over editing Rez resources unless resources are explicitly required.

Do not reformat Rez resource blocks just for style.

---

## Notes for Codex: classic UI code patterns

- Use `WaitNextEvent` and handle `updateEvt`, `mouseDown`, and Cmd-Q (if a matching menu item exists).
- For controls:
  - `NewControl` + `FindControl` + `TrackControl` are expected patterns.
  - If using control IDs, be explicit about where that ID is stored (control reference vs control value).

Keep the codebase consistent: prefer the existing `window_ui.c` approach unless the operator explicitly asks to change direction.
