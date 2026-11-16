# Native VOX beat rendering, November 17

The encoder was retaining the reference beat structure, but its Mac adapter
collapsed internal breaks to spaces. In the full Black Mesa transit script,
this produced connected speech separated by long sentence pauses. Converting
breaks to text periods also failed: MacInTalk added excessive sentence timing.

## Implemented path

For the measured MacInTalk 3 provider (`soSynthType.synthManufacturer == 'mtk3'`),
MacVox now asks the current channel's TextToPhonemes for each encoder beat,
including a terminal period for independent phrasing. Each native beat has its
own `[[inpt PHON]] ... [[inpt TEXT]]` boundary. Add 100 ms at internal joins;
retain the encoder's explicit sf500 at sentence ends. A br immediately before
sf is one boundary, with sf replacing the internal join. AM/PM remain one spoken
beat ("ay em"/"pee em"); standalone FlexTalk letter metadata is normalized.
The reference preprocessing is unchanged. Other providers use the text adapter.

Pronunciation comes from the installed synthesizer, rather than a hardcoded
phoneme transcription of the test script. TextToPhonemes runs with embedded
commands disabled; generated phonemes cannot contain command delimiters or
non-ASCII controls. Per-beat conversion is bounded to 16384 bytes and aggregate
native output to 256 KiB. Conversion failures propagate without silently
substituting smooth text output. Asynchronous speech retains its owned buffer
until completion, successful stop, or channel disposal.

Set soInputMode directly to modeText before each conversion/utterance: cancelling
native speech can prevent its final TEXT command from executing. Volume, rate
and pitch continue to use the application's channel settings. No callbacks or
raw command editor are added to production.

References: corpus 23624 (phonemic representation/prosodic controls), 23674
(TextToPhonemes), 23628 (soInputMode takes a pointer to OSType), 23640 (zero
command delimiters disable commands), 23645 (asynchronous input ownership).
In this guest, NewHandle(0) gave bufTooSmall despite the documented automatic
resizing; a preallocated 16384-byte conversion handle works.

## Recorded comparison

Same user-supplied full script beginning "Good morning and welcome to the Black
Mesa Transit System." Original NetTTS/FlexTalk executable captured under Wine;
MacVox captured in Basilisk through its private Pulse monitor. No hardware
forwarding or viewer was needed. The three preprocessors agree on this script
and on 68/68 existing ASCII regression cases.

| Recording | Signal span | Detected quiet gaps | Median gap | Total quiet time |
| --- | ---: | ---: | ---: | ---: |
| FlexTalk reference VOX | 50.42 s | 57 | 0.26 s | 15.92 s |
| Previous Mac adapter | 31.15 s | 6 | 1.56 s | 6.79 s |
| Native MacVox adapter | 48.65 s | 49 | 0.26 s | 15.34 s |

Measures use 30 ms RMS windows, 10 ms hops, a per-file peak-relative -35 dB
threshold and minimum 60 ms quiet run; interruptions below 30 ms are closed.
Quiet consonants can count as silence, and these are not aligned word labels.
Additional -30/-40 dB threshold results are retained in the lab report. Duration
and gap agreement support the cadence change; they do not establish matching
intonation, timbre or listener acceptance. FlexTalk VOX has insufficient
periodicity for a reliable F0 comparison with this estimator. Its wH aspiration
still has no established equivalent in this Mac provider.

The independent native probe (51 beats, without the production AM merge) had
49.00 s signal span and 15.73 s quiet time. It supported the 100 ms join setting
before production integration. A continuous PHON-mode experiment did not finish
within its test timeout; the tested renderer explicitly switches mode per beat.

## Validation

- ASan/UBSan parser checks and fake speech/network contracts pass. Tests cover
  independent beats, single sf boundaries, AM grouping, converter failure,
  rejection of command-bearing phonemes, ownership and cancellation-mode reset.
- Host CTest and instrumented/production Retro68 builds pass. The generated disk
  was not mounted or rewritten while open; the test app was staged via Shared.
- The production full-script guest capture completed, released its input buffer
  and reported no speech/embedded-command errors.
- Opening-sentence settings checks: signal span 6.77 s normally, 5.22 s at 150%
  rate. Pitch +6 changed estimated median F0 from 88.9 to 125.0 Hz. Volume 25%
  produced 0.243 of the normal peak amplitude. Cancelling native speech then
  speaking literal Clean text containing `[[volm 0]]` remained audible and
  completed without errors. Defaults were restored afterwards.

Lab artifacts live under OldMacStuff/.mac-lab/bmas-reference-20261117:
production-native-comparison/comparison.json and comparison.png, input.txt,
parity-native12.json, native12-test.log, native12-settings.log,
settings-comparison/comparison.json and settings-levels.json. The raw production
capture is ../bmas-native12-0.wav; reference-vox-listen.wav and
macvox-native-listen.wav have only outer silence trimmed, with no time stretching
or gain changes. File hashes and source paths are in the comparison report.

The staged instrumented app is Unix:MacVox-native12:MacVox68.APPL. The production
build is .mac-lab/build/MacVox68/MacVox68.dsk. Listening acceptance is pending.
