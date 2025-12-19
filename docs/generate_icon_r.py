"""Generate a Rez .r file for MacVox68 icons from palette PNG sources.

The script expects six input PNGs:
- 32x32 256-color icon
- 16x16 256-color icon
- 32x32 black and white icon (1-bit)
- 16x16 black and white icon (1-bit)
- 32x32 mask (black=opaque, white=transparent)
- 16x16 mask (black=opaque, white=transparent)

Outputs a Rez source file containing 'icl8', 'ics8', 'icl4', 'ics4',
'ICN#', and 'ics#' resources that share the same resource ID and name.

Example:
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

Requires Pillow for PNG decoding.
"""

import argparse
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

from PIL import Image


DEFAULT_OUTPUT = Path(__file__).resolve().parent.parent / "macvox68_icon.r"


def _ensure_paletted(image: Image.Image, expected_size: tuple[int, int]) -> Image.Image:
    if image.size != expected_size:
        raise ValueError(f"Expected paletted image size {expected_size}, got {image.size}")
    if image.mode != "P":
        raise ValueError(
            f"Palette mode required for accurate 8-bit export (got {image.mode}). "
            "Re-save the source as a paletted PNG."
        )
    palette = image.getpalette()
    if palette is None:
        raise ValueError("Paletted icon is missing palette data.")
    if len(palette) != 768:
        raise ValueError(
            "Paletted icon must contain 256 color entries (768 palette bytes)."
        )
    return image


def _ensure_one_bit(image: Image.Image, expected_size: tuple[int, int]) -> Image.Image:
    if image.size != expected_size:
        raise ValueError(f"Expected 1-bit image size {expected_size}, got {image.size}")
    return image.convert("1")


def _pack_bits(rows: Iterable[Iterable[bool]]) -> bytes:
    packed = bytearray()
    for row in rows:
        byte = 0
        bit_count = 0
        for pixel_on in row:
            byte = (byte << 1) | (1 if pixel_on else 0)
            bit_count += 1
            if bit_count == 8:
                packed.append(byte)
                byte = 0
                bit_count = 0
        if bit_count:
            packed.append(byte << (8 - bit_count))
    return bytes(packed)


def _bool_rows_from_1bit(image: Image.Image, invert: bool = False) -> List[List[bool]]:
    pixels = list(image.getdata())
    rows: List[List[bool]] = []
    width, height = image.size
    for y in range(height):
        row = []
        for x in range(width):
            value = pixels[y * width + x]
            is_black = value == 0
            row.append(not is_black if invert else is_black)
        rows.append(row)
    return rows


def _pack_nibbles(index_bytes: bytes) -> bytes:
    if len(index_bytes) % 2:
        raise ValueError("4-bit icon data must have an even pixel count for nibble packing.")
    packed = bytearray()
    for i in range(0, len(index_bytes), 2):
        high = index_bytes[i] & 0x0F
        low = index_bytes[i + 1] & 0x0F
        packed.append((high << 4) | low)
    return bytes(packed)


def _palette_from_image(image: Image.Image, expected_entries: int) -> List[Tuple[int, int, int]]:
    palette = image.getpalette()
    if palette is None:
        raise ValueError("Palette source image is missing palette data.")
    expected_length = expected_entries * 3
    if len(palette) < expected_length:
        raise ValueError(
            f"Palette source image must contain at least {expected_entries} entries."
        )
    colors = []
    for i in range(expected_entries):
        offset = i * 3
        colors.append((palette[offset], palette[offset + 1], palette[offset + 2]))
    return colors


def _index_image_to_palette(
    image: Image.Image, palette: Sequence[Tuple[int, int, int]]
) -> bytes:
    base = image.convert("RGB")
    pixels = list(base.getdata())
    lookup = {rgb: idx for idx, rgb in enumerate(palette)}
    indices = bytearray(len(pixels))
    for i, rgb in enumerate(pixels):
        if rgb in lookup:
            indices[i] = lookup[rgb]
        else:
            r, g, b = rgb
            best_index = 0
            best_distance = None
            for idx, (pr, pg, pb) in enumerate(palette):
                dr = r - pr
                dg = g - pg
                db = b - pb
                distance = dr * dr + dg * dg + db * db
                if best_distance is None or distance < best_distance:
                    best_distance = distance
                    best_index = idx
            indices[i] = best_index
    return bytes(indices)


