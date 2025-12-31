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
