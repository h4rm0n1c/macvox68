"""Quantize RGBA artwork into classic Mac icon resources.

Usage:
    python docs/tools/generate_mac_icons.py <source_base64_or_png> --output macvox68_icons.r

The source can be either a PNG file or a text file containing base64-encoded
PNG data. The output is a Rez file that defines icl4/icl8/ics4/ics8 alongside
ICN#/ics# and a BNDL/FREF pair.
"""

import argparse
import base64
import io
from pathlib import Path
from typing import Iterable, List, Tuple

from PIL import Image


def _load_image(source: Path) -> Image.Image:
    data = source.read_text().strip()
    try:
        raw = base64.b64decode(data, validate=True)
        return Image.open(io.BytesIO(raw)).convert("RGBA")
    except Exception:
        return Image.open(source).convert("RGBA")


def _resize(image: Image.Image, size: int) -> Image.Image:
    return image.resize((size, size), resample=Image.LANCZOS)


def _quantize(image: Image.Image, colors: int) -> Image.Image:
    # Pillow only supports Mediancut/libimagequant for RGB/P images, so flatten
    # the alpha channel into RGB before quantizing.
    rgb_image = image.convert("RGB")
    return rgb_image.quantize(colors=colors, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)


def _pack_nibbles(indices: Iterable[int]) -> bytes:
    values = list(indices)
    assert len(values) % 2 == 0
    out = bytearray()
    for hi, lo in zip(values[0::2], values[1::2]):
        out.append(((hi & 0xF) << 4) | (lo & 0xF))
    return bytes(out)


def _rows_to_hex(rows: Iterable[bytes]) -> List[str]:
    lines: List[str] = []
    for row in rows:
        hex_bytes = row.hex().upper()
        groups = [hex_bytes[i : i + 4] for i in range(0, len(hex_bytes), 4)]
        lines.append("$\"" + " ".join(groups) + "\"")
    return lines


def _plane_to_rows(data: bytes, row_bytes: int) -> List[bytes]:
    return [data[i : i + row_bytes] for i in range(0, len(data), row_bytes)]


def _bitplane(mask: Image.Image) -> bytes:
    pixels = list(mask.getdata())
    width, height = mask.size
    row_bytes = (width + 7) // 8
    out = bytearray()
    for y in range(height):
        row_bits = 0
        bits_filled = 0
        for x in range(width):
            row_bits = (row_bits << 1) | (1 if pixels[y * width + x] else 0)
            bits_filled += 1
            if bits_filled == 8:
                out.append(row_bits)
                row_bits = 0
                bits_filled = 0
        if bits_filled:
            out.append(row_bits << (8 - bits_filled))
        while len(out) % row_bytes:
            out.append(0)
    return bytes(out)


def _make_mask(image: Image.Image, threshold: int = 8) -> Image.Image:
    alpha = image.getchannel("A")
    return alpha.point(lambda a: 255 if a >= threshold else 0)


def _make_bw(image: Image.Image) -> Image.Image:
    gray = image.convert("L")
    return gray.point(lambda v: 255 if v < 128 else 0)


def _icon_bytes(img: Image.Image, bits: int) -> Tuple[bytes, Image.Image]:
    if bits == 8:
        pal_img = _quantize(img, 256)
        return bytes(pal_img.getdata()), pal_img
    if bits == 4:
        pal_img = _quantize(img, 16)
        return _pack_nibbles(pal_img.getdata()), pal_img
    raise ValueError("bits must be 4 or 8")


def _format_resource(name: str, res_id: int, payload_lines: List[str]) -> str:
    body = "\n".join(f"        {line}" for line in payload_lines)
    return f"resource '{name}' ({res_id}, purgeable) {{\n{body}\n}};\n"


def _format_icn(mask_bytes: bytes, image_bytes: bytes, row_bytes: int) -> str:
    mask_lines = _rows_to_hex(_plane_to_rows(mask_bytes, row_bytes))
    image_lines = _rows_to_hex(_plane_to_rows(image_bytes, row_bytes))
    lines = ["        {       /* array: 2 elements */", "                /* [1] */"]
    lines += [f"                {line}" for line in mask_lines]
    lines.append(",")
    lines.append("                /* [2] */")
    lines += [f"                {line}" for line in image_lines]
    lines.append("        }")
    body = "\n".join(lines)
    return f"resource 'ICN#' (128, purgeable) {{\n{body}\n}};\n"


def _format_ics(mask_bytes: bytes, image_bytes: bytes, row_bytes: int) -> str:
    mask_lines = _rows_to_hex(_plane_to_rows(mask_bytes, row_bytes))
    image_lines = _rows_to_hex(_plane_to_rows(image_bytes, row_bytes))
    lines = ["        {       /* array: 2 elements */", "                /* [1] */"]
    lines += [f"                {line}" for line in mask_lines]
    lines.append(",")
    lines.append("                /* [2] */")
    lines += [f"                {line}" for line in image_lines]
    lines.append("        }")
    body = "\n".join(lines)
    return f"resource 'ics#' (128, purgeable) {{\n{body}\n}};\n"


def generate_rez(source: Image.Image) -> str:
    large = _resize(source, 32)
    small = _resize(source, 16)

    # Color icons
    icl8_bytes, _ = _icon_bytes(large, 8)
    icl4_bytes, _ = _icon_bytes(large, 4)
    ics8_bytes, _ = _icon_bytes(small, 8)
    ics4_bytes, _ = _icon_bytes(small, 4)

    icn_mask = _bitplane(_make_mask(large))
    icn_bits = _bitplane(_make_bw(large))
    ics_mask = _bitplane(_make_mask(small))
    ics_bits = _bitplane(_make_bw(small))

    rez = []
    rez.append("#include \"Types.r\"")
    rez.append("#include \"SysTypes.r\"")
    rez.append("#include \"Icons.r\"")
    rez.append("#include \"Finder.r\"")
    rez.append("")
    rez.append(_format_resource("icl8", 128, _rows_to_hex(_plane_to_rows(icl8_bytes, 32))))
    rez.append(_format_resource("icl4", 128, _rows_to_hex(_plane_to_rows(icl4_bytes, 16))))
    rez.append(_format_icn(icn_mask, icn_bits, 4))
    rez.append(_format_resource("ics8", 128, _rows_to_hex(_plane_to_rows(ics8_bytes, 16))))
    rez.append(_format_resource("ics4", 128, _rows_to_hex(_plane_to_rows(ics4_bytes, 8))))
    rez.append(_format_ics(ics_mask, ics_bits, 2))

    rez.append(
        "resource 'FREF' (128, purgeable) {\n"
        "    'APPL',\n"
        "    0,\n"
        "    \"MacVox68\"\n"
        "};\n"
    )

    rez.append(
        "resource 'BNDL' (128, purgeable) {\n"
        "    'MV68',\n"
        "    0,\n"
        "    {\n"
        "        'ICN#', { 0, 128 },\n"
        "        'icl4', { 0, 128 },\n"
        "        'icl8', { 0, 128 },\n"
        "        'ics#', { 0, 128 },\n"
        "        'ics4', { 0, 128 },\n"
        "        'ics8', { 0, 128 },\n"
        "        'FREF', { 0, 128 }\n"
        "    }\n"
        "};\n"
    )

    return "\n".join(rez)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="PNG file or base64 text containing the RGBA icon")
    parser.add_argument("--output", type=Path, default=Path("macvox68_icons.r"), help="Rez output path")
    args = parser.parse_args()

    image = _load_image(args.source)
    rez_text = generate_rez(image)
    args.output.write_text(rez_text)


if __name__ == "__main__":
    main()
