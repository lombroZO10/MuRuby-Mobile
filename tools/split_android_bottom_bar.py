from pathlib import Path

from PIL import Image, ImageEnhance


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (
    ROOT
    / "android"
    / "app"
    / "src"
    / "main"
    / "assets"
    / "ui"
    / "bottombar"
    / "bottom-bar-fx-trim.png"
)
OUTPUT_DIR = SOURCE.parent

BUTTON_NAMES = (
    "menu",
    "shop",
    "rank",
    "bag",
    "char",
    "bank",
    "event",
    "move",
    "chat",
    "cmd",
    "opt",
)

CANVAS_WIDTH = 189
CANVAS_HEIGHT = 217


def alpha_column_runs(image: Image.Image) -> list[tuple[int, int]]:
    alpha = image.getchannel("A")
    occupied = [
        alpha.crop((x, 0, x + 1, image.height)).getbbox() is not None
        for x in range(image.width)
    ]

    runs: list[tuple[int, int]] = []
    start: int | None = None
    for x, is_occupied in enumerate((*occupied, False)):
        if is_occupied and start is None:
            start = x
        elif not is_occupied and start is not None:
            runs.append((start, x))
            start = None
    return runs


def sanitize_transparent_pixels(image: Image.Image) -> Image.Image:
    pixels = []
    for red, green, blue, alpha in image.getdata():
        if alpha == 0:
            pixels.append((0, 0, 0, 0))
        else:
            pixels.append((red, green, blue, alpha))
    clean = Image.new("RGBA", image.size)
    clean.putdata(pixels)
    return clean


def make_active(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    saturated_rgb = ImageEnhance.Color(image.convert("RGB")).enhance(1.08)
    bright_rgb = ImageEnhance.Brightness(saturated_rgb).enhance(1.14)
    bright = bright_rgb.convert("RGBA")
    bright.putalpha(alpha)
    return sanitize_transparent_pixels(bright)


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    runs = alpha_column_runs(source)
    if len(runs) != len(BUTTON_NAMES):
        raise RuntimeError(
            f"Expected {len(BUTTON_NAMES)} buttons, found {len(runs)} alpha runs: {runs}"
        )

    for name, (left, right) in zip(BUTTON_NAMES, runs):
        crop = source.crop((left, 0, right, source.height))
        canvas = Image.new("RGBA", (CANVAS_WIDTH, CANVAS_HEIGHT), (0, 0, 0, 0))
        x = (CANVAS_WIDTH - crop.width) // 2
        y = (CANVAS_HEIGHT - crop.height) // 2
        canvas.alpha_composite(crop, (x, y))
        canvas = sanitize_transparent_pixels(canvas)

        normal_path = OUTPUT_DIR / f"mu_button_base_{name}.png"
        active_path = OUTPUT_DIR / f"mu_button_base_{name}_active.png"
        canvas.save(normal_path, optimize=True)
        make_active(canvas).save(active_path, optimize=True)
        print(f"{name}: source=({left},{right}) output={canvas.size}")


if __name__ == "__main__":
    main()
