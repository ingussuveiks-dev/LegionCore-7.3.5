"""Verify that effect249 reaches inventory validation and swap in the deployed PE."""
from pathlib import Path
import runpy

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
handler = '?EffectForceEquipItem@Spell@@'
targets = native['calls'](handler)
for prefix in ('?GetItemByEntry@Player@@', '?CanEquipItem@Player@@',
               '?CanEquipChildItem@Player@@', '?SendEquipError@Player@@', '?SwapItem@Player@@'):
    assert native['symbol'](prefix) in targets, f'Deployed force-equip handler lacks {prefix}'
assert native['symbol']('?EquipItem@Player@@') not in targets, 'Unsafe direct equip bypasses inventory removal/swap'
print('PASS: deployed effect249 validates the existing item and paired item, reports errors and calls native SwapItem')
