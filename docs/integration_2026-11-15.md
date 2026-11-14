# MacVox integration — 2026-11-15

## Repository reconciliation

Preserved the complete working tree, patch and all Git refs in the workspace's
`.mac-lab/backups/macvox-before-sync-20261115/` before integration. Local fixes
were committed as `3d0ff7f`, then origin/master `801598f` was merged on
`integration/macvox-lab-refresh` (`9e458ce`). No historical local branches were
deleted or blindly cherry-picked. The old feature branch and local master are
historical refs; the integration branch is the working branch.

The upstream src/ reorganization now includes the local parser ownership fixes,
Open Transport result/flags correction, owned asynchronous editor speech and
Mac speech-command adapter fixes. Host tests and comparison scripts follow the
new paths. CMake supports host checks without requiring Retro68's add_application.

## Verification

- Host CMake build and CTest: pass.
- Parser fixtures under address/leak and undefined-behavior sanitizers: pass.
- Actual speech/network sources with fake Mac APIs: all six receive cases and
  copy, completion, stop, quit and failure ownership checks pass.
- Uninstrumented Retro68 application and disk-image build: pass.
- Lab instrumented Retro68 application and disk-image build: pass. Application
  data/resource/Finder forks staged in `.mac-lab/Shared/MacVox-sync2`.
- Reference preprocessing comparison: **0/68 match**, with exactly the same
  differences as the November 14 baseline. This is a known failing parity check,
  not a passing test or proof of acceptable cadence.

The workspace lab builder now uses src/ paths, discards stale generated source
copies and fails on missing/ambiguous injection anchors. Lab changes live under
OldMacStuff/tools, outside this application's Git repository.

Basilisk and the browser viewer were not started; the USB audio dongle remains
released. This integration has not been run in the guest or listened to. Earlier
guest checks validate the pre-layout speech changes only.

## Reusable code and remaining work

The parser is independently host-testable; speech, transport and UI helpers have
separate source modules. They are not an exported/versioned library. Preserve
those boundaries and audit ownership, callback/context assumptions and failures
before extracting a public library. ThreadLib is documentation, not a linked
runtime. The older assessment lists the substantial UI/resource work to preserve.

Next functional work: reconcile preprocessing and cadence against the two
Windows references, validate in the guest when audio hardware is available,
connect sliders to speech settings, define the network framing/queue/stop
contract and exercise real Open Transport lifecycle behavior. Audit Appearance
availability and UI focus/activation before claiming broad System 7 compatibility.
No claim is made that all historical code has now been exhaustively audited.
