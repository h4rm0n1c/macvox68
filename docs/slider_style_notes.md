# Classic slider style notes (Speech CP look)

These notes capture what we learned while inspecting the System 7.5.3 Speech control panel slider and comparing it to MacVox68.

## Key takeaways

- The Speech control panel slider look is **not a special built‑in “Speech slider” control**. It is **classic Control Manager behavior + custom drawing** for track/ticks/labels.
- The visual style people describe (Speech CP / “rate” slider) can be matched by:
  - Using the **standard slider proc** (`kControlSliderProc`, proc ID `48`) for behavior, and
  - **Drawing the chrome (track/ticks/labels)** in app code.
- **Custom CDEFs are not required** for this look. We did not find any non‑system CDEFs in the Speech CP resources.
- The slider labels in the Speech CP are **separate DITL text items**, not embedded in the slider control.

## Resource inspection notes (ResEdit)

- The Speech CP DITL shows the slider as a **single control item**. There are no separate DITL items for the track/knob; the labels are separate text items.
- The control panel does **not expose a standard slider control (proc ID 48)** in its CNTL resources.
- The only visible CDEFs were **system IDs (2, 4, 5)**, implying no custom slider CDEF for the Speech CP.
- ResEdit displays the slider correctly but sometimes **fails to redraw the control after dragging**; closing/reopening the DITL window forces a refresh. This suggests custom drawing or a nonstandard proc that the editor does not invalidate live.

## Practical guidance for MacVox68

- **Use `kControlSliderProc`** for volume/rate/pitch sliders to avoid end‑button scroll bars.
- **Custom‑draw the track/ticks/labels** around the slider in `main_window_draw_contents` if we want the Speech CP look.
- **Set the background color before drawing sliders**: classic sliders pick up the current `RGBBackColor`. Use the group fill color so the slider well isn’t white.

## When someone asks for “that Speech slider look”

Interpret the request as:

> “Use the standard slider control for behavior, but draw the track/ticks/labels in the window code to match the Speech control panel styling.”

This stays compatible with System 7.5.3 and avoids custom CDEF maintenance.
