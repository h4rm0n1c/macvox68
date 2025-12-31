# MacVox68 networking architecture

The networking stack is split into two layers so we can reuse the low-level plumbing and swap transport backends later (for example, to add a MacTCP/classic networking path without rewriting application logic).

## High-level facade (`network.c` / `network.h`)
- Exposes a minimal API used by the UI and event loop (`network_start_server`, `network_stop_server`, `network_poll`, `network_is_running`).
- Accepts UI callbacks for inbound data and log lines through `network_set_handlers`.
- Owns backend selection: currently routes everything to the Open Transport backend returned by `network_backend_ot()`.

## Backend contract (`network_backend.h`)
- `NetworkBackend` provides function pointers for init/shutdown, start/stop, run-state query, and polling.
- `NetworkCallbacks` keeps the data/log handlers that backends invoke when traffic arrives or status changes.
- Additional backends can be added alongside `network_backend_ot.c` and returned by a new selector in `network.c`.

## Open Transport backend (`network_backend_ot.c`)
- Handles endpoint creation, binding, accept/poll loops, cleanup, and OT-specific option/errno handling.
- Uses non-blocking endpoints and explicit polling to fit the cooperative event loop.
- Manages multiple clients and dispatches data or status messages through the shared callbacks supplied by the facade.

## UI integration
- `main_window.c` wires Start/Stop to the facade API and feeds inbound TCP data to the scrolling log text area.
- `ui_text_fields.c` provides safe scrolling/metric refresh helpers used when appending network-delivered text to keep scrollbar and caret behavior consistent with keyboard input.

## Extending the stack
- To add a new transport (e.g., MacTCP), implement another `NetworkBackend` in its own module, then update the selector in `network.c` to choose it based on availability or a build-time flag.
- Reuse the existing callbacks and buffer constants so UI-facing behavior remains consistent across backends.
- Pair new transport work with option/keepalive verification (see `network_keepalive_notes.md`) to avoid half-open connections on long-lived sessions.
