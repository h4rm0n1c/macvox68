#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Events.h>
#include <Menus.h>
#include <Memory.h>
#include <TextEdit.h>
#include <ToolUtils.h>
#include <string.h>

#include "about_box.h"
#include "main_window.h"

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif
#ifndef kControlPushButtonDefaultTag
    #define kControlPushButtonDefaultTag 'dflt'
#endif
#ifndef kControlSliderProc
    #define kControlSliderProc 48
#endif

enum
{
    kTextEditID       = 1,
    kSpeakStopBtnID   = 3
};

typedef struct UILayout
{
    Rect editText;
    Rect speakStopButton;
    Rect soundGroup;
    Rect soundPopup;
    Rect applyButton;
    Rect prosodyGroup;
    Rect prosodyClean;
    Rect prosodyLQ;
    Rect prosodyHQ;
    Rect settingsGroup;
    Rect volumeSlider;
    Rect rateSlider;
    Rect pitchSlider;
    Rect tcpGroup;
    Rect hostField;
    Rect portField;
    Rect startButton;
} UILayout;

typedef enum
{
    kSpeechIdleState,
    kSpeechSpeakingState
} SpeechUIState;

static WindowPtr      gMainWin  = NULL;
static ControlHandle  gSpeakBtn = NULL;
static ControlHandle  gSoundPop = NULL;
static ControlHandle  gApplyBtn = NULL;
static ControlHandle  gProsodyClean = NULL;
static ControlHandle  gProsodyLQ    = NULL;
static ControlHandle  gProsodyHQ    = NULL;
static ControlHandle  gVolumeSlider = NULL;
static ControlHandle  gRateSlider   = NULL;
static ControlHandle  gPitchSlider  = NULL;
static ControlHandle  gStartBtn     = NULL;
static ControlHandle  gTextScroll  = NULL;
static TEHandle       gTextEdit     = NULL;
static TEHandle       gHostEdit     = NULL;
static TEHandle       gPortEdit     = NULL;
static TEHandle       gActiveEdit   = NULL;
static UILayout       gLayout;
static const RGBColor kTextColor = { 0x0000, 0x0000, 0x0000 };
static const RGBColor kTextBackColor = { 0xFFFF, 0xFFFF, 0xFFFF };
static const short    kScrollBarWidth = 16;
static short          gTextScrollPos = 0;

static void main_window_prepare_text_port(GrafPtr *outPort, RGBColor *outFore, RGBColor *outBack)
{
    if (outPort)
        GetPort(outPort);
    if (outFore)
        GetForeColor(outFore);
    if (outBack)
        GetBackColor(outBack);

    if (gMainWin)
        SetPort(gMainWin);
    ForeColor(blackColor);
    BackColor(whiteColor);
    RGBForeColor(&kTextColor);
    RGBBackColor(&kTextBackColor);
}

static void main_window_restore_text_port(GrafPtr port, const RGBColor *fore, const RGBColor *back)
{
    if (port)
        SetPort(port);
    if (fore)
        RGBForeColor(fore);
    if (back)
        RGBBackColor(back);
}

static Rect main_window_text_view_rect(Boolean withScroll)
{
    Rect viewRect = gLayout.editText;

    InsetRect(&viewRect, 6, 4);
    if (withScroll)
        viewRect.right -= (kScrollBarWidth + 2);

    return viewRect;
}

static void main_window_apply_text_view_rect(TEHandle handle, Boolean withScroll)
{
    Rect viewRect;
    Rect destRect;
    TEPtr te;
    short lineH;
    short maxLines;

    if (!handle)
        return;

    te = *handle;
    if (!te)
        return;

    viewRect = main_window_text_view_rect(withScroll);
    destRect = viewRect;

    if (te->crOnly)
    {
        destRect.bottom = viewRect.bottom;
    }
    else
    {
        lineH = te->lineHeight;
        if (lineH <= 0)
            lineH = 1;

        maxLines = (short)((viewRect.bottom - viewRect.top) / lineH);
        if (maxLines < 1)
            maxLines = 1;

        destRect.bottom = destRect.top + (lineH * maxLines);
        if (destRect.bottom > viewRect.bottom)
            destRect.bottom = viewRect.bottom;

        viewRect.bottom = destRect.bottom;
    }

    te->viewRect = viewRect;
    te->destRect = destRect;
}

static short main_window_visible_lines(TEHandle handle)
{
    TEPtr te;
    short lineH;
    short height;

    if (!handle)
        return 0;

    te = *handle;
    if (!te)
        return 0;

    lineH = te->lineHeight;
    if (lineH <= 0)
        return 0;

    height = te->viewRect.bottom - te->viewRect.top;
    if (height <= 0)
        return 0;

    return (short)(height / lineH);
}

