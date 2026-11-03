"""Registered Hati aura: primary-pet reaction zero reaches native setter.

The complete extracted update/dispatcher/getter/setter and PetAI consumer
matrix runs separately; linked code alone does not verify actual client use.
"""
from pathlib import Path
import hashlib,json,re,runpy,struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,floats
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins=n['instructions'];pairs=lambda body:[(o.mnemonic,o.op_str) for o in body]
def refs(body):return {o.address+o.size+a.mem.disp for o in body for a in o.operands if a.type==CS_OP_MEM and a.mem.base==X86_REG_RIP}
def table(name,count):
 text=(n['args'].release/'worldserver.map').read_text(errors='replace')
 matches=re.findall(r'\s'+re.escape(name)+r'\s+([0-9A-Fa-f]{16})\s',text)
 assert len(matches)==1,(name,len(matches))
 address=int(matches[0],16);return address,struct.unpack_from('<'+'Q'*count,n['binary'],n['offset'](address))
factory=ins('?GetAuraScript@spell_hun_hatis_bond@@')
register=ins('?Register@spell_hun_hatis_bond_AuraScript@spell_hun_hatis_bond@@')
update=ins('?OnUpdate@spell_hun_hatis_bond_AuraScript@spell_hun_hatis_bond@@')
vtable,raw=table('??_7spell_hun_hatis_bond_AuraScript@spell_hun_hatis_bond@@6B@',16)
assert vtable in refs(factory)
assert n['symbol']('?Register@spell_hun_hatis_bond_AuraScript@spell_hun_hatis_bond@@') in [n['resolve'](v) for v in raw if v>=n['base']]
assert n['symbol']('?OnUpdate@spell_hun_hatis_bond_AuraScript@spell_hun_hatis_bond@@') in refs(register)
loader,slots=table('??_7spell_hun_hatis_bond@@6B@',3)
assert loader in refs(ins('?AddSC_hunter_spell_scripts@@'))
assert n['symbol']('?GetAuraScript@spell_hun_hatis_bond@@')==n['resolve'](slots[2])
get,=sites(n,update,'?GetReactState@Creature@@');set_state,=sites(n,update,'?SetReactState@Creature@@')
assert pairs(update[get+1:set_state])==[('movzx','edx, al'),('xor','r8d, r8d'),('mov','rcx, rdi')]
primary=[i for i in sites(n,update,'?GetPet@Player@@') if i<get][-1]
assert pairs(update[primary+1:get])==[('test','rax, rax'),('je',update[primary+2].op_str),('mov','rcx, rax')]
assert primary+4==get and update[primary+2].operands[0].imm>update[set_state].address
lookup,=sites(n,update,'?GetCreatureOrPetOrVehicle@ObjectAccessor@@')
assert pairs(update[lookup+1:lookup+3])==[('mov','rdi, rax'),('test','rax, rax')]
assert {('cmp','dword ptr [rdi + 0xa18], r13d'),('cmp','byte ptr [rsi + 0x16], 4')}<=set(pairs(update[lookup:get]))
assert {2000,100324,0x1850,1000}<=values(update) and 100.0 in floats(n,update)
assert {('add','dword ptr [rcx + 0x360], edx'),('mov','dword ptr [rcx + 0x360], r13d')}<=set(pairs(update[:lookup]))
assert len(sites(n,update,'?CastSpell@Unit@@'))==1 and sites(n,update,'?DespawnOrUnsummon@Creature@@')
for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?SetMinion@Unit@@','?SetPet@Player@@','?MoveFollow@MotionMaster@@','?CombatStop@Unit@@'):
 assert not sites(n,update,forbidden),forbidden
print('PASS actual registered Hati Bond aura/update: living controlled five-entry Hati receives native primary Pet GetReactState byte unconditionally, including zero, through native delay0 SetReactState; native2s/summon/custom-data/despawn/range retained. No forced attack/movement/primary-pet or quest mutation. Client/full Titanstrike separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'spell':197344,'passive':0,'interval_ms':2000}))
