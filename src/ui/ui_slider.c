#include "ui/ui_slider.h"

#include <Quickdraw.h>
#include <Events.h>
#include <Gestalt.h>
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
            snprintf(buffer, sizeof(buffer), "%.2fx", scaled);
            ui_slider_cstr_to_pascal(out, buffer);
            break;
        }
        case kUISliderValueSemitones:
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%+d st", (int)GetControlValue(slider->control));
            ui_slider_cstr_to_pascal(out, buffer);
            break;
        }
        case kUISliderValueNone:
        default:
            break;
    }
}

/* Appearance live sliders update the value before this callback (corpus 162).
   Merely invalidating does not paint until TrackControl returns. */
static pascal void ui_slider_track_action(ControlHandle control, short part)
{
    UISlider *slider = control ? (UISlider *)GetControlReference(control) : NULL;
    GrafPtr saved;
    RGBColor fore, back;
    (void)part;
    if (!slider) return;
    GetPort(&saved);
    SetPort((**control).contrlOwner);
    GetForeColor(&fore); GetBackColor(&back);
    ui_slider_draw(slider, NULL);
    RGBForeColor(&fore); RGBBackColor(&back);
    SetPort(saved);
}

ControlActionUPP ui_slider_get_action(const UISlider *slider)
{
    return slider ? slider->action : NULL;
}

static short ui_slider_thumb_center(ControlHandle c, const Rect *frame)
{
    Point p;
    short first = -1, last = -1;
    p.v = (frame->top + frame->bottom) / 2;
    for (p.h = frame->left; p.h < frame->right; ++p.h)
        if (TestControl(c, p) == kControlIndicatorPart)
        {
            if (first < 0) first = p.h;
            last = p.h;
        }
    return first < 0 ? 0 : (first + last) / 2;
}

void ui_slider_init(UISlider *slider, WindowPtr window, const Rect *frame,
                    ConstStr255Param label, short initial, short min, short max,
                    UISliderValueFormat format, short fixedScale)
{
    ControlHandle h = NULL;
    long appearance = 0;

    if (!slider || !window || !frame)
        return;

    slider->frame      = *frame;
    slider->valueFormat = format;
    slider->fixedScale = fixedScale > 0 ? fixedScale : 100;
    ui_slider_copy_label(slider->label, label);

    slider->action = NULL;
    slider->control = NULL;
    slider->minCenter = slider->maxCenter = 0;
    /* Proc 48/49 is an Appearance control, not a stock System 7 slider. */
    if (Gestalt('appr', &appearance) != noErr || !(appearance & 1))
        return;
    slider->action = NewControlActionUPP(ui_slider_track_action);
    h = NewControl(window, frame, "\p", true, initial, min, max,
                   kControlSliderProc | kControlSliderLiveFeedback, 0);
    slider->control = h;

    if (slider->control)
    {
        SetControlReference(slider->control, (long)slider);
        /* Test the CDEF's real thumb at each endpoint. GetControlRegion is
           not supplied by this classic 68k link library. No guessed insets. */
        SetControlValue(h, min);
        slider->minCenter = ui_slider_thumb_center(h, frame);
        SetControlValue(h, max);
        slider->maxCenter = ui_slider_thumb_center(h, frame);
        SetControlValue(h, initial);
    }

    slider->valueRect.left   = (short)(frame->right + kUISliderValueGap - 2);
    slider->valueRect.right  = (short)(slider->valueRect.left + kUISliderValueWidth);
    slider->valueRect.top    = frame->top;
    slider->valueRect.bottom = frame->bottom;
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

/* The native CDEF only hits the thumb, so intercept the surrounding track.
   GetMouse is already local to the owning window (no GlobalToLocal). */
Boolean ui_slider_track(UISlider *slider, Point local)
{
    ControlHandle c;
    short part;
    RGBColor back;
    if (!slider || !(c = slider->control) || !PtInRect(local, &slider->frame) ||
        (**c).contrlHilite == 255)
        return false;
    GetBackColor(&back);
    RGBBackColor(&ui_theme_get()->colors.groupFill);
    part = TestControl(c, local);
    if (part == kControlIndicatorPart)
        TrackControl(c, local, slider->action);
    else if (slider->maxCenter > slider->minCenter)
    {
        short min = GetControlMinimum(c), max = GetControlMaximum(c);
        do
        {
            long x = local.h;
            if (x < slider->minCenter) x = slider->minCenter;
            if (x > slider->maxCenter) x = slider->maxCenter;
            SetControlValue(c, min + ((x - slider->minCenter) * (max - min) +
                (slider->maxCenter - slider->minCenter) / 2) /
                (slider->maxCenter - slider->minCenter));
            ui_slider_track_action(c, kControlIndicatorPart);
            GetMouse(&local);
        } while (StillDown());
    }
    RGBBackColor(&back);
    ui_slider_invalidate_value(slider);
    return true;
}
