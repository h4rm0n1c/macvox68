# Documentation Lookup Plan for MacVox68

This document tracks where to pull authoritative information when filling in MacVox68’s missing pieces (TCP speech control, Speech Manager wiring, and classic UI polish) and records the sticking points from recent work. Use it as a map into the `/opt` reference caches and the in-repo notes so we can quickly answer implementation questions.

## Recent friction points
- **Speech/network pipeline is still a stub.** The app loop leaves hooks for TCP polling and speech pumping but nothing is wired yet, so we keep bouncing between reference sources to confirm expected System 7-era speech and socket APIs.
- **UI details vs. classic conventions.** Translating the Windows GUI layout into System 7 controls (e.g., radio groups, sliders, pop-up menus) needs stronger grounding in Toolbox-era examples to avoid modern assumptions.
- **Event loop/responsiveness questions.** We frequently revisit whether to lean on `WaitNextEvent` vs. `GetNextEvent` and how to integrate network polling without starving QuickDraw updates.

## Reference caches to mine (read-only)
- **Inside Macintosh + Tech Notes (`/opt/MacDevDocs`)**: Use the multi-volume Inside Macintosh set and Tech Notes for Window Manager, Dialog Manager, Control Manager, Menu Manager, and Speech Manager behavior (look for Inside Macintosh: Sound and related volumes). Search PDFs with `pdftotext` to jump to specific managers.
- **MacDevDocs example corpus (`/opt/MacDevDocs`)**: Besides the formal docs, this tree also contains Apple-published example code from 1992–1996 (including *develop* magazine issues, sample drivers, and demo projects). Treat it as an additional code corpus alongside `/opt/MacExamples*` when hunting for Speech Manager, MacTCP, or UI idioms.
  - Expect classic encodings: some samples keep resource forks as BinHex/MacBinary blobs or `.rsrc` sidecars, and many Rez `.r` files mirror the idioms we need. Use these as syntax references rather than trying to “clean” them.
- **Sample code (`/opt/MacExamples_TextOnly`, `/opt/MacExamples`)**: Grep for `TrackControl`, `MacTCP`, `PBRead`, `PBWrite`, and Speech Manager calls to see practical event-loop integration.
- **System 7.1 sources (`/opt/sys71src`)**: Inspect `Interfaces` and `Toolbox` source for Speech Manager, QuickDraw update paths, and MacTCP glue when API docs leave gaps.
- **Retro68 reference (`/opt/Retro68`)**: Confirm CMake/Rez patterns and minimal Toolbox startup (helps keep our build files aligned with upstream).

## Lookup plan by feature area
- **Speech Manager hookup**
  - Pull routine signatures and initialization order from Inside Macintosh (Sound/Speech) in `/opt/MacDevDocs`.
  - Cross-check handle/queue management in the System 7.1 sources under `/opt/sys71src` (search for Speech Manager units and `Snd` routines) to confirm memory handling in cooperative multitasking.
  - Use sample code hits in `/opt/MacExamples_TextOnly` to see how real apps schedule speech completion callbacks alongside UI events.
- **TCP control channel**
  - Review Inside Macintosh: Networking (MacTCP) in `/opt/MacDevDocs` for asynchronous vs synchronous calls and required `WaitNextEvent`/`SystemTask` servicing.
  - Compare against System 7.1 MacTCP sources for edge cases (timeouts, async parameter blocks) before layering on our socket pump.
  - Borrow structure from sample MacTCP clients in `/opt/MacExamples_TextOnly`, focusing on how they integrate idle processing without blocking QuickDraw.
- **Window/controls layout**
  - Use Inside Macintosh volumes covering Window Manager, Control Manager, and Dialog Manager to verify pop-up menu construction, radio cluster handling, and slider ranges.
  - Mine sample UI code in `/opt/MacExamples_TextOnly` that builds controls at runtime (no resources) to mirror our current code-driven layout.
  - Keep Retro68 sample app scaffolding from `/opt/Retro68` in mind for window creation and update-region handling so our main loop stays minimal.
- **Event loop + idle time**
  - Revisit `WaitNextEvent` guidance in the System 7 docs; confirm minimum set of events to service while still calling `SystemTask` when using MacTCP or Speech Manager callbacks.
  - Check System 7.1 source for `WaitNextEvent`/`GetNextEvent` implementations to understand how they schedule VBL tasks and whether speech/network callbacks rely on them.

## Quick-internal references to keep open
- **Startup sequence cheat sheet**: `docs/system7_dev_notes.md` (Toolbox init order, string conventions) for fast recall while coding.
- **/opt overview**: `docs/opt_reference_notes.md` for paths and usage reminders before jumping into the caches.
