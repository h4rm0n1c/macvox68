#include <Quickdraw.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <TextEdit.h>
#include <ToolUtils.h>
#include <Memory.h>
#include <string.h>

#include "ui/ui_text_fields.h"
#include "ui/ui_theme.h"

#ifndef kControlSliderProc
    #define kControlSliderProc 48
#endif
#ifndef inUpButton
    #define inUpButton 20
#endif
#ifndef inDownButton
    #define inDownButton 21
#endif
#ifndef inPageUp
    #define inPageUp 22
#endif
#ifndef inPageDown
    #define inPageDown 23
#endif
#ifndef inThumb
    #define inThumb 129
#endif

static const short kMaxTextDestGrowth = 30000;

GrafPtr ui_text_set_active_port(TEHandle handle, WindowPtr fallback)
{
    GrafPtr savePort = NULL;
    GrafPtr target   = fallback;

    if (handle)
    {
        TEPtr te = *handle;

        if (te && te->inPort)
            target = te->inPort;
    }

    if (target)
    {
        GetPort(&savePort);
        SetPort(target);
    }

    return savePort;
}

void ui_text_restore_port(GrafPtr savePort)
{
    if (savePort)
        SetPort(savePort);
}
static void ui_text_set_colors(void)
{
    ui_theme_apply_field_colors();
}

void ui_text_fields_set_colors(void)
{
    ui_theme_apply_field_colors();
}

void ui_text_field_scroll_rect(const Rect *frame, Rect *outScrollRect)
{
    const UITheme *theme = ui_theme_get();

    if (!frame || !outScrollRect)
        return;

    *outScrollRect = *frame;
    InsetRect(outScrollRect, theme->metrics.textInsetH, theme->metrics.textInsetV);
    outScrollRect->left = (short)(outScrollRect->right - theme->metrics.textScrollbarW);
}

void ui_text_field_init(UITextField *field, WindowPtr window, const Rect *frame, const char *text, Boolean singleLine, Boolean reserveScrollbar)
{
    Rect viewRect;
    Rect destRect;
    const UITheme *theme = ui_theme_get();

    if (!field || !window || !frame)
        return;

    viewRect = *frame;
    InsetRect(&viewRect, theme->metrics.textInsetH, theme->metrics.textInsetV);
    if (reserveScrollbar)
        viewRect.right = (short)(viewRect.right - theme->metrics.textScrollbarW);

    destRect = viewRect;
    destRect.bottom = (short)(destRect.top + kMaxTextDestGrowth);
    if (destRect.bottom < destRect.top)
        destRect.bottom = 32767;

    SetPort(window);
    ui_theme_apply_field_colors();

    field->handle = TENew(&destRect, &viewRect);
    field->singleLine = singleLine;

    if (field->handle)
    {
        (**field->handle).viewRect = viewRect;

        if (singleLine)
        {
            Rect singleRect = viewRect;
            short lineH    = (**field->handle).lineHeight;
            short slack    = (short)((singleRect.bottom - singleRect.top) - lineH);

            if (slack > 0)
            {
                singleRect.top += slack / 2;
                singleRect.bottom = (short)(singleRect.top + lineH);
            }
            else
            {
                singleRect.bottom = (short)(singleRect.top + lineH);
            }

            (**field->handle).destRect = singleRect;
            (**field->handle).viewRect = singleRect;
            (**field->handle).crOnly   = true;
        }
        else
        {
            (**field->handle).destRect = destRect;
            (**field->handle).viewRect = viewRect;
        }

        if (text)
            TEInsert(text, strlen(text), field->handle);
    }
}

static short ui_text_view_height(TEHandle handle)
{
    TEPtr te;

    if (!handle)
        return 0;

    te = *handle;
    if (!te)
        return 0;

    return (short)(te->viewRect.bottom - te->viewRect.top);
}

static short ui_text_height(TEHandle handle)
{
    TEPtr te;

    if (!handle)
        return 0;

    te = *handle;
    if (!te)
        return 0;

    return (short)(te->nLines * te->lineHeight);
}

static short ui_text_scroll_max(const UIScrollingText *area)
{
    short viewH;
    short textH;
    short max;

    if (!area || !area->field.handle)
        return 0;

    viewH = ui_text_view_height(area->field.handle);
    textH = ui_text_height(area->field.handle);
    max   = (short)(textH - viewH);

    return (max > 0) ? max : 0;
}

