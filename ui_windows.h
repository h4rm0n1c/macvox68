#ifndef UI_WINDOWS_H
#define UI_WINDOWS_H

#include <Types.h>
#include <Windows.h>
#include <Controls.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIWindowSpec
{
    ConstStr255Param title;
    short width;
    short height;
    short margin;
} UIWindowSpec;

typedef struct UIControlTrackingSpec
{
    ControlHandle control;
    ControlActionUPP action;
    const RGBColor *background;
} UIControlTrackingSpec;

typedef void (*UIWindowDrawProc)(WindowPtr window, void *refCon);

WindowPtr ui_windows_create_standard(const UIWindowSpec *spec);
void ui_windows_draw(WindowPtr window, UIWindowDrawProc drawProc, void *refCon);

ControlHandle ui_windows_new_button(WindowPtr window, const Rect *frame,
                                    ConstStr255Param title, Boolean isDefault, short controlID);
ControlHandle ui_windows_new_radio(WindowPtr window, const Rect *frame,
                                   ConstStr255Param title, short initialValue);
ControlHandle ui_windows_new_slider(WindowPtr window, const Rect *frame,
                                    short initial, short min, short max);

Boolean ui_windows_track_hit_control(WindowPtr window, Point local,
                                     const UIControlTrackingSpec *specs, short count);

#ifdef __cplusplus
}
#endif

#endif /* UI_WINDOWS_H */
