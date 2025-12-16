#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Controls.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif

enum
{
    kQuitButtonID = 1
};

static WindowPtr      gMainWin  = NULL;
static ControlHandle  gQuitBtn  = NULL;

static void ui_init_toolbox(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
}

static void ui_init_menus_codeonly(void)
{
    /* Code-only menu bar (no Rez). This keeps the app feeling “real” without resources.
       Minimal Apple menu + File menu with Quit (Cmd-Q). */

    MenuHandle apple = NewMenu(128, "\p\024");
    if (apple)
    {
        AppendMenu(apple, "\pAbout MacVox68...");
        InsertMenu(apple, 0);
    }

    MenuHandle file = NewMenu(129, "\pFile");
    if (file)
    {
        AppendMenu(file, "\pQuit/Q");
        InsertMenu(file, 0);
    }

    DrawMenuBar();
}

static void ui_create_main_window(void)
{
    Rect r;
    SetRect(&r, 60, 60, 420, 240);

    gMainWin = NewWindow(
        NULL,
        &r,
        "\pMacVox68",
        true,
        documentProc,
        (WindowPtr)-1L,
        true,
        0
    );

    if (!gMainWin)
        return;

    SetPort(gMainWin);

    /* Quit button */
    {
        Rect br;

        /* SetRect(left, top, right, bottom) */
        SetRect(&br, 14, 12, 84, 34); /* width ~70, height ~22 */

        gQuitBtn = NewControl(
            gMainWin,
            &br,
            "\pQuit",
            true,
            0,
            0,
            1,
            kClassicPushButtonProc,
            kQuitButtonID
        );
    }

    ShowWindow(gMainWin);
}

static void ui_draw_contents(WindowPtr w)
{
    Rect content;
    short x, y;

    SetPort(w);
    content = w->portRect;

    EraseRect(&content);

    x = content.left + 14;
    y = content.top + 12 + 22 + 16; /* button top + button height + padding */

    MoveTo(x, y);
    DrawString("\pMacVox68 is live.");

    y += 16;
    MoveTo(x, y);
    DrawString("\pTCP + TTS will be pumped from the event loop.");
}

static void ui_handle_update(WindowPtr w)
{
    GrafPtr savePort;
    GetPort(&savePort);
    SetPort(w);

    BeginUpdate(w);
    ui_draw_contents(w);
    EndUpdate(w);

    SetPort(savePort);
}

static Boolean ui_handle_menu(long menuChoice, Boolean *outQuit)
{
    short menuID = HiWord(menuChoice);
    short item   = LoWord(menuChoice);

    if (menuID == 129) /* File */
    {
        if (item == 1) /* Quit */
            *outQuit = true;
    }

    HiliteMenu(0);
    return true;
}

static Boolean ui_handle_mouse_down(EventRecord *ev, Boolean *outQuit)
{
    WindowPtr w;
    short part = FindWindow(ev->where, &w);

    switch (part)
    {
        case inMenuBar:
        {
            long choice = MenuSelect(ev->where);
            if (choice)
                ui_handle_menu(choice, outQuit);
            return true;
        }

        case inDrag:
            DragWindow(w, ev->where, &qd.screenBits.bounds);
            return true;

        case inGoAway:
            if (TrackGoAway(w, ev->where))
                *outQuit = true;
            return true;

        case inContent:
            if (w != FrontWindow())
            {
                SelectWindow(w);
                return true;
            }
            else
            {
                ControlHandle c;
                short cpart;
                Point local = ev->where;

                SetPort(w);
                GlobalToLocal(&local);

                cpart = FindControl(local, w, &c);
                if (cpart)
                {
                    if (TrackControl(c, local, NULL))
                    {
                        /* Control reference holds our ID */
                        long ref = 0;
                        ref = GetControlReference(c);
                        if ((short)ref == kQuitButtonID)
                            *outQuit = true;
                    }
                    return true;
                }
            }
            return false;

        default:
            return false;
    }
}

void ui_app_init(void)
{
    ui_init_toolbox();
    ui_init_menus_codeonly();
    ui_create_main_window();
}

Boolean ui_app_pump_events(void)
{
    EventRecord ev;
    Boolean quit = false;

    if (WaitNextEvent(everyEvent, &ev, 30, NULL))
    {
        switch (ev.what)
        {
            case mouseDown:
                (void)ui_handle_mouse_down(&ev, &quit);
                break;

            case updateEvt:
                ui_handle_update((WindowPtr)ev.message);
                break;

            case keyDown:
            case autoKey:
                /* Cmd-Q quit shortcut (matches the menu item) */
                if ((ev.modifiers & cmdKey) != 0)
                {
                    char c = (char)(ev.message & charCodeMask);
                    if (c == 'q' || c == 'Q')
                        quit = true;
                }
                break;

            default:
                break;
        }
    }

    /* Integration point:
       Later, call tcp_poll() and speech_pump() here (nonblocking), while the loop stays responsive. */

    return !quit;
}
