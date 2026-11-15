"""Exact independent actor-class changes; original callbacks remain byte-pinned."""
from pathlib import Path
import json,re,subprocess
from lifetime_source_amendments import lifetime_baseline_view
from lifetime_queue_ownership import queue_owner
def verify(plan_path,base):
 r=Path(__file__).resolve().parents[2];plan=json.loads(Path(plan_path).read_text(encoding='utf8'));assert len(plan)==10
 for e in plan:
  for ed in e['edits']:
   restored=ed['new'].replace(ed['anchor_addition'],'',1).replace(', '+e['guard_init'],'')
   restored=re.sub(r'(?:\n[ \t]*| )'+re.escape(e['guard']),'',restored)
   assert restored==ed['old'],(e['key'],'payload/gates/delays changed')
   assert ed['new'].count(e['member'])==1
  for cb in e['callbacks']:
   capture='['+cb['capture']+', '+e['guard_init']+']';at=cb['new'].index('{',cb['new'].index(capture)+len(capture))+1
   assert cb['new'][at:].lstrip().startswith(e['guard']),(e['key'],'first statement')
 texts={p:lifetime_baseline_view(p,(r/p).read_text(encoding='utf-8-sig'))for e in plan for p in e['paths']}
 for later in reversed(json.loads((r/'tools/tests/FortySixthTenRepairs.json').read_text(encoding='utf8'))):
  for ed in reversed(later['edits']):
   p=ed['path']
   if p not in texts:continue
   assert texts[p].count(ed['new'])==1,(later['key'],'later death-owner payload differs')
   texts[p]=texts[p].replace(ed['new'],ed['old'],1)
 # Later lifetime groups can touch another class in the very same file.
 # Invert only their complete declared class edits, in chronological order;
 # the final whole-file comparison still rejects any undeclared change.
 chronology=('ThirtySeventhTenRepairs.json','ThirtyEighthTenRepairs.json','ThirtyNinthTenRepairs.json','FortiethTenRepairs.json','FortyFirstTenRepairs.json','FortySecondTenRepairs.json','FortyThirdTenRepairs.json','FortyFourthTenRepairs.json','FortyFifthTenRepairs.json')
 for name in reversed(chronology[chronology.index(Path(plan_path).name)+1:]):
  for later in reversed(json.loads((r/'tools/tests'/name).read_text(encoding='utf8'))):
   if later.get('withdrawn'):continue
   for ed in reversed(later['edits']):
    p=ed['path']
    if p not in texts:continue
    assert texts[p].count(ed['new'])==1,(later['key'],'later complete class differs')
    texts[p]=texts[p].replace(ed['new'],ed['old'],1)
 # ad8f15d59 widened both native healing hooks after these pinned plans.
 heal_path='src/server/scripts/Draenor/Highmaul/boss_brackenspore.cpp'
 if heal_path in texts:
  old='void HealReceived(Unit* /*healer*/, uint32& heal) override';new=old.replace('uint32&','uint64&')
  assert texts[heal_path].count(new)==2 and texts[heal_path].count(old)==0,'Two native64 healing signatures changed'
  texts[heal_path]=texts[heal_path].replace(new,old)
 for e in reversed(plan):
  if e.get('withdrawn'):
   for ed in e['edits']:
    assert texts[ed['path']].count(ed['old'])==1,(e['key'],'withdrawn original class changed')
   for cb in e['callbacks']:assert queue_owner(e,texts[e['paths'][0]],cb)=='AI',(e['key'],'withdrawn queue is not AI-owned')
   continue
  for ed in reversed(e['edits']):
   p=ed['path'];assert texts[p].count(ed['new'])==1,(e['key'],p);texts[p]=texts[p].replace(ed['new'],ed['old'],1)
 for p,s in texts.items():
  pinned=subprocess.check_output(['git','-C',str(r),'show',base+':'+p]).decode('utf-8-sig').replace('\r\n','\n')
  # A later group's baseline can predate the withdrawal of an earlier invalid
  # CosmeticAI guard. Reconstruct that pinned historical state only when the
  # complete old/new class bytes prove it; never alter the production source.
  for name in chronology:
   for prior in json.loads((r/'tools/tests'/name).read_text(encoding='utf8')):
    if not prior.get('withdrawn'):continue
    for ed in prior['edits']:
     if ed['path']!=p or ed['new'] not in pinned:continue
     assert s.count(ed['old'])==1,(prior['key'],'withdrawn current class differs')
     assert all(queue_owner(prior,s,cb)=='AI' for cb in prior['callbacks'])
     s=s.replace(ed['old'],ed['new'],1)
  assert s==pinned,p
 active=sum(not e.get('withdrawn') for e in plan);print(f'PASS exact lifetime scope:{active} active independent actor classes/{len(plan)-active} withdrawn AI-owned queues verified at original source;complete file reversal to '+base+';original payloads,prerequisites,delays,credit,reset and cleanup retained. No SQL/core/packet/character change.')
