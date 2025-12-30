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
#include <stdio.h>
#include <string.h>

#include "about_box.h"
#include "main_window.h"
#include "ui_input.h"
#include "ui_layout.h"
#include "ui_text_fields.h"
#include "ui_theme.h"
#include "ui_windows.h"

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
static const UITheme *sTheme = NULL;

static void main_window_demo_log_event(const InputEvent *ev, const char *context)
{
    char buffer[96];

    if (!ev || !context)
        return;

    if (!ev->optionDown || !gTextArea.field.handle || !gMainWin)
        return;

    buffer[0] = '\0';

    if (ev->type == kInputEventMouseDown)
    {
        snprintf(buffer, sizeof(buffer), "[demo] %s mouse (%d,%d)", context,
                 ev->local.h, ev->local.v);
    }
    else if (ev->type == kInputEventKeyDown)
    {
        snprintf(buffer, sizeof(buffer), "[demo] %s key '%c'%s", context,
                 ev->keyChar, ev->commandDown ? " +Cmd" : "");
    }

    if (buffer[0])
    {
        TEHandle te = gTextArea.field.handle;
        long end = (**te).teLength;

        SetPort(gMainWin);
        TESetSelect(end, end, te);
        TEInsert(buffer, strlen(buffer), te);
        TEInsert("\r", 1, te);
        ui_text_scrolling_scroll_selection_into_view(&gTextArea);
    }
}

static short main_window_layout_height(const UILayoutMetrics *m)
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
    const UILayoutMetrics *m = &kUILayoutMetrics;

    if (!gMainWin)
        return;

    content = gMainWin->portRect;
    startY = content.top + m->margin;
    buttonColumnLeft  = (short)(content.left + m->margin);
    buttonColumnWidth = kUILayoutStartButtonW;
    buttonLeft        = (short)(buttonColumnLeft + kUILayoutButtonInset);
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

    gSpeakBtn = ui_windows_new_button(gMainWin, &gLayout.speakStopButton, "\pSpeak", true, kSpeakStopBtnID);

    gProsodyClean = ui_windows_new_radio(gMainWin, &gLayout.prosodyClean, "\pClean", 1);
    gProsodyLQ = ui_windows_new_radio(gMainWin, &gLayout.prosodyLQ, "\pHL VOX Prosody", 0);

    gVolumeSlider = ui_windows_new_slider(gMainWin, &gLayout.volumeSlider, 100, 0, 100);
    gRateSlider = ui_windows_new_slider(gMainWin, &gLayout.rateSlider, 10, -10, 10);
    gPitchSlider = ui_windows_new_slider(gMainWin, &gLayout.pitchSlider, 0, -10, 10);

    gStartBtn = ui_windows_new_button(gMainWin, &gLayout.startButton, "\pStart Server", true, 0);

    gQuitBtn = ui_windows_new_button(gMainWin, &gLayout.quitButton, "\pQuit", false, 0);
}

static void main_window_draw_text_field(const Rect *frame)
{
    Rect inner = *frame;
    const UITheme *theme = sTheme ? sTheme : ui_theme_get();

    RGBForeColor(&theme->colors.textFieldFill);
    RGBBackColor(&theme->colors.textFieldFill);
    FillRect(frame, &qd.white);

    RGBForeColor(&theme->colors.textFieldBorder);
    RGBBackColor(&theme->colors.textFieldFill);
    PenNormal();
    FrameRect(frame);

    InsetRect(&inner, 1, 1);
    RGBForeColor(&theme->colors.textFieldInner);
    RGBBackColor(&theme->colors.textFieldFill);
    PenNormal();
    FrameRect(&inner);
}

static void main_window_draw_group(const Rect *r, ConstStr255Param title)
{
    Rect shade = *r;
    Rect inner = *r;
    const UITheme *theme = sTheme ? sTheme : ui_theme_get();

    RGBForeColor(&theme->colors.groupFill);
    RGBBackColor(&theme->colors.groupFill);
    FillRect(&shade, &qd.white);

    RGBForeColor(&theme->colors.groupBorder);
    RGBBackColor(&theme->colors.groupFill);
    PenNormal();
    FrameRect(&shade);

    InsetRect(&inner, 1, 1);
    RGBForeColor(&theme->colors.groupInner);
    RGBBackColor(&theme->colors.groupFill);
    PenNormal();
    FrameRect(&inner);

    if (title)
    {
        RGBForeColor(&theme->colors.text);
        RGBBackColor(&theme->colors.groupFill);
        MoveTo(r->left + theme->metrics.groupLabelInsetH, r->top + theme->metrics.groupLabelBaseline);
        DrawString(title);
    }
}

