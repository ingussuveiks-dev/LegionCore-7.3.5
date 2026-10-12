"""Read-only native 7.3.5 support spell contract (no hotfix overlays)."""
import hashlib
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/worm-support-no-overlays')
assert not d.sql.exists()
ids = {195144,195145,195146,195148,195149,195150,194019,195155,12548,192412,195025,195024}
rows = [r for r in d.rows('SpellEffect').values() if r['SpellID'] in ids]
fx = {(r['SpellID'],r['EffectIndex']):r for r in rows}
for call,bar,override,event in ((195144,195145,634,47244),(195146,195148,635,47245),(195149,195150,636,47246)):
    assert (fx[call,0]['Effect'],fx[call,0]['ImplicitTarget1'],fx[call,0]['EffectMiscValue1']) == (61,18,event)
    assert (fx[bar,0]['EffectAura'],fx[bar,0]['EffectMiscValue1']) == (293,override)
    assert d.rows('OverrideSpellData')[override]['Spells1'] == call
    cooldown = [r for r in d.rows('SpellCooldowns').values() if r['SpellID'] == call]
    assert len(cooldown) == 1 and cooldown[0]['RecoveryTime'] == 60000
assert fx[194019,0]['Effect'] == 31  # Native Bash weapon damage.
assert fx[195155,1]['EffectAura'] == 87 and fx[195155,1]['EffectBasePoints'] == 50
assert fx[12548,1]['Effect'] == 2
assert fx[192412,0]['Effect'] == 28 and fx[192412,0]['EffectMiscValue1'] == 97508
assert fx[195025,0]['EffectAura'] == 23 and fx[195025,0]['EffectTriggerSpell'] == 195024
assert fx[195024,0]['Effect'] == 10
out = {'client':'7.3.5.26972','effects':rows,'cooldowns':[r for r in d.rows('SpellCooldowns').values() if r['SpellID'] in (195144,195146,195149)],'hashes':d.hashes}
path = ROOT / 'docs/audits/highmountain-worm-support-native-2026-10-12.json'
if '--write-evidence' in sys.argv:
    path.write_text(json.dumps(out,indent=2)+'\n')
else:
    assert json.loads(path.read_text()) == out
print('PASS: native three event calls, matching override bars, 60s cooldowns, Bash/Hook Shot/Frost Shock and complete Healing Stream summon-to-heal chain.')
