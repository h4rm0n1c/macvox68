# System 7 Development Notes (MacVox68)

These notes capture quick references for System 7-era development patterns we keep hitting while working on MacVox68. Sources are local to the repo unless noted.

## Toolbox startup sequence

A minimal System 7 GUI app typically initializes the Toolbox managers in this order before creating windows:

```c
InitGraf(&qd.thePort);
InitFonts();
InitWindows();
InitMenus();
TEInit();
InitDialogs(nil);
InitCursor();
```

Even if the app does not directly use fonts or dialogs, this sequence readies shared managers for other routines and keeps window creation safe. See *Classic Mac OS 7 Application Basics.pdf* in the repo for the walkthrough and rationale.

## Event loop habits

- System 7 introduces `WaitNextEvent` for cooperative idle time, but simple loops often use `GetNextEvent` paired with `SystemTask()` to let the OS do housekeeping.
- Use `WaitNextEvent` when you need better idle behavior; otherwise `GetNextEvent` will spin the CPU when idle.
- The loop should handle `updateEvt` repaints and at minimum `mouseDown`/`keyDown` cases.

## String types

Many Toolbox calls (window titles, some text routines) require Pascal strings (`Str255`). Use the `\p` prefix or construct `Str255` buffers instead of C strings when calling these APIs.

## UI layout lessons from the NetTTS visual pass

- Keep the window background a neutral gray and paint group panes and text fields white with simple double-line borders; this preserves contrast without leaving controls in tinted states. The helpers `main_window_draw_group` and `main_window_draw_text_field` handle the gray outer fill and two-step framing used throughout the window chrome.【F:main_window.c†L270-L305】【F:main_window.c†L328-L374】
- Text inputs rely on `TextEdit` handles created with `TENew` and drawn inside white rectangles rather than default control definitions; padding the view rect (`InsetRect(&viewRect, 6, 4)`) leaves room for the frame while keeping caret alignment predictable.【F:main_window.c†L225-L247】
- Layout spacing that landed well in the NetTTS-aligned mock matches these constants: 16 px margins, 12 px gutters, 14 px section gaps, 26 px text-field heights, and 210 px slider spans. Radio buttons sit at 44 px from the prosody group top with a 22 px gap and 140 px width to keep labels readable. These values live in `main_window_plan_layout` for easy reuse.【F:main_window.c†L54-L106】【F:main_window.c†L130-L170】
- For TCP inputs, separating the host and port labels from their fields (host label shifted right, port label left of the port box) prevents overlap once the white field borders are drawn. Coordinate choices mirror the final callouts in `main_window_draw_contents`, which positions the host label at `tcpGroup.left + 36` and the port label relative to the port rect.【F:main_window.c†L354-L368】【F:main_window.c†L386-L398】

## Local references worth revisiting

- **Classic Mac OS 7 Application Basics.pdf** (root of repo): concise reminders for Toolbox initialization, window creation in code, and basic event loop structure.
- **/opt/MacDevDocs**: Inside Macintosh volumes (multiple revisions), *develop* magazine issues, and sample projects. Useful for checking API evolution across System 7 releases.
- **/opt/MacExamples** and **/opt/MacExamples_TextOnly**: extracted sample code with resource sidecars; good for seeing how real projects structure resources and code.

These notes are intentionally short; expand them with specific System 7 insights as we find more useful patterns.
