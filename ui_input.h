#ifndef UI_INPUT_H
#define UI_INPUT_H

#include <Events.h>
#include <Types.h>
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum InputEventType
{
    kInputEventNone = 0,
    kInputEventMouseDown,
    kInputEventKeyDown,
    kInputEventUpdate,
    kInputEventMouseWheel
} InputEventType;

typedef struct InputEvent
{
    InputEventType type;
    WindowPtr      window;
    Point          global;
    Point          local;
    char           keyChar;
    short          wheelDelta;
    Boolean        commandDown;
    Boolean        shiftDown;
    Boolean        optionDown;
    Boolean        controlDown;
    EventRecord    raw;
} InputEvent;

typedef Boolean (*InputMouseHandler)(const InputEvent *ev, Boolean *outQuit);
typedef Boolean (*InputKeyHandler)(const InputEvent *ev, Boolean *outQuit);
typedef Boolean (*InputMouseWheelHandler)(const InputEvent *ev, Boolean *outQuit);
typedef void (*InputUpdateHandler)(const InputEvent *ev);

typedef struct InputWindowHandlers
{
    InputMouseHandler  onMouseDown;
    InputKeyHandler    onKeyDown;
    InputMouseWheelHandler onMouseWheel;
    InputUpdateHandler onUpdate;
} InputWindowHandlers;

typedef struct InputDispatcher
{
    InputWindowHandlers mainHandlers;
    InputWindowHandlers overlayHandlers;
    Boolean (*overlayIsWindow)(WindowPtr w);
} InputDispatcher;

void ui_input_dispatcher_init(InputDispatcher *dispatcher, const InputWindowHandlers *mainHandlers);
void ui_input_dispatcher_set_overlay(InputDispatcher *dispatcher,
                                     Boolean (*isWindow)(WindowPtr w),
                                     const InputWindowHandlers *handlers);
Boolean ui_input_dispatcher_handle_event(InputDispatcher *dispatcher, const EventRecord *ev, Boolean *outQuit);

#ifdef __cplusplus
}
#endif

#endif /* UI_INPUT_H */
