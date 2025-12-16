// NOTE: Retro68 Rez is sensitive to DITL formatting.
// DO NOT REFORMAT the DITL block (keep rect line + item line + semicolons).
#include "Types.r"
#include "Dialogs.r"

resource 'DLOG' (128) {
    { 50, 100, 240, 420 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    128,
    "MacVox68",
    centerMainScreen
};

resource 'DITL' (128) {
    {
        { 160, 230, 180, 310 },
        Button { enabled, "Quit" };

        { 155, 225, 185, 315 },
        UserItem { enabled };

        { 10, 10, 30, 310 },
        StaticText { enabled, "Static Text Item" };

        { 40, 10, 56, 310 },
        EditText { enabled, "Edit Text Item" };

        { 70, 10, 86, 310 },
        CheckBox { enabled, "Check Box" };

        { 90, 10, 106, 310 },
        RadioButton { enabled, "Radio 1" };

        { 110, 10, 126, 310 },
        RadioButton { enabled, "Radio 2" };
    }
};
