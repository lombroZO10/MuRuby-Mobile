from __future__ import annotations

import io
from pathlib import Path

from PIL import Image, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "ClientBuild_192.168.99.200" / "Data" / "Interface"
OUT_DIR = ROOT / "android" / "app" / "src" / "main" / "assets" / "ui" / "skill_slots"

CELL_W = 20
CELL_H = 28
ATLAS_SIZE = 256
OUT_SIZE = 64
ICON_W = 46
ICON_H = 56

ATLASES = {
    0: "command.ozj",
    1: "newui_skill.OZJ",
    2: "newui_skill2.OZJ",
    3: "newui_skill3.OZJ",
}


def load_ozj_jpeg(path: Path) -> Image.Image:
    data = path.read_bytes()
    starts = [idx for idx in range(len(data) - 1) if data[idx] == 0xFF and data[idx + 1] == 0xD8]
    for start in starts:
        try:
            image = Image.open(io.BytesIO(data[start:]))
            image.load()
            return image.convert("RGBA")
        except Exception:
            continue
    raise RuntimeError(f"Could not decode OZJ/JPEG payload: {path}")


def soft_circle_mask(size: int, radius: float, feather: float) -> Image.Image:
    scale = 4
    mask = Image.new("L", (size * scale, size * scale), 0)
    cx = cy = (size * scale - 1) / 2.0
    r = radius * scale
    f = feather * scale
    px = mask.load()
    for y in range(mask.height):
        for x in range(mask.width):
            d = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            if d <= r - f:
                a = 255
            elif d >= r:
                a = 0
            else:
                a = int(255 * (1.0 - ((d - (r - f)) / f)))
            px[x, y] = a
    return mask.resize((size, size), Image.Resampling.LANCZOS)


def make_slot_icon(cell: Image.Image) -> Image.Image:
    # Keep the original MU icon identity, but make it readable in the small
    # mobile circular slot.
    icon = cell.resize((ICON_W, ICON_H), Image.Resampling.LANCZOS)
    icon = ImageEnhance.Color(icon).enhance(1.18)
    icon = ImageEnhance.Contrast(icon).enhance(1.16)
    icon = ImageEnhance.Sharpness(icon).enhance(1.45)

    canvas = Image.new("RGBA", (OUT_SIZE, OUT_SIZE), (0, 0, 0, 0))

    # Subtle inner dark plate prevents bright skill art from fighting the gold
    # slot frame, while the circular alpha removes the square edges completely.
    plate = Image.new("RGBA", (OUT_SIZE, OUT_SIZE), (8, 6, 5, 170))
    plate.putalpha(soft_circle_mask(OUT_SIZE, 27.5, 2.5))
    canvas.alpha_composite(plate)

    x = (OUT_SIZE - ICON_W) // 2
    y = (OUT_SIZE - ICON_H) // 2
    canvas.alpha_composite(icon, (x, y))

    vignette = Image.new("RGBA", (OUT_SIZE, OUT_SIZE), (0, 0, 0, 0))
    ring = Image.new("L", (OUT_SIZE, OUT_SIZE), 0)
    ring_mask = soft_circle_mask(OUT_SIZE, 29.0, 4.0)
    inner_mask = soft_circle_mask(OUT_SIZE, 21.0, 7.0)
    ring_px = ring.load()
    for yy in range(OUT_SIZE):
        for xx in range(OUT_SIZE):
            ring_px[xx, yy] = max(0, ring_mask.getpixel((xx, yy)) - inner_mask.getpixel((xx, yy)))
    vignette.putalpha(ring.point(lambda a: int(a * 0.40)))
    canvas.alpha_composite(vignette)

    canvas.putalpha(soft_circle_mask(OUT_SIZE, 29.0, 2.0))
    return canvas


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    generated = 0
    for kind, name in ATLASES.items():
        atlas = load_ozj_jpeg(SRC_DIR / name)
        for row in range(ATLAS_SIZE // CELL_H):
            for col in range(12):
                left = col * CELL_W
                top = row * CELL_H
                if left + CELL_W > ATLAS_SIZE or top + CELL_H > ATLAS_SIZE:
                    continue
                cell = atlas.crop((left, top, left + CELL_W, top + CELL_H))
                out = make_slot_icon(cell)
                out.save(OUT_DIR / f"k{kind}_c{col:02d}_r{row:02d}.png", optimize=True)
                generated += 1
    print(f"generated {generated} skill slot assets in {OUT_DIR}")


if __name__ == "__main__":
    main()
