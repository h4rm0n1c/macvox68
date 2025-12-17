#include <Types.h>
#include <Quickdraw.h>
#include <QDColors.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>
#include <string.h>

#include "main_window.h"

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif

enum
{
    kTextEditID       = 1,
    kVoicePopupID     = 2,
    kSpeakStopBtnID   = 3
};

typedef struct UILayout
{
    Rect editText;
    Rect voicePopup;
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
static ControlHandle  gVoicePop = NULL;
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
static TEHandle       gTextEdit     = NULL;
static TEHandle       gHostEdit     = NULL;
static TEHandle       gPortEdit     = NULL;
static TEHandle       gActiveEdit   = NULL;
static UILayout       gLayout;

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
    short margin        = 16;
    short gutter        = 12;
    short buttonW       = 86;
    short buttonH       = 22;
    short popupW        = 160;
    short sectionGutter = 14;
    short textAreaH     = 170;
    short soundH        = 52;
    short prosodyH      = 54;
    short settingsH     = 94;
    short tcpH          = 52;
    short sliderH       = 16;
    short sliderW       = 200;
    short fieldH        = 18;
    short fieldW        = 120;

    if (!gMainWin)
        return;

    content = gMainWin->portRect;

    /* Top row: Speak/Stop (left) and Voice popup (right). */
    SetRect(&gLayout.speakStopButton,
            content.left + margin,
            content.top + margin,
            content.left + margin + buttonW,
            content.top + margin + buttonH);

    SetRect(&gLayout.voicePopup,
            content.right - margin - popupW,
            content.top + margin,
            content.right - margin,
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
            gLayout.soundGroup.left + 96 + popupW,
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

    SetRect(&gLayout.prosodyClean,
            gLayout.prosodyGroup.left + 90,
            gLayout.prosodyGroup.top + 12,
            gLayout.prosodyGroup.left + 150,
            gLayout.prosodyGroup.top + 28);

    SetRect(&gLayout.prosodyLQ,
            gLayout.prosodyClean.right + 18,
            gLayout.prosodyGroup.top + 12,
            gLayout.prosodyClean.right + 78,
            gLayout.prosodyGroup.top + 28);

    SetRect(&gLayout.prosodyHQ,
            gLayout.prosodyLQ.right + 18,
            gLayout.prosodyGroup.top + 12,
            gLayout.prosodyLQ.right + 100,
            gLayout.prosodyGroup.top + 28);

    y = gLayout.prosodyGroup.bottom + sectionGutter - 2;

    SetRect(&gLayout.settingsGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + settingsH);

    SetRect(&gLayout.volumeSlider,
            gLayout.settingsGroup.left + 90,
            gLayout.settingsGroup.top + 14,
            gLayout.settingsGroup.left + 90 + sliderW,
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
            gLayout.tcpGroup.top + 14,
            gLayout.tcpGroup.left + 80 + fieldW,
            gLayout.tcpGroup.top + 14 + fieldH);

    SetRect(&gLayout.portField,
            gLayout.hostField.right + 16,
            gLayout.tcpGroup.top + 14,
            gLayout.hostField.right + 16 + 46,
            gLayout.tcpGroup.top + 14 + fieldH);

    SetRect(&gLayout.startButton,
            gLayout.tcpGroup.right - 90,
            gLayout.tcpGroup.top + 10,
            gLayout.tcpGroup.right - 90 + 74,
            gLayout.tcpGroup.top + 10 + buttonH);
}

static void main_window_update_control_enabling(SpeechUIState state)
{
    /* Controls respond to speech activity. Menu-based Quit remains available. */
    if (gVoicePop)
        HiliteControl(gVoicePop, (state == kSpeechSpeakingState) ? 255 : 0);

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

static TEHandle main_window_create_text_field(const Rect *frame, const char *text)
{
    Rect viewRect;
    Rect destRect;
    TEHandle handle = NULL;

    viewRect = *frame;
    InsetRect(&viewRect, 6, 4);

    destRect = viewRect;
    destRect.bottom += 2000; /* Allow scrolling room for pasted content. */

    SetPort(gMainWin);
    BackColor(whiteColor);
    ForeColor(blackColor);

    handle = TENew(&destRect, &viewRect);
    if (handle)
    {
        if (text)
            TEInsert(text, strlen(text), handle);
    }

    return handle;
}

static void main_window_create_text_edit(void)
{
    static const char kInitialText[] =
        "MacVox68 is live.\r"
        "TCP + TTS will be pumped from the event loop.\r"
        "Voice requests will appear here.";

    if (!gMainWin)
        return;

    gTextEdit = main_window_create_text_field(&gLayout.editText, kInitialText);
    gHostEdit = main_window_create_text_field(&gLayout.hostField, "127.0.0.1");
    gPortEdit = main_window_create_text_field(&gLayout.portField, "5555");

    if (gTextEdit)
        main_window_switch_active_edit(gTextEdit);
}

static void main_window_create_controls(void)
{
    MenuHandle voiceMenu = NULL;
    MenuHandle soundMenu = NULL;
    short voiceMenuID = 200;
    short soundMenuID = 201;
    short voiceCount = 0;
    short soundCount = 0;

    if (!gMainWin)
        return;

    /* Build runtime menus so we do not rely on absent Toolbox resources. */
    voiceMenu = NewMenu(voiceMenuID, "\pVoices");
    if (voiceMenu)
    {
        AppendMenu(voiceMenu, "\pDefault\xA5Prose\xA5Narrator");
        InsertMenu(voiceMenu, -1);
        voiceCount = CountMItems(voiceMenu);
    }

    soundMenu = NewMenu(soundMenuID, "\pSound");
    if (soundMenu)
    {
        AppendMenu(soundMenu, "\p(Default output device)\xA5Line Out\xA5Headphones");
        InsertMenu(soundMenu, -1);
        soundCount = CountMItems(soundMenu);
    }

    gSpeakBtn = NewControl(gMainWin, &gLayout.speakStopButton, "\pSpeak", true,
                           0, 0, 0, pushButProc, kSpeakStopBtnID);

    if (voiceMenu && voiceCount > 0)
    {
        gVoicePop = NewControl(gMainWin, &gLayout.voicePopup, "\pVoice", true,
                               1, voiceMenuID, voiceCount, popupMenuProc, kVoicePopupID);
    }

    if (soundMenu && soundCount > 0)
    {
        gSoundPop = NewControl(gMainWin, &gLayout.soundPopup, "\pSound", true,
                               1, soundMenuID, soundCount, popupMenuProc, 0);
    }

    gApplyBtn = NewControl(gMainWin, &gLayout.applyButton, "\pApply Audio", true,
                           0, 0, 0, pushButProc, 0);

    gProsodyClean = NewControl(gMainWin, &gLayout.prosodyClean, "\pClean", true,
                               1, 0, 0, radioButProc, 0);
    gProsodyLQ = NewControl(gMainWin, &gLayout.prosodyLQ, "\pHL VOX Prosody LQ", true,
                            0, 0, 0, radioButProc, 0);
    gProsodyHQ = NewControl(gMainWin, &gLayout.prosodyHQ, "\pHL VOX Prosody HQ", true,
                            0, 0, 0, radioButProc, 0);

    gVolumeSlider = NewControl(gMainWin, &gLayout.volumeSlider, "\p", true,
                               100, 0, 100, scrollBarProc, 0);
    gRateSlider = NewControl(gMainWin, &gLayout.rateSlider, "\p", true,
                             10, -10, 10, scrollBarProc, 0);
    gPitchSlider = NewControl(gMainWin, &gLayout.pitchSlider, "\p", true,
                              0, -10, 10, scrollBarProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
}

static void main_window_set_light_background(void)
{
    RGBColor lightGray = {0xB000, 0xB000, 0xB000};

    RGBBackColor(&lightGray);
    ForeColor(blackColor);
}

static void main_window_draw_text_field(const Rect *frame)
{
    Rect inner = *frame;

    BackColor(whiteColor);
    PaintRect(frame);

    PenPat(&qd.gray);
    FrameRect(frame);

    InsetRect(&inner, 1, 1);
    PenNormal();
    FrameRect(&inner);
}

static void main_window_draw_group(const Rect *r, ConstStr255Param title)
{
    Rect shade = *r;

    main_window_set_light_background();
    PaintRect(&shade);

    PenPat(&qd.gray);
    FrameRect(&shade);
    PenNormal();

    if (title)
    {
        MoveTo(r->left + 10, r->top + 14);
        DrawString(title);
    }
}

static void main_window_draw_contents(WindowPtr w)
{
    Rect content;
    Rect textFrame;
    Rect hostFrame;
    Rect portFrame;

    SetPort(w);
    content = w->portRect;

    main_window_set_light_background();
    EraseRect(&content);

    /* Header row accents */
    MoveTo(gLayout.voicePopup.left - 94, gLayout.voicePopup.top + 15);
    DrawString("\pVoice Selection:");

    PenPat(&qd.gray);
    MoveTo(content.left + 8, gLayout.speakStopButton.bottom + 6);
    LineTo(content.right - 8, gLayout.speakStopButton.bottom + 6);
    PenNormal();

    /* Text entry area with a soft border. */
    textFrame = gLayout.editText;
    main_window_draw_text_field(&textFrame);

    if (gTextEdit)
    {
        TEUpdate(&textFrame, gTextEdit);
    }

    /* Sound group */
    main_window_draw_group(&gLayout.soundGroup, "\pSound");
    MoveTo(gLayout.soundGroup.left + 16, gLayout.soundGroup.top + 32);
    DrawString("\pDevice:");

    /* Prosody group */
    main_window_draw_group(&gLayout.prosodyGroup, "\pProsody/Enunciation");
    MoveTo(gLayout.prosodyGroup.left + 16, gLayout.prosodyGroup.top + 32);
    DrawString("\pChoose clarity or HL VOX coloration.");

    /* Settings group */
    main_window_draw_group(&gLayout.settingsGroup, "\pSettings");
    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 30);
    DrawString("\pVolume");
    MoveTo(gLayout.settingsGroup.right - 40, gLayout.settingsGroup.top + 30);
    DrawString("\p100%");

    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 60);
    DrawString("\pRate");
    MoveTo(gLayout.settingsGroup.right - 40, gLayout.settingsGroup.top + 60);
    DrawString("\p1.00");

    MoveTo(gLayout.settingsGroup.left + 52, gLayout.settingsGroup.top + 90);
    DrawString("\pPitch");
    MoveTo(gLayout.settingsGroup.right - 40, gLayout.settingsGroup.top + 90);
    DrawString("\p1.00");

    /* TCP group */
    main_window_draw_group(&gLayout.tcpGroup, "\pNetCat Receiver/TCP Server");
    MoveTo(gLayout.tcpGroup.left + 16, gLayout.tcpGroup.top + 32);
    DrawString("\pHost:");
    MoveTo(gLayout.portField.left - 22, gLayout.tcpGroup.top + 32);
    DrawString("\pPort:");

    hostFrame = gLayout.hostField;
    portFrame = gLayout.portField;

    main_window_draw_text_field(&hostFrame);
    main_window_draw_text_field(&portFrame);

    if (gHostEdit)
        TEUpdate(&hostFrame, gHostEdit);

    if (gPortEdit)
        TEUpdate(&portFrame, gPortEdit);

    /* Draw controls after the background/text so chrome paints over the framing. */
    DrawControls(w);
}

static Boolean main_window_handle_menu(long menuChoice, Boolean *outQuit)
{
    short menuID = HiWord(menuChoice);
    short item   = LoWord(menuChoice);

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
    Rect r;
    SetRect(&r, 40, 40, 620, 620);

    gMainWin = NewWindow(
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
                    (void)TrackControl(c, local, NULL);
                    return true;
                }

                if (gTextEdit)
                {
                    Rect textRect = gLayout.editText;

                    if (PtInRect(local, &textRect))
                    {
                        main_window_switch_active_edit(gTextEdit);
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

    (void)outQuit;

    if ((ev->modifiers & cmdKey) != 0)
        return false;

    if (gActiveEdit)
    {
        TEKey(c, gActiveEdit);
        return true;
    }

    if (gTextEdit)
    {
        main_window_switch_active_edit(gTextEdit);
        TEKey(c, gTextEdit);
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
        TEIdle(target);
        SetPort(savePort);
    }
}

WindowPtr main_window_get(void)
{
    return gMainWin;
}
