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

enum
{
    kTextEditID       = 1,
    kSpeakStopBtnID   = 3
};

typedef struct UILayout
{
    Rect editText;
    Rect editScroll;
    Rect speakStopButton;
    Rect prosodyGroup;
    Rect prosodyClean;
    Rect prosodyLQ;
    Rect settingsGroup;
    Rect volumeSlider;
    Rect rateSlider;
    Rect pitchSlider;
    Rect tcpGroup;
    Rect hostField;
    Rect portField;
    Rect startButton;
} UILayout;

typedef struct LayoutMetrics
{
    short margin;
    short gutter;
    short buttonW;
    short buttonH;
    short sectionGutter;
    short textAreaH;
    short prosodyH;
    short settingsH;
    short tcpH;
    short sliderH;
    short sliderW;
    short fieldH;
    short fieldW;
    short portFieldW;
} LayoutMetrics;

typedef enum
{
    kSpeechIdleState,
    kSpeechSpeakingState
} SpeechUIState;

static WindowPtr      gMainWin  = NULL;
static ControlHandle  gSpeakBtn = NULL;
static ControlHandle  gTextScroll = NULL;
static ControlHandle  gProsodyClean = NULL;
static ControlHandle  gProsodyLQ    = NULL;
static ControlHandle  gVolumeSlider = NULL;
static ControlHandle  gRateSlider   = NULL;
static ControlHandle  gPitchSlider  = NULL;
static ControlHandle  gStartBtn     = NULL;
static TEHandle       gTextEdit     = NULL;
static TEHandle       gHostEdit     = NULL;
static TEHandle       gPortEdit     = NULL;
static TEHandle       gActiveEdit   = NULL;
static short          gTextScrollOffset = 0;
static UILayout       gLayout;
static const LayoutMetrics kLayoutMetrics = {
    8,   /* margin */
    6,   /* gutter */
    86,  /* buttonW */
    20,  /* buttonH */
    6,   /* sectionGutter */
    120, /* textAreaH */
    57,  /* prosodyH */
    100, /* settingsH */
    52,  /* tcpH */
    16,  /* sliderH */
    210, /* sliderW */
    24,  /* fieldH */
    152, /* fieldW */
    64   /* portFieldW */
};

static const RGBColor kSettingsGroupFill = { 0xF2F2, 0xF2F2, 0xF2F2 };

static const short kTextInsetH = 6;
static const short kTextInsetV = 4;
static const short kTextScrollbarW = 15;
static const short kMaxTextDestGrowth = 30000;
static const short kStartButtonW = 104;
static const short kButtonInset = 5;

static short main_window_layout_height(const LayoutMetrics *m)
{
    return (short)(m->margin * 2 +
                   m->textAreaH + m->gutter + m->prosodyH +
                   (m->sectionGutter - 2) + m->settingsH +
                   (m->sectionGutter - 2) + m->tcpH);
}

static void main_window_switch_active_edit(TEHandle h)
{
    if (gActiveEdit == h)
        return;

    if (gActiveEdit)
        TEDeactivate(gActiveEdit);

    gActiveEdit = h;

    if (gActiveEdit)
        TEActivate(gActiveEdit);
}

static Rect main_window_text_scroll_rect(const Rect *frame)
{
    Rect r = *frame;

    InsetRect(&r, kTextInsetH, kTextInsetV);
    r.left = (short)(r.right - kTextScrollbarW);

    return r;
}