static short main_window_line_for_offset(TEHandle handle, short offset)
{
    TEPtr te;
    short *starts;
    short i;
    short line = 0;

    if (!handle)
        return 0;

    te = *handle;
    if (!te || !te->lineStarts || te->nLines <= 0)
        return 0;

    HLock((Handle)te->lineStarts);
    starts = (short *)(*(te->lineStarts));

    if (!starts)
    {
        HUnlock((Handle)te->lineStarts);
        return 0;
    }

    for (i = 0; i < te->nLines; i++)
    {
        short start = starts[i];
        short end = (i + 1 < te->nLines) ? starts[i + 1] : te->teLength;

        if (offset >= start && offset <= end)
        {
            line = i;
            break;
        }
    }

    HUnlock((Handle)te->lineStarts);
    return line;
}

static void main_window_scroll_text_to(short newTopLine)
{
    TEPtr te;
    short lineH;
    short delta;
    GrafPtr savePort = NULL;
    RGBColor saveFore;
    RGBColor saveBack;

    if (!gTextEdit)
        return;

    te = *gTextEdit;
    if (!te)
        return;

    lineH = te->lineHeight;
    if (lineH <= 0)
        return;

    if (newTopLine < 0)
        newTopLine = 0;

    delta = (short)(newTopLine - gTextScrollPos);
    if (delta == 0)
        return;

    main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
    TEScroll(0, (short)(-delta * lineH), gTextEdit);
    main_window_restore_text_port(savePort, &saveFore, &saveBack);

    gTextScrollPos = newTopLine;
    if (gTextScroll)
        SetControlValue(gTextScroll, gTextScrollPos);
}

static void main_window_update_text_scrollbar(Boolean scrollToCaret)
{
    TEPtr te;
    short visibleLines;
    short maxScroll;
    short totalLines;
    Boolean needsScroll;
    Rect scrollRect;
    GrafPtr savePort = NULL;
    RGBColor saveFore;
    RGBColor saveBack;

    if (!gTextEdit)
        return;

    main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
    TECalText(gTextEdit);
    main_window_restore_text_port(savePort, &saveFore, &saveBack);

    te = *gTextEdit;
    if (!te)
        return;

    visibleLines = main_window_visible_lines(gTextEdit);
    if (visibleLines < 1)
        visibleLines = 1;

    totalLines = te->nLines;
    maxScroll = (short)(totalLines - visibleLines);
    if (maxScroll < 0)
        maxScroll = 0;

    needsScroll = (maxScroll > 0);

    if (!needsScroll)
    {
        if (gTextScrollPos != 0)
            main_window_scroll_text_to(0);

        if (gTextScroll)
        {
            DisposeControl(gTextScroll);
            gTextScroll = NULL;
        }

        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
        main_window_apply_text_view_rect(gTextEdit, false);
        TECalText(gTextEdit);
        main_window_restore_text_port(savePort, &saveFore, &saveBack);
        return;
    }

    if (!gTextScroll)
    {
        scrollRect = gLayout.editText;
        InsetRect(&scrollRect, 1, 1);
        scrollRect.left = (short)(scrollRect.right - kScrollBarWidth);

        gTextScroll = NewControl(gMainWin, &scrollRect, "\p", true, 0, 0, maxScroll, scrollBarProc, 0);
        gTextScrollPos = 0;
        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
        main_window_apply_text_view_rect(gTextEdit, true);
        TECalText(gTextEdit);
        main_window_restore_text_port(savePort, &saveFore, &saveBack);
    }
    else
    {
        SetControlMaximum(gTextScroll, maxScroll);
    }

    if (gTextScrollPos > maxScroll)
        main_window_scroll_text_to(maxScroll);

    if (scrollToCaret)
    {
        short line = main_window_line_for_offset(gTextEdit, te->selEnd);
        short target = gTextScrollPos;

        if (line < gTextScrollPos)
            target = line;
        else if (line > (short)(gTextScrollPos + visibleLines - 1))
            target = (short)(line - (visibleLines - 1));

        if (target < 0)
            target = 0;
        if (target > maxScroll)
            target = maxScroll;

        if (target != gTextScrollPos)
            main_window_scroll_text_to(target);
    }

    if (gTextScroll)
        SetControlValue(gTextScroll, gTextScrollPos);
}

