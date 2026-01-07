#include "ui_slider.h"

#include <Quickdraw.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Strings.h>
#include <string.h>
#include <stdio.h>

#ifndef kControlSliderProc
    #define kControlSliderProc 48
#endif

enum
{
    kUISliderLabelInset    = 52,
    kUISliderValueGap      = 10,
    kUISliderBaselineInset = 16,
    kUISliderValueWidth    = 52
};

static void ui_slider_copy_label(Str255 dest, ConstStr255Param src)
{
    short len = 0;

    if (src)
        len = src[0];

    dest[0] = (unsigned char)len;
    if (len > 0)
        BlockMoveData(&src[1], &dest[1], len);
}

static void ui_slider_cstr_to_pascal(Str255 dest, const char *src)
{
    size_t len = strlen(src);
    if (len > 255)
        len = 255;

    dest[0] = (unsigned char)len;
    if (len > 0)
        BlockMoveData(src, &dest[1], len);
}

static short ui_slider_baseline(const UISlider *slider)
{
    return (short)(slider->frame.top + kUISliderBaselineInset);
}

static void ui_slider_format_value(const UISlider *slider, Str255 out)
{
    out[0] = 0;

    if (!slider || !slider->control)
        return;

    switch (slider->valueFormat)
    {
        case kUISliderValuePercent:
        {
            short value = GetControlValue(slider->control);
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d%%", (int)value);
            ui_slider_cstr_to_pascal(out, buffer);
            break;
        }
        case kUISliderValueFixed2:
        {
            short value = GetControlValue(slider->control);
            short scale = slider->fixedScale > 0 ? slider->fixedScale : 100;
            char buffer[32];
            double scaled = (double)value / (double)scale;
            snprintf(buffer, sizeof(buffer), "%.2f", scaled);
            ui_slider_cstr_to_pascal(out, buffer);
            break;
        }
        case kUISliderValueNone:
        default:
            break;
    }
}

void ui_slider_init(UISlider *slider, WindowPtr window, const Rect *frame,
                    ConstStr255Param label, short initial, short min, short max,
                    UISliderValueFormat format, short fixedScale)
{
    ControlHandle h = NULL;

    if (!slider || !window || !frame)
        return;

    slider->frame      = *frame;
    slider->valueFormat = format;
    slider->fixedScale = fixedScale > 0 ? fixedScale : 100;
    ui_slider_copy_label(slider->label, label);

    h = NewControl(window, frame, "\p", true, initial, min, max, kControlSliderProc, 0);
    slider->control = h;

    if (slider->control)
        SetControlReference(slider->control, (long)slider);

    slider->valueRect.left   = (short)(frame->right + kUISliderValueGap - 2);
    slider->valueRect.right  = (short)(slider->valueRect.left + kUISliderValueWidth);
    slider->valueRect.top    = (short)(frame->top + 4);
    slider->valueRect.bottom = (short)(frame->bottom - 4);
}

void ui_slider_draw(const UISlider *slider, const UITheme *theme)
{
    Str255 value;
    const UITheme *useTheme = theme ? theme : ui_theme_get();

    if (!slider)
        return;

    RGBForeColor(&useTheme->colors.text);
    RGBBackColor(&useTheme->colors.windowFill);

    MoveTo((short)(slider->frame.left - kUISliderLabelInset), ui_slider_baseline(slider));
    DrawString(slider->label);

    if (!slider->control || slider->valueFormat == kUISliderValueNone)
        return;

    ui_slider_format_value(slider, value);
    RGBForeColor(&useTheme->colors.groupFill);
    RGBBackColor(&useTheme->colors.groupFill);
    EraseRect(&slider->valueRect);
    RGBForeColor(&useTheme->colors.text);
    RGBBackColor(&useTheme->colors.groupFill);
    MoveTo((short)(slider->frame.right + kUISliderValueGap), ui_slider_baseline(slider));
    DrawString(value);
}

ControlHandle ui_slider_get_handle(const UISlider *slider)
{
    return slider ? slider->control : NULL;
}

short ui_slider_get_value(const UISlider *slider)
{
    return slider && slider->control ? GetControlValue(slider->control) : 0;
}

void ui_slider_set_value(UISlider *slider, short value)
{
    if (!slider || !slider->control)
        return;

    SetControlValue(slider->control, value);
    ui_slider_invalidate_value(slider);
}

void ui_slider_invalidate_value(const UISlider *slider)
{
    if (!slider)
        return;

    InvalRect(&slider->valueRect);
}