void ui_text_scrolling_apply_scroll(UIScrollingText *area, short offset)
{
    short max;
    GrafPtr savePort;

    if (!area || !area->field.handle)
        return;

    savePort = ui_text_set_active_port(area->field.handle, area->window);

    TECalText(area->field.handle);

    max = ui_text_scroll_max(area);
    if (offset < 0)
        offset = 0;
    if (offset > max)
        offset = max;

    if (offset != area->scrollOffset)
    {
        TEPinScroll(0, (short)(area->scrollOffset - offset), area->field.handle);
        area->scrollOffset = offset;
    }

    if (area->scroll)
        SetControlValue(area->scroll, area->scrollOffset);

    ui_text_restore_port(savePort);
}

void ui_text_scrolling_update_scrollbar(UIScrollingText *area)
{
    short max;
    GrafPtr savePort;

    if (!area || !area->field.handle || !area->scroll)
        return;

    savePort = ui_text_set_active_port(area->field.handle, area->window);

    TECalText(area->field.handle);

    max = ui_text_scroll_max(area);
    SetControlMaximum(area->scroll, max);

    if (area->scrollOffset > max)
        ui_text_scrolling_apply_scroll(area, max);
    else
        SetControlValue(area->scroll, area->scrollOffset);

    ui_text_restore_port(savePort);
}

void ui_text_scrolling_scroll_selection_into_view(UIScrollingText *area)
{
    TEPtr te;
    short viewH;
    Point caret;
    short caretTop;
    short caretBottom;
    short contentTop;
    short contentBottom;
    GrafPtr savePort;

    if (!area || !area->field.handle)
        return;

    savePort = ui_text_set_active_port(area->field.handle, area->window);

    te = *area->field.handle;
    if (!te)
    {
        ui_text_restore_port(savePort);
        return;
    }

    TECalText(area->field.handle);
    caret = TEGetPoint(te->selEnd, area->field.handle);
    viewH = ui_text_view_height(area->field.handle);

    caretTop = (short)(caret.v + area->scrollOffset - te->lineHeight);
    caretBottom = (short)(caret.v + area->scrollOffset + te->lineHeight);

    contentTop = area->scrollOffset;
    contentBottom = (short)(contentTop + viewH);

    if (caretTop < contentTop)
    {
        ui_text_scrolling_apply_scroll(area, caretTop);
    }
    else if (caretBottom > contentBottom)
    {
        ui_text_scrolling_apply_scroll(area, (short)(caretBottom - viewH));
    }

    ui_text_restore_port(savePort);
}

pascal void ui_text_scrolling_track(ControlHandle control, short part)
{
    UIScrollingText *area = (UIScrollingText *)(control ? (Ptr) GetControlReference(control) : NULL);
    short value = (short)(control ? GetControlValue(control) : 0);
    short newValue = value;
    short lineStep = 0;
    short pageStep = 0;
    short max = control ? GetControlMaximum(control) : 0;

    if (area && area->field.handle)
    {
        TEPtr te = *area->field.handle;
        if (te)
        {
            lineStep = te->lineHeight;
            pageStep = (short)(ui_text_view_height(area->field.handle) - te->lineHeight);
        }
    }

    if (lineStep <= 0)
        lineStep = 1;
    if (pageStep <= 0)
        pageStep = lineStep;

    switch (part)
    {
        case inUpButton:
            newValue = (short)(value - lineStep);
            break;
        case inDownButton:
            newValue = (short)(value + lineStep);
            break;
        case inPageUp:
            newValue = (short)(value - pageStep);
            break;
        case inPageDown:
            newValue = (short)(value + pageStep);
            break;
        case inThumb:
            newValue = GetControlValue(control);
            break;
        default:
            return;
    }

    if (newValue < 0)
        newValue = 0;
    if (newValue > max)
        newValue = max;

    if (newValue != value)
        SetControlValue(control, newValue);

    ui_text_scrolling_apply_scroll(area, newValue);
}

void ui_text_field_update(const UITextField *field, WindowPtr window)
{
    Rect view;
    GrafPtr savePort;

    if (!field || !field->handle || !window)
        return;

    savePort = ui_text_set_active_port(field->handle, window);

    ui_text_set_colors();
    view = (**field->handle).viewRect;
    TEUpdate(&view, field->handle);

    ui_text_restore_port(savePort);
}

