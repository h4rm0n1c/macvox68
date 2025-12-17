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

## Local references worth revisiting

- **Classic Mac OS 7 Application Basics.pdf** (root of repo): concise reminders for Toolbox initialization, window creation in code, and basic event loop structure.
- **/opt/MacDevDocs**: Inside Macintosh volumes (multiple revisions), *develop* magazine issues, and sample projects. Useful for checking API evolution across System 7 releases.
- **/opt/MacExamples** and **/opt/MacExamples_TextOnly**: extracted sample code with resource sidecars; good for seeing how real projects structure resources and code.

These notes are intentionally short; expand them with specific System 7 insights as we find more useful patterns.
