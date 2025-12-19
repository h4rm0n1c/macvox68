# Icon generation wins

## 2025-09-24

### ICN#/ics# array formatting fixes Rez assertion

**Symptom**
- Retro68 Rez failed with `ArrayField::compile` assertion errors when compiling `macvox68_icon.r`.

**Root cause**
- `ICN#` and `ics#` resources were emitted as a single flattened hex block.
- Retro68 expects these resources to be **arrays of two elements** (icon bits + mask bits), matching the canonical format in Retro68 samples.

**Fix**
- Emit `ICN#` and `ics#` resources as two-element arrays (`[1]` icon bits, `[2]` mask bits).
- Regenerate `macvox68_icon.r` with the array form.

**References**
- Pattern example: `/opt/Retro68/LaunchAPPL/Server/LauncherIcon.r`
- Generator: `docs/generate_icon_r.py`