static void main_window_switch_active_edit(TEHandle h)
{
    GrafPtr savePort = NULL;
    RGBColor saveFore;
    RGBColor saveBack;

    if (gActiveEdit == h)
        return;

    main_window_prepare_text_port(&savePort, &saveFore, &saveBack);

    if (gActiveEdit)
        TEDeactivate(gActiveEdit);

    gActiveEdit = h;

    if (gActiveEdit)
        TEActivate(gActiveEdit);

    main_window_restore_text_port(savePort, &saveFore, &saveBack);
}

static void main_window_plan_layout(void)
{
    Rect content;
    short margin        = 16;
    short gutter        = 12;
    short buttonW       = 86;
    short buttonH       = 22;
    short soundPopupW   = 214;
    short sectionGutter = 14;
    short textAreaH     = 170;
    short soundH        = 60;
    short prosodyH      = 76;
    short settingsH     = 108;
    short tcpH          = 60;
    short sliderH       = 16;
    short sliderW       = 210;
    short fieldH        = 26;
    short fieldW        = 152;
    short portFieldW    = 64;

    if (!gMainWin)
        return;

    content = gMainWin->portRect;

    /* Top row: Speak/Stop (left). */
    SetRect(&gLayout.speakStopButton,
            content.left + margin,
            content.top + margin,
            content.left + margin + buttonW,
            content.top + margin + buttonH);

    /* Text area sits beneath the control row. */
    SetRect(&gLayout.editText,
            content.left + margin,
            gLayout.speakStopButton.bottom + gutter,
            content.right - margin,
            gLayout.speakStopButton.bottom + gutter + textAreaH);

    /* Section sequencing mirrors the Windows UI rows. */
    short y = gLayout.editText.bottom + sectionGutter;

    SetRect(&gLayout.soundGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + soundH);

    SetRect(&gLayout.soundPopup,
            gLayout.soundGroup.left + 96,
            gLayout.soundGroup.top + 16,
            gLayout.soundGroup.left + 96 + soundPopupW,
            gLayout.soundGroup.top + 16 + buttonH);

    SetRect(&gLayout.applyButton,
            gLayout.soundGroup.right - margin - 86,
            gLayout.soundGroup.top + 14,
            gLayout.soundGroup.right - margin,
            gLayout.soundGroup.top + 14 + buttonH);

    y = gLayout.soundGroup.bottom + sectionGutter - 2;

    SetRect(&gLayout.prosodyGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + prosodyH);

    {
        short radioTop        = gLayout.prosodyGroup.top + 44;
        short radioLeft       = gLayout.prosodyGroup.left + 62;
        short radioGap        = 22;
        short radioWidth      = 140;
        short radioCleanWidth = 70;

        SetRect(&gLayout.prosodyClean,
                radioLeft,
                radioTop,
                radioLeft + radioCleanWidth,
                radioTop + 18);

        radioLeft = gLayout.prosodyClean.right + radioGap - 15;
        SetRect(&gLayout.prosodyLQ,
                radioLeft,
                radioTop,
                radioLeft + radioWidth,
                radioTop + 18);

        radioLeft = gLayout.prosodyLQ.right + radioGap - 15;
        SetRect(&gLayout.prosodyHQ,
                radioLeft,
                radioTop,
                radioLeft + radioWidth,
                radioTop + 18);
    }

    y = gLayout.prosodyGroup.bottom + sectionGutter - 2;

    SetRect(&gLayout.settingsGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + settingsH);

    SetRect(&gLayout.volumeSlider,
            gLayout.settingsGroup.left + 124,
            gLayout.settingsGroup.top + 14,
            gLayout.settingsGroup.left + 124 + sliderW,
            gLayout.settingsGroup.top + 14 + sliderH);

    SetRect(&gLayout.rateSlider,
            gLayout.volumeSlider.left,
            gLayout.volumeSlider.bottom + 14,
            gLayout.volumeSlider.right,
            gLayout.volumeSlider.bottom + 14 + sliderH);

    SetRect(&gLayout.pitchSlider,
            gLayout.rateSlider.left,
            gLayout.rateSlider.bottom + 14,
            gLayout.rateSlider.right,
            gLayout.rateSlider.bottom + 14 + sliderH);

    y = gLayout.settingsGroup.bottom + sectionGutter - 2;

    SetRect(&gLayout.tcpGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + tcpH);

    SetRect(&gLayout.hostField,
            gLayout.tcpGroup.left + 80,
            gLayout.tcpGroup.top + 19,
            gLayout.tcpGroup.left + 80 + fieldW,
            gLayout.tcpGroup.top + 19 + fieldH);

    SetRect(&gLayout.portField,
            gLayout.hostField.right + 56,
            gLayout.tcpGroup.top + 19,
            gLayout.hostField.right + 56 + portFieldW,
            gLayout.tcpGroup.top + 19 + fieldH);

    SetRect(&gLayout.startButton,
            gLayout.tcpGroup.right - 118,
            gLayout.tcpGroup.top + 18,
            gLayout.tcpGroup.right - 118 + 104,
            gLayout.tcpGroup.top + 18 + buttonH);
}

