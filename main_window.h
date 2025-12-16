#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Types.h>
#include <Events.h>
#include <Windows.h>

void main_window_create(void);
void main_window_handle_update(WindowPtr w);
Boolean main_window_handle_mouse_down(EventRecord *ev, Boolean *outQuit);
Boolean main_window_handle_key(EventRecord *ev);
WindowPtr main_window_get(void);

#endif /* MAIN_WINDOW_H */