static void main_window_plan_layout(void)
{
    Rect content;
    short startY;
    short buttonColumnLeft;
    short buttonColumnWidth;
    short sectionLeft;
    short buttonLeft;
    short buttonCenter;
    short y;
    const LayoutMetrics *m = &kLayoutMetrics;

    if (!gMainWin)
        return;

    content = gMainWin->portRect;
    startY = content.top + m->margin;
    buttonColumnLeft  = (short)(content.left + m->margin);
    buttonColumnWidth = kStartButtonW;
    buttonLeft        = (short)(buttonColumnLeft + kButtonInset);
    buttonCenter      = (short)(buttonLeft + (buttonColumnWidth / 2));
    sectionLeft       = (short)(buttonLeft + buttonColumnWidth + m->gutter + 5);

    /* Text area sits beneath the top margin. */
    SetRect(&gLayout.editText,
            content.left + m->margin,
            startY,
            content.right - m->margin,
            startY + m->textAreaH);
    gLayout.editScroll = main_window_text_scroll_rect(&gLayout.editText);

    /* Section sequencing mirrors the Windows UI rows. */
    y = gLayout.editText.bottom + m->gutter;

    SetRect(&gLayout.prosodyGroup,
            sectionLeft,
            y,
            content.right - m->margin,
            y + m->prosodyH);

    /* Speak/Stop button sits in the reserved prosody margin. */
    {
        short verticalCenter = (short)(gLayout.prosodyGroup.top + ((m->prosodyH - m->buttonH) / 2));
        short halfButtonW    = (short)(m->buttonW / 2);

        SetRect(&gLayout.speakStopButton,
                (short)(buttonCenter - halfButtonW),
                verticalCenter,
                (short)(buttonCenter + halfButtonW),
                (short)(verticalCenter + m->buttonH));
    }

    {
        short labelBaseline   = (short)(gLayout.prosodyGroup.top + 14);
        short radioHeight     = 18;
        short radioTop        = (short)(labelBaseline +
                                 ((gLayout.prosodyGroup.bottom - labelBaseline - radioHeight) / 2));
        short radioLeft       = gLayout.prosodyGroup.left + 54;
        short radioGap        = 24;
        short radioWidth      = 140;
        short radioCleanWidth = 70;

        SetRect(&gLayout.prosodyClean,
                radioLeft,
                radioTop,
                radioLeft + radioCleanWidth,
                radioTop + radioHeight);

        radioLeft = gLayout.prosodyClean.right + radioGap - 15;
        SetRect(&gLayout.prosodyLQ,
                radioLeft,
                radioTop,
                radioLeft + radioWidth,
                radioTop + radioHeight);
    }

    y = gLayout.prosodyGroup.bottom + m->sectionGutter - 2;

    SetRect(&gLayout.settingsGroup,
            sectionLeft,
            y,
            content.right - m->margin,
            y + m->settingsH);

    SetRect(&gLayout.volumeSlider,
            gLayout.settingsGroup.left + 124,
            gLayout.settingsGroup.top + 14,
            gLayout.settingsGroup.left + 124 + m->sliderW,
            gLayout.settingsGroup.top + 14 + m->sliderH);

    SetRect(&gLayout.rateSlider,
            gLayout.volumeSlider.left,
            gLayout.volumeSlider.bottom + 14,
            gLayout.volumeSlider.right,
            gLayout.volumeSlider.bottom + 14 + m->sliderH);

    SetRect(&gLayout.pitchSlider,
            gLayout.rateSlider.left,
            gLayout.rateSlider.bottom + 14,
            gLayout.rateSlider.right,
            gLayout.rateSlider.bottom + 14 + m->sliderH);

    y = gLayout.settingsGroup.bottom + m->sectionGutter - 2;

    SetRect(&gLayout.tcpGroup,
            sectionLeft,
            y,
            content.right - m->margin,
            y + m->tcpH);

    SetRect(&gLayout.hostField,
            gLayout.tcpGroup.left + 80,
            gLayout.tcpGroup.top + 19,
            gLayout.tcpGroup.left + 80 + m->fieldW,
            gLayout.tcpGroup.top + 19 + m->fieldH);

    SetRect(&gLayout.portField,
            gLayout.hostField.right + 56,
            gLayout.tcpGroup.top + 19,
            gLayout.hostField.right + 56 + m->portFieldW,
            gLayout.tcpGroup.top + 19 + m->fieldH);

    {
        short halfButtonW = (short)(m->buttonW / 2);
        short startTop    = (short)(gLayout.speakStopButton.bottom + m->gutter);

        SetRect(&gLayout.startButton,
                (short)(buttonCenter - halfButtonW),
                startTop,
                (short)(buttonCenter + halfButtonW),
                (short)(startTop + m->buttonH));
    }
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

    viewRect = *frame;
    InsetRect(&viewRect, kTextInsetH, kTextInsetV);

    if (!singleLine)
        viewRect.right = (short)(viewRect.right - kTextScrollbarW);

    destRect = viewRect;
    destRect.bottom = (short)(destRect.top + kMaxTextDestGrowth);
    if (destRect.bottom < destRect.top)
        destRect.bottom = 32767;

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
            (**handle).destRect = destRect;
            (**handle).viewRect = viewRect;
        }

        if (text)
            TEInsert(text, strlen(text), handle);
    }

    return handle;
}

static short main_window_view_height(TEHandle handle)
{
    TEPtr te;

    if (!handle)
        return 0;

    te = *handle;
    if (!te)
        return 0;

    return (short)(te->viewRect.bottom - te->viewRect.top);
}

