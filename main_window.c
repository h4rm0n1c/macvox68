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

#include "main_window.h"

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif

#ifndef kControlEditTextProc
    /* Prefer the Appearance Manager edit text control proc ID when available. */
    #define kControlEditTextProc 272
#endif

#ifndef kControlSliderProc
    /* Classic slider control proc ID (Appearance-compatible). */
    #define kControlSliderProc 48
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
    Rect speakStopButton;
    Rect voiceGroup;
    Rect voicePopup;
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
static ControlHandle  gTextEdit = NULL;
static ControlHandle  gVoicePop = NULL;
static ControlHandle  gSpeakBtn = NULL;
static ControlHandle  gActiveControl = NULL;
static ControlHandle  gProsodyClean = NULL;
static ControlHandle  gProsodyLQ    = NULL;
static ControlHandle  gProsodyHQ    = NULL;
static ControlHandle  gVolumeSlider = NULL;
static ControlHandle  gRateSlider   = NULL;
static ControlHandle  gPitchSlider  = NULL;
static ControlHandle  gHostField    = NULL;
static ControlHandle  gPortField    = NULL;
static ControlHandle  gStartBtn     = NULL;
static UILayout       gLayout;

static void main_window_plan_layout(void)
{
    Rect content;
    short margin        = 18;
    short gutter        = 14;
    short buttonW       = 96;
    short buttonH       = 24;
    short popupW        = 160;
    short sectionGutter = 18;
    short textAreaH     = 168;
    short voiceH        = 64;
    short prosodyH      = 66;
    short settingsH     = 124;
    short tcpH          = 78;
    short sliderH       = 20;
    short sliderW       = 220;
    short fieldH        = 20;
    short fieldW        = 140;

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

    SetRect(&gLayout.voiceGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + voiceH);

    SetRect(&gLayout.voicePopup,
            gLayout.voiceGroup.left + 96,
            gLayout.voiceGroup.top + 20,
            gLayout.voiceGroup.left + 96 + popupW,
            gLayout.voiceGroup.top + 20 + buttonH);

    y = gLayout.voiceGroup.bottom + sectionGutter;

    SetRect(&gLayout.prosodyGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + prosodyH);

    SetRect(&gLayout.prosodyClean,
            gLayout.prosodyGroup.left + 96,
            gLayout.prosodyGroup.top + 18,
            gLayout.prosodyGroup.left + 186,
            gLayout.prosodyGroup.top + 34);

    SetRect(&gLayout.prosodyLQ,
            gLayout.prosodyClean.right + 18,
            gLayout.prosodyGroup.top + 18,
            gLayout.prosodyClean.right + 116,
            gLayout.prosodyGroup.top + 34);

    SetRect(&gLayout.prosodyHQ,
            gLayout.prosodyLQ.right + 18,
            gLayout.prosodyGroup.top + 18,
            gLayout.prosodyLQ.right + 140,
            gLayout.prosodyGroup.top + 34);

    y = gLayout.prosodyGroup.bottom + sectionGutter;

    SetRect(&gLayout.settingsGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + settingsH);

    SetRect(&gLayout.volumeSlider,
            gLayout.settingsGroup.left + 124,
            gLayout.settingsGroup.top + 18,
            gLayout.settingsGroup.left + 124 + sliderW,
            gLayout.settingsGroup.top + 18 + sliderH);

    SetRect(&gLayout.rateSlider,
            gLayout.volumeSlider.left,
            gLayout.volumeSlider.bottom + 18,
            gLayout.volumeSlider.right,
            gLayout.volumeSlider.bottom + 18 + sliderH);

    SetRect(&gLayout.pitchSlider,
            gLayout.rateSlider.left,
            gLayout.rateSlider.bottom + 18,
            gLayout.rateSlider.right,
            gLayout.rateSlider.bottom + 18 + sliderH);

    y = gLayout.settingsGroup.bottom + sectionGutter;

    SetRect(&gLayout.tcpGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + tcpH);

    SetRect(&gLayout.hostField,
            gLayout.tcpGroup.left + 104,
            gLayout.tcpGroup.top + 18,
            gLayout.tcpGroup.left + 104 + fieldW,
            gLayout.tcpGroup.top + 18 + fieldH);

    SetRect(&gLayout.portField,
            gLayout.hostField.right + 24,
            gLayout.tcpGroup.top + 18,
            gLayout.hostField.right + 24 + 64,
            gLayout.tcpGroup.top + 18 + fieldH);

    SetRect(&gLayout.startButton,
            gLayout.tcpGroup.right - 120,
            gLayout.tcpGroup.top + 20,
            gLayout.tcpGroup.right - 120 + 104,
            gLayout.tcpGroup.top + 20 + buttonH);
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

static void main_window_create_controls(void)
{
    MenuHandle voiceMenu = NULL;
    short voiceMenuID = 200;
    short voiceCount = 0;

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

    gTextEdit = NewControl(gMainWin, &gLayout.editText,
                           "\pMacVox68 is live.\rTCP + TTS will be pumped from the event loop.",
                           true, 0, 0, 0, kControlEditTextProc, kTextEditID);

    gSpeakBtn = NewControl(gMainWin, &gLayout.speakStopButton, "\pSpeak", true,
                           0, 0, 0, pushButProc, kSpeakStopBtnID);

    if (voiceMenu && voiceCount > 0)
    {
        gVoicePop = NewControl(gMainWin, &gLayout.voicePopup, "\pVoice", true,
                               1, voiceMenuID, voiceCount, popupMenuProc, kVoicePopupID);
    }

    gProsodyClean = NewControl(gMainWin, &gLayout.prosodyClean, "\pClean", true,
                               1, 0, 0, radioButProc, 0);
    gProsodyLQ = NewControl(gMainWin, &gLayout.prosodyLQ, "\pHL VOX Prosody LQ", true,
                            0, 0, 0, radioButProc, 0);
    gProsodyHQ = NewControl(gMainWin, &gLayout.prosodyHQ, "\pHL VOX Prosody HQ", true,
                            0, 0, 0, radioButProc, 0);

    gVolumeSlider = NewControl(gMainWin, &gLayout.volumeSlider, "\p", true,
                               75, 0, 100, kControlSliderProc, 0);
    gRateSlider = NewControl(gMainWin, &gLayout.rateSlider, "\p", true,
                             10, -10, 10, kControlSliderProc, 0);
    gPitchSlider = NewControl(gMainWin, &gLayout.pitchSlider, "\p", true,
                              0, -10, 10, kControlSliderProc, 0);

    gHostField = NewControl(gMainWin, &gLayout.hostField, "\p127.0.0.1", true,
                            0, 0, 0, kControlEditTextProc, 0);

    gPortField = NewControl(gMainWin, &gLayout.portField, "\p5555", true,
                            0, 0, 0, kControlEditTextProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
}

static Boolean main_window_handle_key(EventRecord *ev)
{
    if (!gActiveControl)
        return false;

    short keyCode = (ev->message & keyCodeMask) >> 8;
    char charCode = (char)(ev->message & charCodeMask);

    return HandleControlKey(gActiveControl, keyCode, charCode, ev->modifiers);
}

static void main_window_draw_contents(WindowPtr w)
{
    Rect content;

    SetPort(w);
    content = w->portRect;

    EraseRect(&content);

    /* Voice selection group */
    FrameRect(&gLayout.voiceGroup);
    MoveTo(gLayout.voiceGroup.left + 10, gLayout.voiceGroup.top + 18);
    DrawString("\pVoice Selection:");
    MoveTo(gLayout.voiceGroup.left + 32, gLayout.voiceGroup.top + 36);
    DrawString("\pVoice:");

    /* Prosody group */
    FrameRect(&gLayout.prosodyGroup);
    MoveTo(gLayout.prosodyGroup.left + 10, gLayout.prosodyGroup.top + 18);
    DrawString("\pProsody/Enunciation:");

    /* Settings group */
    FrameRect(&gLayout.settingsGroup);
    MoveTo(gLayout.settingsGroup.left + 10, gLayout.settingsGroup.top + 18);
    DrawString("\pSettings:");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 30);
    DrawString("\pVolume");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 60);
    DrawString("\pRate");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 90);
    DrawString("\pPitch");

    /* TCP group */
    FrameRect(&gLayout.tcpGroup);
    MoveTo(gLayout.tcpGroup.left + 10, gLayout.tcpGroup.top + 18);
    DrawString("\pNetCat Receiver/TCP Server");
    MoveTo(gLayout.tcpGroup.left + 20, gLayout.tcpGroup.top + 32);
    DrawString("\pHost:");
    MoveTo(gLayout.portField.left - 20, gLayout.tcpGroup.top + 32);
    DrawString("\pPort:");

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
    Rect screen = qd.screenBits.bounds;
    short winW = 660;
    short winH = 680;
    short left = (screen.right - screen.left - winW) / 2;
    short top  = (screen.bottom - screen.top - winH) / 2;

    SetRect(&r, left, top, left + winW, top + winH);

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
    main_window_create_controls();
    gActiveControl = gTextEdit;

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
                    gActiveControl = c;
                    (void)TrackControl(c, local, NULL);
                    return true;
                }
            }
            return false;

        default:
            return false;
    }
}

WindowPtr main_window_get(void)
{
    return gMainWin;
}
