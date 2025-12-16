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
#include <Gestalt.h>

#include "main_window.h"

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif

#ifndef editTextProc
    /* Classic edit text control proc ID for non-Appearance systems. */
    #define editTextProc 16
#endif

#ifndef kControlEditTextProc
    /* Prefer the Appearance Manager edit text control proc ID when available. */
    #define kControlEditTextProc editTextProc
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
    Rect editTextText;
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
static TEHandle       gTextTE   = NULL;
static ControlHandle  gVoicePop = NULL;
static ControlHandle  gSpeakBtn = NULL;
static ControlHandle  gActiveControl = NULL;
static Boolean        gTextActive = false;
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
    short margin        = 16;
    short gutter        = 10;
    short buttonW       = 116;
    short buttonH       = 24;
    short popupW        = 300;
    short sectionGutter = 18;
    short textAreaH     = 140;
    short voiceH        = 72;
    short prosodyH      = 86;
    short settingsH     = 128;
    short tcpH          = 96;
    short sliderH       = 22;
    short sliderW       = 260;
    short fieldH        = 20;
    short fieldW        = 168;

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

    /* Inset for the TextEdit view/dest rects so text stays off the frame. */
    SetRect(&gLayout.editTextText,
            gLayout.editText.left + 6,
            gLayout.editText.top + 6,
            gLayout.editText.right - 6,
            gLayout.editText.bottom - 6);

    /* Section sequencing mirrors the Windows UI rows. */
    short y = gLayout.editText.bottom + sectionGutter;

    SetRect(&gLayout.voiceGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + voiceH);

    SetRect(&gLayout.voicePopup,
            gLayout.voiceGroup.left + 104,
            gLayout.voiceGroup.top + 32,
            gLayout.voiceGroup.left + 104 + popupW,
            gLayout.voiceGroup.top + 32 + buttonH);

    y = gLayout.voiceGroup.bottom + sectionGutter;

    SetRect(&gLayout.prosodyGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + prosodyH);

    SetRect(&gLayout.prosodyClean,
            gLayout.prosodyGroup.left + 110,
            gLayout.prosodyGroup.top + 40,
            gLayout.prosodyGroup.left + 202,
            gLayout.prosodyGroup.top + 58);

    SetRect(&gLayout.prosodyLQ,
            gLayout.prosodyClean.right + 26,
            gLayout.prosodyGroup.top + 40,
            gLayout.prosodyClean.right + 190,
            gLayout.prosodyGroup.top + 58);

    SetRect(&gLayout.prosodyHQ,
            gLayout.prosodyLQ.right + 26,
            gLayout.prosodyGroup.top + 40,
            gLayout.prosodyLQ.right + 214,
            gLayout.prosodyGroup.top + 58);

    y = gLayout.prosodyGroup.bottom + sectionGutter;

    SetRect(&gLayout.settingsGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + settingsH);

    SetRect(&gLayout.volumeSlider,
            gLayout.settingsGroup.left + 144,
            gLayout.settingsGroup.top + 44,
            gLayout.settingsGroup.left + 144 + sliderW,
            gLayout.settingsGroup.top + 44 + sliderH);

    SetRect(&gLayout.rateSlider,
            gLayout.volumeSlider.left,
            gLayout.volumeSlider.bottom + 26,
            gLayout.volumeSlider.right,
            gLayout.volumeSlider.bottom + 26 + sliderH);

    SetRect(&gLayout.pitchSlider,
            gLayout.rateSlider.left,
            gLayout.rateSlider.bottom + 26,
            gLayout.rateSlider.right,
            gLayout.rateSlider.bottom + 26 + sliderH);

    y = gLayout.settingsGroup.bottom + sectionGutter;

    SetRect(&gLayout.tcpGroup,
            content.left + margin,
            y,
            content.right - margin,
            y + tcpH);

    SetRect(&gLayout.hostField,
            gLayout.tcpGroup.left + 116,
            gLayout.tcpGroup.top + 46,
            gLayout.tcpGroup.left + 116 + fieldW,
            gLayout.tcpGroup.top + 46 + fieldH);

    SetRect(&gLayout.portField,
            gLayout.hostField.right + 32,
            gLayout.tcpGroup.top + 46,
            gLayout.hostField.right + 32 + 90,
            gLayout.tcpGroup.top + 46 + fieldH);

    SetRect(&gLayout.startButton,
            gLayout.tcpGroup.right - 152,
            gLayout.tcpGroup.top + 42,
            gLayout.tcpGroup.right - 152 + 132,
            gLayout.tcpGroup.top + 42 + buttonH);
}