static void main_window_update_control_enabling(SpeechUIState state)
{
    /* Controls respond to speech activity. Menu-based Quit remains available. */
    if (gSpeakBtn)
    {
        if (state == kSpeechSpeakingState)
        {
            SetControlTitle(gSpeakBtn, "\pStop");
            HiliteControl(gSpeakBtn, 0);
        }
        else
        {
            SetControlTitle(gSpeakBtn, "\pSpeak");
            HiliteControl(gSpeakBtn, 0);
        }
    }
}

static TEHandle main_window_create_text_field(const Rect *frame, const char *text, Boolean singleLine)
{
    Rect viewRect;
    Rect destRect;
    TEHandle handle = NULL;
    short maxLines = 0;

    viewRect = *frame;
    InsetRect(&viewRect, 6, 4);

    destRect = viewRect;
    /* Keep the caret constrained to the visible box. */
    destRect.bottom = viewRect.bottom;

    SetPort(gMainWin);
    BackColor(whiteColor);
    ForeColor(blackColor);

    handle = TENew(&destRect, &viewRect);
    if (handle)
    {
        (**handle).viewRect = viewRect;

        if (singleLine)
        {
            Rect singleRect = viewRect;
            short lineH    = (**handle).lineHeight;
            short slack    = (singleRect.bottom - singleRect.top) - lineH;

            if (slack > 0)
            {
                singleRect.top += slack / 2;
                singleRect.bottom = singleRect.top + lineH;
            }
            else
            {
                singleRect.bottom = singleRect.top + lineH;
            }

            (**handle).destRect = singleRect;
            (**handle).viewRect = singleRect;
            (**handle).crOnly   = true;
        }
        else
        {
            short lineH = (**handle).lineHeight;

            if (lineH > 0)
            {
                maxLines = (short)((viewRect.bottom - viewRect.top) / lineH);
                if (maxLines < 1)
                    maxLines = 1;
            }

            destRect.bottom = destRect.top + (lineH * maxLines);
            if (destRect.bottom > viewRect.bottom)
                destRect.bottom = viewRect.bottom;

            (**handle).destRect      = destRect;
            (**handle).viewRect.bottom = destRect.bottom;
        }

        if (text)
            TEInsert(text, strlen(text), handle);
    }

    return handle;
}

static short main_window_max_lines_for(TEHandle handle)
{
    TEPtr te;
    short lineH;

    if (!handle)
        return 0;

    te = *handle;
    lineH = te->lineHeight;

    if (lineH <= 0)
        return 0;

    return (short)((te->destRect.bottom - te->destRect.top) / lineH);
}

static Boolean main_window_current_line_bounds(TEHandle handle, short *outStart, short *outEnd, short *outLine)
{
    TEPtr te;
    short *starts;
    short i;

    if (!handle || !outStart || !outEnd)
        return false;

    te = *handle;

    if (!te || !te->lineStarts || te->nLines <= 0)
        return false;

    HLock((Handle)te->lineStarts);
    starts = (short *)(*(te->lineStarts));

    if (!starts)
    {
        HUnlock((Handle)te->lineStarts);
        return false;
    }

    for (i = 0; i < te->nLines; i++)
    {
        short start = starts[i];
        short end = (i + 1 < te->nLines) ? starts[i + 1] : te->teLength;

        if (te->selStart >= start && te->selStart <= end)
        {
            *outStart = start;
            *outEnd = end;
            if (outLine)
                *outLine = i;
            HUnlock((Handle)te->lineStarts);
            return true;
        }
    }

    HUnlock((Handle)te->lineStarts);
    return false;
}

