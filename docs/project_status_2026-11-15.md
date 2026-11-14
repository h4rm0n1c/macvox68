# MacVox68 project, repository and reusable-code assessment — 2026-11-15

This is the pre-integration assessment. See [subsequent integration](integration_2026-11-15.md) for the current branch, fixes and verification.

## Checked state

Fetched origin with prune and queried GitHub repository, open PRs/issues and
PRs 57/58 through gh. No checkout, merge, commit, push or guest launch was done.
Basilisk remains shut down; the USB audio route remains stopped.

- Repository: https://github.com/h4rm0n1c/macvox68
- Current local branch: codex/create-standard-ui-file-for-sliders
- Local HEAD and that remote branch: caeb8ad (2026-01-07).
- Fetched origin/master: 801598f (merge dated 2026-07-09).
- HEAD...origin/master: 0 local-only committed changes, 6 upstream-only commits.
- Before this report: 13 tracked files modified, 7 untracked documents/tests.
  The recent fixes are not commits and are not backed up on GitHub by being
  present on the old remote feature branch.
- Local master is itself 15 commits behind origin/master; it is not checked out.
- GitHub reported no open pull requests or open issues at inspection time.

Upstream-only commits: bb71a46 (merge sliders), e3dde4f (README), 4ee490e and
bf99921 (ThreadLib notes and merge), 86b6d0e (source reorganization), 801598f
(merge reorganization). The source move's author date is January 7; its merge
was July 9. The later merge date does not imply months of new functional work.

PR 58: https://github.com/h4rm0n1c/macvox68/pull/58
PR 57: https://github.com/h4rm0n1c/macvox68/pull/57

## Application coverage

The programmatic main window, TextEdit fields/scrolling, About box, radio groups,
sliders/value labels, layout/theme, icon generator and nonblocking OT backend
are implemented code, not merely plans. Recent local work connects editor text
to owned asynchronous speech, with Clean and HL VOX paths, buffer ownership,
Stop/completion/Quit handling and embedded-command error inspection.

Validated locally: sanitizer parser/ownership contracts, six fake OT receive
scenarios, earlier live 68k speech arguments and buffer lifetime checks, and
uninterrupted Clean/HL VOX/Clean audio playback in B2 through ATR2USB. The latest
cadence build has playback/error checks, not human approval of its prosody.
No broad UI or network end-to-end qualification was performed by this review.

Still incomplete or requiring audit:

- TCP receives append chunks to the editor/log; they do not define a framed
  speech request protocol or automatically speak requests.
- Rate/pitch/volume sliders are not connected to the speech channel.
- Classic networking backend refuses service; it is not a MacTCP implementation.
- Parser retains the 68 recorded Windows-reference differences. Translating
  br as sentence boundaries corrects the adapter, not those preprocessing
  differences. FlexTalk aspiration/breathiness is not implemented.
- Appearance availability/fallbacks, About/focus/activation routing, allocation
  failures and full OT initialization/endpoint lifecycle remain audit work.
- Current helpers and speech/network services retain app/process global state.

See audit_2026-11-14.md for checked historical findings. Its statements that
speech is only a smoke test or that no owned channel exists are superseded by
clean_speech_channel.md and hl_vox_speech.md. Other old plans are not completion
records. Runtime and reference evidence takes precedence over optimistic prose.

## Reusable code structure

The local working tree keeps modules at repository root. GitHub master already
moves core modules into src/ and UI modules into src/ui/. This is organization,
not a separately built or exported library: CMake has an application target and
a host parser check, but no add_library/install/export target.

| Area | Existing boundary | Reuse assessment |
| --- | --- | --- |
| Text editing | ui_text_fields: single-line and scrolling TE helpers | Substantial reusable implementation; audit focus, ownership, limits and failure paths. |
| Controls | ui_radio, ui_slider | Real grouping/tracking/value-label helpers; audit Appearance requirements and redraw behavior. |
| Windows/layout/theme | ui_windows, ui_layout, ui_theme | Useful foundation; distinguish generic mechanics from MacVox styling and dimensions. |
| Input/event pump | ui_input, window_ui | Dispatcher plus app wiring; currently main/overlay-specific, lacking explicit activation/focus contract. |
| Networking | network facade + NetworkBackend + OT/classic modules | Useful separation; protocol, error handling and lifecycle need work. |
| Speech | speech.c/h | Owned async implementation now tested; one global channel, HL parser coupled into the module. |
| VOX parser | vox_parser.c/h | Host-buildable core and command adapter, but parity and allocation-failure gaps remain. |
| Icons | generator, raw PNGs, icon resources, notes | Valuable build-time tooling; preserve palette/mask/Rez lessons. |
| Main controller/About | main_window, about_box, window_ui | Primarily application code rather than a general library. |

ThreadLib is documentation only in upstream; it is not linked or used.
No library extraction or API redesign was performed by this assessment.

## Preserve the development history

Origin/master has 201 reachable commits. The work spans a dialog/Rez starter,
programmatic windows, TextEdit clipping/color/scroll fixes, icon palettes/masks,
About-window QuickDraw port repair, event/layout/theme extraction, OT layering,
speech declaration/packing fixes, parser porting and radio/slider tracking.
These are reasons to retain specific regression cases when moving code.

Eight local branches contain commits not reachable from origin/master:

- codex/change-buttons-to-double-border-style (6)
- codex/create-.r-file-for-macvox68-icon (17)
- codex/create-program-icon-for-macvox68 (9)
- codex/create-reusable-slider-ui-component (7)
- codex/design-control-placements-and-states (1)
- codex/fix-text-entry-field-background-color (12)
- codex/fix-undeclared-identifier-in-main_window.c (7)
- codex/test-macintalk/speech-manager-integration (11)

These counts are ancestry differences, not counts of missing fixes: equivalent
or superseding code may exist elsewhere. PRs 18/21/25/27/37/48/55 are closed
without a merge. The slider experiment has repeated snapping fixes; another
speech branch experimented with import libraries including a PPC archive.
Do not blindly cherry-pick or revive those paths. Compare their intent and
actual code with the present implementation. No branches were deleted.

## Verified integration hazards and recommended order

A temporary archive of fetched origin/master was compiled with:
cc -DVOX_PARSER_STANDALONE -Isrc src/vox_parser.c tests/vox_parser_host_checks.c
It fails because tests/vox_parser_host_checks.c still includes ../vox_parser.h,
which moved to src/. This is a reproduced upstream test-build defect.

Our lab instrumentation in tools/mac-lab/build_guest.py also assumes root-level
speech.c, main_window.c and window_ui.c and inserts CMake entries by textual
replacement. tools/check_macvox_clean.py and the new host tests/reference
comparison also use old paths. Simply updating the checkout would break this
integration unless these consumers move with it.

Recommended next change: preserve tracked and untracked local work, use an
integration branch based on fetched master, apply recent fixes with rename-aware
mapping, repair test/lab paths, then run host checks and a fresh out-of-tree
Retro68 build. Runtime testing must wait until the user makes the lab/audio
hardware available again; this review does not authorize restarting the guest.
Commit a reviewable checkpoint and publish it only within the user's requested
sync scope. Keep layout migration separate from functional library extraction.
After reconciliation, audit reusable components individually and turn each
historical bug into a meaningful regression case before treating its API as
stable. This preserves the hard-won implementation rather than replacing it
with an unverified framework.
