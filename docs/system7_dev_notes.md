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
- Layout spacing that landed well in the NetTTS-aligned mock matches these constants: 16 px margins, 12 px gutters, 14 px section gaps, 26 px text-field heights, and 210 px slider spans. Prosody radio buttons sit 44 px below the group top; the Clean option uses a narrower 70 px width with the other two at 140 px, and the LQ/HQ buttons are shifted left slightly so the cluster reads as a unit. These values live in `main_window_plan_layout` for easy reuse.【F:main_window.c†L54-L106】【F:main_window.c†L130-L174】
- For TCP inputs, separating the host and port labels from their fields (host label shifted right, port label left of the port box) prevents overlap once the white field borders are drawn. Coordinate choices mirror the final callouts in `main_window_draw_contents`, which positions the host label at `tcpGroup.left + 36` and the port label relative to the port rect.【F:main_window.c†L354-L368】【F:main_window.c†L386-L398】
- Single-line TextEdit fields (used for Host/Port) are treated as a standard control: create them with `crOnly` enabled and a dest/view rect narrowed to the line height, block overflow by measuring `TextWidth` against the view width before calling `TEKey`, but always allow backspace/delete to pass so users can recover even when the field is “full.”【F:main_window.c†L225-L266】【F:main_window.c†L585-L641】
- Multiline TextEdit fields (the main request area) are likewise bounded to their visible frame: compute `maxLines` from the view height and line height, clamp `destRect.bottom` to that limit, and reject returns once `nLines` reaches the maximum. Before inserting any other character on the last visible line, build the would-be line content and `TextWidth` check it against the available width, blocking the keystroke when it would wrap below the border.【F:main_window.c†L225-L273】【F:main_window.c†L353-L470】【F:main_window.c†L612-L641】【F:main_window.c†L830-L881】

## Color vs. black-and-white windows (System 7.5.3)

- A window created with `NewWindow` defaults to black-and-white drawing; color icon resources (for example `icl8`/`icl4`) will still exist, but they will render in monochrome if the window isn't color-capable.
- Use `NewCWindow` to make a color-aware window that respects the current system color depth; this is required if you want the main UI and About box to render in color on System 7.5.3.
- Once using color windows, set colors explicitly with `RGBForeColor`/`RGBBackColor` before drawing fills, borders, or text. This avoids the UI inheriting unexpected colors from prior drawing ops and keeps the chrome consistent.
- Classic sliders (`kControlSliderProc`) will pick up the current background color when drawn. To avoid white slider wells, set `RGBBackColor` to the owning group’s fill before drawing the slider controls.

## Appearance Manager usage (System 7.5.3 compatibility)

- System 7.5.3 can run with the Appearance extension, but it should not be assumed. Prefer Control Manager–level features that degrade gracefully.
- Use the Control Manager default push-button tag (`kControlPushButtonDefaultTag` / `'dflt'`) to request the classic double-border/default-ring appearance instead of hand-drawing the ring.
- Avoid `DrawTheme*` or other Appearance Manager calls unless we add runtime checks (for example, `Gestalt(gestaltAppearanceAttr, ...)`) and clearly handle the no-Appearance case.
- Stick to `NewCWindow`, standard controls (`pushButProc`, `radioButProc`, `scrollBarProc`, etc.), and explicit `RGBForeColor`/`RGBBackColor` for a consistent classic look on 7.5.3.

## Local references worth revisiting

- **Classic Mac OS 7 Application Basics.pdf** (root of repo): concise reminders for Toolbox initialization, window creation in code, and basic event loop structure.
- **/opt/MacDevDocs**: Inside Macintosh volumes (multiple revisions), *develop* magazine issues, and sample projects. Useful for checking API evolution across System 7 releases.
- **/opt/MacExamples** and **/opt/MacExamples_TextOnly**: extracted sample code with resource sidecars; good for seeing how real projects structure resources and code.

These notes are intentionally short; expand them with specific System 7 insights as we find more useful patterns.
