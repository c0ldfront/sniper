"""Pixel comparison of separately captured, unmodified original and recreation.
Run parity-editor/run-parity.sh first with refreshed recreated/RECREATED.EXE.
This validates the deterministic editor fixture, not arbitrary gameplay state.
"""
from pathlib import Path
from PIL import Image,ImageChops
import hashlib,json
root=Path(__file__).resolve().parents[2]
p=root/'research/sniper87/parity-editor'
a=Image.open(p/'original-editor.png').convert('RGB');b=Image.open(p/'recreated-editor.png').convert('RGB')
assert a.size==b.size==(640,480)
diff=ImageChops.difference(a,b)
count=sum(1 for pixel in diff.get_flattened_data() if pixel!=(0,0,0))
result={'scope':'640x480 editor,cursor0/0,tile1,stars disabled,same local.lvl,no editing',
 'different_pixels':count,'pixels_compared':640*480,'difference_bbox':diff.getbbox()}
for name in ['original/SNIPER.EXE','original/LOCAL.LVL','recreated/RECREATED.EXE','recreated/LOCAL.LVL','original-editor.png','recreated-editor.png']:
 result[name]=hashlib.sha256((p/name).read_bytes()).hexdigest()
assert result['original/SNIPER.EXE']=='8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0'
assert result['original/LOCAL.LVL']==result['recreated/LOCAL.LVL']
(p/'editor-parity-result.json').write_text(json.dumps(result,indent=2)+'\n')
diff.save(p/'editor-difference.png')
assert count==0,result
print('PASS307200 exact RGB pixels: unmodified original versus recreated editor fixture')
