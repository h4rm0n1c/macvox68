#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#include "main_window.h"
#include "ui_app.h"

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

void ui_app_init(void)
{
    ui_init_toolbox();
    ui_init_menus_codeonly();
    main_window_create();
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
                (void)main_window_handle_mouse_down(&ev, &quit);
                break;

            case updateEvt:
                main_window_handle_update((WindowPtr)ev.message);
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

                if (!quit)
                    (void)main_window_handle_key(&ev);
                break;

            default:
                break;
        }
    }

    /* Integration point:
       Later, call tcp_poll() and speech_pump() here (nonblocking), while the loop stays responsive. */

    return !quit;
}
