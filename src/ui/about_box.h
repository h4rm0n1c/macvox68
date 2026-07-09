#ifndef ABOUT_BOX_H
#define ABOUT_BOX_H

#include <Types.h>
#include <Events.h>
#include <Windows.h>

void about_box_show(void);
void about_box_close(void);
void about_box_handle_update(WindowPtr w);
Boolean about_box_is_window(WindowPtr w);
Boolean about_box_handle_mouse_down(EventRecord *ev);

#endif /* ABOUT_BOX_H */