static Boolean main_window_insertion_overflows(TEHandle handle, char c)
{
    TEPtr te;
    short maxLines;
    short available;
    GrafPtr savePort = NULL;
    Boolean overflows = false;
    short lineStart = 0;
    short lineEnd = 0;
    short lineIndex = 0;

    if (!handle)
        return false;

    te = *handle;
    maxLines = main_window_max_lines_for(handle);

    if (!te || maxLines <= 0)
        return false;

    if (te->nLines < maxLines)
        return false;

    if (!main_window_current_line_bounds(handle, &lineStart, &lineEnd, &lineIndex))
        return false;

    /* Allow edits that occur fully above the last visible line. */
    if (lineIndex < maxLines - 1)
        return false;

    available = te->viewRect.right - te->viewRect.left - 2;
    if (available <= 0)
        return true;

    if (te->hText)
        HLock((Handle)te->hText);

    {
        short selStart = te->selStart;
        short selEnd = te->selEnd;
        short prefixLen;
        short suffixLen;
        short newLen;
        char buffer[512];
        Ptr text = te->hText ? *(te->hText) : NULL;

        if (selStart < lineStart)
            selStart = lineStart;
        if (selEnd < lineStart)
            selEnd = lineStart;
        if (selEnd > lineEnd)
            selEnd = lineEnd;

        prefixLen = selStart - lineStart;
        suffixLen = lineEnd - selEnd;
        newLen = prefixLen + 1 + suffixLen;

        if (newLen <= 0)
        {
            overflows = true;
            goto cleanup;
        }

        if (prefixLen > 0 && text)
            BlockMoveData(text + lineStart, buffer, prefixLen);

        buffer[prefixLen] = c;

        if (suffixLen > 0 && text)
            BlockMoveData(text + selEnd, buffer + prefixLen + 1, suffixLen);

        if (text)
        {
            GetPort(&savePort);
            if (te->inPort)
                SetPort(te->inPort);
            else
                SetPort(gMainWin);

            if (TextWidth(buffer, 0, newLen) > available)
                overflows = true;

            if (savePort)
                SetPort(savePort);
        }
    }

cleanup:
    if (te->hText)
        HUnlock((Handle)te->hText);

    return overflows;
}

static void main_window_update_text(TEHandle handle)
{
    Rect view;
    GrafPtr savePort = NULL;
    RGBColor saveFore;
    RGBColor saveBack;

    if (!handle)
        return;

    main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
    view = (**handle).viewRect;
    FillRect(&view, &qd.white);
    TEUpdate(&view, handle);
    main_window_restore_text_port(savePort, &saveFore, &saveBack);
}

static void main_window_create_text_edit(void)
{
    static const char kInitialText[] =
        "MacVox68 is live.\r"
        "TCP + TTS will be pumped from the event loop.\r"
        "Voice requests will appear here.";

    if (!gMainWin)
        return;

    gTextEdit = main_window_create_text_field(&gLayout.editText, kInitialText, false);
    gHostEdit = main_window_create_text_field(&gLayout.hostField, "127.0.0.1", true);
    gPortEdit = main_window_create_text_field(&gLayout.portField, "5555", true);

    if (gTextEdit)
        main_window_switch_active_edit(gTextEdit);

    main_window_update_text_scrollbar(false);
}

