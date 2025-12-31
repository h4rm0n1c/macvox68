# FlexTalk Escape Sequences vs. Macintosh Speech Manager Embedded Commands

This note maps FlexTalk's `\!` escape-sequence conventions to classic Macintosh Speech Manager embedded commands (the `[[command]]` syntax used by MacinTalk). It is written for re-implementing the NetTTS prosody encoder logic so that it emits Speech Manager markup directly, using FlexTalk's feature set as the reference for behavior.

This is the second pass now that we have the Speech Manager trap path working in MacVox68. The goal is to keep the VOX prosody engine logic but swap the emission target from FlexTalk escapes to Speech Manager commands as we bring the Mac speech stack online. A deeper porting plan that mirrors the existing `vox_process` pipeline lives in `docs/vox_parser_porting_notes.md`.

## Speech Manager vs. MacinTalk

- **Speech Manager is the OS API** that accepts embedded commands and routes text to whichever synthesizer is installed. It defines the command grammar (`[[rate]]`, `[[slnc]]`, etc.), manages voices, and abstracts queueing/audio output.
- **MacinTalk is Apple's synthesizer implementation** that plugs into Speech Manager, ships with several voices, and supports the same embedded-command grammar (plus optional `[[xtnd …]]` vendor extensions). Applications talk to Speech Manager; MacinTalk is one provider behind that API, not a separate markup language.

## Syntax basics
- **FlexTalk**: escape sequences start with `\!` and often come in begin/end pairs (for modes) or take inline numeric parameters (for rate, pitch scaling, pauses, etc.). They operate in-line within otherwise unmarked text.
- **Speech Manager**: embedded commands are enclosed by delimiters, `[[` and `]]` by default (changeable with `[[dlim ...]]`). Commands accept parameters separated by whitespace or semicolons and apply to subsequent speech. Recommended to start streams with `[[vers 1]]` to pin the command format version.

## Quick conversion table (prosody-encoder focus)

| Intent | FlexTalk escape(s) | Speech Manager equivalent(s) | Notes when porting |
| --- | --- | --- | --- |
| Set speaking rate | `\!r<rate>` (phrase-scoped), `\!R<rate>` (permanent) | `[[rate <fixed-point>]]` | FlexTalk rates are relative scalars (e.g., `1.2` for 20% slower). Speech Manager expects a words-per-minute fixed-point value; pick defaults (~190 WPM) and scale accordingly; keep a cached baseline WPM so you can restore after phrase-scoped changes. |
| Insert silence | `\!s<position><csec>` (initial/final, centiseconds) | `[[slnc <milliseconds>]]` | Convert centiseconds to milliseconds (×10). Speech Manager silence is duration-only; position must be handled by where you emit the command. |
| Emphasize or de-emphasize a word | `\!#<emphasis>` / `\!|<emphasis>` (emphasis scalar), `\!\!*<tone><prominence>` (de-accent), `\!!*<tone><prominence>` (accent + de-accent trailing words), `\!_` (cliticize) | `[[emph +]]` / `[[emph -]]` | Speech Manager only supports a simple emphasize/de-emphasize toggle. Map high FlexTalk emphasis to `[[emph +]]`, low or de-accent cases to `[[emph -]]`; finer-grain tones are lost. |
| Pitch baseline or modulation | `\!%<pitch>` (phrase), `\!%%<pitch>` (sticky); `\!{<param><value>` / `\!{!<param><value>` for per-phrase F0 params like `T`, `R`, `B`, `K` | `[[pbas <fixed-point>]]` for base pitch; `[[pmod <fixed-point>]]` for modulation depth | Use relative changes when FlexTalk uses ratios. Speech Manager values are absolute or relative with `+`/`-`. For per-phrase parameters, approximate with baseline/modulation adjustments before the phrase. |
| Phrase accent / boundary tones | `\!p<tone><prominence>`, `\!i<tone><prominence>`, `\!b<tone><prominence>` | No direct embedded command | Speech Manager lacks TOBI-style tone targets. Approximate by nudging `[[pbas]]`/`[[pmod]]` or rely on punctuation-driven prosody. |
| Mode: spell/math/raw/proofread | `\!sb...\!se`, `\!mb...\!me`, `\!rb...\!re`, `\!pb...\!pe`, `\!ab...\!ae`, `\!eb...\!ee` | `[[char LTRL]]` / `[[char NORM]]`, `[[nmbr LTRL]]` for digit-by-digit reading | Speech Manager has literal character and number modes but no direct equivalents for FlexTalk's acronym/proofread behaviors; apply preprocessing (e.g., insert spaces or punctuation) when needed. |
| Case and proper-noun hints | `\!cb` / `\!ce` (case significance), `\!pn` (mark proper noun) | None | Speech Manager ignores case; approximate with `[[emph +]]` for `\!pn` or leave unchanged. |
| Text-class detectors (addresses, dates, etc.) | `\!n<class><mode>`; `\!de` reset | None | Speech Manager has no built-in detectors. Normalize text before speaking (expand abbreviations, format numbers/times). |
| Gender / vocal tract tweaks | `\!us<gender>` / `\!uS<gender>`, `\!u<position><scale>` | Select voice via API or `[[prnn <voiceName>]]` if supported | MacinTalk chooses voices by name/ID rather than gender/tract parameters. Map gender to the closest installed voice; ignore tract scaling unless the synthesizer exposes an `xtnd` command. |
| Breathiness / spectral tilt | `\!wH<value>`, `\!WS<value>` | None (except synthesizer-specific `[[xtnd …]]`) | Classic Speech Manager voices do not expose these parameters; omit or approximate with volume/pitch choices per voice. |
| Comments in stream | `\!C<string>` | `[[cmnt any text]]` | Purely ignored by both engines; safe to drop or convert verbatim. |
| Force sentence break | `\!br` | Insert explicit punctuation and optionally `[[slnc …]]` | Speech Manager derives phrasing from punctuation; insert period/semicolon and a short silence to mimic FlexTalk's forced break. |

