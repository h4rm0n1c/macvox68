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

#include "main_window.h"

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
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
        short radioTop   = gLayout.prosodyGroup.top + 44;
        short radioLeft  = gLayout.prosodyGroup.left + 72;
        short radioGap   = 22;
        short radioWidth = 140;

        SetRect(&gLayout.prosodyClean,
                radioLeft,
                radioTop,
                radioLeft + radioWidth,
                radioTop + 18);

        radioLeft = gLayout.prosodyClean.right + radioGap;
        SetRect(&gLayout.prosodyLQ,
                radioLeft,
                radioTop,
                radioLeft + radioWidth,
                radioTop + 18);

        radioLeft = gLayout.prosodyLQ.right + radioGap;
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

static void main_window_update_text(TEHandle handle)
{
    Rect view;

    if (!handle)
        return;

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

    if (gTextEdit)
        main_window_switch_active_edit(gTextEdit);
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
    BackPat(&qd.white);
    ForeColor(blackColor);
}

static void main_window_draw_text_field(const Rect *frame)
{
    Rect inner = *frame;

    FillRect(frame, &qd.white);

    PenPat(&qd.black);
    FrameRect(frame);

    InsetRect(&inner, 1, 1);
    PenPat(&qd.gray);
    FrameRect(&inner);
    PenNormal();
}

static void main_window_draw_group(const Rect *r, ConstStr255Param title)
{
    Rect shade = *r;
    Rect inner = *r;

    main_window_set_light_background();
    FillRect(&shade, &qd.white);

    PenPat(&qd.black);
    FrameRect(&shade);

    InsetRect(&inner, 1, 1);
    PenPat(&qd.gray);
    FrameRect(&inner);
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
    FillRect(&content, &qd.gray);

    PenPat(&qd.gray);
    MoveTo(content.left + 8, gLayout.speakStopButton.bottom + 6);
    LineTo(content.right - 8, gLayout.speakStopButton.bottom + 6);
    PenNormal();

    /* Text entry area with a soft border. */
    textFrame = gLayout.editText;
    main_window_draw_text_field(&textFrame);

    main_window_update_text(gTextEdit);

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
        else if (isReturn)
        {
            short maxLines = main_window_max_lines_for(gActiveEdit);

            if (maxLines > 0 && te->nLines >= maxLines)
                return true;
        }

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
