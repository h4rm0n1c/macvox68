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
    #define kControlEditTextProc 16
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
static ControlHandle  gHostField    = NULL;
static ControlHandle  gPortField    = NULL;
static ControlHandle  gStartBtn     = NULL;
static UILayout       gLayout;

static void main_window_plan_layout(void)
{
    Rect content;
    short margin        = 12;
    short gutter        = 10;
    short buttonW       = 80;
    short buttonH       = 22;
    short popupW        = 130;
    short sectionGutter = 12;
    short textAreaH     = 160;
    short soundH        = 44;
    short prosodyH      = 50;
    short settingsH     = 80;
    short tcpH          = 48;
    short sliderH       = 16;
    short sliderW       = 180;
    short fieldH        = 18;
    short fieldW        = 110;

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
            gLayout.soundGroup.left + 90,
            gLayout.soundGroup.top + 14,
            gLayout.soundGroup.left + 90 + popupW,
            gLayout.soundGroup.top + 14 + buttonH);

    SetRect(&gLayout.applyButton,
            gLayout.soundGroup.right - 90,
            gLayout.soundGroup.top + 14,
            gLayout.soundGroup.right - 90 + 60,
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

    gHostField = NewControl(gMainWin, &gLayout.hostField, "\p127.0.0.1", true,
                            0, 0, 0, kControlEditTextProc, 0);

    gPortField = NewControl(gMainWin, &gLayout.portField, "\p5555", true,
                            0, 0, 0, kControlEditTextProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
}

static void main_window_draw_contents(WindowPtr w)
{
    Rect content;
    short x, y;

    SetPort(w);
    content = w->portRect;

    EraseRect(&content);

    /* Text entry area */
    ForeColor(whiteColor);
    PaintRect(&gLayout.editText);
    ForeColor(blackColor);
    FrameRect(&gLayout.editText);

    x = gLayout.editText.left + 10;
    y = gLayout.editText.top + 18;

    MoveTo(x, y);
    DrawString("\pMacVox68 is live.");

    y += 16;
    MoveTo(x, y);
    DrawString("\pTCP + TTS will be pumped from the event loop.");

    /* Sound group */
    FrameRect(&gLayout.soundGroup);
    MoveTo(gLayout.soundGroup.left + 10, gLayout.soundGroup.top + 18);
    DrawString("\pSound:");
    MoveTo(gLayout.soundGroup.left + 60, gLayout.soundGroup.top + 30);
    DrawString("\pDevice:");

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
    SetRect(&r, 40, 40, 560, 520);

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