static void main_window_create_controls(void)
{
    MenuHandle soundMenu = NULL;
    short soundMenuID = 201;
    short soundCount = 0;

    if (!gMainWin)
        return;

    /* Build runtime menus so we do not rely on absent Toolbox resources. */
    soundMenu = NewMenu(soundMenuID, "\pSound");
    if (soundMenu)
    {
        AppendMenu(soundMenu, "\p(Default output device)\xA5Line Out\xA5Headphones");
        InsertMenu(soundMenu, -1);
        soundCount = CountMItems(soundMenu);
    }

    gSpeakBtn = NewControl(gMainWin, &gLayout.speakStopButton, "\pSpeak", true,
                           0, 0, 0, pushButProc, kSpeakStopBtnID);
    if (gSpeakBtn)
    {
        Boolean isDefault = true;
        SetControlData(gSpeakBtn, kControlEntireControl,
                       kControlPushButtonDefaultTag,
                       sizeof(isDefault), &isDefault);
    }

    if (soundMenu && soundCount > 0)
    {
        gSoundPop = NewControl(gMainWin, &gLayout.soundPopup, "\pSound", true,
                               1, soundMenuID, soundCount, popupMenuProc, 0);
    }

    gApplyBtn = NewControl(gMainWin, &gLayout.applyButton, "\pApply Audio", true,
                           0, 0, 0, pushButProc, 0);
    if (gApplyBtn)
    {
        Boolean isDefault = true;
        SetControlData(gApplyBtn, kControlEntireControl,
                       kControlPushButtonDefaultTag,
                       sizeof(isDefault), &isDefault);
    }

    gProsodyClean = NewControl(gMainWin, &gLayout.prosodyClean, "\pClean", true,
                               1, 0, 0, radioButProc, 0);
    gProsodyLQ = NewControl(gMainWin, &gLayout.prosodyLQ, "\pHL VOX Prosody LQ", true,
                            0, 0, 0, radioButProc, 0);
    gProsodyHQ = NewControl(gMainWin, &gLayout.prosodyHQ, "\pHL VOX Prosody HQ", true,
                            0, 0, 0, radioButProc, 0);

    gVolumeSlider = NewControl(gMainWin, &gLayout.volumeSlider, "\p", true,
                               100, 0, 100, kControlSliderProc, 0);
    gRateSlider = NewControl(gMainWin, &gLayout.rateSlider, "\p", true,
                             10, -10, 10, kControlSliderProc, 0);
    gPitchSlider = NewControl(gMainWin, &gLayout.pitchSlider, "\p", true,
                              0, -10, 10, kControlSliderProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
}

static void main_window_draw_text_field(const Rect *frame)
{
    static const RGBColor kFieldFill = { 0xFFFF, 0xFFFF, 0xFFFF };
    static const RGBColor kFieldBorder = { 0x4444, 0x4444, 0x4444 };
    static const RGBColor kFieldInner = { 0x9C9C, 0x9C9C, 0x9C9C };
    Rect inner = *frame;

    RGBForeColor(&kFieldFill);
    RGBBackColor(&kFieldFill);
    FillRect(frame, &qd.white);

    RGBForeColor(&kFieldBorder);
    RGBBackColor(&kFieldFill);
    PenNormal();
    FrameRect(frame);

    InsetRect(&inner, 1, 1);
    RGBForeColor(&kFieldInner);
    RGBBackColor(&kFieldFill);
    PenNormal();
    FrameRect(&inner);
}

static void main_window_draw_group(const Rect *r, ConstStr255Param title)
{
    static const RGBColor kGroupFill = { 0xF2F2, 0xF2F2, 0xF2F2 };
    static const RGBColor kGroupBorder = { 0x4444, 0x4444, 0x4444 };
    static const RGBColor kGroupInner = { 0xB5B5, 0xB5B5, 0xB5B5 };
    static const RGBColor kText = { 0x0000, 0x0000, 0x0000 };
    Rect shade = *r;
    Rect inner = *r;

    RGBForeColor(&kGroupFill);
    RGBBackColor(&kGroupFill);
    FillRect(&shade, &qd.white);

    RGBForeColor(&kGroupBorder);
    RGBBackColor(&kGroupFill);
    PenNormal();
    FrameRect(&shade);

    InsetRect(&inner, 1, 1);
    RGBForeColor(&kGroupInner);
    RGBBackColor(&kGroupFill);
    PenNormal();
    FrameRect(&inner);

    if (title)
    {
        RGBForeColor(&kText);
        RGBBackColor(&kGroupFill);
        MoveTo(r->left + 10, r->top + 14);
        DrawString(title);
    }
}

static void main_window_draw_contents(WindowPtr w)
{
    static const RGBColor kWindowFill = { 0xD8D8, 0xD8D8, 0xD8D8 };
    static const RGBColor kSeparatorDark = { 0x7A7A, 0x7A7A, 0x7A7A };
    static const RGBColor kSeparatorLight = { 0xEEEE, 0xEEEE, 0xEEEE };
    static const RGBColor kGroupFill = { 0xF2F2, 0xF2F2, 0xF2F2 };
    static const RGBColor kText = { 0x0000, 0x0000, 0x0000 };
    Rect content;
    Rect textFrame;
    Rect hostFrame;
    Rect portFrame;

    SetPort(w);
    content = w->portRect;

    RGBForeColor(&kWindowFill);
    RGBBackColor(&kWindowFill);
    FillRect(&content, &qd.gray);

    RGBForeColor(&kSeparatorDark);
    RGBBackColor(&kWindowFill);
    PenNormal();
    MoveTo(content.left + 8, gLayout.speakStopButton.bottom + 6);
    LineTo(content.right - 8, gLayout.speakStopButton.bottom + 6);
    RGBForeColor(&kSeparatorLight);
    MoveTo(content.left + 8, gLayout.speakStopButton.bottom + 7);
    LineTo(content.right - 8, gLayout.speakStopButton.bottom + 7);

    /* Text entry area with a soft border. */
    textFrame = gLayout.editText;
    main_window_draw_text_field(&textFrame);

    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    main_window_update_text_scrollbar(false);
    main_window_update_text(gTextEdit);

    /* Sound group */
    main_window_draw_group(&gLayout.soundGroup, "\pSound");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    MoveTo(gLayout.soundGroup.left + 16, gLayout.soundGroup.top + 32);
    DrawString("\pDevice:");

    /* Prosody group */
    main_window_draw_group(&gLayout.prosodyGroup, "\pProsody/Enunciation");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    MoveTo(gLayout.prosodyGroup.left + 16, gLayout.prosodyGroup.top + 32);
    DrawString("\pChoose clarity or HL VOX coloration.");

    /* Settings group */
    main_window_draw_group(&gLayout.settingsGroup, "\pSettings");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 30);
    DrawString("\pVolume");
    MoveTo(gLayout.volumeSlider.right + 10, gLayout.settingsGroup.top + 30);
    DrawString("\p100%");

    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 60);
    DrawString("\pRate");
    MoveTo(gLayout.rateSlider.right + 10, gLayout.settingsGroup.top + 60);
    DrawString("\p1.00");

    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 90);
    DrawString("\pPitch");
    MoveTo(gLayout.pitchSlider.right + 10, gLayout.settingsGroup.top + 90);
    DrawString("\p1.00");

    /* TCP group */
    main_window_draw_group(&gLayout.tcpGroup, "\pNetCat Receiver/TCP Server");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    MoveTo(gLayout.tcpGroup.left + 36, gLayout.tcpGroup.top + 32);
    DrawString("\pHost:");
    MoveTo(gLayout.portField.left - 36, gLayout.tcpGroup.top + 32);
    DrawString("\pPort:");

    hostFrame = gLayout.hostField;
    portFrame = gLayout.portField;

    main_window_draw_text_field(&hostFrame);
    main_window_draw_text_field(&portFrame);

    main_window_update_text(gHostEdit);
    main_window_update_text(gPortEdit);

    /* Draw controls after the background/text so chrome paints over the framing. */
    ForeColor(blackColor);
    BackColor(grayColor);
    DrawControls(w);

    RGBBackColor(&kGroupFill);
    if (gVolumeSlider)
        Draw1Control(gVolumeSlider);
    if (gRateSlider)
        Draw1Control(gRateSlider);
    if (gPitchSlider)
        Draw1Control(gPitchSlider);
    if (gProsodyClean)
        Draw1Control(gProsodyClean);
    if (gProsodyLQ)
        Draw1Control(gProsodyLQ);
    if (gProsodyHQ)
        Draw1Control(gProsodyHQ);
}

