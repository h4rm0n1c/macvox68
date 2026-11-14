# Clean editor speech

The Clean Speak button now submits the editor text through an owned SpeechChannel
and SpeakText. It no longer speaks a fixed Pascal string. The length is a long;
the live test exercised 1,433 bytes, beyond SpeakString's 255-byte limit.

Speech owns a nonrelocatable copy of the editor bytes until completion. Editing
while speaking therefore cannot change or relocate the synthesizer's input.
GetSpeechInfo(soStatus) polls completion; successful StopSpeech releases the
buffer. Quit disposes the channel before releasing its buffer. Failed stop or
disposal retains the buffer rather than risking use after free. Embedded speech
command delimiters are disabled so Clean input is literal text.

HL VOX now uses the parser and the same owned channel; see hl_vox_speech.md. This change does not wire network input or voice sliders.

## Verification

Run tests/check_mac_api_contracts.py for fake-provider failure and ownership
checks under ASan/UBSan. It also retains the six network receive checks.

From the workspace root, run tools/check_macvox_clean.py with LabAgent and the
current instrumented MacVox running. The check quits MacVox at the end. It checks
empty input, a 1,433-byte SpeakText call, big-endian Pascal stack arguments and
zero return at CPU breakpoints, independent buffer lifetime after editor changes,
repeated Stop, natural completion and Quit during speech. Both clean1 and the
final clean2 build passed. Evidence: .mac-lab/macvox-clean-proof.json.

The instrumented bridge adds ui_set_text and reports editor/speech state through
ui_inspect. These are cooperative application hooks, not arbitrary Toolbox text
editing in other applications. The editor input limit is 32,767 MacRoman bytes;
NUL and unrepresentable characters are rejected by the host bridge.

## Reference

macos7-corpus records 23645 (SpeakText), 23647 (StopSpeech), 23660
(DisposeSpeechChannel), 23640 (DelimiterInfo), and 23634 (SpeechStatusInfo)
establish asynchronous lifetime, cancellation and completion behavior. The local
SpeechSynthesis.h trap declarations and live 68k stack observations establish the
ABI used here; no Ghidra compiler-spec assumption is involved.
