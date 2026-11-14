# ThreadLib (Thread Library) — reference notes

Source: user-provided summary of ThreadLib 1.0 (Ari Halberstadt, 1994–1995), with download links from Macintosh Garden.

## Summary
- Cooperative threading library for Classic Mac apps (single-process, non-preemptive).
- Compatible with System 6.x (Finder/MultiFinder) through Mac OS 8.1; Mac Plus and later.
- Ships as full C source plus prebuilt libs for 68k and PPC (including debug builds).
- Provides a Thread Manager–like compatibility layer (Thread Library Manager / “TLM”).

## Notable claims
- 68k build reported ~2–3× faster than Apple Thread Manager; PPC ~2× faster.
- Small footprint (~3–8 KB).
- Works with Symantec, Metrowerks CodeWarrior, and MPW toolchains.

## Headers
- `ThreadLibrary.h`
- `ThreadLibraryManager.h` (Thread Manager–like layer, “TLM”)
- `ThreadLibraryStubs.h` (compile-out macros)

## Minimal API calls (typical usage)
- `ThreadBeginMain`
- `ThreadBegin`
- `ThreadEnd`
- `ThreadYield`

## Additional useful calls
- `ThreadYieldInterval`
- `ThreadMain`
- `ThreadActive`
- `ThreadEndAll`

## Error handling
- Call `ThreadError()`; success indicated by `THREAD_ERROR_NONE`.

## Compatibility cautions
- If using the TLM compatibility layer, do **not** mix raw Thread Library calls (thread IDs differ).
- No synchronization primitives (similar limitation to Apple Thread Manager).
- Exceptions require per-thread exception stacks + suspend/resume callbacks; do not let exceptions propagate beyond a thread entry point.

## Debugging
- Disable optimizations and link the appropriate debug object:
  - `ThreadLibrary-68K-Debug.o` or `ThreadLibrary-PPC-Debug.o`

## Downloads (Macintosh Garden)
- `threadlib1.0.sit_.hqx` (411.53 KB, MD5: 4adf3a14d1b2675111388c54f2aaf316)
- `threadlib1.0d4.cpt_.hqx` (117.88 KB, MD5: c5d5c747dfa8d91f2521f1b9281a2c0b)
- `ThreadLib_1.0_Guide.pdf`

Sources:
- https://macintoshgarden.org/apps/threadlib
- https://download.macintoshgarden.org/manuals/ThreadLib_1.0_Guide.pdf

## Historical note
- Apple trademark guidance for MacDNS docs references “ThreadLib 1.04 © 1994”, suggesting ThreadLib shipped in at least one Apple product.