static Boolean main_window_handle_menu(long menuChoice, Boolean *outQuit)
{
    short menuID = HiWord(menuChoice);
    short item   = LoWord(menuChoice);

    if (menuID == 128) /* Apple */
    {
        if (item == 1)
            about_box_show();
    }

    if (menuID == 129) /* File */
    {
        if (item == 1) /* Quit */
            *outQuit = true;
    }

    HiliteMenu(0);
    return true;
}

void main_window_create(void)
{
    Rect bounds;
    Rect r;
    short width = 580;
    short height = 580;

    bounds = qd.screenBits.bounds;
    SetRect(&r,
            (short)((bounds.left + bounds.right - width) / 2),
            (short)((bounds.top + bounds.bottom - height) / 2),
            (short)((bounds.left + bounds.right + width) / 2),
            (short)((bounds.top + bounds.bottom + height) / 2));

    gMainWin = (WindowPtr)NewCWindow(
        NULL,
        &r,
        "\pMacVox68",
        true,
        documentProc,
        (WindowPtr)-1L,
        true,
        0
    );

    if (!gMainWin)
        return;

    SetPort(gMainWin);

    main_window_plan_layout();
    main_window_create_text_edit();
    main_window_create_controls();

    /* Speech-related controls will be updated once the engine is present. */
    main_window_update_control_enabling(kSpeechIdleState);

    ShowWindow(gMainWin);
}

void main_window_handle_update(WindowPtr w)
{
    GrafPtr savePort;
    GetPort(&savePort);
    SetPort(w);

    BeginUpdate(w);
    main_window_draw_contents(w);
    EndUpdate(w);

    SetPort(savePort);
}

