"""Selected actual removal hook and original suspended/current dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_combat_ai_pe import guard
from ashbringer_resume_scope import verify
verify()
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_scene_expiry_release.py')))
n,ins=p['n'],p['ins'];sites=p['sites']
scope='spell_bs_dominate_mind_AuraScript@spell_bs_dominate_mind@@'
remove=ins('?OnRemove@'+scope)
assert sites(remove,'?GetTarget@AuraScript@@') and sites(remove,'?FindNearestCreature@WorldObject@@')
assert {90981} <= {o.imm for x in remove for o in x.operands if o.type==CS_OP_IMM}
select,=[i for i,x in enumerate(remove)if x.op_str=='rcx, qword ptr [rax + 0x9c8]']
missing=guard(remove,select,'rax')
assert remove[select+6].op_str=='rax, qword ptr [rcx]'and remove[select+7].op_str=='edx, 1'
assert remove[select+8].mnemonic=='call'and remove[select+8].op_str=='qword ptr [rax + 0x48]'
assert missing==remove[select+10].address,'Missing AI must take the diagnostic path,not virtual dispatch'
tables=[]
for x in ins('?GetAI@npc_bs_balnazzar@@'):
 for o in x.operands:
  if x.mnemonic=='lea'and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
   at=n['offset'](x.address+x.size+o.mem.disp)
   if struct.unpack_from('<Q',n['binary'],at+0x48)[0]==n['symbol']('?DoAction@npc_bs_balnazzarAI@npc_bs_balnazzar@@'):tables.append(at)
assert len(tables)==1,'Actual Balnazzar factory must select the same native DoAction48 slot'
assert b'Ashbringer domination: Balnazzar %u has no current or suspended AI; combat resumption not dispatched' in n['binary'],'Missing AI must remain diagnosed'
for forbidden in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?SetHealth@Unit@@'):
 assert not sites(remove,forbidden)
print('PASS linked registered domination remove retains target/90981 native search and original action,selects suspended content/current fallback and diagnoses no-AI without progression/health/items. Native callback and ASAN cases separate;full encounter remains pending.')
