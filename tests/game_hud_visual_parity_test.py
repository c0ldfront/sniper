"""Compare fixed HUD regions in retained original/recreated gameplay captures.
World and radar state are deliberately outside this fixture's asserted scope.
"""
from pathlib import Path
from PIL import Image, ImageChops
import hashlib
import json

root = Path(__file__).resolve().parents[2]
p = root / 'research/sniper87/parity-editor'
a = Image.open(p / 'original-game.png').convert('RGB')
b = Image.open(p / 'recreated-game.png').convert('RGB')
assert a.size == b.size == (640, 480)
regions = {
    'statistics': (0, 8, 114, 34),
    'equipment': (0, 240, 30, 248),
    'team_bounty_energy': (598, 0, 640, 24),
    'energy_bar_capacity': (212, 0, 428, 6),
}
result = {
    'scope': 'Fixed initial gameplay HUD only; spawn, world, radar and chat background excluded',
    'capture_build_sha256': '3bc2d42417ed6355e7990c26490a34799a7def21469a06711430c3d5825e940d',
    'regions': {},
}
for name, box in regions.items():
    diff = ImageChops.difference(a.crop(box), b.crop(box))
    count = sum(pixel != (0, 0, 0) for pixel in diff.get_flattened_data())
    result['regions'][name] = {'box': box, 'different_pixels': count,
                             'pixels_compared': diff.width * diff.height}
for name in ('original-game.png', 'recreated-game.png'):
    result[name] = hashlib.sha256((p / name).read_bytes()).hexdigest()
(p / 'game-hud-parity-result.json').write_text(json.dumps(result, indent=2) + '\n')
assert all(item['different_pixels'] == 0 for item in result['regions'].values()), result
print('PASS: four fixed gameplay HUD regions match exactly (5508 RGB pixels)')
