#ifndef UI_RADIO_H
#define UI_RADIO_H

#include <Controls.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIRadioGroup
{
    ControlHandle *buttons;
    short          count;
} UIRadioGroup;

ControlHandle ui_radio_create(WindowPtr window, const Rect *frame,
                              ConstStr255Param title, short initialValue);

void ui_radio_group_init(UIRadioGroup *group, ControlHandle *buttons, short count);

Boolean ui_radio_set_selected(UIRadioGroup *group, ControlHandle selected);

short ui_radio_selected_index(const UIRadioGroup *group);

Boolean ui_radio_handle_hit(UIRadioGroup *group, ControlHandle hit, short part);

#ifdef __cplusplus
}
#endif

#endif /* UI_RADIO_H */
