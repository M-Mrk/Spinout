#!/usr/bin/env python3

from __future__ import annotations

import argparse
import math
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


FRAME_COUNT = 360
PIXELS_PER_FRAME = 36
TARGET_SIZE = 72


def find_magick() -> str:
    for candidate in ("magick", "convert"):
        executable = shutil.which(candidate)
        if executable:
            return executable
    raise SystemExit("ImageMagick not found. Install `magick` or `convert`.")


def resize_with_imagemagick(source: Path, destination: Path) -> None:
    executable = find_magick()
    command = [
        executable,
        str(source),
        "-auto-orient",
        "-resize",
        f"{TARGET_SIZE}x{TARGET_SIZE}^",
        "-gravity",
        "center",
        "-extent",
        f"{TARGET_SIZE}x{TARGET_SIZE}",
        "-depth",
        "8",
        f"ppm:{destination}",
    ]
    completed = subprocess.run(command, check=False, capture_output=True, text=True)
    if completed.returncode != 0:
        stderr = completed.stderr.strip()
        raise SystemExit(stderr or "ImageMagick resize failed.")


def read_ppm(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    index = 0

    def skip_whitespace_and_comments() -> None:
        nonlocal index
        while index < len(data):
            while index < len(data) and data[index] in b" \t\r\n":
                index += 1
            if index < len(data) and data[index : index + 1] == b"#":
                while index < len(data) and data[index : index + 1] not in (b"\n", b""):
                    index += 1
                continue
            break

    def read_token() -> str:
        nonlocal index
        skip_whitespace_and_comments()
        start = index
        while index < len(data) and data[index] not in b" \t\r\n#":
            index += 1
        if start == index:
            raise ValueError("Malformed PPM header.")
        return data[start:index].decode("ascii")

    magic = read_token()
    if magic != "P6":
        raise ValueError(f"Unsupported PPM format: {magic}")
    width = int(read_token())
    height = int(read_token())
    max_value = int(read_token())
    if max_value != 255:
        raise ValueError(f"Unsupported PPM max value: {max_value}")

    while index < len(data) and data[index] in b" \t\r\n":
        index += 1

    pixel_bytes = data[index:]
    expected = width * height * 3
    if len(pixel_bytes) != expected:
        raise ValueError("PPM payload is incomplete.")

    pixels: list[tuple[int, int, int]] = []
    for offset in range(0, expected, 3):
        pixels.append(
            (pixel_bytes[offset], pixel_bytes[offset + 1], pixel_bytes[offset + 2])
        )
    return width, height, pixels


def sample_bilinear(
    width: int,
    height: int,
    pixels: list[tuple[int, int, int]],
    x: float,
    y: float,
) -> tuple[int, int, int]:
    x = max(0.0, min(x, width - 1.0))
    y = max(0.0, min(y, height - 1.0))

    x0 = int(math.floor(x))
    y0 = int(math.floor(y))
    x1 = min(x0 + 1, width - 1)
    y1 = min(y0 + 1, height - 1)

    fx = x - x0
    fy = y - y0

    def at(px: int, py: int) -> tuple[int, int, int]:
        return pixels[py * width + px]

    c00 = at(x0, y0)
    c10 = at(x1, y0)
    c01 = at(x0, y1)
    c11 = at(x1, y1)

    result = []
    for channel in range(3):
        top = c00[channel] * (1.0 - fx) + c10[channel] * fx
        bottom = c01[channel] * (1.0 - fx) + c11[channel] * fx
        result.append(int(round(top * (1.0 - fy) + bottom * fy)))
    return result[0], result[1], result[2]


def build_frames(
    width: int, height: int, pixels: list[tuple[int, int, int]]
) -> list[list[int]]:
    center_x = (width - 1) / 2.0
    center_y = (height - 1) / 2.0
    max_radius = min(width, height) / 2.0

    frames: list[list[int]] = []
    for degree in range(FRAME_COUNT):
        angle = math.radians(degree)
        cos_angle = math.cos(angle)
        sin_angle = math.sin(angle)
        frame: list[int] = []
        for pixel_index in range(PIXELS_PER_FRAME):
            radius = max_radius * (pixel_index + 0.5) / PIXELS_PER_FRAME
            x = center_x + cos_angle * radius
            y = center_y - sin_angle * radius
            red, green, blue = sample_bilinear(width, height, pixels, x, y)
            frame.append((red << 16) | (green << 8) | blue)
        frames.append(frame)
    return frames


def format_header(frames: list[list[int]]) -> str:
    lines = [
        "#include <Arduino.h>",
        "",
        f"const uint32_t image_frames[{FRAME_COUNT}][{PIXELS_PER_FRAME}] PROGMEM = {{",
    ]
    for frame in frames:
        values = ", ".join(f"0x00{pixel:06X}" for pixel in frame)
        lines.append(f"    {{{values}}},")
    lines.extend(
        [
            "};",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert an image into a 360x36 Arduino header for the FOV fan.",
    )
    parser.add_argument("input_image", type=Path, help="Source image path")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path(__file__).with_name("frames.h"),
        help="Output header path (default: frames.h next to this script)",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    source = args.input_image
    if not source.is_file():
        raise SystemExit(f"Input image not found: {source}")

    output = args.output
    output.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as temporary_dir:
        resized_path = Path(temporary_dir) / "resized.ppm"
        resize_with_imagemagick(source, resized_path)
        shutil.copy(str(resized_path), Path(__file__).with_name("resized.ppm"))
        width, height, pixels = read_ppm(resized_path)
        if width != TARGET_SIZE or height != TARGET_SIZE:
            raise SystemExit(f"Unexpected resized dimensions: {width}x{height}")
        frames = build_frames(width, height, pixels)

    output.write_text(format_header(frames), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
