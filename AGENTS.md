# Classic Mac OS 7 TTS App — Retro68 Dev Notes (Repo Workflow)

## Current state
- Retro68 toolchain successfully built and installed at:
  - `/home/harri/OldMacStuff/Retro68/toolchain-full`
- Toolchain is using Apple Universal Interfaces (from MPW 3.6 “Interfaces & Libraries”).
- BasiliskII + Mac OS 7.5.3 is working.
- Proof of end-to-end pipeline:
  - Retro68 `HelloWorld` ran successfully in BasiliskII.
- Project repo created from Retro68 `Samples/Dialog`:
  - Source: `~/OldMacStuff/Retro68Projects/MyDialogApp`
  - Out-of-tree build: `~/OldMacStuff/Retro68Projects/MyDialogApp-build`
  - Build produces: `~/OldMacStuff/Retro68Projects/MyDialogApp-build/Dialog.dsk`
  - BasiliskII mounts the build output `.dsk` directly (no copying).

## Non-negotiable safety rules
- Do NOT rebuild or regenerate `Dialog.dsk` while BasiliskII is running and has it mounted.
  - Always quit BasiliskII before `ninja`/rebuild, then relaunch.
- Keep the repo clean:
  - Do not commit build outputs (`*-build/`, `CMakeFiles/`, `.dsk`, etc).
- Retro68 itself is NOT vendored into this repo.
  - Treat Retro68 checkout + toolchain as external dependencies.

## Toolchain configuration (68k)
- CMake toolchain file:
  - `$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake`
- Configure/build commands (from build dir):
  - `cmake -G Ninja ../MyDialogApp -DCMAKE_TOOLCHAIN_FILE="$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake"`
  - `ninja`
- Disk image output:
  - Expect `Dialog.dsk` (or project-named `.dsk`) in the build directory.

## Repo conventions (required)
- Source directory layout is whatever the Dialog sample uses.
- All builds are out-of-tree into `../MyDialogApp-build` (or similarly named build dirs).
- `.gitignore` should ignore at least:
  - `*-build/`, `build/`, `CMakeFiles/`, `CMakeCache.txt`, `cmake_install.cmake`, `CTestTestfile.cmake`, `*.ninja`, `.ninja_*`
  - Output artifacts: `*.dsk`, `*.hda`, `*.img`, `*.bin`, `*.ad`, `*.gdb`
  - Finder/metadata dirs produced by Retro68 copies: `.finf/`, `.rsrc/`

## Immediate project tasks
### Task A: Rename the app (stop being “Dialog”)
Goal: the built app and disk image should reflect the real project name, not “Dialog”.

Work items:
1) Identify where the target/disk image name comes from:
   - `CMakeLists.txt` in repo root and/or subdirs
   - Look for:
     - `add_application(...)`
     - `add_executable(...)`
     - `set_target_properties(... OUTPUT_NAME ...)`
     - any mention of `Dialog` / `Dialog.dsk`
2) Rename the build target and output artifacts:
   - Choose a stable project name (e.g. `ClassicTTS` or similar).
   - Ensure the `.dsk` output name follows the new project name.
3) Set a consistent 4-char Creator code:
   - Keep it stable forever once chosen.
   - Typical pattern:
     - Type: `'APPL'`
     - Creator: `'TTSA'` (example; pick a unique 4-char code).
4) Update visible strings:
   - Window title, dialog title, About box, etc.

Acceptance criteria:
- After rebuild, `.dsk` and the app inside it no longer say “Dialog”.
- Finder shows correct app name and signature.

### Task B: Create a proper window (not just a dialog box)
Goal: app launches to a real Window Manager window with an event loop.

Work items:
1) Add a `WIND` resource (if not already present):
   - Define at least one window template (`'WIND'` ID e.g. 128).
2) Implement window creation + event loop:
   - Init Toolbox basics: `InitGraf`, `InitFonts`, `InitWindows`, `InitMenus`, `TEInit`, `InitDialogs`, `InitCursor`
   - Create window from resource: `GetNewWindow(128, ...)`
   - Show window: `ShowWindow`
   - Event loop using `WaitNextEvent`
   - Handle at minimum:
     - `updateEvt` (BeginUpdate/EndUpdate + some drawing)
     - `mouseDown` for dragging (`FindWindow`, `DragWindow`)
     - optionally menu handling, close box, etc.
3) Keep dialog sample logic as optional:
   - Either:
     - Show dialog from a menu item
     - Or show dialog after window comes up
   - But primary UI should be a window.

Acceptance criteria:
- On launch, a normal window appears (not only a modal dialog).
- App remains responsive (event loop running).
- Runs correctly from the mounted `.dsk` in BasiliskII.

## How to iterate safely (expected workflow)
1) Quit BasiliskII.
2) Make code/resource changes in repo.
3) Build:
   - `cd ~/OldMacStuff/Retro68Projects/MyDialogApp-build`
   - `ninja`
4) Launch BasiliskII and mount/run the updated `.dsk`.
5) Repeat.

## Notes about prior build issues (Retro68 toolchain)
- During toolchain build, `hfsutils` out-of-tree build did not produce `version.o` in expected directories.
- Manual workaround was applied (building `version.o` in libhfs/top/librsrc build dirs).
- Final toolchain build completed successfully, so this is only relevant if rebuilding Retro68 toolchain itself.

## Next steps after A+B
Once the app name + real window exist:
- Add a simple text input UI (TextEdit) and a “Speak” button/menu item.
- Decide where TTS runs:
  - inside BasiliskII (classic-side TTS engine), OR
  - host-side synthesis + audio injection (if that’s the project’s direction).
- Define a minimal IPC strategy if host interaction is needed (file drop, serial emulation, shared folder polling, etc).