Boolean ui_text_field_get_text(const UITextField *field, char *buffer, short maxLen)
{
    TEPtr te;
    long len;
    Ptr text;

    if (!field || !field->handle || !buffer || maxLen <= 0)
        return false;

    te = *field->handle;
    if (!te)
        return false;

    len = te->teLength;
    if (len >= maxLen)
        len = maxLen - 1;

    text = *(te->hText);
    if (!text)
        return false;

    HLock(te->hText);
    BlockMove(text, buffer, len);
    HUnlock(te->hText);

    buffer[len] = '\0';
    return true;
}

static Boolean ui_text_is_navigation_character(char c)
{
    unsigned char uc = (unsigned char)c;

    return (Boolean)((uc >= 0x1C && uc <= 0x1F));
}

static Boolean ui_text_should_accept_character(TEHandle handle, WindowPtr window, char c)
{
    TEPtr te;
    Boolean isBackspace;
    Boolean isReturn;
    Boolean isTab;
    Boolean isPrintable;
    Boolean isNavigation;
    unsigned char uc;

    if (!handle)
        return false;

    te = *handle;
    if (!te)
        return false;

    uc         = (unsigned char)c;
    isBackspace  = (Boolean)(c == 0x08 || c == 0x7F);
    isReturn     = (Boolean)(c == '\r' || c == '\n');
    isTab        = (Boolean)(c == '\t');
    isPrintable  = (Boolean)(uc >= 0x20);
    isNavigation = ui_text_is_navigation_character(c);

    if (!isBackspace && !isReturn && !isTab && !isPrintable && !isNavigation)
        return false;

    if (isNavigation)
        return true;

    if (te->crOnly)
    {
        if (isReturn)
            return false;

        if (!isBackspace)
        {
            short available = (short)(te->viewRect.right - te->viewRect.left - 2);
            short prefix    = te->selStart;
            short suffixLen = (short)(te->teLength - te->selEnd);
            short newLen    = (short)(prefix + 1 + suffixLen);

            if (newLen > 0 && newLen < 512)
            {
                char buffer[512];
                GrafPtr savePort = NULL;
                GrafPtr targetPort = te->inPort ? te->inPort : window;
                Ptr text = *(te->hText);

                BlockMoveData(text, buffer, prefix);
                buffer[prefix] = c;
                if (suffixLen > 0)
                    BlockMoveData(text + te->selEnd, buffer + prefix + 1, suffixLen);

                GetPort(&savePort);
                if (targetPort)
                    SetPort(targetPort);

                if (TextWidth(buffer, 0, newLen) > available)
                {
                    if (savePort)
                        SetPort(savePort);
                    return false;
                }

                if (savePort)
                    SetPort(savePort);
            }
        }
    }

    return true;
}

Boolean ui_text_field_key(UITextField *field, WindowPtr window, char c)
{
    GrafPtr savePort;

    if (!field || !field->handle || !window)
        return false;

    if (!ui_text_should_accept_character(field->handle, window, c))
        return true;

    GetPort(&savePort);
    SetPort(window);
    ui_text_set_colors();
    TEKey(c, field->handle);
    if (savePort)
        SetPort(savePort);

    return true;
}

void ui_text_field_idle(const UITextField *field, WindowPtr window)
{
    GrafPtr savePort;

    if (!field || !field->handle || !window)
        return;

    GetPort(&savePort);
    SetPort(window);
    ui_text_set_colors();
    TEIdle(field->handle);
    if (savePort)
        SetPort(savePort);
}

Boolean ui_text_scrolling_handle_key(UIScrollingText *area, WindowPtr window, char c)
{
    Boolean handled = ui_text_field_key(&area->field, window, c);

    if (handled)
    {
        ui_text_scrolling_update_scrollbar(area);
        ui_text_scrolling_scroll_selection_into_view(area);
    }

    return handled;
}

void ui_text_scrolling_init(UIScrollingText *area, WindowPtr window, const Rect *frame, const Rect *scrollRect, const char *text)
{
    if (!area)
        return;

    area->field.handle = NULL;
    area->field.singleLine = false;
    area->scroll = NULL;
    area->scrollOffset = 0;
    area->window = window;

    ui_text_field_init(&area->field, window, frame, text, false, true);

    if (window && scrollRect)
    {
        area->scroll = NewControl(window, scrollRect, "\p", true, 0, 0, 0, scrollBarProc, 0);
        if (area->scroll)
            SetControlReference(area->scroll, (long)area);
    }

    if (area->field.handle)
        ui_text_scrolling_update_scrollbar(area);
}
