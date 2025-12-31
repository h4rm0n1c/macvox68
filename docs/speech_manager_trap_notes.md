# Speech Manager vs. SpeechLib (68K System 7)

MacVox68 targets **68K System 7.x**. The Speech Manager lives behind toolbox traps defined in `SpeechSynthesis.h` / `Speech.h`. The PPC-era **SpeechLib** import library is for the **CFM/PowerPC** world and does **not** apply to this app; treat it as unusable “nuclear waste” for 68K.

## What to use
- Call the Speech Manager traps from `SpeechSynthesis.h` (`/opt/Interfaces&Libraries/Interfaces/CIncludes/SpeechSynthesis.h`). These headers match Inside Macintosh: Sound.
- Gate speech features with `Gestalt(gestaltSpeechAttr, ...)` checks; System 7 installs vary and Speech Manager may be absent until the PlainTalk extensions are present.
- Expect classic patterns: `NewSpeechChannel`, `SpeakText`/`SpeakBuffer`, callbacks via `SpeechDoneProc`, and disposing channels when done.

## What to avoid
- Do **not** link `SpeechLib` for 68K targets. Retro68’s `libSpeechLib.a` only exists under `libppc/` because it is a CFM import stub for PowerPC. Mixing it into 68K builds leads nowhere.
- Do not assume CFM init rules or PPC calling conventions. Stick to the 68K trap-based interfaces documented in Inside Macintosh.

## Finding the docs quickly (read-only)
Use the readable overlay in `/opt/MacDevDocs` instead of web searches. Quick hits:
- The Inside Macintosh subject index at `/opt/MacDevDocs/1994-12-DevCD-RL/Subject\ Index/ Document\ Collections/Inside Macintosh/Inside Macintosh.txt` lists the Speech Manager chapter (“Inside Macintosh: Sound”, Chapter 4). Jump from there into the PDF on the same disc.
- To locate Speech Manager references fast, ripgrep the overlay text indexes, e.g.:
  ```
  rg "Speech Manager" /opt/MacDevDocs -g"*.txt"
  ```
  This points straight at the relevant Inside Macintosh entries and tech note summaries without manual browsing.

## Runtime reminders
- Verify the Speech Manager components are actually installed on the target System 7 install (PlainTalk extensions and voices). The toolbox traps will return errors if the synthesizer stack is missing.
- Keep embedded command handling aligned with the Speech Manager grammar (`[[rate]]`, `[[slnc]]`, etc.). MacinTalk voices implement these commands; applications should pass text through the Speech Manager and avoid private PPC-era APIs.

## MacVox68 implementation notes (68K trap path)
- Gate all speech features behind `Gestalt(gestaltSpeechAttr, ...)` during startup and no-op the UI when absent.
- Convert C strings to Pascal (`len` byte + text) before calling `SpeakString`; Retro68’s 68K headers expect the trap-friendly Pascal signature.
- Use `SpeechBusy` to poll state from the main event loop instead of assuming callbacks; this keeps the Speak/Stop button state synced without adding asynchronous plumbing.
- A minimal trap-based path works: `SpeakString("I've got balls of steel")` plus `StopSpeech(NULL)` for cancellation is enough to prove the Speech Manager is alive in BasiliskII.

## Pitfall log: PPC SpeechLib dead end (what to avoid next time)
- The PPC-only `SpeechLib` import stub (CFM) in Retro68’s `libppc/` looked tempting but is **not valid** for 68K builds. Attempting to link or hunt for SpeechLib symbols on 68K wasted several hours and produced no usable entry points.
- Speech on System 7 must use the classic trap interfaces from `Speech.h` / `SpeechSynthesis.h`; they live in the Toolbox and do not require any PPC libraries.
- When uncertain about 68K vs. PPC availability, check `/opt/Interfaces&Libraries/Interfaces/CIncludes/` and Inside Macintosh (see `/opt/MacDevDocs`) first. If the header shows a trap-based interface or only PPC code fragments, choose the trap path for MacVox68.
