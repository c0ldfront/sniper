"""Original resource payloads for tests, read from the user-supplied SNIPER.EXE.

`make assets` (tools/extract_assets.py) places the user's own SNIPER.EXE under
build/assets/files; no resource bytes are stored in this repository.
"""
import sys
from pathlib import Path

CODE = Path(__file__).resolve().parents[1]
EXE = CODE / "build/assets/files/SNIPER.EXE"
sys.path.insert(0, str(CODE / "tools"))
import extract_assets  # noqa: E402

_cache = None


def payloads():
    """{"TYPE-ID-LANG": bytes}, e.g. "WAVE-121-1033" or "4-103-1033"."""
    global _cache
    if _cache is None:
        if not EXE.exists():
            sys.exit(f"{EXE} is missing: run 'make assets SNIPER_ZIP=/path/to/sniper87.zip' first")
        _cache = {f"{t}-{i}-{l}": data for t, i, l, _, data in extract_assets.resources(EXE.read_bytes())}
    return _cache
