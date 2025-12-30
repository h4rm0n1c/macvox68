# MacVox68 UI Architecture Notes

## Current entry points and coupling
- **`main.c`**: Initializes the UI via `ui_app_init()` and pumps the cooperative loop with `ui_app_pump_events()`. No other responsibilities; future service pumps are hinted at in comments.
- **`window_ui.c`**: Owns Toolbox startup (GrafPort, menus, dialogs, cursor), builds a code-only menu bar, and delegates UI work. Dispatches events to the About box helpers and `main_window_*` handlers, and keeps a minimal quit flag (Cmd-Q and menu parity). Also provides a per-tick idle hook for main window logic.
- **`ui_app.h`**: Declares the minimal app lifecycle surface (`ui_app_init`, `ui_app_pump_events`) consumed by `main.c`. Implemented in `window_ui.c`, so the header is the only public touchpoint for the event loop today.
- **`ui_text_fields.[hc]`**: Reusable text control helpers. Provide TE setup, color selection, keyboard filtering (with single-line width checks), idle/update hooks, and an attached scrollbar helper (`UIScrollingText`). Used by `main_window.c` to build the console-like text area and TCP host/port fields.

## Direction for reusable layers
We will keep the shared layers in **C** for now. The existing build is `project(MacVox68 C)` and all UI helpers are plain C; staying in C avoids changing the Retro68 configuration and keeps headers compatible with the Toolbox-era patterns we use. If we later need C++, we can flip the target to CXX and compile with Retro68's `m68k-apple-macos-g++`; we would also need to add `<MacTypes.h>`/`<Types.h>` in `extern "C"` blocks for mixed linkage and ensure `-fno-exceptions`/`-fno-rtti` if we want minimal runtime. Until that need is clear, C keeps the surface small and predictable.

## Proposed modules
- **`ui/input`**: An event dispatcher that wraps `WaitNextEvent`, normalizes quit/menu shortcuts, and routes to window-level controllers. This would replace the ad-hoc switch in `window_ui.c` with a table of handlers per event class and allow later insertion of TCP/speech polling without bloating the main loop.
- **`ui/windows`**: Small structs that pair a window handle with its controller callbacks (create, update, mouse, key, idle). `main_window.c` already acts as one such controller; we can lift its signatures into a header and keep per-window private state in accompanying `*_priv.h` files.
- **`ui/theme`**: Central place for colors, metrics, and spacing constants. The current text helpers define colors locally; moving them into a theme module keeps text fields, sliders, and buttons consistent, and offers a single knob for classic/greyscale palettes.
- **Text controls integration**: `ui_text_fields` would consume `ui/theme` for colors and padding, expose creation/update/key/idle hooks to `ui/windows`, and register its scrollbar tracking proc with `ui/input` so mouse/scroll events flow through the same dispatcher.

## Naming and header layout
- **Public headers**: Keep the existing prefix and short nouns (`ui_app.h`, `ui_text_fields.h`, `main_window.h`). These live in the repo root for now; as modules grow, mirror the path (e.g., `ui/input.h`, `ui/theme.h`). Public headers should only expose the opaque state and callback signatures needed by other modules.
- **Private headers**: Use a `_priv` suffix next to the implementation file when helpers must be shared within a module but not exported app-wide (e.g., `ui/text_fields_priv.h`, `ui/windows_priv.h`). Avoid leaking Toolbox globals; pass ports/handles explicitly.
- **Function naming**: Prefix module names (`ui_input_dispatch`, `ui_window_create`, `ui_theme_apply_text_colors`) to avoid collisions with classic Toolbox symbols. Keep parameters explicit about ownership (e.g., pass `WindowPtr` and `Boolean *outQuit`).
- **Types**: Prefer small structs with clear ownership fields (`WindowPtr window; ControlHandle scroll;`) rather than globals; if C++ is introduced later, these map cleanly to lightweight classes without changing public APIs.

## Next steps
- Extract the event switch from `window_ui.c` into a minimal `ui/input` dispatcher that owns quit/menu handling and delegates to registered window controllers.
- Move shared colors/metrics from `ui_text_fields.c` and `main_window.c` into a `ui/theme` header to keep spacing and palette consistent.
- Define a `ui/windows` header that formalizes the create/update/mouse/key/idle callbacks and stores per-window state, using private headers for internal structs.
- Keep `ui_app.h` as the top-level entry so `main.c` stays trivial and swap-free.
