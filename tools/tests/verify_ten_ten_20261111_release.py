from pathlib import Path
import re
from ten_ten_20261111_scope import PLAN,ROOT,require_fixed
from five_ten_20261111_pe import n,ins,values,sites
require_fixed()
language=int(re.search(r'LANG_PLAYER_NOT_FOUND\s*=\s*(\d+)',(ROOT/'src/server/game/Miscellaneous/Language.h').read_text())[1])
for e in PLAN['changes']:
 methods=[e['method']]if e['kind']=='console'else e.get('methods',[])
 for method in methods:
  rows=ins('?'+method+'@'+e.get('cls','misc_commandscript')+'@');assert language in values(rows),(e['key'],'explicit localized missing-player failure')
  assert sum(x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None for x in rows[:45])>=2,(e['key'],'session and actual player gates')
  assert any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [')and x.op_str.endswith(', 1')for x in rows),(e['key'],'single custom error reporting remains explicit')
  assert sites(rows,'?SendSysMessage@ChatHandler@@'),(e['key'],'visible diagnostic retained')
for e in PLAN['changes']:
 if e['batch']!=2 or e['case']=='char_display':continue
 method={'scaling':'CalculateAmount','pursuit':'HandleExtraEffect','xt002':'OnRemove','augur':'OnTick'if 'FelEjection'in e['key']else'OnRemove','molten':'OnUpdate','absorb':'Absorb'}[e['case']]
 rows=ins('?'+method+'@'+e['cls']+'@')
 if e['case']=='absorb':
  assert sites(rows,'?GetTarget@AuraScript@@')and not sites(rows,'?GetCaster@AuraScript@@'),'Linked protection uses the actual aura recipient'
  assert {220110,220113,220073,220111,220103,250}<=values(rows),'Original protection branches/children/duration retained'
  assert len(sites(rows,'?CastSpell@Unit@@'))==2,'Both original protection casts remain'
  continue
 getter=sites(rows,'?GetCaster@AuraScript@@');assert len(getter)==(2 if e['case']=='molten'else 1),(e['key'],'actual independent caster lookups')
 for at in getter:
  next_rows=rows[at+1:at+6];test=next((i for i,x in enumerate(next_rows)if x.mnemonic=='test'and x.op_str=='rax, rax'),None)
  assert test is not None and next_rows[test+1].mnemonic=='je',(e['key'],'nullable caster exit dominates its dependent dereference')
 if e['case']=='pursuit':assert sum(x.mnemonic=='test'and x.op_str=='rax, rax'for x in rows)>=2,'Current typed Ick AI must also pass its own nullable gate'
rows=ins('?HandleScript@spell_sinestra_indomitable_AuraScript@');assert sum(x.mnemonic=='cmp'and x.op_str=='al, 5'for x in rows)==2 and any(x.mnemonic=='inc'and x.op_str=='al'for x in rows)and any(x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x360], al'for x in rows),'Actual per-instance bounded control counter'
factory=ins('?GetAuraScript@spell_sinestra_indomitable@@');assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rbx + 0x360], 0'for x in factory),'Each actual new AuraScript initializes its own counter'
raw=(n['args'].release/'worldserver.map').read_text(errors='replace');assert not any('controlTimer@'in line and'spell_sinestra_indomitable_AuraScript'in line for line in raw.splitlines()),'Shared function-static control counter remains in linked data'
print('PASS linked implemented repairs:actual command context gates/visible localized refusal,per-AuraScript bounded Indomitable counter,independent foreign-caster gates and actual protection recipient/children/duration;ASAN/native/full gameplay scopes separate')
