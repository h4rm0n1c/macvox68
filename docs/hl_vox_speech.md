# HL VOX speech integration — 2026-11-15

The HL VOX Prosody radio now routes editor text through vox_format_text and then
the owned asynchronous speech channel. Clean keeps embedded-command processing
disabled. Every submission sets the appropriate delimiters, including after a
mode switch. Blank HL VOX input stops without creating markup-only speech.

speech_speak_vox accepts at most 32,767 MacRoman bytes, rejects embedded NUL and
copies a terminated input for the parser. It locks the returned Handle only while
speech_submit makes its own stable copy. Parser temporaries can then be disposed;
Stop, completion and Quit use the same ownership rules as Clean speech.

## Translation corrections

- Wrapper markers are five bytes, not four. They now become silent cmnt markers;
  unsupported FlexTalk vocal-quality controls are not reproduced.
- Final-silence values are milliseconds: sf500 becomes slnc 500, not slnc 5000.
  The old mapping notes' centisecond assumption was incorrect. The Windows
  parser emits sf500 at sentence ends. The newer local VoxTalk TRUTH_LEDGER
  describes its underlying field as plain milliseconds, capped at 1000; the
  adapter follows that cap and bounds numeric parsing to the token.
- Each literal input '[' gains a separating space during markup emission so user
  bracket sequences cannot become Speech Manager commands. Generated commands
  are emitted separately. Clean speaks its original input without this change.
- Allocation failures during final markup appends now return NULL rather than
  an incomplete final markup buffer. Earlier preprocessing stages still need a
  comprehensive allocation-failure audit; this is not an OOM-hardening claim.

Apple corpus 23623 defines delimiters, vers, cmnt and millisecond slnc commands.
Corpus 23635 documents asynchronous embedded-command errors via soErrors.
The instrumentation now accumulates those query results in ui_inspect's speech
object, with command_error_count, command_error, position and query status.
These counters are diagnostic and process-lifetime scoped; queries consume the
Speech Manager's unread error record. They are not an interrupt callback trace.

## Verification

- ASan/UBSan parser fixtures and speech ownership contracts pass, including
  generated markup, millisecond pause, wrapper conversion and literal brackets.
- 2,000 repeated parser/formatter ownership checks pass.
- tools/check_macvox_clean.py --vox verifies the complete generated buffer at
  the actual 68k SpeakText trap, big-endian stack arguments and zero return,
  independent lifetime while editing, repeated cancellation, completion and Quit.
  Evidence: .mac-lab/macvox-vox-proof.json. The same check without --vox verifies
  the Clean regression path. It deliberately causes clipped speech during Stop
  checks; those fragments are not a natural-playback pronunciation test.
- tools/check_macvox_audio_modes.py plays the same complete text in Clean,
  HL VOX, then Clean. All produced nonzero signal, silent tails and no reported
  embedded-command errors. Hello. Hello. took approximately 2.98, 5.21 and 2.76
  seconds respectively, including cooperative UI polling overhead. Those are
  whole-utterance observations, not precise per-command timing measurements.
  Evidence: .mac-lab/macvox-audio-modes-proof.json and corresponding PCM files.
- USB ATR2USB output is enabled independently of the browser. The user confirmed
  audible output before this integration; no claim of human approval of HL VOX
  pronunciation follows from the signal checks.

## Remaining work

The Windows-reference comparison retains exactly the 68 recorded differences
from docs/parser_reference_comparison_2026-11-14.json. This change deliberately
preserves preprocessing behavior; it does not establish Windows parity or exact
Half-Life VOX cadence. Phrase boundaries now use sentence punctuation rather than a fixed 250 ms silence.
This is a semantic correction, not a claim of matching the Windows voice.
Network input and rate/pitch/volume sliders are not connected to this new path.

## Cadence correction after listening feedback

The user heard fast words separated by large pauses. The original adapter
translated every br into 250 ms silence, including duplicate boundaries, and
added an unconditional trailing 250 ms. This was not supported by the reference.
The local Windows reference docs/FLEXTALK_VENDOR_REFERENCE_MANUAL.md explicitly
lists br as "Force End of Sentence" and wH as voice aspiration/breathiness.
Its description supersedes NETTTS_BEHAVIOR.md's unsupported word-emphasis claim.

The adapter now terminates the current phrase with sentence punctuation, preserving
existing . ! ? endings and replacing trailing comma/colon/semicolon. Consecutive
boundaries with no intervening text do nothing. An sf command first terminates
its phrase and emits its explicit millisecond silence; the following br adds
nothing. There is no unconditional trailing silence. This lets the Macintosh
voice provide sentence-final intonation and duration instead of inserting dead
air at every beat. It does not implement FlexTalk aspiration or establish an
identical speaking rate. Listening feedback is still needed for cadence quality.

Host sanitizer fixtures cover sentence termination, repeated-boundary collapse,
no synthetic 250 ms commands and exactly one explicit final silence in the
single-sentence fixture. The preprocessing output is unchanged.

The corrected build (Shared/MacVox-vox4) passed uninterrupted Clean → HL VOX →
Clean USB playback, with nonzero audio, silent tails and zero reported command
errors. The same Hello. Hello. sample took 2.96, 4.15, 2.71 seconds.
Earlier audio/evidence is retained in .mac-lab/cadence-before; current evidence
is .mac-lab/macvox-audio-modes-proof.json. These measurements establish playback,
not perceptual quality. The full announcement was then played without Stop.

## November 16 follow-up

[Reference boundary corrections](cadence_2026-11-16.md) bring the existing
preprocessing matrix to 68/68 exact matches. The earlier 68-difference result is
historical. Editor sliders are also now connected; see the slider implementation
note. Listening and synthesizer-specific cadence mapping remain open.

## November 17 listening correction

The listener rejected the full-sentence mapping above. Internal br now maps to
a word boundary with no added punctuation or silence; real sentence endings
and sf remain. Commas and short explicit silences both made this guest voice
slower. See [the measurements and limitations](cadence_2026-11-17.md).
