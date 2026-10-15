"""Read installed Legion26972 ritual spells/locks; never modify runtime data."""
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from audit_spell_mechanics import Data

data = Data(Path("build-extractors/bin/Release/dbc/enUS"), Path("__native_only__"))
spell_ids = {219917,219877,219839,219920,219923,219927,220374,225886}
fields = ("SpellID","Effect","EffectIndex","EffectAura","EffectAuraPeriod",
          "EffectTriggerSpell","EffectMiscValue1","EffectMiscValue2","ImplicitTarget1","ImplicitTarget2")
effects = [{k:r[k] for k in fields} for r in data.rows("SpellEffect").values() if r["SpellID"] in spell_ids]
assert any(r["SpellID"]==219917 and r["EffectAura"]==428 and r["EffectTriggerSpell"]==219877 for r in effects)
assert any(r["SpellID"]==219877 and r["Effect"]==28 and r["EffectMiscValue1"]==110480 and r["ImplicitTarget1"]==46 for r in effects)
assert any(r["SpellID"]==219839 and r["Effect"]==249 and r["EffectMiscValue1"]==128941 for r in effects)
assert not any(r["SpellID"]==219839 and r["Effect"] in (24,157,206) for r in effects)
assert any(r["SpellID"]==219923 and r["Effect"]==90 and r["EffectMiscValue1"]==110606 for r in effects)
assert any(r["SpellID"]==220374 and r["Effect"]==252 for r in effects)
assert data.rows("SpellDuration")[5]["Duration"]==300000
assert data.rows("SpellDuration")[1]["Duration"]==10000
assert data.rows("SpellDuration")[105]["Duration"]==9000
result = {
    "build":"7.3.5.26972", "quest":43254,
    "objectives":[110523,110581,110556,110603,110606,110607],
    "effects":effects,
    "durations":{i:data.rows("SpellDuration")[i] for i in (1,5,105)},
    "artifact_lock":data.rows("Lock")[1691],
    "hashes":data.hashes,
    "limits":"Native spell/lock data establishes routing, not actual scene playback or full server execution. SQL objectives/phase/loot links are tested separately."
}
out = Path("docs/audits/ritual-ruination-native-2026-10-15.json")
out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print("PASS: native Fel Bat controller, artifact equip/credit, ten-second ritual, exit teleport and scene duration; exported",out)
