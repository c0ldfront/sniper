"""Compare retained original/recreated full frames at all seven native sizes."""
from pathlib import Path
from PIL import Image, ImageChops
import hashlib
import json

root = Path(__file__).resolve().parents[2]
p = root / 'research/sniper87/parity-editor'
results = []
for width, height in [(320, 200), (320, 240), (640, 480), (800, 600),
                      (1024, 768), (1280, 1024), (1600, 1200)]:
    folder = p / (f'viewport-{width}x{height}' if width != 640 else 'state-fixture-2')
    assert folder.is_dir(), f'missing capture {folder}'
    assert (folder / 'original-ship.bin').read_bytes() == (folder / 'original-ship-before.bin').read_bytes()
    if width != 640:
        assert f'native viewport={width}x{height} ' in (folder / 'state-dump.txt').read_text()
    a = Image.open(folder / 'original-state.png').convert('RGB')
    b = Image.open(folder / 'recreated-state.png').convert('RGB')
    assert a.size == b.size == (width, height)
    diff = ImageChops.difference(a, b)
    count = sum(pixel != (0, 0, 0) for pixel in diff.get_flattened_data())
    item = {'viewport': [width, height], 'pixels_compared': width * height,
            'different_pixels': count, 'difference_bbox': diff.getbbox()}
    for name in ('original-state.png', 'recreated-state.png', 'original-ship.bin'):
        item[name] = hashlib.sha256((folder / name).read_bytes()).hexdigest()
    results.append(item)
(p / 'viewport-parity-result.json').write_text(json.dumps(results, indent=2) + '\n')
assert all(item['different_pixels'] == 0 for item in results), results
print(f'PASS: all seven native viewport dimensions, {sum(item["pixels_compared"] for item in results)} exact RGB pixels')
