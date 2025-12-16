# AGENTS.md — MacVox68 (Classic Mac OS 7 TTS App) — Codex Operating Notes

MacVox68 is a **Classic Mac OS 7.x (68k)** application built with **Retro68** and tested in **BasiliskII (Mac OS 7.5.3)**.

This file exists to keep Codex from:
- inventing modern APIs,
- touching emulator-mounted disk images unsafely,
- committing junk / vendoring external caches,
- “cleaning up” in ways that break Rez / resources / classic patterns.

Keep changes small and reversible.

---

## Environments (don’t mix them up)

### Codex container
- Used to **edit repo files**.
- Has **reference caches** under `/opt/` for lookup.
- Might not have a working Retro68 toolchain.

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

## `/opt` reference caches (reference only)

Populated by setup script. Use for reading/lookup. Do not commit contents.

Typical locations:
- `/opt/Retro68` — Retro68 repo checkout (reference).
- `/opt/Interfaces&Libraries` — Apple headers/libs (MPW era).
- `/opt/Latest Notes From Apple` — Apple dev notes.
- `/opt/MPW` — MPW tools/docs (reference).
- `/opt/MacDevDocs` — Inside Macintosh, Tech Notes, HIG.

Additional curated code/resource examples (extracted from Classic-era archives; reference only):
- `/opt/MacExamples` — extracted examples including `.rsrc` sidecar files (resource forks flattened).
- `/opt/MacExamples_TextOnly` — text-only subset (fast grep/search; `.c/.h/.r` etc).

Prefer these over web searches for API truth and for Rez/resource idioms.

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

Do not run host build steps in the container unless explicitly asked.

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
  - remove them from the build inputs,
  - either delete them or move them into a clearly named “legacy/sample” area (do not break licensing headers),
  - ensure no Rez step still tries to compile `dialog.r`.
- If still used somewhere:
  - do not “fix formatting” inside Rez DITL blocks; Retro68 Rez is sensitive.
  - prefer removing the dependency over editing Rez resources unless resources are explicitly required.

Do not reformat Rez resource blocks as a “style cleanup”.

---

## Notes for Codex: classic UI code patterns

- Use `WaitNextEvent` and handle `updateEvt`, `mouseDown`, and Cmd-Q (if menu item exists).
- For controls:
  - `NewControl` + `FindControl` + `TrackControl` are expected patterns.
  - If using control IDs, be explicit about where that ID is stored (reference vs control value).

Keep the codebase consistent: prefer the existing `window_ui.c` approach unless told to revert to dialogs.
