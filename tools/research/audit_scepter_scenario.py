"""Read native 7.3.5 scenario 1155; no database or runtime modifications."""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data
from audit_spell_visuals import Table

d = Data(Path('build-extractors/bin/Release/dbc/enUS'), Path('__native_only__'))
steps = sorted((r for r in d.rows('ScenarioStep').values() if r['ScenarioID'] == 1155),
               key=lambda r: r['OrderIndex'])
meta = d.meta.split('struct ScenarioStepMeta')[1].split('\n};')[0]
index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)', meta).groups()
table = Table(d.dbc / 'ScenarioStep.db2', int(index), int(layout, 16))
for row in steps:
    for field in ('Title', 'Description'):
        offset = table.strings + row[field]
        row[field] = table.data[offset:table.data.index(0, offset)].decode('utf-8')
trees, criteria = d.rows('CriteriaTree'), d.rows('Criteria')
ids = {r['Criteriatreeid'] for r in steps}
while True:
    new = ids | {r['ID'] for r in trees.values() if r.get('Parent') in ids}
    if new == ids:
        break
    ids = new
result = {'build': '7.3.5.26972', 'scenario': 1155, 'map': 1630, 'quest': 43153,
          'steps': steps, 'trees': [r for r in trees.values() if r['ID'] in ids]}
result['criteria'] = [criteria[r['CriteriaID']] for r in result['trees']
                      if r.get('CriteriaID') in criteria]
spell_ids = {218595, 218995, 219203, 88283, 219338, 219706, 220374, 204465}
fields = ('ID','SpellID','Effect','EffectIndex','EffectAura','EffectTriggerSpell',
          'EffectMiscValue1','EffectMiscValue2','ImplicitTarget1','ImplicitTarget2')
result['spell_effects'] = [{k: r[k] for k in fields} for r in d.rows('SpellEffect').values()
                           if r['SpellID'] in spell_ids]
result['target_restrictions'] = [r for r in d.rows('SpellTargetRestrictions').values()
                                 if r['SpellID'] == 219203]
result['factions'] = [r for r in d.rows('FactionTemplate').values()
                       if r['ID'] in (14,35,190,1665)]
result['source_sha256'] = d.hashes
out = Path('docs/audits/scepter-native-2026-10-14.json')
out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print('Exported', len(steps), 'steps,', len(result['criteria']), 'criteria,',
      len(result['spell_effects']), 'spell effects:', out)
