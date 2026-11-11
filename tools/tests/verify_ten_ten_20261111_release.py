from pathlib import Path
import re
from ten_ten_20261111_scope import PLAN,ROOT,require_fixed
from five_ten_20261111_pe import n,ins,values,sites
require_fixed()
language=int(re.search(r'LANG_PLAYER_NOT_FOUND\s*=\s*(\d+)',(ROOT/'src/server/game/Miscellaneous/Language.h').read_text())[1])
for e in PLAN['changes']:
 if e['kind']!='console':continue
 rows=ins('?'+e['method']+'@'+e['cls']+'@');assert language in values(rows),(e['key'],'explicit localized missing-player failure')
 assert sum(x.mnemonic=='test'and re.fullmatch(r'(r\w+), \1',x.op_str)is not None for x in rows[:45])>=2,(e['key'],'session and actual player gates')
 assert any(x.mnemonic=='mov'and x.op_str.startswith('byte ptr [')and x.op_str.endswith(', 1')for x in rows),(e['key'],'single custom error reporting remains explicit')
 assert sites(rows,'?SendSysMessage@ChatHandler@@'),(e['key'],'visible diagnostic retained')
rows=ins('?HandleScript@spell_sinestra_indomitable_AuraScript@');assert sum(x.mnemonic=='cmp'and x.op_str=='al, 5'for x in rows)==2 and any(x.mnemonic=='inc'and x.op_str=='al'for x in rows)and any(x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x360], al'for x in rows),'Actual per-instance bounded control counter'
factory=ins('?GetAuraScript@spell_sinestra_indomitable@@');assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rbx + 0x360], 0'for x in factory),'Each actual new AuraScript initializes its own counter'
raw=(n['args'].release/'worldserver.map').read_text(errors='replace');assert not any('controlTimer@'in line and'spell_sinestra_indomitable_AuraScript'in line for line in raw.splitlines()),'Shared function-static control counter remains in linked data'
print('PASS linked first10repairs:actual9command context gates/visible localized refusal and per-AuraScript bounded Indomitable counter;ASAN/native/full gameplay scopes separate')
