# MacVox68 icon workflow

This repository keeps the MacVox68 program icon in Rez source form so it can be
built directly into the application. The original RGBA artwork is stored as a
base64-encoded PNG (text, not binary) at
`docs/artwork/nettts_icon_base64.txt`. A helper script quantizes it into
classic Mac icon resources so the Finder can pick up color and monochrome
variants.

## Rebuilding the icon resources

1. Ensure Pillow is available in your Python environment (`python -m pip install
   pillow`).
2. Run the quantizer script, pointing it at the base64 artwork file:

   ```sh
   python docs/tools/generate_mac_icons.py docs/artwork/nettts_icon_base64.txt --output macvox68_icons.r
   ```

   The script emits:
   - `icl4`/`icl8` for 4-bit and 8-bit 32×32 color icons
   - `ics4`/`ics8` for 4-bit and 8-bit 16×16 color icons
   - `ICN#`/`ics#` monochrome icon/mask pairs
   - `BNDL`/`FREF` entries that associate the icon family with the `MV68`
     creator/type.

3. Re-run CMake/Ninja on the host build to bundle the updated resources.

The generator matches Retro68/BNDL expectations by writing each icon plane
row-by-row (no compression) and keeping the mask planes aligned so System 7/8
Finder desktops respect both the color data and the one-bit masks.
