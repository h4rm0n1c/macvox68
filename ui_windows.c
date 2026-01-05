#include "ui_windows.h"

#include <Quickdraw.h>
#include <Menus.h>
#include <ControlDefinitions.h>
#include <Events.h>

static short ui_windows_value_from_point(ControlHandle c, Point local)
{
    Rect r = (**c).contrlRect;
    short width = (short)(r.right - r.left);

    if (width <= 0)
        return GetControlValue(c);

    {
        short min    = GetControlMinimum(c);
        short max    = GetControlMaximum(c);
        long range   = (long)max - (long)min;
        short offset = (short)(local.h - r.left);

        if (offset < 0)
            offset = 0;
        if (offset > width)
            offset = width;

        return (short)(min + ((range * offset + (width / 2)) / width));
    }
}

static short ui_windows_track_slider(ControlHandle c)
{
    Point mouse;
    short lastValue;

    GetMouse(&mouse);
    lastValue = ui_windows_value_from_point(c, mouse);
    SetControlValue(c, lastValue);
    Draw1Control(c);

    while (StillDown())
    {
        GetMouse(&mouse);

        {
            short newValue = ui_windows_value_from_point(c, mouse);
            if (newValue != lastValue)
            {
                lastValue = newValue;
                SetControlValue(c, newValue);
                Draw1Control(c);
            }
        }
    }

    return kControlIndicatorPart;
}

#ifndef kControlIndicatorPart
    #define kControlIndicatorPart 129
#endif

WindowPtr ui_windows_create_standard(const UIWindowSpec *spec)
{
    Rect screenBounds;
    Rect r;
    short topInset;
    short bottomInset;
    short width;
    short height;

    if (!spec)
        return NULL;

    screenBounds = qd.screenBits.bounds;
    width = spec->width;
    height = spec->height;
    topInset = (short)(GetMBarHeight() + spec->margin + 16);
    bottomInset = spec->margin;

    {
        short availableTop    = screenBounds.top + topInset;
        short availableBottom = screenBounds.bottom - bottomInset;
        short availableHeight = availableBottom - availableTop;
        short startY;

        if (availableHeight > height)
            startY = (short)(availableTop + (availableHeight - height) / 2);
        else
            startY = availableTop;

        SetRect(&r,
                (short)((screenBounds.left + screenBounds.right - width) / 2),
                startY,
                (short)((screenBounds.left + screenBounds.right + width) / 2),
                (short)(startY + height));

        if (r.bottom > availableBottom)
            OffsetRect(&r, 0, (short)(availableBottom - r.bottom));
    }

    return (WindowPtr)NewCWindow(
        NULL,
        &r,
        spec->title,
        true,
        documentProc,
        (WindowPtr)-1L,
        true,
        0
    );
}

void ui_windows_draw(WindowPtr window, UIWindowDrawProc drawProc, void *refCon)
{
    GrafPtr savePort;

    if (!window || !drawProc)
        return;

    GetPort(&savePort);
    SetPort(window);

    BeginUpdate(window);
    drawProc(window, refCon);
    EndUpdate(window);

    SetPort(savePort);
}

ControlHandle ui_windows_new_button(WindowPtr window, const Rect *frame,
                                    ConstStr255Param title, Boolean isDefault, short controlID)
{
    ControlHandle h;

    h = NewControl(window, frame, title, true, 0, 0, 0, pushButProc, controlID);
    if (h && isDefault)
    {
        Boolean flag = true;
        SetControlData(h, kControlEntireControl, kControlPushButtonDefaultTag, sizeof(flag), &flag);
    }

    return h;
}

ControlHandle ui_windows_new_slider(WindowPtr window, const Rect *frame,
                                    short initial, short min, short max)
{
    return NewControl(window, frame, "\p", true, initial, min, max, kControlSliderProc, 0);
}

Boolean ui_windows_track_hit_control(WindowPtr window, Point local,
                                     const UIControlTrackingSpec *specs, short count,
                                     ControlHandle *outHit, short *outPart)
{
    ControlHandle c;
    short part;

    if (!window || !specs || count <= 0)
        return false;

    if (outHit)
        *outHit = NULL;
    if (outPart)
        *outPart = 0;

    part = FindControl(local, window, &c);
    if (!part || !c)
        return false;

    while (count-- > 0)
    {
        RGBColor prevBack;
        Boolean restoreColor = false;

        if (specs->control == c)
        {
            if (specs->background)
            {
                GetBackColor(&prevBack);
                RGBBackColor(specs->background);
                restoreColor = true;
            }

            if (specs->snapToClick && !specs->action)
            {
                part = ui_windows_track_slider(c);
            }
            else
            {
                if (specs->snapToClick)
                {
                    Rect r = (**c).contrlRect;
                    short width = (short)(r.right - r.left);

                    if (width > 0)
                    {
                        short min    = GetControlMinimum(c);
                        short max    = GetControlMaximum(c);
                        long range   = (long)max - (long)min;
                        short offset = (short)(local.h - r.left);

                        if (offset < 0)
                            offset = 0;
                        if (offset > width)
                            offset = width;

                        SetControlValue(c, (short)(min + ((range * offset + (width / 2)) / width)));

                        local.h = (short)(r.left + offset);
                        local.v = (short)(r.top + (r.bottom - r.top) / 2);
                    }
                }

                part = TrackControl(c, local, specs->action);
            }

            if (restoreColor)
                RGBBackColor(&prevBack);

            if (outHit)
                *outHit = c;
            if (outPart)
                *outPart = part;

            return true;
        }

        ++specs;
    }

    return false;
}
