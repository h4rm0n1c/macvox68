# MacVox68 icon Rez generation

Use `docs/generate_icon_r.py` to turn the supplied palette PNGs into a Rez `.r` source file with the Classic Mac icon resources required by the app (`'icl8'`, `'ics8'`, `'ICN#'`, and `'ics#'`). The script keeps palette indices intact to preserve the 68k-friendly color tables and builds 1-bit bitmaps for both the B&W icons and masks (black = opaque/ink, white = transparent/background). It can also emit a matching `BNDL` + `FREF` pair so Finder picks up the custom app icon automatically. The generated `.r` is expected to live next to the C and header sources (project root) for Retro68 to find it.

## Inputs
Provide six PNGs:

- 32x32 256-color icon (`--color32`)
- 16x16 256-color icon (`--color16`)
- 32x32 1-bit icon (`--bw32`)
- 16x16 1-bit icon (`--bw16`)
- 32x32 1-bit mask, black = opaque (`--mask32`)
- 16x16 1-bit mask, black = opaque (`--mask16`)

The color icons **must** be paletted PNGs (mode `P`) so palette indices stay stable. The B&W icons and masks are converted to 1-bit internally; anything that is black becomes a set bit in the Rez output.

## Usage
```
python docs/generate_icon_r.py \
    --name "MacVox68" \
    --res-id 128 \
    --color32 macvox68_32.png \
    --color16 macvox68_16.png \
    --bw32 macvox68_bw32.png \
    --bw16 macvox68_bw16.png \
    --mask32 macvox68_mask32.png \
    --mask16 macvox68_mask16.png \
    --output macvox68_icon.r
```

`--name` and `--res-id` let you align all icon variants to the same resource metadata. The output file is plain text and can be checked into the repo or fed directly to Rez. By default, a `BNDL`/`FREF` bundle is included using creator `MV68`, file type `APPL`, and the same resource ID as the icons; pass `--no-bundle` if you only want the icon data. If you omit `--output`, the script writes `macvox68_icon.r` to the project root automatically (the conventional Retro68 location).

The `.r` header includes `Types.r`, `Finder.r`, and `Icons.r` so the Rez tool can see the templates for `icl8`/`ics8`, `ICN#`/`ics#`, and the `BNDL`/`FREF` bundle mappings without requiring extra hand edits. Bundle entries use the standard comma-delimited mappings Retro68's Rez expects.

Once you generate `macvox68_icon.r` in the project root, the CMake build will pick it up automatically when the file exists. You can also point `-DMACVOX68_ICON_REZ=/path/to/icon.r` at a different location. If the file is missing, the build falls back to the default Retro68 icon; a legacy fallback at `docs/macvox68_icon.r` is still honored, but the root-level `.r` is preferred so Rez sits beside the C/H sources.

### Notes on resource packing
- `icl8` and `ics8` store raw palette indices in row-major order and **must share the exact same palette** (the script enforces this).
- `ICN#` and `ics#` pack two bitmaps each (icon first, then mask). Bits are written most-significant-bit first per byte.
- Mask files are treated as classic 1-bit masks where black marks opaque pixels; white stays transparent.
- Bundles follow the standard Finder mapping: local bundle ID `0` maps to the icon resource ID, and the `FREF` uses the same resource ID as the bundle. The generated `BNDL` maps `FREF`, `ICN#`, `icl8`, `ics#`, and `ics8` with comma-separated entries so Finder can see both the color and B&W icon variants for small and large sizes without triggering Rez array assertions.

If you need to re-encode the PNGs, keep the palette ordering identical to avoid remapping color indices.
