# Speech Manager smoke test hook

MacVox68 now carries a minimal Speech Manager hook so we can verify MacinTalk is wired up in System 7 before attempting the NetTTS pipeline.

## What the Speak button does
- Uses the **main log text** as the utterance. Up to 511 bytes are copied from the log TEHandle into a buffer and sent to `SpeakText`.
- If the log is empty, falls back to the built-in test string: `[[vers 1]]MacVox68 speech smoke test. This exercises the Speech Manager channel.`
- When Speech Manager is unavailable (no synthesizer installed), the Speak button is **disabled** and a log line is added explaining the situation.
- Clicking **Speak** while audio is busy issues **Stop** to the current channel and logs `Speech stopped.`

## Implementation notes
- Speech code lives in `speech.c` / `speech.h` and is driven from the main event loop via `speech_pump()`.
- Speech text is copied into a temporary Handle and released once `SpeechBusy()` reports idle so the Speech Manager always owns stable memory during playback.
- The UI polls `speech_is_busy()` during `main_window_idle()` to keep the Speak button label synced (Speak vs. Stop) and to gray the button when no Speech Manager channel is available.
- The Retro68 target must link against the Speech Manager import archive so `InitSpeech`, `SpeechBusySystemTask`, and `StopSpeechChannel` resolve. Some kits name it `libSpeechLib.a` (Retro68 import libraries), while MPW 3.x era kits often ship it as `Speech.a` or `SpeechSynthesis.a`; any of these are valid and can be pointed to with `-DSPEECH_IMPORT_LIB_DIR=/path/to/ImportLibraries`. Stick to the 68K import archive for System 7.5.x; PPC stubs are not guaranteed to work. If CMake reports the library missing, check both `ImportLibraries` and `Interfaces&Libraries/Libraries` (and `SharedLibraries`, or `Interfaces and Libraries/Libraries/Shared Libraries` if your kit spells it with spaces) under the Retro68 root. CMake now also searches a sibling `InterfacesAndLibraries` tree beside the toolchain root (for example `/home/.../Retro68/InterfacesAndLibraries`) and an additional nested `Retro68/InterfacesAndLibraries` folder; if your SpeechLib/SpeechSynthesis lives somewhere else, set `SPEECH_IMPORT_LIB_DIR` to that folder.

## Quick checklist for host testing
1. Ensure **MacinTalk/Speech Manager** is installed in the System Folder inside BasiliskII.
2. Launch MacVox68 and click **Speak** (or populate the log first, then Speak) to hear the smoke-test phrase.
3. Click **Speak** again mid-playback to confirm Stop works.