static short main_window_text_height(TEHandle handle)
{
    TEPtr te;

    if (!handle)
        return 0;

    te = *handle;
    if (!te)
        return 0;

    return (short)(te->nLines * te->lineHeight);
}

static short main_window_scroll_max(void)
{
    short viewH;
    short textH;
    short max;

    if (!gTextEdit)
        return 0;

    viewH = main_window_view_height(gTextEdit);
    textH = main_window_text_height(gTextEdit);
    max   = (short)(textH - viewH);

    return (max > 0) ? max : 0;
}

static void main_window_apply_scroll(short offset)
{
    short max;

    if (!gTextEdit)
        return;

    max = main_window_scroll_max();
    if (offset < 0)
        offset = 0;
    if (offset > max)
        offset = max;

    if (offset != gTextScrollOffset)
    {
        TEPinScroll(0, (short)(gTextScrollOffset - offset), gTextEdit);
        gTextScrollOffset = offset;
    }

    if (gTextScroll)
        SetControlValue(gTextScroll, gTextScrollOffset);
}

static void main_window_update_scrollbar(void)
{
    short max;

    if (!gTextEdit || !gTextScroll)
        return;

    TECalText(gTextEdit);

    max = main_window_scroll_max();
    SetControlMaximum(gTextScroll, max);

    if (gTextScrollOffset > max)
        main_window_apply_scroll(max);
    else
        SetControlValue(gTextScroll, gTextScrollOffset);
}

static void main_window_scroll_selection_into_view(void)
{
    TEPtr te;

    if (!gTextEdit)
        return;

    te = *gTextEdit;
    if (!te)
        return;

    if (te->selRect.top < te->viewRect.top)
    {
        main_window_apply_scroll((short)(gTextScrollOffset + (te->selRect.top - te->viewRect.top)));
    }
    else if (te->selRect.bottom > te->viewRect.bottom)
    {
        main_window_apply_scroll((short)(gTextScrollOffset + (te->selRect.bottom - te->viewRect.bottom)));
    }
}

