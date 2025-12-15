# Classic Mac OS 7 TTS App — Retro68 Dev Notes (Repo Workflow)

## Current state
- Retro68 toolchain successfully built and installed at:
  - `/home/harri/OldMacStuff/Retro68/toolchain-full`
- Toolchain is using Apple Universal Interfaces (from MPW 3.6 “Interfaces & Libraries”).
- BasiliskII + Mac OS 7.5.3 is working.
- Proof of end-to-end pipeline:
  - Retro68 `HelloWorld` ran successfully in BasiliskII.
- Project repo created from Retro68 `Samples/Dialog` and renamed to **MacVox68**:
  - Source: `~/OldMacStuff/Retro68Projects/MacVox68`
  - Out-of-tree build: `~/OldMacStuff/Retro68Projects/MacVox68-build`
  - Build produces: `~/OldMacStuff/Retro68Projects/MacVox68-build/MacVox68.dsk`
  - BasiliskII mounts the build output `.dsk` directly (no copying).

## Non-negotiable safety rules
- Do NOT rebuild or regenerate `MacVox68.dsk` while BasiliskII is running and has it mounted.
  - Always quit BasiliskII before `ninja`/rebuild, then relaunch.
- Keep the repo clean:
  - Do not commit build outputs (`*-build/`, `CMakeFiles/`, `.dsk`, etc).
- Retro68 itself is NOT vendored into this repo.
  - Treat Retro68 checkout + toolchain as external dependencies.

## Toolchain configuration (68k)
- CMake toolchain file:
  - `$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake`
- Configure/build commands (from build dir):
  - `cmake -G Ninja ../MacVox68 -DCMAKE_TOOLCHAIN_FILE="$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake"`
  - `ninja`
- Disk image output:
  - Expect `MacVox68.dsk` (project-named `.dsk`) in the build directory.

## Repo conventions (required)
- Source directory layout is whatever the Dialog sample uses.
- All builds are out-of-tree into `../MacVox68-build` (or similarly named build dirs).
- `.gitignore` should ignore at least:
  - `*-build/`, `build/`, `CMakeFiles/`, `CMakeCache.txt`, `cmake_install.cmake`, `CTestTestfile.cmake`, `*.ninja`, `.ninja_*`
  - Output artifacts: `*.dsk`, `*.hda`, `*.img`, `*.bin`, `*.ad`, `*.gdb`
  - Finder/metadata dirs produced by Retro68 copies: `.finf/`, `.rsrc/`

## Immediate project tasks
### Task A: Rename the app (stop being “Dialog”)
Goal: the built app and disk image should reflect the real project name, **MacVox68**, not “Dialog”.

Work items:
1) Identify where the target/disk image name comes from:
   - `CMakeLists.txt` in repo root and/or subdirs
   - Look for:
     - `add_application(...)`
     - `add_executable(...)`
     - `set_target_properties(... OUTPUT_NAME ...)`
     - any mention of `Dialog` / `Dialog.dsk`
2) Rename the build target and output artifacts:
    - Use the project name `MacVox68` for targets and outputs.
    - Ensure the `.dsk` output name follows the new project name.
3) Set a consistent 4-char Creator code:
   - Keep it stable forever once chosen.
   - Project choice (already applied):
     - Type: `'APPL'`
     - Creator: `'MV68'`
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
    - `cd ~/OldMacStuff/Retro68Projects/MacVox68-build`
   - `ninja`
4) Launch BasiliskII and mount/run the updated `.dsk`.
5) Repeat.

## Notes about prior build issues (Retro68 toolchain)
- During toolchain build, `hfsutils` out-of-tree build did not produce `version.o` in expected directories.
- Manual workaround was applied (building `version.o` in libhfs/top/librsrc build dirs).
- Final toolchain build completed successfully, so this is only relevant if rebuilding Retro68 toolchain itself.

## Container cache directories (do not commit)
The container setup script pre-populates several directories under `/opt` to speed up Retro68 builds and provide reference materials. These paths are **external cache locations** and must **never** be added to the repository history:

- `/opt/Retro68`: Cached Retro68 toolchain installation used by the build toolchain file.
- `/opt/Interfaces&Libraries`: Apple Universal Interfaces and Libraries (MPW 3.6 era) used by Retro68; referenced by the toolchain during compilation.
- `/opt/Latest Notes From Apple`: Documentation/reference set shipped with the toolchain; useful for API lookups.

If any environment variables or scripts point at the toolchain, they are expected to resolve to the cached installation (e.g., a `CMAKE_TOOLCHAIN_FILE` under `/opt/Retro68/m68k-apple-macos/cmake/retro68.toolchain.cmake`). Avoid copying or committing files from these paths—treat them as read-only shared caches.

## Next steps after A+B
Once the app name + real window exist:
- Add a simple text input UI (TextEdit) and a “Speak” button/menu item.
- Decide where TTS runs:
  - inside BasiliskII (classic-side TTS engine), OR
  - host-side synthesis + audio injection (if that’s the project’s direction).
- Define a minimal IPC strategy if host interaction is needed (file drop, serial emulation, shared folder polling, etc).
