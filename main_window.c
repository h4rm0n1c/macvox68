#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#include "about_box.h"
#include "main_window.h"
#include "ui_text_fields.h"

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
    kSpeakStopBtnID   = 3
};

typedef struct UILayout
{
    Rect editText;
    Rect editScroll;
    Rect speakStopButton;
    Rect startButton;
    Rect quitButton;
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
static ControlHandle  gProsodyClean = NULL;
static ControlHandle  gProsodyLQ    = NULL;
static ControlHandle  gVolumeSlider = NULL;
static ControlHandle  gRateSlider   = NULL;
static ControlHandle  gPitchSlider  = NULL;
static ControlHandle  gStartBtn     = NULL;
static ControlHandle  gQuitBtn      = NULL;
static UIScrollingText gTextArea;
static UITextField     gHostField;
static UITextField     gPortField;
static TEHandle       gActiveEdit   = NULL;
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

static void main_window_plan_layout(void)
{
    Rect content;
    short startY;
    short buttonColumnLeft;
    short buttonColumnWidth;
    short sectionLeft;
    short buttonLeft;
    short buttonCenter;
    short buttonStackGap;
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
    buttonStackGap    = (short)(m->gutter + 4);
    sectionLeft       = (short)(buttonLeft + buttonColumnWidth + m->gutter + 5);

    /* Text area sits beneath the top margin. */
    SetRect(&gLayout.editText,
            content.left + m->margin,
            startY,
            content.right - m->margin,
            startY + m->textAreaH);
    ui_text_field_scroll_rect(&gLayout.editText, &gLayout.editScroll);

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
            gLayout.settingsGroup.left + 88,
            gLayout.settingsGroup.top + 14,
            gLayout.settingsGroup.left + 88 + m->sliderW,
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
        short startTop    = (short)(gLayout.speakStopButton.bottom + buttonStackGap);
        short tcpCenter   = (short)(gLayout.tcpGroup.top + (m->tcpH / 2));
        short quitTop     = (short)(tcpCenter - (m->buttonH / 2));

        SetRect(&gLayout.startButton,
                (short)(buttonCenter - halfButtonW),
                startTop,
                (short)(buttonCenter + halfButtonW),
                (short)(startTop + m->buttonH));

        SetRect(&gLayout.quitButton,
                (short)(buttonCenter - halfButtonW),
                quitTop,
                (short)(buttonCenter + halfButtonW),
                (short)(quitTop + m->buttonH));
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

static void main_window_create_text_edit(void)
{
    static const char kInitialText[] =
        "MacVox68 is live.\r"
        "TCP + TTS will be pumped from the event loop.\r"
        "Voice requests will appear here.";

    if (!gMainWin)
        return;

    ui_text_scrolling_init(&gTextArea, gMainWin, &gLayout.editText, &gLayout.editScroll, kInitialText);
    ui_text_field_init(&gHostField, gMainWin, &gLayout.hostField, "127.0.0.1", true, false);
    ui_text_field_init(&gPortField, gMainWin, &gLayout.portField, "5555", true, false);

    if (gTextArea.field.handle)
        main_window_switch_active_edit(gTextArea.field.handle);
}

static void main_window_create_controls(void)
{
    if (!gMainWin)
        return;

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

    gQuitBtn = NewControl(gMainWin, &gLayout.quitButton, "\pQuit", true,
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
    if (gTextArea.field.handle)
        ui_text_scrolling_update_scrollbar(&gTextArea);
    ui_text_field_update(&gTextArea.field, gMainWin);

    /* Prosody group */
    main_window_draw_group(&gLayout.prosodyGroup, "\pProsody/Enunciation");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);

    /* Settings group */
    main_window_draw_group(&gLayout.settingsGroup, "\pSettings");
    RGBForeColor(&kText);
    RGBBackColor(&kWindowFill);
    MoveTo(gLayout.settingsGroup.left + 36, gLayout.settingsGroup.top + 30);
    DrawString("\pVolume");
    MoveTo(gLayout.volumeSlider.right + 10, gLayout.settingsGroup.top + 30);
    DrawString("\p100%");

    MoveTo(gLayout.settingsGroup.left + 36, gLayout.settingsGroup.top + 60);
    DrawString("\pRate");
    MoveTo(gLayout.rateSlider.right + 10, gLayout.settingsGroup.top + 60);
    DrawString("\p1.00");

    MoveTo(gLayout.settingsGroup.left + 36, gLayout.settingsGroup.top + 90);
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

    ui_text_field_update(&gHostField, gMainWin);
    ui_text_field_update(&gPortField, gMainWin);

    /* Draw controls after the background/text so chrome paints over the framing. */
    DrawControls(w);

    if (gTextArea.scroll)
        Draw1Control(gTextArea.scroll);

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
    short width = 508;
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

                    if (c == gTextArea.scroll)
                        (void)TrackControl(c, local, (ControlActionUPP)ui_text_scrolling_track);
                    else
                        (void)TrackControl(c, local, NULL);

                    if (setGroupFill)
                        RGBBackColor(&prevBack);
                    return true;
                }

                if (gTextArea.field.handle)
                {
                    Rect textRect = gLayout.editText;

                    if (PtInRect(local, &textRect))
                    {
                        main_window_switch_active_edit(gTextArea.field.handle);
                        ui_text_fields_set_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gTextArea.field.handle);
                        return true;
                    }
                }

                if (gHostField.handle)
                {
                    Rect hostRect = gLayout.hostField;

                    if (PtInRect(local, &hostRect))
                    {
                        main_window_switch_active_edit(gHostField.handle);
                        ui_text_fields_set_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gHostField.handle);
                        return true;
                    }
                }

                if (gPortField.handle)
                {
                    Rect portRect = gLayout.portField;

                    if (PtInRect(local, &portRect))
                    {
                        main_window_switch_active_edit(gPortField.handle);
                        ui_text_fields_set_colors();
                        TEClick(local, (ev->modifiers & shiftKey) != 0, gPortField.handle);
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

    (void)outQuit;

    if ((ev->modifiers & cmdKey) != 0)
        return false;

    if (gActiveEdit)
    {
        if (gActiveEdit == gTextArea.field.handle)
            return ui_text_scrolling_handle_key(&gTextArea, gMainWin, c);
        if (gActiveEdit == gHostField.handle)
            return ui_text_field_key(&gHostField, gMainWin, c);
        if (gActiveEdit == gPortField.handle)
            return ui_text_field_key(&gPortField, gMainWin, c);
    }

    if (gTextArea.field.handle)
    {
        main_window_switch_active_edit(gTextArea.field.handle);
        return ui_text_scrolling_handle_key(&gTextArea, gMainWin, c);
    }

    return false;
}

void main_window_idle(void)
{
    TEHandle target = gActiveEdit ? gActiveEdit : gTextArea.field.handle;

    if (target == gTextArea.field.handle)
        ui_text_field_idle(&gTextArea.field, gMainWin);
    else if (target == gHostField.handle)
        ui_text_field_idle(&gHostField, gMainWin);
    else if (target == gPortField.handle)
        ui_text_field_idle(&gPortField, gMainWin);
}

WindowPtr main_window_get(void)
{
    return gMainWin;
}
