# MacVox68

![MacVox68 icon](icon_raws/macicon_32.png)

MacVox68 is a Classic Mac OS 7.x (68k) application targeting Mac OS 7.5.3 and built with the Retro68 toolchain. It aims to deliver a TCP-driven text-to-speech client while keeping the UI and build flow faithful to 1990s System 7 conventions.

## Target platform and runtime
- **OS**: Classic Mac OS 7.5.3 (tested in BasiliskII).  
- **CPU**: 68k.  
- **Application signature**: Type `APPL`, Creator `MV68`.

## Build expectations
MacVox68 is built with Retro68 and depends on the MPW Interfaces & Libraries headers (Built against 3.6!). On the host machine, builds are performed out-of-tree via CMake and Ninja with the Retro68 toolchain file. The maintainer drives these builds on a Linux host (not in the Codex container), so avoid container-specific tweaks and keep configuration changes aligned with the host workflow.

### Optional Appearance SDK headers
If you need Appearance Manager headers, pass `APPEARANCE_SDK_ROOT` when configuring. It should point at the Appearance SDK headers directory that contains `CIncludes` and `RIncludes` (the project stays opt-in so local paths are not baked into the repo).

Example:
```
cmake -G Ninja ../MacVox68 \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/OldMacStuff/Retro68/toolchain-full/m68k-apple-macos/cmake/retro68.toolchain.cmake" \
  -DAPPEARANCE_SDK_ROOT="$HOME/OldMacStuff/TheBigOS7DocBlob/ReleaseBits/AppearanceSDK-1.0.4-ReadableOverlay/Appearance Sample Code/Headers"
```

### Include-path quirks (Rez / shell)
Retro68 Rez is invoked through the shell, so include paths that contain special characters must be quoted to avoid being split. In particular, MPW's `Interfaces&Libraries` path must be passed as a single token (or replaced with a symlink that avoids `&`). The `CMakeLists.txt` uses a quoted `-I` argument to keep the path intact when generating Ninja rules, following the safety guidance from `docs/rez_gotchas.md`.

## Project sources & credits
- **Retro68** (toolchain + canonical samples): https://github.com/autc04/Retro68
- **BasiliskII (KanjiTalk fork)** (emulator reference + hardware behavior): https://github.com/kanjitalk755/macemu
- **Inside Macintosh (Apple developer documentation)**: https://developer.apple.com/library/archive/documentation/mac/
- **Classic Mac OS Interfaces & Libraries (MPW headers)**: https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MPW_Ref/

## Reference caches
The container provides lookup-only classic Mac references sourced from the projects above along with *develop* magazine content and sample code. For details on what is available in the container and how to use it, consult the local documentation in the `docs/` folder.

## Source layout and reusable components

- `src/vox_parser.c`: text preprocessing and FlexTalk-marker to Mac speech-command translation; standalone host checks need no Toolbox headers.
- `src/speech.c`: owned asynchronous Speech Manager channel and text lifetime.
- `src/network*.c`: transport facade and Open Transport backend. The classic backend reports unsupported operation; it is not a MacTCP implementation.
- `src/ui/`: TextEdit fields/scrolling, input dispatch, layouts, windows, radio groups, sliders and theme helpers. `main_window.c` binds these to this application; `window_ui.c` owns startup and the event loop.
- `macvox68_icon.r`: application icon resources. Menus and controls are created in code; the legacy sample dialogs are not built.

These are reusable components inside one application target, not an independently packaged library. Keep the parser independent of UI, speech ownership separate from transport, and app policy out of the generic UI helpers. A stable exported API and standalone Toolbox harness still need an audit before a library release. ThreadLib documentation records an option, not an implemented dependency.

## Host verification

From the repository root (ordinary Linux compiler, CMake and Python):

```sh
cmake -S . -B /tmp/macvox-host
cmake --build /tmp/macvox-host
ctest --test-dir /tmp/macvox-host --output-on-failure
python3 tests/check_mac_api_contracts.py
cc -g -O1 -fsanitize=address,undefined -DVOX_PARSER_STANDALONE -Isrc \
  src/vox_parser.c tests/vox_parser_host_checks.c -o /tmp/macvox-parser-check
/tmp/macvox-parser-check
```

The contract checks compile the actual speech/network source with fake APIs. They verify ownership and receive-status behavior, not the 68k ABI or real OS scheduling. A Retro68 build and guest verification remain separate requirements.

## Current status

See [integration status](docs/integration_2026-11-15.md) for tested builds, repository reconciliation and remaining work, and [project assessment](docs/project_status_2026-11-15.md) for the historical audit. Clean and HLVOX editor speech use owned asynchronous channels. VOX timing still needs work against the external NetTTS and VoxTalk references; the network receive path does not yet implement a complete framed automatic-speech protocol. UI sliders are not yet connected to synthesizer settings.