static void main_window_draw_contents(WindowPtr w)
{
    Rect content;
    Rect textFrame;
    Rect hostFrame;
    Rect portFrame;
    const UITheme *theme = sTheme ? sTheme : ui_theme_get();

    SetPort(w);
    content = w->portRect;

    RGBForeColor(&theme->colors.windowFill);
    RGBBackColor(&theme->colors.windowFill);
    FillRect(&content, &qd.gray);

    RGBForeColor(&theme->colors.separatorDark);
    RGBBackColor(&theme->colors.windowFill);
    PenNormal();
    MoveTo(gLayout.prosodyGroup.left, gLayout.editText.bottom + kUILayoutMetrics.gutter);
    LineTo(content.right - 8, gLayout.editText.bottom + kUILayoutMetrics.gutter);
    RGBForeColor(&theme->colors.separatorLight);
    MoveTo(gLayout.prosodyGroup.left, gLayout.editText.bottom + kUILayoutMetrics.gutter + 1);
    LineTo(content.right - 8, gLayout.editText.bottom + kUILayoutMetrics.gutter + 1);

    /* Text entry area with a soft border. */
    textFrame = gLayout.editText;
    main_window_draw_text_field(&textFrame);

    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.windowFill);
    if (gTextArea.field.handle)
        ui_text_scrolling_update_scrollbar(&gTextArea);
    ui_text_field_update(&gTextArea.field, gMainWin);

    /* Prosody group */
    main_window_draw_group(&gLayout.prosodyGroup, "\pProsody/Enunciation");
    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.windowFill);

    /* Settings group */
    main_window_draw_group(&gLayout.settingsGroup, "\pSettings");
    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.windowFill);
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
    RGBForeColor(&theme->colors.text);
    RGBBackColor(&theme->colors.windowFill);
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

    RGBBackColor(&theme->colors.groupFill);
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

static void main_window_draw_proc(WindowPtr w, void *refCon)
{
    (void)refCon;
    main_window_draw_contents(w);
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
    UIWindowSpec spec;

    spec.title  = "\pMacVox68";
    spec.width  = 508;
    spec.height = main_window_layout_height(&kUILayoutMetrics);
    spec.margin = kUILayoutMetrics.margin;

    sTheme = ui_theme_get();

    gMainWin = ui_windows_create_standard(&spec);

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

void main_window_handle_update(const InputEvent *ev)
{
    WindowPtr w = ev ? ev->window : NULL;

    if (!w)
        return;

    ui_windows_draw(w, main_window_draw_proc, NULL);
}

Boolean main_window_handle_mouse_down(const InputEvent *ev, Boolean *outQuit)
{
    WindowPtr w;
    short part;
    Point local;

    if (!ev)
        return false;

    part = FindWindow(ev->global, &w);
    local = ev->local;

    main_window_demo_log_event(ev, "main");

    switch (part)
    {
        case inMenuBar:
        {
            long choice = MenuSelect(ev->global);
            if (choice)
                main_window_handle_menu(choice, outQuit);
            return true;
        }

        case inSysWindow:
            return SystemClick(&ev->raw, ev->raw.modifiers);

        case inDrag:
            DragWindow(w, ev->global, &qd.screenBits.bounds);
            return true;

        case inGoAway:
            if (TrackGoAway(w, ev->global))
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
                SetPort(w);

                {
                    const RGBColor *sliderBg = sTheme ? &sTheme->colors.groupFill : &ui_theme_get()->colors.groupFill;
                    UIControlTrackingSpec specs[] = {
                        { gTextArea.scroll, (ControlActionUPP)ui_text_scrolling_track, NULL },
                        { gSpeakBtn, NULL, NULL },
                        { gVolumeSlider, NULL, sliderBg },
                        { gRateSlider, NULL, sliderBg },
                        { gPitchSlider, NULL, sliderBg },
                        { gStartBtn, NULL, NULL },
                        { gQuitBtn, NULL, NULL },
                        { gProsodyClean, NULL, NULL },
                        { gProsodyLQ, NULL, NULL }
                    };

                    if (ui_windows_track_hit_control(w, local, specs, (short)(sizeof(specs) / sizeof(specs[0]))))
                        return true;
                }

                if (gTextArea.field.handle)
                {
                    Rect textRect = gLayout.editText;

                    if (PtInRect(local, &textRect))
                    {
                        main_window_switch_active_edit(gTextArea.field.handle);
                        ui_text_fields_set_colors();
                        TEClick(local, ev->shiftDown, gTextArea.field.handle);
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
                        TEClick(local, ev->shiftDown, gHostField.handle);
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
                        TEClick(local, ev->shiftDown, gPortField.handle);
                        return true;
                    }
                }
            }
            return false;

        default:
            return false;
    }
}

Boolean main_window_handle_key(const InputEvent *ev, Boolean *outQuit)
{
    char c;

    (void)outQuit;

    if (!ev)
        return false;

    main_window_demo_log_event(ev, "main");

    if (ev->commandDown)
        return false;

    c = ev->keyChar;

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
