#include "ui/ui_input.h"

#include <Menus.h>
#include <Quickdraw.h>

static void ui_input_fill_modifiers(InputEvent *outEvent, const EventRecord *ev)
{
    if (!outEvent || !ev)
        return;

    outEvent->commandDown = (Boolean)((ev->modifiers & cmdKey) != 0);
    outEvent->shiftDown = (Boolean)((ev->modifiers & shiftKey) != 0);
    outEvent->optionDown = (Boolean)((ev->modifiers & optionKey) != 0);
    outEvent->controlDown = (Boolean)((ev->modifiers & controlKey) != 0);
}

static void ui_input_prepare_points(InputEvent *outEvent)
{
    if (!outEvent)
        return;

    outEvent->global.h = outEvent->raw.where.h;
    outEvent->global.v = outEvent->raw.where.v;
    outEvent->local = outEvent->global;

    if (outEvent->window)
    {
        GrafPtr oldPort = NULL;
        GetPort(&oldPort);
        SetPort(outEvent->window);
        GlobalToLocal(&outEvent->local);
        if (oldPort)
            SetPort(oldPort);
    }
}

static Boolean ui_input_dispatch_mouse(InputDispatcher *dispatcher, InputEvent *ev, Boolean *outQuit)
{
    WindowPtr w = NULL;
    short part;

    if (!dispatcher || !ev)
        return false;

    part = FindWindow(ev->raw.where, &w);
    ev->window = w;
    ev->type = kInputEventMouseDown;
    ui_input_prepare_points(ev);

    if (dispatcher->overlayIsWindow && dispatcher->overlayIsWindow(ev->window) &&
        dispatcher->overlayHandlers.onMouseDown)
    {
        if (dispatcher->overlayHandlers.onMouseDown(ev, outQuit))
            return true;
    }

    if (dispatcher->mainHandlers.onMouseDown)
        return dispatcher->mainHandlers.onMouseDown(ev, outQuit);

    (void)part;
    return false;
}

static Boolean ui_input_dispatch_key(InputDispatcher *dispatcher, InputEvent *ev, Boolean *outQuit)
{
    if (!dispatcher || !ev)
        return false;

    ev->window = FrontWindow();
    ev->type = kInputEventKeyDown;
    ev->keyChar = (char)(ev->raw.message & charCodeMask);

    if (ev->commandDown && (ev->keyChar == 'q' || ev->keyChar == 'Q'))
    {
        if (outQuit)
            *outQuit = true;
    }

    if (dispatcher->overlayIsWindow && dispatcher->overlayIsWindow(ev->window) &&
        dispatcher->overlayHandlers.onKeyDown)
    {
        if (dispatcher->overlayHandlers.onKeyDown(ev, outQuit))
            return true;
    }

    if (dispatcher->mainHandlers.onKeyDown)
        return dispatcher->mainHandlers.onKeyDown(ev, outQuit);

    return false;
}

static Boolean ui_input_dispatch_update(InputDispatcher *dispatcher, InputEvent *ev)
{
    if (!dispatcher || !ev)
        return false;

    ev->window = (WindowPtr)ev->raw.message;
    ev->type = kInputEventUpdate;
    ui_input_prepare_points(ev);

    if (dispatcher->overlayIsWindow && dispatcher->overlayIsWindow(ev->window) &&
        dispatcher->overlayHandlers.onUpdate)
    {
        dispatcher->overlayHandlers.onUpdate(ev);
        return true;
    }

    if (dispatcher->mainHandlers.onUpdate)
    {
        dispatcher->mainHandlers.onUpdate(ev);
        return true;
    }

    return false;
}

void ui_input_dispatcher_init(InputDispatcher *dispatcher, const InputWindowHandlers *mainHandlers)
{
    if (!dispatcher)
        return;

    dispatcher->mainHandlers = *mainHandlers;
    dispatcher->overlayHandlers.onMouseDown = NULL;
    dispatcher->overlayHandlers.onKeyDown = NULL;
    dispatcher->overlayHandlers.onUpdate = NULL;
    dispatcher->overlayIsWindow = NULL;
}

void ui_input_dispatcher_set_overlay(InputDispatcher *dispatcher,
                                     Boolean (*isWindow)(WindowPtr w),
                                     const InputWindowHandlers *handlers)
{
    if (!dispatcher || !handlers)
        return;

    dispatcher->overlayIsWindow = isWindow;
    dispatcher->overlayHandlers = *handlers;
}

Boolean ui_input_dispatcher_handle_event(InputDispatcher *dispatcher, const EventRecord *ev, Boolean *outQuit)
{
    InputEvent input;

    if (!dispatcher || !ev)
        return false;

    input.type = kInputEventNone;
    input.window = NULL;
    input.global.h = input.global.v = 0;
    input.local = input.global;
    input.keyChar = 0;
    input.commandDown = false;
    input.shiftDown = false;
    input.optionDown = false;
    input.controlDown = false;
    input.raw = *ev;

    ui_input_fill_modifiers(&input, ev);

    switch (ev->what)
    {
        case mouseDown:
            return ui_input_dispatch_mouse(dispatcher, &input, outQuit);

        case keyDown:
        case autoKey:
            return ui_input_dispatch_key(dispatcher, &input, outQuit);

        case updateEvt:
            return ui_input_dispatch_update(dispatcher, &input);

        default:
            break;
    }

    return false;
}
