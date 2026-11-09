"""Selected compiled callbacks;behavior is separately tested with ASAN/native."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
nested=runpy.run_path(str(Path(__file__).with_name('verify_seven_lifecycle_release.py')))
n=nested['n'];ins=n['instructions'];symbols=n['symbols'];resolve=n['resolve']
def sites(rows,prefix):
 targets={resolve(v)for k,v in symbols.items()if k.startswith(prefix)}
 assert targets,prefix
 return [i for i,x in enumerate(rows)if x.mnemonic in ('call','jmp')and x.operands[0].type==CS_OP_IMM and resolve(x.operands[0].imm)in targets]
def values(rows):return{o.imm for x in rows for o in x.operands if o.type==CS_OP_IMM}
def ref(rows,address):return any(x.mnemonic=='lea'and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP and resolve(x.address+x.size+o.mem.disp)==address for x in rows for o in x.operands)
owners=[
 ('spell_underlight_blessing@@',['OnUpdate']),
 ('spell_underlight_main_aura@@',['HandleEffectApply','HandleEffectRemove']),
 ('spell_monk_remove_zen_flight_AuraScript@spell_monk_remove_zen_flight@@',['OnUpdate']),
 ('spell_request_the_master_call_on_you@@',['OnApply','OnTick']),
 ('spell_gen_winning_hand@@',['OnApply']),
 ('spell_gen_gobelin_gumbo_AuraScript@spell_gen_gobelin_gumbo@@',['OnUpdate']),
 ('spell_item_camera_selfie_AuraScript@spell_item_camera_selfie@@',['OnRemove'])]
for cls,methods in owners:
 register=ins('?Register@'+cls)
 for method in methods:
  prefix='?'+method+'@'+cls;body=ins(prefix)
  assert sites(body,'?GetUnitOwner@AuraScript@@'),(cls,method,'owner')
  assert not any(k.startswith('?GetCaster@AuraScript@@')and resolve(v)in n['calls'](prefix)for k,v in symbols.items()),(cls,method,'old caster')
  assert ref(register,n['symbol'](prefix)),(cls,method,'native hook registration')
body=ins('?CalculateMaxDuration@spell_stonehide_leather_barding@@')
assert sites(body,'?GetCaster@AuraScript@@') and sites(body,'?HasSkill@Player@@')
skill,=sites(body,'?HasSkill@Player@@')
caster,=sites(body,'?GetCaster@AuraScript@@')
kind=next(i for i in range(caster+1,skill)if body[i].mnemonic=='cmp'and body[i].op_str=='byte ptr [rax + 0x16], 4')
assert body[kind+1].mnemonic=='jne'and body[kind+1].operands[0].imm>body[skill].address,'Native inlined ToPlayer must skip HasSkill for a nonplayer caster'
assert {7200000,165}<=values(body)
body=ins('?OnUpdate@spell_pal_t21_prot@@')
cast,=sites(body,'?CastCustomSpell@Unit@@')
assert any(x.mnemonic=='mov'and x.op_str.endswith(', 0x3e8')and 'dword ptr ['in x.op_str for x in body[:cast]),'Native1s rearm absent before T21cast'
assert {251870,132403,253331}<=values(body)
assert sites(body,'?HasAura@Unit@@')and sites(body,'?GetAuraEffect@Unit@@')
assert ref(ins('?Register@spell_pal_t21_prot@@'),n['symbol']('?OnUpdate@spell_pal_t21_prot@@'))
names=[k for k in symbols if k.startswith('?_Do_call@')and 'spell_undercurrent_fishing_tele@@'in k]
name,=names;body=ins(name)
for dependency in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?NearTeleportTo@Unit@@'):assert sites(body,dependency),dependency
teleport,=sites(body,'?NearTeleportTo@Unit@@')
for offset in (0xa18,0xe0,0x3b8,0x6848,0x6849,0x6851):
 assert any(x.mnemonic=='cmp'and any(o.type==CS_OP_MEM and o.mem.disp==offset for o in x.operands)for x in body[:teleport]),('Undercurrent lifecycle guard',hex(offset))
schedule=ins('?HandleAfterCast@spell_undercurrent_fishing_tele@@')
assert 500 in values(schedule)and sites(schedule,'?AddDelayedEvent@FunctionProcessor@@')
assert ref(ins('?Register@spell_undercurrent_fishing_tele@@'),n['symbol']('?HandleAfterCast@spell_undercurrent_fishing_tele@@'))
assert sites(ins('?OnRemove@spell_item_camera_selfie_AuraScript@spell_item_camera_selfie@@'),'?CancelSpellVisualKit@Unit@@')
print('PASS selected actual Release ten-feature callbacks:owner getters/native hook registrations,guarded Stonehide profession,periodic1000ms T21 before253331 with actual4P/Shield gates,500ms Undercurrent origin/life/pending-teleport refusal,selfie visual cleanup;retained seven linked paths. Native/source behavior and actual client checks remain distinct.')