static pascal void main_window_track_text_scroll(ControlHandle control, short part)
{
    short value = (short)(gTextScroll ? GetControlValue(gTextScroll) : 0);
    short newValue = value;
    short lineStep = 0;
    short pageStep = 0;
    short max = control ? GetControlMaximum(control) : 0;

    if (gTextEdit)
    {
        TEPtr te = *gTextEdit;
        if (te)
        {
            lineStep = te->lineHeight;
            pageStep = (short)(main_window_view_height(gTextEdit) - te->lineHeight);
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

    main_window_apply_scroll(newValue);
}

static void main_window_set_text_colors(void)
{
    static const RGBColor kText = { 0x0000, 0x0000, 0x0000 };
    static const RGBColor kTextBack = { 0xFFFF, 0xFFFF, 0xFFFF };

    RGBForeColor(&kText);
    RGBBackColor(&kTextBack);
}

static void main_window_update_text(TEHandle handle)
{
    Rect view;

    if (!handle)
        return;

    main_window_set_text_colors();
    view = (**handle).viewRect;
    TEUpdate(&view, handle);
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

    gTextScrollOffset = 0;

    if (gTextEdit)
        main_window_switch_active_edit(gTextEdit);
}

static void main_window_create_controls(void)
{
    if (!gMainWin)
        return;

    if (!gTextScroll)
    {
        Rect scrollRect = gLayout.editScroll;
        gTextScroll = NewControl(gMainWin, &scrollRect, "\p", true,
                                 0, 0, 0, scrollBarProc, 0);
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

    gProsodyClean = NewControl(gMainWin, &gLayout.prosodyClean, "\pClean", true,
                               1, 0, 0, radioButProc, 0);
    gProsodyLQ = NewControl(gMainWin, &gLayout.prosodyLQ, "\pHL VOX Prosody", true,
                            0, 0, 0, radioButProc, 0);

    gVolumeSlider = NewControl(gMainWin, &gLayout.volumeSlider, "\p", true,
                               100, 0, 100, kControlSliderProc, 0);
    gRateSlider = NewControl(gMainWin, &gLayout.rateSlider, "\p", true,
                             10, -10, 10, kControlSliderProc, 0);
    gPitchSlider = NewControl(gMainWin, &gLayout.pitchSlider, "\p", true,
                              0, -10, 10, kControlSliderProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
    if (gStartBtn)
    {
        Boolean isDefault = true;
        SetControlData(gStartBtn, kControlEntireControl,
                       kControlPushButtonDefaultTag,
                       sizeof(isDefault), &isDefault);
    }

    main_window_update_scrollbar();
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
    MoveTo(gLayout.prosodyGroup.left, gLayout.editText.bottom + kLayoutMetrics.gutter);
    LineTo(content.right - 8, gLayout.editText.bottom + kLayoutMetrics.gutter);
    RGBForeColor(&kSeparatorLight);
    MoveTo(gLayout.prosodyGroup.left, gLayout.editText.bottom + kLayoutMetrics.gutter + 1);
    LineTo(content.right - 8, gLayout.editText.bottom + kLayoutMetrics.gutter + 1);

    /* Text entry area with a soft border. */
    textFrame = gLayout.editText;
    main_window_draw_text_field(&textFrame);

    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    if (gTextEdit)
        main_window_update_scrollbar();
    main_window_update_text(gTextEdit);

    /* Prosody group */
    main_window_draw_group(&gLayout.prosodyGroup, "\pProsody/Enunciation");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);

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
    DrawControls(w);

    if (gTextScroll)
        Draw1Control(gTextScroll);

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
    short height;
    short topInset;
    short bottomInset = kLayoutMetrics.margin;

    bounds = qd.screenBits.bounds;
    height = main_window_layout_height(&kLayoutMetrics);
    topInset = GetMBarHeight() + bottomInset + 16;

    {
        short availableTop    = bounds.top + topInset;
        short availableBottom = bounds.bottom - bottomInset;
        short availableHeight = availableBottom - availableTop;
        short startY;

        if (availableHeight > height)
            startY = (short)(availableTop + (availableHeight - height) / 2);
        else
            startY = availableTop;

        SetRect(&r,
                (short)((bounds.left + bounds.right - width) / 2),
                startY,
                (short)((bounds.left + bounds.right + width) / 2),
                (short)(startY + height));

        if (r.bottom > availableBottom)
            OffsetRect(&r, 0, (short)(availableBottom - r.bottom));
    }

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
                    Boolean setGroupFill =
                        (c == gVolumeSlider || c == gRateSlider || c == gPitchSlider);
                    RGBColor prevBack;

                    if (setGroupFill)
                        GetBackColor(&prevBack);

                    if (setGroupFill)
                        RGBBackColor(&kSettingsGroupFill);

                    if (c == gTextScroll)
                        (void)TrackControl(c, local, (ControlActionUPP)main_window_track_text_scroll);
                    else
                        (void)TrackControl(c, local, NULL);

                    if (setGroupFill)
                        RGBBackColor(&prevBack);
                    return true;
                }

                if (gTextEdit)
                {
                    Rect textRect = gLayout.editText;

                    if (PtInRect(local, &textRect))
                    {
                        main_window_switch_active_edit(gTextEdit);
                        main_window_set_text_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gTextEdit);
                        return true;
                    }
                }

                if (gHostEdit)
                {
                    Rect hostRect = gLayout.hostField;

                    if (PtInRect(local, &hostRect))
                    {
                        main_window_switch_active_edit(gHostEdit);
                        main_window_set_text_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gHostEdit);
                        return true;
                    }
                }

                if (gPortEdit)
                {
                    Rect portRect = gLayout.portField;

                    if (PtInRect(local, &portRect))
                    {
                        main_window_switch_active_edit(gPortEdit);
                        main_window_set_text_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gPortEdit);
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
            (void)isReturn;
        }

        {
            GrafPtr savePort;
            GetPort(&savePort);
            SetPort(gMainWin);
            main_window_set_text_colors();
            TEKey(c, gActiveEdit);
            SetPort(savePort);
        }

        if (gActiveEdit == gTextEdit)
        {
            main_window_update_scrollbar();
            main_window_scroll_selection_into_view();
        }
        return true;
    }

    if (gTextEdit)
    {
        main_window_switch_active_edit(gTextEdit);
        {
            GrafPtr savePort;
            GetPort(&savePort);
            SetPort(gMainWin);
            main_window_set_text_colors();
            TEKey(c, gTextEdit);
            SetPort(savePort);
        }

        main_window_update_scrollbar();
        main_window_scroll_selection_into_view();
        return true;
    }

    return false;
}

void main_window_idle(void)
{
    TEHandle target = gActiveEdit ? gActiveEdit : gTextEdit;

    if (target)
    {
        GrafPtr savePort;
        GetPort(&savePort);
        SetPort(gMainWin);
        main_window_set_text_colors();
        TEIdle(target);
        SetPort(savePort);
    }
}

WindowPtr main_window_get(void)
{
    return gMainWin;
}