Boolean main_window_handle_mouse_down(EventRecord *ev, Boolean *outQuit)
{
    WindowPtr w;
    short part = FindWindow(ev->where, &w);

    switch (part)
    {
        case inMenuBar:
        {
            long choice = MenuSelect(ev->where);
            if (choice)
                main_window_handle_menu(choice, outQuit);
            return true;
        }

        case inDrag:
            DragWindow(w, ev->where, &qd.screenBits.bounds);
            return true;

        case inGoAway:
            if (TrackGoAway(w, ev->where))
                *outQuit = true;
            return true;

        case inContent:
            if (w != FrontWindow())
            {
                SelectWindow(w);
                return true;
            }
            else
            {
                ControlHandle c;
                short cpart;
                Point local = ev->where;

                SetPort(w);
                GlobalToLocal(&local);

                cpart = FindControl(local, w, &c);
                if (cpart)
                {
                    if (c == gTextScroll)
                    {
                        short oldValue = GetControlValue(gTextScroll);
                        (void)TrackControl(c, local, NULL);
                        if (GetControlValue(gTextScroll) != oldValue)
                            main_window_scroll_text_to(GetControlValue(gTextScroll));
                    }
                    else
                    {
                        (void)TrackControl(c, local, NULL);
                    }
                    return true;
                }

                if (gTextEdit)
                {
                    Rect textRect = gLayout.editText;

                    if (PtInRect(local, &textRect))
                    {
                        GrafPtr savePort = NULL;
                        RGBColor saveFore;
                        RGBColor saveBack;

                        main_window_switch_active_edit(gTextEdit);
                        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gTextEdit);
                        main_window_restore_text_port(savePort, &saveFore, &saveBack);
                        main_window_update_text_scrollbar(true);
                        return true;
                    }
                }

                if (gHostEdit)
                {
                    Rect hostRect = gLayout.hostField;

                    if (PtInRect(local, &hostRect))
                    {
                        GrafPtr savePort = NULL;
                        RGBColor saveFore;
                        RGBColor saveBack;

                        main_window_switch_active_edit(gHostEdit);
                        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gHostEdit);
                        main_window_restore_text_port(savePort, &saveFore, &saveBack);
                        return true;
                    }
                }

                if (gPortEdit)
                {
                    Rect portRect = gLayout.portField;

                    if (PtInRect(local, &portRect))
                    {
                        GrafPtr savePort = NULL;
                        RGBColor saveFore;
                        RGBColor saveBack;

                        main_window_switch_active_edit(gPortEdit);
                        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gPortEdit);
                        main_window_restore_text_port(savePort, &saveFore, &saveBack);
                        return true;
                    }
                }
            }
            return false;

        default:
            return false;
    }
}

Boolean main_window_handle_key(EventRecord *ev, Boolean *outQuit)
{
    char c = (char)(ev->message & charCodeMask);
    Boolean isBackspace = (c == 0x08 || c == 0x7F);

    (void)outQuit;

    if ((ev->modifiers & cmdKey) != 0)
        return false;

    if (gActiveEdit)
    {
        TEPtr te = *gActiveEdit;
        Boolean isReturn = (c == '\r' || c == '\n');

        if (te->crOnly)
        {
            if (isReturn)
                return true;

            if (!isBackspace)
            {
                short available = te->viewRect.right - te->viewRect.left - 2;
                short prefix    = te->selStart;
                short suffixLen = te->teLength - te->selEnd;
                short newLen    = prefix + 1 + suffixLen;

                if (newLen > 0 && newLen < 512)
                {
                    char buffer[512];
                    GrafPtr savePort = NULL;
                    GrafPtr targetPort = te->inPort ? te->inPort : gMainWin;
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
                        return true;
                    }

                    if (savePort)
                        SetPort(savePort);
                }
            }
        }
        else
        {
            short maxLines = main_window_max_lines_for(gActiveEdit);

            if (isReturn)
            {
                if (maxLines > 0 && te->nLines >= maxLines)
                    return true;
            }
            else if (main_window_insertion_overflows(gActiveEdit, c))
            {
                return true;
            }
        }

        {
            GrafPtr savePort = NULL;
            RGBColor saveFore;
            RGBColor saveBack;

            main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
            TEKey(c, gActiveEdit);
            main_window_restore_text_port(savePort, &saveFore, &saveBack);
        }

        if (gActiveEdit == gTextEdit)
            main_window_update_text_scrollbar(true);
        return true;
    }

    if (gTextEdit)
    {
        GrafPtr savePort = NULL;
        RGBColor saveFore;
        RGBColor saveBack;

        main_window_switch_active_edit(gTextEdit);
        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
        TEKey(c, gTextEdit);
        main_window_restore_text_port(savePort, &saveFore, &saveBack);
        main_window_update_text_scrollbar(true);
        return true;
    }

    return false;
}

void main_window_idle(void)
{
    TEHandle target = gActiveEdit ? gActiveEdit : gTextEdit;

    if (target)
    {
        GrafPtr savePort = NULL;
        RGBColor saveFore;
        RGBColor saveBack;

        main_window_prepare_text_port(&savePort, &saveFore, &saveBack);
        TEIdle(target);
        main_window_restore_text_port(savePort, &saveFore, &saveBack);
    }
}

WindowPtr main_window_get(void)
{
    return gMainWin;
}
