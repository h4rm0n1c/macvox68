# MacVox68

MacVox68 is a Classic Mac OS 7.x (68k) application targeting Mac OS 7.5.3 and built with the Retro68 toolchain. It aims to deliver a TCP-driven text-to-speech client while keeping the UI and build flow faithful to 1990s System 7 conventions.

## Target platform and runtime
- **OS**: Classic Mac OS 7.5.3 (tested in BasiliskII).  
- **CPU**: 68k.  
- **Application signature**: Type `APPL`, Creator `MV68`.

## Build expectations
MacVox68 is built with Retro68 and depends on the MPW Interfaces & Libraries headers. On the host machine, builds are performed out-of-tree via CMake and Ninja with the Retro68 toolchain file. The maintainer drives these builds on a Linux host (not in the Codex container), so avoid container-specific tweaks and keep configuration changes aligned with the host workflow.

### Include-path quirks (Rez / shell)
Retro68 Rez is invoked through the shell, so include paths that contain special characters must be quoted to avoid being split. In particular, MPW's `Interfaces&Libraries` path must be passed as a single token (or replaced with a symlink that avoids `&`). The `CMakeLists.txt` uses a quoted `-I` argument to keep the path intact when generating Ninja rules, following the safety guidance from `docs/rez_gotchas.md`.

## Current UI
All UI is currently code-driven from [`window_ui.c`](window_ui.c):
- Initializes the Toolbox (`InitGraf`, `InitWindows`, menus, dialogs, cursor).
- Builds a code-only menu bar (Apple + File with Cmd–Q Quit) and creates the main document window.
- Adds a Quit push button and renders placeholder text indicating the window UI is active and will host TCP/TTS integration later.
- Runs an event loop via `ui_app_pump_events()` that handles `mouseDown`, `updateEvt`, and Cmd–Q to quit.

## Legacy dialog resources
The Retro68 sample dialog files now live in [`legacy/dialog.c`](legacy/dialog.c) and [`legacy/dialog.r`](legacy/dialog.r). They are not referenced by the CMake target (which only builds `main.c` and `window_ui.c`) and remain in the tree purely for historical reference.
