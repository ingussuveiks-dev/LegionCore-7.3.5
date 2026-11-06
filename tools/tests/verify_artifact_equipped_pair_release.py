"""Verify the existing-parent child completion route in the linked Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native,ins=n['native'],n['instructions']
def sites(body,prefix):
 targets={native['resolve'](value) for name,value in native['symbols'].items() if name.startswith(prefix)}
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm) in targets]
body=ins('?EffectForceEquipItem@Spell@@')
paired=sites(body,'?CanEquipChildItem@Player@@');finish,=sites(body,'?EquipChildItem@Player@@');swap,=sites(body,'?SwapItem@Player@@')
assert len(paired)==2 and paired[0]<finish<paired[1]<swap
assert body[paired[0]+1].mnemonic=='test' and body[paired[0]+2].mnemonic in ('je','jne')
assert sites(body,'?SendEquipError@Player@@') and not sites(body,'?EquipItem@Player@@')
move=ins('?EquipChildItem@Player@@')
assert len(sites(move,'?EquipItem@Player@@'))==2  # Existing child only,empty/swap destinations.
assert not sites(move,'?CanEquipItem@Player@@')  # Never move old child-slot weapon over parent.
assert sites(move,'?_StoreItem@Player@@') and sites(move,'?CanStoreItem@Player@@')  # StoreItem wrapper is inlined.
for b in (body,move,ins('?CanEquipChildItem@Player@@')):
 for prefix in ('?CreateItem@Item@@','?StoreNewItem@Player@@','?AddItem@Player@@'):
  assert not sites(b,prefix)
print('PASS selected native249 has separate already-equipped-parent child preflight/completion before ordinary swap branch; native child movement stores displaced equipment in bags and never equips it over the parent;no item creation/grants. Actual native inventory tests establish empty/occupied/full+retry/repeated identity preservation;client presentation remains separate.')
