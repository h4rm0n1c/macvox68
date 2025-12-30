# MacVox68 UI + Control Patterns (System 7)

These notes capture reusable patterns observed in MacVox68 for future Classic Mac development and Windows porting.

## Layout and grouping
- The main window avoids Rez layouts; rects are computed in code using a shared `LayoutMetrics` table, keeping button columns and content gutters consistent. The left column reserves fixed-width buttons while the right side hosts grouped content (`Prosody/Enunciation`, `Settings`, `NetCat Receiver/TCP Server`). This mirrors the Windows layout while matching Mac spacing expectations.
- Group boxes are custom-drawn (`main_window_draw_group`) with a shaded border and title text, giving an Appearance-like panel without relying on Appearance Manager.
- Text fields are framed with a two-pass border (`main_window_draw_text_field`) to simulate beveling and are inset before TextEdit handles are created, preventing text from hugging the edge.

## Text input patterns
- Multi-line input uses a TextEdit handle sized to a large `destRect` to allow growth without reallocations, paired with a scrollbar whose maximum tracks `TECalText` output (`main_window_update_scrollbar`). Scrolling is handled manually via `TEPinScroll`, with selection visibility maintained by `main_window_scroll_selection_into_view`.
- Single-line fields (host/port) reuse the same helper but switch TextEdit into `crOnly` mode and clamp the view/dest rect to a single line, producing edit-control-like behavior without accepting returns. Pre-checking `TextWidth` before `TEKey` enforces width limits so typing never overflows the visual field.
- Active editing is tracked and passed through `TEKey`/`TEIdle`, keeping insertion point blink and selection coloring correct even while other controls redraw.

## Control creation and platform equivalence
- Control procs stick to classic IDs: `pushButProc` for push buttons, `radioButProc` for radio selections, `kControlSliderProc` for sliders, and `scrollBarProc` for the text area. Use `SetControlData` with `kControlPushButtonDefaultTag` to mark defaults (parallels Win32 BS_DEFPUSHBUTTON behavior).
- The scrollbar value is treated as pixel offset rather than line index, similar to Win32 `SB_VERT` trackbars when paired with manual drawing; line/page steps mirror TextEdit line height and visible height.
- Menu bar is code-defined (Apple + File/Quit) to keep binaries resource-light; window and control setup avoid Rez dependencies, easing portability.

## Resource/icon defaults
- Windows are created with `NewCWindow` and draw their own chrome (background fill + separators) to stay consistent without Appearance Manager. This makes the window definition a reusable template for other Classic Mac utilities.
- The About box uses a simple icon ID (`128`) drawn with `PlotIconID` and a code-defined OK button, reinforcing the pattern of code-first resources when the Rez pipeline is minimal.
