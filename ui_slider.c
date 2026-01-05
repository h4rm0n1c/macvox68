#include "ui_slider.h"

#include <Quickdraw.h>
#include <Strings.h>
#include <ControlDefinitions.h>
#include <Memory.h>
#include <stdio.h>
#include <string.h>

#include "ui_windows.h"

static void ui_slider_copy_label(ConstStr255Param label, Str255 dest)
{
    if (!label)
    {
        dest[0] = 0;
        return;
    }

    BlockMoveData(label, dest, (size_t)(label[0] + 1));
}

static void ui_slider_c_to_p(const char *src, Str255 dest)
{
    size_t len;

    if (!src)
    {
        dest[0] = 0;
        return;
    }

    len = strlen(src);
    if (len > 255)
        len = 255;

    dest[0] = (unsigned char)len;
    if (len > 0)
        BlockMoveData(src, &dest[1], len);
}

static void ui_slider_format_value(const UISliderRow *slider, Str255 out)
{
    short value;

    if (!slider || !slider->control)
    {
        out[0] = 0;
        return;
    }

    if (slider->valueFormat == kUISliderValueNone)
    {
        out[0] = 0;
        return;
    }

    value = GetControlValue(slider->control);

    if (slider->valueFormat == kUISliderValuePercent)
    {
        char buffer[16];
        sprintf(buffer, "%d%%", value);
        ui_slider_c_to_p(buffer, out);
    }
    else if (slider->valueFormat == kUISliderValueHundredths)
    {
        char buffer[24];
        double scaled = value / 100.0;
        sprintf(buffer, "%.2f", scaled);
        ui_slider_c_to_p(buffer, out);
    }
}

Boolean ui_slider_init(UISliderRow *slider,
                       WindowPtr window,
                       const Rect *frame,
                       ConstStr255Param label,
                       Point labelPos,
                       Point valuePos,
                       short initial,
                       short min,
                       short max,
                       UISliderValueFormat format)
{
    if (!slider || !window || !frame)
        return false;

    slider->control = ui_windows_new_slider(window, frame, initial, min, max);
    if (!slider->control)
        return false;

    slider->frame       = *frame;
    slider->labelPos    = labelPos;
    slider->valuePos    = valuePos;
    slider->valueFormat = format;

    ui_slider_copy_label(label, slider->label);
    return true;
}

Boolean ui_slider_matches(const UISliderRow *slider, ControlHandle handle)
{
    return slider && slider->control == handle;
}

void ui_slider_draw(const UISliderRow *slider, const UITheme *theme)
{
    Str255 valueText;

    if (!slider || !slider->control || !theme)
        return;

    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.groupFill);

    MoveTo(slider->labelPos.h, slider->labelPos.v);
    DrawString(slider->label);

    if (slider->valueFormat != kUISliderValueNone)
    {
        ui_slider_format_value(slider, valueText);
        MoveTo(slider->valuePos.h, slider->valuePos.v);
        DrawString(valueText);
    }

    Draw1Control(slider->control);
}

void ui_slider_snap_to_point(UISliderRow *slider, Point local)
{
    Rect r;
    short width;
    short min;
    short max;
    long range;
    short relative;
    short newValue;

    if (!slider || !slider->control)
        return;

    r = (**slider->control).contrlRect;
    width = (short)(r.right - r.left);
    if (width <= 0)
        return;

    min   = GetControlMinimum(slider->control);
    max   = GetControlMaximum(slider->control);
    range = (long)max - (long)min;

    relative = (short)(local.h - r.left);
    if (relative < 0)
        relative = 0;
    if (relative > width)
        relative = width;

    newValue = (short)(min + ((range * relative + (width / 2)) / width));
    SetControlValue(slider->control, newValue);
}
