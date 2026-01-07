#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include <Controls.h>
#include <Types.h>

#include "ui/ui_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UISliderValueFormat
{
    kUISliderValueNone = 0,
    kUISliderValuePercent,
    kUISliderValueFixed2
} UISliderValueFormat;

typedef struct UISlider
{
    ControlHandle       control;
    Rect                frame;
    Rect                valueRect;
    Str255              label;
    UISliderValueFormat valueFormat;
    short               fixedScale;
} UISlider;

void ui_slider_init(UISlider *slider, WindowPtr window, const Rect *frame,
                    ConstStr255Param label, short initial, short min, short max,
                    UISliderValueFormat format, short fixedScale);

void ui_slider_draw(const UISlider *slider, const UITheme *theme);

ControlHandle ui_slider_get_handle(const UISlider *slider);

short ui_slider_get_value(const UISlider *slider);
void ui_slider_set_value(UISlider *slider, short value);
void ui_slider_invalidate_value(const UISlider *slider);

#ifdef __cplusplus
}
#endif

#endif /* UI_SLIDER_H */
