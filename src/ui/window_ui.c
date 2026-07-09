#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#include "ui/about_box.h"
#include "ui/main_window.h"
#include "ui/ui_input.h"
#include "ui/ui_app.h"
#include "network.h"

static InputDispatcher gInputDispatcher;

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

static Boolean window_ui_about_mouse(const InputEvent *ev, Boolean *outQuit)
{
    EventRecord raw;

    (void)outQuit;

    if (!ev)
        return false;

    raw = ev->raw;
    return about_box_handle_mouse_down(&raw);
}

static void window_ui_about_update(const InputEvent *ev)
{
    if (ev && ev->window)
        about_box_handle_update(ev->window);
}

void ui_app_init(void)
{
    InputWindowHandlers mainHandlers;
    InputWindowHandlers aboutHandlers;

    ui_init_toolbox();
    ui_init_menus_codeonly();

    mainHandlers.onMouseDown = main_window_handle_mouse_down;
    mainHandlers.onKeyDown   = main_window_handle_key;
    mainHandlers.onUpdate    = main_window_handle_update;

    aboutHandlers.onMouseDown = window_ui_about_mouse;
    aboutHandlers.onKeyDown   = NULL;
    aboutHandlers.onUpdate    = window_ui_about_update;

    ui_input_dispatcher_init(&gInputDispatcher, &mainHandlers);
    ui_input_dispatcher_set_overlay(&gInputDispatcher, about_box_is_window, &aboutHandlers);

    main_window_create();
}

Boolean ui_app_pump_events(void)
{
    EventRecord ev;
    Boolean quit = false;

    if (WaitNextEvent(everyEvent, &ev, 30, NULL))
    {
        (void)ui_input_dispatcher_handle_event(&gInputDispatcher, &ev, &quit);
    }

    main_window_idle();

    network_poll();

    /* Integration point:
       Later, call tcp_poll() and speech_pump() here (nonblocking), while the loop stays responsive. */

    return !quit;
}
