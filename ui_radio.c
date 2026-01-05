#include "ui_radio.h"

#include <ControlDefinitions.h>

ControlHandle ui_radio_create(WindowPtr window, const Rect *frame,
                              ConstStr255Param title, short initialValue)
{
    return NewControl(window, frame, title, true, initialValue, 0, 0, radioButProc, 0);
}

void ui_radio_group_init(UIRadioGroup *group, ControlHandle *buttons, short count)
{
    if (!group)
        return;

    group->buttons = buttons;
    group->count   = count;
}

Boolean ui_radio_set_selected(UIRadioGroup *group, ControlHandle selected)
{
    Boolean changed = false;
    short i;

    if (!group || !group->buttons || group->count <= 0 || !selected)
        return false;

    for (i = 0; i < group->count; ++i)
    {
        ControlHandle h = group->buttons[i];
        short value = (h == selected) ? 1 : 0;

        if (h && GetControlValue(h) != value)
        {
            SetControlValue(h, value);
            changed = true;
        }
    }

    return changed;
}

short ui_radio_selected_index(const UIRadioGroup *group)
{
    short i;

    if (!group || !group->buttons || group->count <= 0)
        return -1;

    for (i = 0; i < group->count; ++i)
    {
        ControlHandle h = group->buttons[i];

        if (h && GetControlValue(h) != 0)
            return i;
    }

    return -1;
}

Boolean ui_radio_handle_hit(UIRadioGroup *group, ControlHandle hit, short part)
{
    if (!group || !hit || part == 0)
        return false;

    return ui_radio_set_selected(group, hit);
}
