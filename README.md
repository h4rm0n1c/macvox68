# MacVox68

![MacVox68 icon](icon_raws/macicon_32.png)

MacVox68 is a Classic Mac OS 7.x (68k) application targeting Mac OS 7.5.3 and built with the Retro68 toolchain. It aims to deliver a TCP-driven text-to-speech client while keeping the UI and build flow faithful to 1990s System 7 conventions.

## Target platform and runtime
- **OS**: Classic Mac OS 7.5.3 (tested in BasiliskII).  
- **CPU**: 68k.  
- **Application signature**: Type `APPL`, Creator `MV68`.

## Build expectations
MacVox68 is built with Retro68 and depends on the MPW Interfaces & Libraries headers. On the host machine, builds are performed out-of-tree via CMake and Ninja with the Retro68 toolchain file. The maintainer drives these builds on a Linux host (not in the Codex container), so avoid container-specific tweaks and keep configuration changes aligned with the host workflow.

### Include-path quirks (Rez / shell)
Retro68 Rez is invoked through the shell, so include paths that contain special characters must be quoted to avoid being split. In particular, MPW's `Interfaces&Libraries` path must be passed as a single token (or replaced with a symlink that avoids `&`). The `CMakeLists.txt` uses a quoted `-I` argument to keep the path intact when generating Ninja rules, following the safety guidance from `docs/rez_gotchas.md`.

## Reference caches
The container provides lookup-only classic Mac references under `/opt` (Retro68 checkout, Interfaces & Libraries, multiple Inside Macintosh revisions plus *develop* magazine and sample code, and the System 7.1 source reference). See [`docs/opt_reference_notes.md`](docs/opt_reference_notes.md) for paths and usage tips, and [`docs/system7_dev_notes.md`](docs/system7_dev_notes.md) for quick reminders on Toolbox startup, event loops, and other System 7 development habits.

## Current UI
All UI is currently code-driven from the `ui_app` and `main_window` pair:
- [`window_ui.c`](window_ui.c) initializes the Toolbox (`InitGraf`, `InitWindows`, menus, dialogs, cursor) and runs the main event pump.
- [`main_window.c`](main_window.c) owns the main document window, draws its placeholder chrome, and builds all controls/pop-up menus in code.
- [`ui_app.h`](ui_app.h) and [`main_window.h`](main_window.h) keep the declarations together so each window can live in its own source file as the project grows.

Because the project ships **no Toolbox resources** (no Rez `.r` menus, DITLs, etc.), every menu and control is created programmatically. Avoid adding references to resource IDs that are not constructed in code; if a control or menu is needed, define and build it explicitly at runtime.

## Legacy dialog resources
The Retro68 sample dialog files now live in [`legacy/dialog.c`](legacy/dialog.c) and [`legacy/dialog.r`](legacy/dialog.r). They are not referenced by the CMake target (which only builds `main.c`, `window_ui.c`, and `main_window.c`) and remain in the tree purely for historical reference.
