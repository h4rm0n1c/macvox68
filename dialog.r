/*
    MacVox68 resources (Retro68 / Rez)

    Notes:
    - DITL items must be written as an array of *compound* item records:
        { {top,left,bottom,right}, ItemType { ... } }
      and items are comma-separated.
    - Keep includes at the top so templates are available predictably.
*/

#include "Types.r"
#include "Dialogs.r"
#include "Finder.r"
#include "Icons.r"
#include "Processes.r"

/* -------------------------------------------------------------------------- */
/* Main dialog                                                                */
/* -------------------------------------------------------------------------- */

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

/*
    Dialog item list.
    Item IDs are implicit by order (starting at 1):
      1 = Quit button
      2 = UserItem (reserved for custom drawing)
      3 = StaticText
      4 = EditText
      5 = CheckBox
      6 = Radio 1
      7 = Radio 2
*/
resource 'DITL' (128) {
    {
        { { 160, 230, 180, 310 }, Button      { enabled, "Quit" } },
        { { 155, 225, 185, 315 }, UserItem    { enabled } },

        { {  10,  10,  30, 310 }, StaticText  { enabled, "Static Text Item" } },
        { {  40,  10,  56, 310 }, EditText    { enabled, "Edit Text Item" } },

        { {  70,  10,  86, 310 }, CheckBox    { enabled, "Check Box" } },
        { {  90,  10, 106, 310 }, RadioButton { enabled, "Radio 1" } },
        { { 110,  10, 126, 310 }, RadioButton { enabled, "Radio 2" } }
    }
};

/* -------------------------------------------------------------------------- */
/* App size/feature flags                                                     */
/* -------------------------------------------------------------------------- */

resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    100 * 1024,
    100 * 1024
};

/* -------------------------------------------------------------------------- */
/* Bundle resources to pin Finder signature                                   */
/* -------------------------------------------------------------------------- */

/*
    Bundle resources to pin the Finder signature to Type 'APPL' and Creator 'MV68'.
    Icons are intentionally blank placeholders for now.
*/
resource 'BNDL' (128, purgeable) {
    'MV68', 0, {
        'FREF', { 0, 128 },
        'ICN#', { 0, 128 }
    }
};

resource 'FREF' (128, purgeable) {
    'APPL',
    0,
    "MacVox68"
};

resource 'ICN#' (128, purgeable) {
    /*
        Icon (all transparent) followed by an opaque mask.
        (32x32 icon bits: 128 bytes icon + 128 bytes mask = 256 bytes total)
    */
    $
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "0000 0000 0000 0000 0000 0000 0000 0000"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
    "FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
};