static short main_window_choose_slider_proc(void)
{
    return kControlSliderProc;
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
    short sliderProc = main_window_choose_slider_proc();
    short editProc = kControlEditTextProc;

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

    gTextTE = TENew(&gLayout.editTextText, &gLayout.editText);
    if (gTextTE)
    {
        TESetText("MacVox68 is live.\rTCP + TTS will be pumped from the event loop.",
                  69, gTextTE);
        TEActivate(gTextTE);
        gTextActive = true;
    }

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
                               75, 0, 100, sliderProc, 0);
    gRateSlider = NewControl(gMainWin, &gLayout.rateSlider, "\p", true,
                             10, -10, 10, sliderProc, 0);
    gPitchSlider = NewControl(gMainWin, &gLayout.pitchSlider, "\p", true,
                              0, -10, 10, sliderProc, 0);

    gHostField = NewControl(gMainWin, &gLayout.hostField, "\p127.0.0.1", true,
                            0, 0, 0, editProc, 0);

    gPortField = NewControl(gMainWin, &gLayout.portField, "\p5555", true,
                            0, 0, 0, editProc, 0);

    gStartBtn = NewControl(gMainWin, &gLayout.startButton, "\pStart Server", true,
                           0, 0, 0, pushButProc, 0);
}

Boolean main_window_handle_key(EventRecord *ev)
{
    if (gTextActive && gTextTE)
    {
        TEKey((char)(ev->message & charCodeMask), gTextTE);
        return true;
    }

    if (gActiveControl)
    {
        short keyCode = (ev->message & keyCodeMask) >> 8;
        char charCode = (char)(ev->message & charCodeMask);
        return HandleControlKey(gActiveControl, keyCode, charCode, ev->modifiers);
    }

    return false;
}

static void main_window_draw_contents(WindowPtr w)
{
    Rect content;

    SetPort(w);
    content = w->portRect;

    EraseRect(&content);

    /* Text area frame */
    FrameRect(&gLayout.editText);

    /* Voice selection group */
    FrameRect(&gLayout.voiceGroup);
    MoveTo(gLayout.voiceGroup.left + 10, gLayout.voiceGroup.top + 22);
    DrawString("\pVoice Selection:");
    MoveTo(gLayout.voiceGroup.left + 32, gLayout.voiceGroup.top + 54);
    DrawString("\pVoice:");

    /* Prosody group */
    FrameRect(&gLayout.prosodyGroup);
    MoveTo(gLayout.prosodyGroup.left + 10, gLayout.prosodyGroup.top + 22);
    DrawString("\pProsody/Enunciation:");

    /* Settings group */
    FrameRect(&gLayout.settingsGroup);
    MoveTo(gLayout.settingsGroup.left + 10, gLayout.settingsGroup.top + 22);
    DrawString("\pSettings:");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 62);
    DrawString("\pVolume");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 94);
    DrawString("\pRate");
    MoveTo(gLayout.settingsGroup.left + 60, gLayout.settingsGroup.top + 126);
    DrawString("\pPitch");

    /* TCP group */
    FrameRect(&gLayout.tcpGroup);
    MoveTo(gLayout.tcpGroup.left + 10, gLayout.tcpGroup.top + 26);
    DrawString("\pNetCat Receiver/TCP Server");
    MoveTo(gLayout.tcpGroup.left + 24, gLayout.tcpGroup.top + 60);
    DrawString("\pHost:");
    MoveTo(gLayout.portField.left - 34, gLayout.tcpGroup.top + 60);
    DrawString("\pPort:");

    /* Draw controls after the background/text so chrome paints over the framing. */
    DrawControls(w);

    if (gTextTE)
        TEUpdate(&gLayout.editText, gTextTE);
}

void main_window_idle(void)
{
    if (gTextActive && gTextTE)
        TEIdle(gTextTE);
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
    gActiveControl = NULL;

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
                    gTextActive = false;
                    (void)TrackControl(c, local, NULL);
                    return true;
                }
                else if (PtInRect(local, &gLayout.editText))
                {
                    if (gTextTE)
                    {
                        SelectWindow(w);
                        TEClick(local, ev->modifiers, gTextTE);
                        gTextActive = true;
                        gActiveControl = NULL;
                        return true;
                    }
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
