"""Exact pixels: original runtime versus recreation fed a read-only snapshot.
The target is never patched. Fixture controls stars off, viewport and scene time;
only stationary scenes with unchanged before/after ship/config data are accepted.
"""
from pathlib import Path
from PIL import Image, ImageChops
import hashlib
import json

root = Path(__file__).resolve().parents[2]
p = root / 'research/sniper87/parity-editor'
results = []
for folder in sorted(p.glob('state-fixture-*')):
    before = (folder / 'original-ship-before.bin').read_bytes()
    after = (folder / 'original-ship.bin').read_bytes()
    assert before == after, folder
    if len(after) >= 832:
        count = int.from_bytes(after[828:832], 'little')
        assert count <= 16 and len(after) == 832 + count * 20, folder
    else:
        assert len(after) in (824, 828), folder
    a = Image.open(folder / 'original-state.png').convert('RGB')
    b = Image.open(folder / 'recreated-state.png').convert('RGB')
    assert a.size == b.size == (640, 480)
    diff = ImageChops.difference(a, b)
    count = sum(pixel != (0, 0, 0) for pixel in diff.get_flattened_data())
    # These rectangles exclude all fixed HUD, central ship, radar and chat.
    boxes = [(114, 35, 640, 220), (30, 260, 534, 470)]
    world_pixels = sum(pixel != (0, 0, 0) for box in boxes
                       for pixel in a.crop(box).get_flattened_data())
    item = {'fixture': folder.name, 'pixels_compared': 307200,
            'different_pixels': count, 'world_nonblack_pixels': world_pixels,
            'difference_bbox': diff.getbbox(), 'snapshot_unchanged': True}
    for name in ('original-state.png', 'recreated-state.png', 'original-ship.bin'):
        item[name] = hashlib.sha256((folder / name).read_bytes()).hexdigest()
    results.append(item)
    assert count == 0, item
assert results and any(item['world_nonblack_pixels'] > 0 for item in results), results
(p / 'game-state-parity-result.json').write_text(json.dumps(results, indent=2) + '\n')
print(f'PASS: {len(results)} complete original/recreated gameplay frames match exactly')
