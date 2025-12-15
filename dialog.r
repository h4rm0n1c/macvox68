/*
	Copyright 2015 Wolfgang Thaller.

	This file is part of Retro68.

	Retro68 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Retro68 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Retro68.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Types.r"
#include "Dialogs.r"
#include "Finder.r"
#include "Icons.r"

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

#include "Processes.r"

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
#ifdef TARGET_API_MAC_CARBON
    isHighLevelEventAware,
#else
	notHighLevelEventAware,
#endif
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	reserved,
	reserved,
	reserved,
#ifdef TARGET_API_MAC_CARBON
	500 * 1024,	// Carbon apparently needs additional memory.
	500 * 1024
#else
        100 * 1024,
        100 * 1024
#endif
};

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
        /* Icon (all transparent) followed by an opaque mask */
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
