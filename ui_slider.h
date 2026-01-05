#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include <Types.h>
#include <Windows.h>
#include <Controls.h>

#include "ui_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UISliderValueFormat
{
    kUISliderValueNone,
    kUISliderValuePercent,
    kUISliderValueHundredths
} UISliderValueFormat;

typedef struct UISliderRow
{
    ControlHandle        control;
    Rect                 frame;
    Str255               label;
    Point                labelPos;
    Point                valuePos;
    UISliderValueFormat  valueFormat;
} UISliderRow;

Boolean ui_slider_init(UISliderRow *slider,
                       WindowPtr window,
                       const Rect *frame,
                       ConstStr255Param label,
                       Point labelPos,
                       Point valuePos,
                       short initial,
                       short min,
                       short max,
                       UISliderValueFormat format);

Boolean ui_slider_matches(const UISliderRow *slider, ControlHandle handle);
void ui_slider_draw(const UISliderRow *slider, const UITheme *theme);
void ui_slider_snap_to_point(UISliderRow *slider, Point local);

#ifdef __cplusplus
}
#endif

#endif /* UI_SLIDER_H */
