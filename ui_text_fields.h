#ifndef UI_TEXT_FIELDS_H
#define UI_TEXT_FIELDS_H

#include <Types.h>
#include <Windows.h>
#include <Controls.h>
#include <TextEdit.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UITextField
{
    TEHandle handle;
    Boolean  singleLine;
} UITextField;

typedef struct UIScrollingText
{
    UITextField   field;
    ControlHandle scroll;
    short         scrollOffset;
} UIScrollingText;

void ui_text_field_scroll_rect(const Rect *frame, Rect *outScrollRect);
void ui_text_field_init(UITextField *field, WindowPtr window, const Rect *frame, const char *text, Boolean singleLine, Boolean reserveScrollbar);
void ui_text_field_update(const UITextField *field, WindowPtr window);
Boolean ui_text_field_key(UITextField *field, WindowPtr window, char c);
void ui_text_field_idle(const UITextField *field, WindowPtr window);
void ui_text_fields_set_colors(void);
GrafPtr ui_text_set_active_port(TEHandle handle, WindowPtr fallback);
void ui_text_restore_port(GrafPtr savePort);

void ui_text_scrolling_init(UIScrollingText *area, WindowPtr window, const Rect *frame, const Rect *scrollRect, const char *text);
void ui_text_scrolling_apply_scroll(UIScrollingText *area, short offset);
void ui_text_scrolling_update_scrollbar(UIScrollingText *area);
void ui_text_scrolling_scroll_selection_into_view(UIScrollingText *area);
pascal void ui_text_scrolling_track(ControlHandle control, short part);
Boolean ui_text_scrolling_handle_key(UIScrollingText *area, WindowPtr window, char c);
Boolean ui_text_field_get_text(const UITextField *field, char *buffer, short maxLen);

#ifdef __cplusplus
}
#endif

#endif /* UI_TEXT_FIELDS_H */
