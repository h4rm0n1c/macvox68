#include <Types.h>
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Controls.h>
#include <Events.h>
#include <Menus.h>
#include <TextEdit.h>
#include <ToolUtils.h>

#ifndef kClassicPushButtonProc
    #define kClassicPushButtonProc 0
#endif

#ifndef popupMenuCDefProc
    #define popupMenuCDefProc 1008
#endif

#ifndef popupMenuProc
    #define popupMenuProc popupMenuCDefProc
#endif

enum
{
    kTextEditID       = 1,
    kVoicePopupID     = 2,
    kSpeakStopBtnID   = 3
};

static WindowPtr      gMainWin  = NULL;
static ControlHandle  gTextEdit = NULL;
static ControlHandle  gVoicePop = NULL;
static ControlHandle  gSpeakBtn = NULL;
static MenuHandle     gVoiceMenu = NULL;

typedef struct UILayout
{
    Rect editText;
    Rect voicePopup;
    Rect speakStopButton;
} UILayout;

static UILayout gLayout;

typedef enum
{
    kSpeechIdleState,
    kSpeechSpeakingState
} SpeechUIState;

static void ui_init_toolbox(void)
{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
}

static void ui_init_menus_codeonly(void)
{
    /* Code-only menu bar (no Rez). This keeps the app feeling “real” without resources.
       Minimal Apple menu + File menu with Quit (Cmd-Q). */

    MenuHandle apple = NewMenu(128, "\p\024");
    if (apple)
    {
        AppendMenu(apple, "\pAbout MacVox68...");
        InsertMenu(apple, 0);
    }

    MenuHandle file = NewMenu(129, "\pFile");
    if (file)
    {
        AppendMenu(file, "\pQuit/Q");
        InsertMenu(file, 0);
    }

    DrawMenuBar();
}

static void ui_plan_layout(void)
{
    Rect content;
    short margin      = 14;
    short gutter      = 10;
    short buttonW     = 70;
    short buttonH     = 22;
    short popupW      = 120;

    if (!gMainWin)
        return;

    content = gMainWin->portRect;

    /* Top row: Speak/Stop (left) and Voice popup (right). This mirrors the
       Windows NetTTS layout while leaving the File>Quit menu as the primary
       exit path. */
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

    /* Text area sits beneath the control row, filling the remaining height. */
    SetRect(&gLayout.editText,
            content.left + margin,
            gLayout.speakStopButton.bottom + gutter,
            content.right - margin,
            content.bottom - margin);
}

static void ui_build_controls(void)
{
    Rect r;

    if (!gMainWin)
        return;

    SetPort(gMainWin);

    if (!gSpeakBtn)
    {
        r = gLayout.speakStopButton;
        gSpeakBtn = NewControl(
            gMainWin,
            &r,
            "\pSpeak",
            true,
            0,
            0,
            0,
            kClassicPushButtonProc,
            kSpeakStopBtnID
        );
    }

    if (!gVoiceMenu)
    {
        gVoiceMenu = NewMenu(200, "\pVoices");
        if (gVoiceMenu)
        {
            AppendMenu(gVoiceMenu, "\pDefault");
            AppendMenu(gVoiceMenu, "\pAlt voice");
            InsertMenu(gVoiceMenu, -1);
        }
    }

    if (!gVoicePop)
    {
        r = gLayout.voicePopup;
        gVoicePop = NewControl(
            gMainWin,
            &r,
            "\pVoice",
            (gVoiceMenu != NULL),
            1,
            (gVoiceMenu != NULL) ? GetMenuID(gVoiceMenu) : 0,
            0,
            popupMenuProc,
            kVoicePopupID
        );
    }
}

static void ui_update_control_enabling(SpeechUIState state)
{
    /* Controls respond to speech activity. Menu-based Quit remains available. */
    if (gTextEdit)
        HiliteControl(gTextEdit, (state == kSpeechSpeakingState) ? 255 : 0);

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

static void ui_create_main_window(void)
{
    Rect r;
    SetRect(&r, 60, 60, 420, 240);

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

    ui_plan_layout();

    ui_build_controls();

    /* Initialize titles/enabling now that the chrome exists. */
    ui_update_control_enabling(kSpeechIdleState);

    ShowWindow(gMainWin);
}

static void ui_draw_contents(WindowPtr w)
{
    Rect content;
    short x, y;

    SetPort(w);
    content = w->portRect;

    EraseRect(&content);

    x = gLayout.editText.left + 6;
    y = gLayout.editText.top + 14;

    FrameRect(&gLayout.editText);
    MoveTo(x, y);
    DrawString("\pMacVox68 is live.");

    y += 16;
    MoveTo(x, y);
    DrawString("\pTCP + TTS will be pumped from the event loop.");

    /* Draw controls after the background/text so planned UI chrome paints over
       the placeholder framing. */
    DrawControls(w);
}

static void ui_handle_update(WindowPtr w)
{
    GrafPtr savePort;
    GetPort(&savePort);
    SetPort(w);

    BeginUpdate(w);
    ui_draw_contents(w);
    EndUpdate(w);

    SetPort(savePort);
}

static Boolean ui_handle_menu(long menuChoice, Boolean *outQuit)
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

static Boolean ui_handle_mouse_down(EventRecord *ev, Boolean *outQuit)
{
    WindowPtr w;
    short part = FindWindow(ev->where, &w);

    switch (part)
    {
        case inMenuBar:
        {
            long choice = MenuSelect(ev->where);
            if (choice)
                ui_handle_menu(choice, outQuit);
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

void ui_app_init(void)
{
    ui_init_toolbox();
    ui_init_menus_codeonly();
    ui_create_main_window();
}

Boolean ui_app_pump_events(void)
{
    EventRecord ev;
    Boolean quit = false;

    if (WaitNextEvent(everyEvent, &ev, 30, NULL))
    {
        switch (ev.what)
        {
            case mouseDown:
                (void)ui_handle_mouse_down(&ev, &quit);
                break;

            case updateEvt:
                ui_handle_update((WindowPtr)ev.message);
                break;

            case keyDown:
            case autoKey:
                /* Cmd-Q quit shortcut (matches the menu item) */
                if ((ev.modifiers & cmdKey) != 0)
                {
                    char c = (char)(ev.message & charCodeMask);
                    if (c == 'q' || c == 'Q')
                        quit = true;
                }
                break;

            default:
                break;
        }
    }

    /* Integration point:
       Later, call tcp_poll() and speech_pump() here (nonblocking), while the loop stays responsive. */

    return !quit;
}
