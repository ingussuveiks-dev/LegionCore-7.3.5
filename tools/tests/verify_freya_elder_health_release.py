"""Check actual unsigned64 fifth/remainder and remaining-room operations."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_native_loot_damage_release.py')))
ins,sites,values=p['ins'],p['sites'],p['values']
from verify_freya_health_math import verify_freya_health_math
verify_freya_health_math(ins,sites,values)
print('PASS linked Freya: actual native64 health,zero-elder bypass,unsigned fifth decomposition,remaining-room division gate before multiplication,exact remainder fifth,clamped bonus and native64 SetMaxHealth. Complete counter/Tsulong/Rook/prior367 chain retained;67actual full-AI native inputs and client raid are distinct scopes.')
