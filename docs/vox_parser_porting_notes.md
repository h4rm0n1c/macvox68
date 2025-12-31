# VOX prosody parser porting notes (FlexTalk to Speech Manager)

This note captures how to carry the existing VOX text-normalization/prosody pipeline over to classic Macintosh Speech Manager output. It assumes the C++ parser used in NetTTS (excerpt below) remains the behavioral reference while we redirect its emissions from FlexTalk escape sequences to Speech Manager embedded commands.

## Reference implementation snapshot

Behavioral source of truth lives in NetTTS (`src/vox_parser.cpp` on the `main` branch). Use the raw view while porting so naming and token ordering stay faithful to the shipping Windows build:

- Raw file: https://raw.githubusercontent.com/h4rm0n1c/NetTTS/refs/heads/main/src/vox_parser.cpp

Key stages from the current C++ pipeline (`vox_process` and helpers):

- **Pre-normalization utilities**: trimming, lowercase conversion, digit checks, titlecase detection, trailing punctuation strip, `\!br` detection.
- **Sentence split**: `split_sentences` keeps terminal punctuation attached.
- **Lead-in prosody**: `apply_leadin_break` inserts a forced break after common introductory phrases.
- **"the" → "thee" heuristic**: `apply_thee_rule` rewrites based on nearby prepositions or proper nouns, aware of `\!br` tags and punctuation tails.
- **Time/number/degree expansion**: `apply_time_numbers_degrees` handles meridiem times, 24h "HH00 hours", three-digit decomposition, and degree units.
- **Beat construction**: `build_beats` tokenizes, applies weight-based break insertion (light/medium/heavy), forces breaks around dashes, enforces symmetric linker isolation, pre-copula breaks, and TitleCase runs.
- **Letter normalization**: `normalize_letter_tokens` rewrites single-letter tokens into stable phoneme-like markers near boundaries.
- **Tidy + wrap**: `tidy` collapses spaces and normalizes `\!br`; final wrapping optionally injects `\!wH1`/`\!wH0` span markers.

The pipeline is intentionally linear: sentence split → the/thee → lead-in → time/number → beat build → letter normalization → tidy → optional wrapper.

## Port target: Speech Manager embedded commands

The existing logic emits FlexTalk `\!` escapes. When ported, the same prosody decisions should yield Speech Manager embedded commands:

- Treat `\!br` as a **synthetic boundary**. Emit a short silence plus punctuation, e.g., `[[slnc 250]]` followed by a comma/semicolon equivalent. Surrounding `[[vers 1]]` at stream start stabilizes parsing.
- Where the pipeline inserts **rate/pitch/emphasis spans**, map to `[[rate <wpm>]]`, `[[pbas <fixed-point>]]`/`[[pmod <fixed-point>]]`, and `[[emph +]]` or `[[emph -]]`. Cache the baseline rate/pitch so phrase-scoped adjustments can be restored after the affected run.
- **Letter tokens** should produce literal-letter output via `[[char LTRL]]`/`[[nmbr LTRL]]` bracketing or by inserting spaces plus `[[dlim]]`-stable delimiters if the synthesizer mishandles single characters.
- **Silence insertion** from the number/time rules becomes `[[slnc <ms>]]` (centiseconds ×10). Preserve ordering so surrounding words do not reorder relative to the original pipeline.
- **Wrapper tags** (`\!wH1`/`\!wH0`) can be replaced by a leading `[[vers 1]]` and (optionally) `[[cmnt]]` markers during bring-up to verify placement without affecting audio.

## Implementation strategy in MacVox68

- Add a dedicated translation layer (e.g., `vox_parser.c` + `vox_parser.h`) compiled as C89/C90 to stay Retro68-friendly. The logic can still mirror the C++ pipeline by:
  - Converting wide/UTF-16 uses to simple 8-bit char buffers (System 7 Roman). The original `std::wstring` operations become plain `char`/`Str255` manipulations with manual bounds checks.
  - Replacing `std::regex` with manual pattern checks (prefix/suffix tests) to avoid C++11 dependencies. The outlined heuristics are mostly finite-state and can be expressed with scanning loops.
  - Keeping token lists in small fixed arrays to avoid dynamic allocation where possible.
- Track porting progress against the reference source instead of prose summaries:
  - Mirror function boundaries: start by porting `split_sentences`, `apply_thee_rule`, `apply_leadin_break`, `apply_time_numbers_degrees`, `build_beats`, `normalize_letter_tokens`, and `tidy` as discrete C functions. Preserve the call order used in `vox_process` so later validation can diff strings against the Windows build.
  - Keep temporary instrumentation that can emit both FlexTalk escapes and Speech Manager markup from the same C code path. Toggling the target (FlexTalk vs. Speech Manager) with a flag makes it easier to diff outputs against the upstream executable until the Speech Manager-only build stabilizes.
  - Match the `\!br` placement logic exactly before swapping the emission target. Prosody drift typically comes from missing breaks rather than text differences; port break insertion first, then swap escapes for `[[slnc ...]]`/punctuation.
- Provide a **formatter hook** that accepts a raw C string and returns a newly allocated `Handle` or `Ptr` ready for `SpeakText`/`SpeakString`, already containing Speech Manager commands. This keeps `speech.c` thin: it asks the parser to format text, then hands the result to Speech Manager.
- The formatter in `vox_parser.c` now mirrors the upstream `vox_process` stages (sentence split → “thee” heuristic → lead-in break → time/number/degree handling → beat building → letter normalization → tidy/wrap) before emitting Speech Manager commands. Set `speech_manager_mode` to `false` via `vox_format_text_mode` to inspect raw FlexTalk markers for parity checks against the Windows build during bring-up.
- A host-side harness (`tests/vox_parser_host_checks.c`) builds on Linux with `VOX_PARSER_STANDALONE` defined and checks for expected `\!br` placement, “thee” rewrites, and `[[vers]]`/`[[slnc]]` emission without needing Retro68.
- Begin with a **literal command passthrough mode**: emit the same text plus `[[vers 1]]` and `[[cmnt porting]]` decorations so we can validate parsing in BasiliskII without committing to every heuristic at once. Gradually enable the time/number and break insertion stages as they are reimplemented.
- Keep a **unit test harness** in the host/Linux environment that feeds sample phrases through the C port and inspects the resulting markup (no audio required). Align test cases with the original C++ outputs to verify behavioral parity before layering Speech Manager-specific tweaks.

## Local references while porting

Use the repo’s Speech Manager mapping (`docs/flextalk_speech_manager_mapping.md`) alongside the Apple references in `/opt`:

- Inside Macintosh: Sound, Chapter 4 (Speech Manager command grammar)
- Tech Note HW09 “Speech Manager” (delimiter + parsing edge cases)
- `Speech.h` / `SpeechSynthesis.h` headers in `/opt/Interfaces&Libraries/Interfaces/CIncludes/`

These cover the exact embedded-command spellings and clarify where phrase-scoped parameters must be reset. Do not copy them into the repo; cite paths in commit messages or docs when needed.
