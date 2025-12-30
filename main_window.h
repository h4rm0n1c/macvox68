#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Types.h>
#include <Windows.h>

#include "ui_input.h"

void main_window_create(void);
void main_window_handle_update(const InputEvent *ev);
Boolean main_window_handle_mouse_down(const InputEvent *ev, Boolean *outQuit);
Boolean main_window_handle_key(const InputEvent *ev, Boolean *outQuit);
void main_window_idle(void);
WindowPtr main_window_get(void);

#endif /* MAIN_WINDOW_H */
