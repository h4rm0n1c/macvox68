#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Fonts.h>
#include <Events.h>
#include <Menus.h>
#include <Controls.h>
#include <Icons.h>

#include "about_box.h"

enum
{
    kAboutIconID = 128
};

static WindowPtr gAboutWin = NULL;
static ControlHandle gOkButton = NULL;

static void about_box_draw_contents(WindowPtr w)
{
    Rect bounds;
    Rect iconRect;
    short textLeft;
    short textTop;

    if (!w)
        return;

    SetPort(w);
    bounds = w->portRect;

    BackPat(&qd.white);
    EraseRect(&bounds);

    SetRect(&iconRect, bounds.left + 16, bounds.top + 16,
            bounds.left + 48, bounds.top + 48);

    PlotIconID(&iconRect, atNone, ttNone, kAboutIconID);

    textLeft = iconRect.right + 12;
    textTop = iconRect.top + 14;

    TextFont(0);
    TextSize(14);
    TextFace(bold);
    MoveTo(textLeft, textTop);
    DrawString("\pMacVox68");

    TextSize(9);
    TextFace(0);
    MoveTo(textLeft, textTop + 18);
    DrawString("\pClassic Mac OS 7.x TTS app");

    MoveTo(textLeft, textTop + 32);
    DrawString("\pBuilt for Retro68 + BasiliskII");

    MoveTo(textLeft, textTop + 46);
    DrawString("\pCreated by h4rm0n1c");

    MoveTo(textLeft, textTop + 60);
    DrawString("\phttp://github.com/h4rm0n1c./");

    MoveTo(textLeft, textTop + 74);
    DrawString("\pWTFD license (Do whatever the fk you want)");

    DrawControls(w);
}

void about_box_show(void)
{
    Rect bounds;
    Rect r;
    Rect buttonRect;
    short width = 320;
    short height = 210;

    if (gAboutWin)
    {
        SelectWindow(gAboutWin);
        ShowWindow(gAboutWin);
        return;
    }

    bounds = qd.screenBits.bounds;
    SetRect(&r,
            (short)((bounds.left + bounds.right - width) / 2),
            (short)((bounds.top + bounds.bottom - height) / 2),
            (short)((bounds.left + bounds.right + width) / 2),
            (short)((bounds.top + bounds.bottom + height) / 2));

    gAboutWin = (WindowPtr)NewCWindow(NULL, &r, "\pAbout MacVox68", true,
                                      dBoxProc, (WindowPtr)-1L, true, 0);
    if (!gAboutWin)
        return;

    SetRect(&buttonRect,
            gAboutWin->portRect.right - 96,
            gAboutWin->portRect.bottom - 36,
            gAboutWin->portRect.right - 16,
            gAboutWin->portRect.bottom - 16);

    gOkButton = NewControl(gAboutWin, &buttonRect, "\pOK", true,
                           0, 0, 0, pushButProc, 0);

    ShowWindow(gAboutWin);
    about_box_draw_contents(gAboutWin);
}

void about_box_close(void)
{
    if (!gAboutWin)
        return;

    DisposeWindow(gAboutWin);
    gAboutWin = NULL;
    gOkButton = NULL;
}

void about_box_handle_update(WindowPtr w)
{
    GrafPtr savePort = NULL;

    if (!w)
        return;

    GetPort(&savePort);
    SetPort(w);

    BeginUpdate(w);
    about_box_draw_contents(w);
    EndUpdate(w);

    if (savePort)
        SetPort(savePort);
}

Boolean about_box_is_window(WindowPtr w)
{
    return (gAboutWin != NULL && w == gAboutWin);
}

Boolean about_box_handle_mouse_down(EventRecord *ev)
{
    WindowPtr w = NULL;
    short part;

    if (!gAboutWin)
        return false;

    part = FindWindow(ev->where, &w);
    if (w != gAboutWin)
        return false;

    switch (part)
    {
        case inMenuBar:
            return false;

        case inDrag:
            DragWindow(w, ev->where, &qd.screenBits.bounds);
            return true;

        case inGoAway:
            if (TrackGoAway(w, ev->where))
                about_box_close();
            return true;

        case inContent:
            if (w != FrontWindow())
            {
                SelectWindow(w);
                return true;
            }
            else
            {
                ControlHandle c = NULL;
                short cpart;
                Point local = ev->where;

                SetPort(w);
                GlobalToLocal(&local);

                cpart = FindControl(local, w, &c);
                if (cpart)
                {
                    if (TrackControl(c, local, NULL))
                    {
                        if (c == gOkButton)
                            about_box_close();
                    }
                    return true;
                }
            }
            return true;

        default:
            return false;
    }
}
