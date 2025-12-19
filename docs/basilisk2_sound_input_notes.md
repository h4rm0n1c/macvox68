# Basilisk II Sound Input Emulation Notes (Classic Mac OS 7.x)

This note captures the current understanding of what it would take to add
**Sound Manager input (microphone) emulation** for the **kanjitalk755/macemu
(Basilisk II)** fork on Linux. It is meant as a starting point for future
implementation and references the **Inside Macintosh** and dev CD docs that are
already available under `/opt`.

## High-level requirements

Adding mic input is not just host capture; it requires **guest-side sound input
emulation** so Classic Mac OS sees a valid input device.

Key parts:

1. **Host capture backend (Linux)**
   - Add an input/capture path in the Linux audio backend (ALSA/Pulse/SDL/etc.).
   - Mirror output path structure where appropriate:
     - device open/close
     - format negotiation
     - ring buffer or callback-driven buffer fill
   - Provide a stable PCM buffer format (mono, 8/16‑bit) suitable for the guest.

2. **Guest Sound Input Manager integration**
   - Implement a Classic Mac OS **sound input device/driver** that the Sound
     Input Manager can open.
   - Wire device control/status entry points to expose capabilities, sample
     format, and buffer readiness.
   - Feed the host capture buffer into guest recording calls (`SPBRecord`,
     `SndRecord`, or equivalent Sound Input Manager APIs).

3. **Bridging layer + timing**
   - Convert host PCM to guest-expected format if needed.
   - Provide buffer sizing and timing logic to avoid underruns.
   - Keep latency tolerable for Classic Mac OS apps.

4. **Configuration**
   - Add prefs for capture device selection (ALSA device name, Pulse source, etc.).
   - Add sample rate, mono/stereo, and buffer size settings if needed.

5. **Validation**
   - Verify in Classic Mac OS Sound control panel or a simple recording app.
   - Ensure the device appears and produces data.

## Why output code is only half the story

The Linux output path can guide the structure for capture (threads, buffers,
format negotiation). But **output mirroring does not create a guest‑visible sound
input device**. Classic Mac OS expects the Sound Input Manager to discover and
open an input device/driver. Without that emulation, the guest will never call
into the capture path.

## Documentation to consult (already in `/opt`)

For driver expectations and Sound Input Manager behavior, use:

- **Inside Macintosh: Sound** — Sound Manager + Sound Input Manager APIs,
  `SPBRecord`/`SndRecord`, buffer formats, and timing expectations.
- **Inside Macintosh: Devices** — driver model, `.DRVR` resources, control/status
  calls, and device interaction patterns.
- **Inside Macintosh: Operating System Utilities** — driver/resource conventions.
- **Inside Macintosh: Memory** — handle locking and buffer handling details.

The ReadableOverlay tree contains the relevant CD editions (folder names):

```
1992-03-DevCD
1994-12-DevCD-RL
1995-08-DevCD-TC
1996-07-DevCD-SSW
1996-12-DevCD-RL
```

The 1994–1995 volumes tend to be the most directly relevant for System 7-era
Sound Input Manager behavior.

## Implementation sketch (non-code)

- Add a host capture backend that writes PCM frames into a ring buffer.
- Define a guest‑visible input device and hook it into the Sound Input Manager.
- Implement control/status calls to describe capabilities and to start/stop
  recording.
- On guest buffer requests, copy from the host ring buffer, converting format as
  needed.
- Add prefs for capture device + format.
- Test with Classic Mac OS Sound control panel and a basic recorder.

## Notes

- This document is intended to guide future implementation; it does not attempt
  to change or patch Basilisk II itself.
- Keep emulation changes small and reversible.