def _quantize_to_palette(
    image: Image.Image, palette: Sequence[Tuple[int, int, int]]
) -> Image.Image:
    base = image.convert("RGBA")
    palette_image = Image.new("P", (1, 1))
    flat_palette: List[int] = []
    for r, g, b in palette:
        flat_palette.extend([r, g, b])
    flat_palette.extend([0] * (768 - len(flat_palette)))
    palette_image.putpalette(flat_palette)
    return base.quantize(palette=palette_image, dither=Image.NONE)


def _raw_index_bytes(image: Image.Image) -> bytes:
    return bytes(image.getdata())


def _format_hex_lines(data: bytes, indent: str = "    ", bytes_per_line: int = 16) -> List[str]:
    lines: List[str] = []
    for offset in range(0, len(data), bytes_per_line):
        chunk = data[offset : offset + bytes_per_line]
        hex_chunk = " ".join(f"{byte:02X}" for byte in chunk)
        lines.append(f"{indent}$\"{hex_chunk}\"")
    return lines


def _resource_block(res_type: str, res_id: int, data: bytes) -> str:
    lines = [f"resource '{res_type}' ({res_id}, purgeable) {{"]
    lines.extend(_format_hex_lines(data))
    lines.append("};\n")
    return "\n".join(lines)


def _bundle_block(
    creator: str, bundle_id: int, icon_res_id: int, name: str, file_type: str
) -> str:
    mappings = [
        "        'ICN#', { 0, %d }," % icon_res_id,
        "        'icl8', { 0, %d }," % icon_res_id,
        "        'icl4', { 0, %d }," % icon_res_id,
        "        'ics#', { 0, %d }," % icon_res_id,
        "        'ics8', { 0, %d }," % icon_res_id,
        "        'ics4', { 0, %d }," % icon_res_id,
        "        'FREF', { 0, %d }" % bundle_id,
    ]
    return "\n".join(
        [
            f"resource 'BNDL' ({bundle_id}, purgeable) {{",
            f"    '{creator}',",
            "    0,",
            "    {",
            "        " + "\n        ".join(mappings),
            "    }",
            "};\n",
            f"resource 'FREF' ({bundle_id}, purgeable) {{",
            f"    '{file_type}',",
            "    0,",
            f"    \"{name}\"",
            "};\n",
        ]
    )


def _build_icn_resource(icon_bits: bytes, mask_bits: bytes) -> bytes:
    return icon_bits + mask_bits


