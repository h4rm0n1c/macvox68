# MacVox68 icon Rez generation

Use `docs/generate_icon_r.py` to turn the supplied palette PNGs into a Rez `.r` source file with the Classic Mac icon resources required by the app (`'icl8'`, `'icl4'`, `'ics8'`, `'ics4'`, `'ICN#'`, and `'ics#'`). The script can either preserve palette indices as-is or remap pixels by RGB into a canonical palette source (recommended when your PNG palette order doesn't match the system palette). It also quantizes a 16-color variant for the 4-bit resources, and builds 1-bit bitmaps for both the B&W icons and masks (black = opaque/ink, white = transparent/background). It can emit a matching purgeable `BNDL` + `FREF` pair so Finder picks up the custom app icon automatically. The generated `.r` is expected to live next to the C and header sources (project root) for Retro68 to find it.

## Inputs
Provide six PNGs:

- 32x32 256-color icon (`--color32`)
- 16x16 256-color icon (`--color16`)
- 32x32 1-bit icon (`--bw32`)
- 16x16 1-bit icon (`--bw16`)
- 32x32 1-bit mask, black = opaque (`--mask32`)
- 16x16 1-bit mask, black = opaque (`--mask16`)

The color icons **must** be paletted PNGs (mode `P`) so palette indices stay stable. The B&W icons and masks are converted to 1-bit internally; masks are inverted on output so black (opaque) pixels in the PNG become cleared mask bits for Finder transparency.

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

If the PNG palette order is not the standard system palette ordering, pass a canonical palette PNG (for example, from a known-good icon export) and the generator will remap pixels by RGB to the proper indices:

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
    --palette-source known_good_palette.png \
    --palette4-source known_good_palette_4bit.png \
    --output macvox68_icon.r
```

`--name` and `--res-id` let you align all icon variants to the same resource metadata. The output file is plain text and can be checked into the repo or fed directly to Rez. By default, a `BNDL`/`FREF` bundle is included using creator `MV68`, file type `APPL`, and the same resource ID as the icons; pass `--no-bundle` if you only want the icon data. If you omit `--output`, the script writes `macvox68_icon.r` to the project root automatically (the conventional Retro68 location).

The `.r` header includes `Types.r`, `SysTypes.r`, `Icons.r`, and `Finder.r` so the Rez tool can see the templates for `icl8`/`icl4`/`ics8`/`ics4`, `ICN#`/`ics#`, and the `BNDL`/`FREF` bundle mappings without requiring extra hand edits.

Once you generate `macvox68_icon.r` in the project root, the CMake build will pick it up automatically because the icon Rez file is listed directly alongside the C sources. Keep it in the root next to the `.c`/`.h` files so Retro68 finds it consistently.

### Notes on resource packing
- `icl8` and `ics8` store raw palette indices in row-major order. When `--palette-source` is omitted, the script preserves the incoming palette order verbatim and enforces that the 16px/32px PNGs share the exact same palette. When `--palette-source` is set, pixel colors are remapped by RGB into the supplied palette ordering.
- 4-bit `icl4`/`ics4` data is derived by mapping the 8-bit color pixels into a 16-color palette. By default this uses the first 16 colors from the palette source (or input palette), but you can provide a separate `--palette4-source` if you have a canonical CLUT4 ordering to match.
- `ICN#` and `ics#` pack two bitmaps each (icon first, then mask). Bits are written most-significant-bit first per byte.
- Mask files are inverted on write so black-as-opaque source pixels yield cleared mask bits and white pixels become set bits, matching the Rez/Finder transparency expectations.
- Bundles follow the standard Finder mapping: each mapped type declares one local ID (`0`) pointing at the shared icon resource ID, and the `FREF` uses the same resource ID as the bundle. This keeps Retro68 Rez happy while still exposing the color and B&W icon variants for small and large sizes.

If you need to re-encode the PNGs, keep the palette ordering identical to avoid remapping color indices.
