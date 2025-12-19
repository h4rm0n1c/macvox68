# MacVox68 icon Rez generation

Use `docs/generate_icon_r.py` to turn the supplied PNGs into a Rez `.r` source file with the Classic Mac icon resources required by the app (`'icl8'`, `'icl4'`, `'ics8'`, `'ics4'`, `'ICN#'`, and `'ics#'`). The script remaps pixels by RGB into canonical Classic Mac CLUT8/CLUT4 ordering internally, so palette ordering in the source PNGs is no longer a dependency. It also quantizes a 16-color variant for the 4-bit resources, and builds 1-bit bitmaps for both the B&W icons and masks (black = opaque/ink, white = transparent/background). It can emit a matching purgeable `BNDL` + `FREF` pair so Finder picks up the custom app icon automatically. The generated `.r` is expected to live next to the C and header sources (project root) for Retro68 to find it.

## Inputs
Provide six PNGs:

- 32x32 256-color icon (`--color32`)
- 16x16 256-color icon (`--color16`)
- 32x32 1-bit icon (`--bw32`)
- 16x16 1-bit icon (`--bw16`)
- 32x32 1-bit mask, black = opaque (`--mask32`)
- 16x16 1-bit mask, black = opaque (`--mask16`)

The color icons can be paletted or truecolor PNGs (mode `P`, `RGB`, or `RGBA`); the generator remaps by RGB into the Classic Mac CLUT8/CLUT4 palette ordering. The B&W icons and masks are converted to 1-bit internally; masks are inverted on output so black (opaque) pixels in the PNG become cleared mask bits for Finder transparency.

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

To generate canonical palette PNGs for the classic Mac CLUTs (useful when you need a stable palette reference), run:

```
python docs/make_mac_icon_palette_pngs.py --out-dir .
```

This produces `mac_clut8_palette256.png` and `mac_clut4_palette16.png` locally. You do not need these for the generator anymore, but they can help when you need to inspect or validate palette ordering (do not check the PNGs into the repo).

`--name` and `--res-id` let you align all icon variants to the same resource metadata. The output file is plain text and can be checked into the repo or fed directly to Rez. By default, a `BNDL`/`FREF` bundle is included using creator `MV68`, file type `APPL`, and the same resource ID as the icons; pass `--no-bundle` if you only want the icon data. If you omit `--output`, the script writes `macvox68_icon.r` to the project root automatically (the conventional Retro68 location).

The `.r` header includes `Types.r`, `SysTypes.r`, `Icons.r`, and `Finder.r` so the Rez tool can see the templates for `icl8`/`icl4`/`ics8`/`ics4`, `ICN#`/`ics#`, and the `BNDL`/`FREF` bundle mappings without requiring extra hand edits.

Once you generate `macvox68_icon.r` in the project root, the CMake build will pick it up automatically because the icon Rez file is listed directly alongside the C sources. Keep it in the root next to the `.c`/`.h` files so Retro68 finds it consistently.

### Notes on resource packing
- `icl8` and `ics8` store raw palette indices in row-major order. The generator maps pixels by RGB into the Classic Mac CLUT8 palette so the output indices match the system ordering.
- 4-bit `icl4`/`ics4` data is derived by mapping the 8-bit color pixels into the Classic Mac CLUT4 palette.
- `ICN#` and `ics#` pack two bitmaps each (icon first, then mask). Bits are written most-significant-bit first per byte.
- Mask files are inverted on write so black-as-opaque source pixels yield cleared mask bits and white pixels become set bits, matching the Rez/Finder transparency expectations.
- Bundles follow the standard Finder mapping: each mapped type declares one local ID (`0`) pointing at the shared icon resource ID, and the `FREF` uses the same resource ID as the bundle. This keeps Retro68 Rez happy while still exposing the color and B&W icon variants for small and large sizes.

If you re-encode the PNGs, the generator will still remap RGB values into the Classic Mac palettes, so palette ordering in the PNG is not required (but keeping it consistent can make manual inspection easier).