def generate_rez(
    name: str,
    res_id: int,
    paths: dict[str, Path],
    *,
    creator: str,
    file_type: str,
    include_bundle: bool,
    bundle_id: int,
    palette_source: Path | None,
    palette4_source: Path | None,
) -> str:
    color32 = _ensure_paletted(Image.open(paths["color32"]), (32, 32))
    color16 = _ensure_paletted(Image.open(paths["color16"]), (16, 16))
    bw32 = _ensure_one_bit(Image.open(paths["bw32"]), (32, 32))
    bw16 = _ensure_one_bit(Image.open(paths["bw16"]), (16, 16))
    mask32 = _ensure_one_bit(Image.open(paths["mask32"]), (32, 32))
    mask16 = _ensure_one_bit(Image.open(paths["mask16"]), (16, 16))

    if palette_source:
        palette_image = _ensure_paletted(Image.open(palette_source), (16, 16))
        palette = _palette_from_image(palette_image, 256)
        icl8_data = _index_image_to_palette(color32, palette)
        ics8_data = _index_image_to_palette(color16, palette)
    else:
        if color32.getpalette() != color16.getpalette():
            raise ValueError(
                "16px and 32px color icons must share the exact palette ordering."
            )
        icl8_data = _raw_index_bytes(color32)
        ics8_data = _raw_index_bytes(color16)
        palette = _palette_from_image(color32, 256)

    if palette4_source:
        palette4_image = _ensure_paletted(Image.open(palette4_source), (16, 16))
        palette4 = _palette_from_image(palette4_image, 16)
    else:
        palette4 = palette[:16]

    color32_4bit = _quantize_to_palette(color32, palette4)
    color16_4bit = _quantize_to_palette(color16, palette4)
    icl4_bytes = _index_image_to_palette(color32_4bit, palette4)
    ics4_bytes = _index_image_to_palette(color16_4bit, palette4)
    icl4_data = _pack_nibbles(icl4_bytes)
    ics4_data = _pack_nibbles(ics4_bytes)

    icn_icon = _pack_bits(_bool_rows_from_1bit(bw32))
    icn_mask = _pack_bits(_bool_rows_from_1bit(mask32, invert=True))
    ics_icon = _pack_bits(_bool_rows_from_1bit(bw16))
    ics_mask = _pack_bits(_bool_rows_from_1bit(mask16, invert=True))

    icn_data = _build_icn_resource(icn_icon, icn_mask)
    ics_data = _build_icn_resource(ics_icon, ics_mask)

    sections = [
        _resource_block("icl8", res_id, icl8_data),
        _resource_block("icl4", res_id, icl4_data),
        _resource_block("ics8", res_id, ics8_data),
        _resource_block("ics4", res_id, ics4_data),
        _resource_block("ICN#", res_id, icn_data),
        _resource_block("ics#", res_id, ics_data),
    ]
    if include_bundle:
        sections.append(_bundle_block(creator, bundle_id, res_id, name, file_type))
    header = [
        "/* Generated by generate_icon_r.py. */",
        "/* Resource types: 'icl8', 'icl4', 'ics8', 'ics4', 'ICN#', 'ics#', optional 'BNDL' + 'FREF'. */",
        '#include "Types.r"',
        '#include "SysTypes.r"',
        '#include "Icons.r"',
        '#include "Finder.r"',
        "",
    ]
    return "\n".join(header + sections)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a Rez .r file for MacVox68 icons.")
    parser.add_argument("--name", default="MacVox68", help="Resource name used for all icon variants.")
    parser.add_argument("--res-id", type=int, default=128, help="Resource ID used for all icon variants.")
    parser.add_argument("--color32", type=Path, required=True, help="Path to 32x32 8-bit color PNG.")
    parser.add_argument("--color16", type=Path, required=True, help="Path to 16x16 8-bit color PNG.")
    parser.add_argument("--bw32", type=Path, required=True, help="Path to 32x32 1-bit PNG.")
    parser.add_argument("--bw16", type=Path, required=True, help="Path to 16x16 1-bit PNG.")
    parser.add_argument("--mask32", type=Path, required=True, help="Path to 32x32 mask PNG (black=opaque).")
    parser.add_argument("--mask16", type=Path, required=True, help="Path to 16x16 mask PNG (black=opaque).")
    parser.add_argument(
        "--palette-source",
        type=Path,
        default=None,
        help=(
            "Optional paletted PNG supplying the canonical 256-color palette ordering. "
            "When set, input pixels are remapped by RGB to this palette."
        ),
    )
    parser.add_argument(
        "--palette4-source",
        type=Path,
        default=None,
        help=(
            "Optional paletted PNG supplying the canonical 16-color palette ordering "
            "for 4-bit icons. Defaults to the first 16 entries of --palette-source or "
            "the input icon palette."
        ),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=(
            "Destination Rez source file (defaults to macvox68_icon.r in the project root)."
        ),
    )
    parser.add_argument(
        "--creator",
        default="MV68",
        help="Creator code used in the bundle (defaults to MV68).",
    )
    parser.add_argument(
        "--file-type", default="APPL", help="File type used in the FREF (defaults to APPL)."
    )
    parser.add_argument(
        "--bundle-id",
        type=int,
        default=None,
        help="Resource ID for BNDL/FREF (defaults to res-id when omitted).",
    )
    parser.add_argument(
        "--no-bundle",
        action="store_true",
        help="Skip emitting BNDL/FREF resources (icons only).",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    paths = {
        "color32": args.color32,
        "color16": args.color16,
        "bw32": args.bw32,
        "bw16": args.bw16,
        "mask32": args.mask32,
        "mask16": args.mask16,
    }
    bundle_id = args.bundle_id if args.bundle_id is not None else args.res_id
    rez_text = generate_rez(
        args.name,
        args.res_id,
        paths,
        creator=args.creator,
        file_type=args.file_type,
        include_bundle=not args.no_bundle,
        bundle_id=bundle_id,
        palette_source=args.palette_source,
        palette4_source=args.palette4_source,
    )
    args.output.write_text(rez_text, encoding="utf-8")


if __name__ == "__main__":
    main()