## Where to read about Speech Manager embedded commands

- **Inside Macintosh: Sound, Chapter 4 (Speech Manager)** — The canonical Toolbox description of the embedded-command grammar (delimiters, `[[vers]]`, `[[rate]]`, `[[pbas]]`, `[[pmod]]`, `[[slnc]]`, `[[dlim]]`, etc.), callback rules, and voice selection. Find it in the local overlay at `/opt/MacDevDocs/1994-12-DevCD-RL/Inside Macintosh/Sound/IM_Sound.pdf`.
- **Tech Note HW09 “Speech Manager”** — A concise summary of the same grammar plus edge cases for delimiters and command parsing. In `/opt/Latest Notes From Apple/Hardware/HW 09 - Speech Manager`.
- **Headers that reflect the grammar** — `Speech.h` / `SpeechSynthesis.h` in `/opt/Interfaces&Libraries/Interfaces/CIncludes/` show the Speech Manager constants and comments for embedded commands (including the `[[vers 1]]` recommendation and the `[[cmnt]]` behavior). Useful when tuning the encoder for traps instead of PPC SpeechLib.
- **PlainTalk User’s Guide (optional)** — Provides MacinTalk voice-side notes on how punctuation and emphasis are treated; good for sanity-checking phrasing when our markup is light. Look under `/opt/MacDevDocs/1997-09-DevCD/Development_Platforms/PlainTalk/`.

These are all read-only references; do not copy them into the repo. Use them to confirm the command spellings and parameter formats when porting the VOX prosody output.

## Practical translation tips
- Keep delimiters consistent: surround Speech Manager commands with `[[` and `]]`, and consider starting strings with `[[vers 1]]` to avoid parser ambiguity.
- Speech Manager commands apply until overridden; emit "phrase-scoped" equivalents by resetting afterward (for example, `[[rate 180]] ... [[rate -0]]` to restore default if the synthesizer honors relative zero).
- FlexTalk's detailed prosody controls (TOBI tones, prominence scalars, phrase break hints) have no one-to-one match. Capture intent with punctuation, `[[emph ±]]`, and modest `[[pbas]]`/`[[pmod]]` changes rather than trying to mirror every code point.
- Preprocess text when FlexTalk relied on detectors (addresses, dates, telephone numbers). Use your own normalization before handing text to Speech Manager so the spoken form stays consistent.
- When porting the NetTTS prosody encoder, mirror its prosody decisions but target Speech Manager commands directly: carry over its heuristics for phrasing and emphasis, but emit `[[rate]]`, `[[slnc]]`, `[[pbas]]`, `[[emph]]`, etc. inline instead of generating `\!` escapes first.

## Porting checklist for the NetTTS prosody encoder

1. **Fork the logic, not the syntax**: Reuse FlexTalk's prosody heuristics as the behavioral template, but emit Speech Manager commands directly instead of producing `\!` escapes.
2. **Map every decision point**: For each place the FlexTalk encoder would write an escape (rate change, silence, emphasis, pitch tweak, forced break), write the Speech Manager equivalent from the table above. Drop or approximate features with no analog.
3. **Cache defaults**: Track baseline speaking rate (WPM) and pitch so relative FlexTalk ratios have an anchor; restore defaults after phrase-scoped adjustments. Emit `[[vers 1]]` early to lock the parser version and avoid synthesizer defaults changing expectations.
4. **Normalize text classes**: Where the encoder would have relied on FlexTalk detectors (`\!n*`, acronym/proofread modes), run your own pre-normalization pass to insert spaces/punctuation that Speech Manager will parse naturally.
5. **Pad missing prosody with punctuation**: When FlexTalk signaled break strength (`\!br`, trailing de-accent codes), inject punctuation and `[[slnc …]]` to create similar phrasing.
6. **Voice selection**: Map FlexTalk gender/tract hints to the closest installed Speech Manager voice via the API rather than embedded commands, unless a vendor-specific `[[xtnd]]` capability exists. Cache the selected channel so follow-on `[[pbas]]`/`[[pmod]]` adjustments remain channel-local.
7. **Debugging with live speech**: Wrap emitted commands with `[[cmnt ...]]` during bring-up to verify placements without affecting audio output. Now that the Speech Manager trap path is available, exercise the encoder against BasiliskII/PlainTalk and log which embedded commands are honored to refine the mapping table.
