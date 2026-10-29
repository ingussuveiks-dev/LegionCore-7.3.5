"""Trace the selected split controller and native pending-cast guards in PE."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p=runpy.run_path(str(Path(__file__).with_name('verify_stromkar_defeat_pose_release.py')))
n,ins=(p[k] for k in ('n','ins'))
def references(method):return {x.address+x.size+op.mem.disp for x in ins(method) for op in x.operands if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP}
def table(address,count=40):return struct.unpack_from('<'+str(count)+'Q',n['binary'],n['offset'](address))
def needs(method,*prefixes):
    calls=n['calls'](method)
    for prefix in prefixes:assert any(n['resolve'](a) in calls for name,a in n['symbols'].items() if name.startswith(prefix)),(method,prefix)
def hook(register,callback):assert n['symbol'](callback) in references(register),(register,callback)
def values(method):return {op.imm for x in ins(method) for op in x.operands if op.type==CS_OP_IMM}
factory='?GetAI@?$GenericCreatureScript@Unpc_stromkar_illusionist@@'
spell_factory='?GetSpellScript@?$GenericSpellScriptLoader@Vspell_stromkar_split_transport@@'
add='?AddSC_artifact_acquisition@@'
for target in (factory,spell_factory):
    assert any(n['symbol'](target) in table(at,12) for at in references(add)),('Registration missing',target)
methods=['?'+name+'@npc_stromkar_illusionist@@' for name in ('Reset','EnterCombat','JustDied','SpellHit','JustSummoned','UpdateAI')]
assert any(all(n['symbol'](m) in table(at,100) for m in methods) for at in references(factory)), 'Actual creature factory does not select all tested hooks'
register='?Register@spell_stromkar_split_transport@@'
validate='?Validate@spell_stromkar_split_transport@@'
assert any(n['symbol'](register) in table(at,12) and n['symbol'](validate) in table(at,12) for at in references(spell_factory))
for method in ('Check','Guard'):hook(register,'?'+method+'@spell_stromkar_split_transport@@')
stage='?Stage@npc_stromkar_illusionist@@'
assert {103339,370996,1539,1037,6}<=values(stage)
eligible='?Eligible@npc_stromkar_illusionist@@'
assert 41105 in values(eligible)
current='?Current@npc_stromkar_illusionist@@'
witness='?Witness@npc_stromkar_illusionist@@'
needs(current,stage,witness)
needs(witness,'?getQuestStatus@Player@@')
update='?UpdateAI@npc_stromkar_illusionist@@'
# Current's small body is inlined here; follow its actual scope/lease/combat
# consumers instead of requiring an out-of-line call the optimizer removed.
needs(update,'?UpdateAI@SmartAI@@',stage,witness,'?isInCombat@Unit@@','?CastSpell@Unit@@')
assert {207297,207249,207258,207275,1500,20000,3}<=values(update)
hit='?SpellHit@npc_stromkar_illusionist@@'
needs(hit,current,'?MovePositionToFirstCollision@WorldObject@@','?SpellHit@SmartAI@@')
assert 207244 in values(hit)
summoned='?JustSummoned@npc_stromkar_illusionist@@'
needs(summoned,'?JustSummoned@SmartAI@@','?DespawnOrUnsummon@Creature@@')
assert {104737,207299}<=values(summoned)
death='?JustDied@npc_stromkar_illusionist@@'
needs(death,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?IsInSameGroupWith@Player@@','?JustDied@SmartAI@@')
clear='?Clear@npc_stromkar_illusionist@@'
needs(clear,'?DespawnOrUnsummon@Creature@@')
pending='?Current@spell_stromkar_split_transport@@'
needs(pending,'?Controller@spell_stromkar_split_transport@@','?Allow@npc_stromkar_illusionist@@')
needs('?Guard@spell_stromkar_split_transport@@',pending,'?PreventHitDefaultEffect@SpellScript@@')
for method in (update,hit,death,clear):
    calls=n['calls'](method)
    for prefix in ('?KilledMonsterCredit@Player@@','?RewardPlayerAndGroupAtEvent@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?NearTeleportTo@Unit@@'):
        assert not any(n['resolve'](a) in calls for name,a in n['symbols'].items() if name.startswith(prefix)),(method,prefix)
print('PASS selected split AI/spell factories, all native delegate hooks,103339/370996/1539/1037/6 and41105 lease, two native missile/teleport/channel retry paths and pending-cast default-effect guard. Native owned-death/group sink retained; no direct item/progress/teleport API. SQL, receipt/epoch/failure host and actual client visual checks are separate.')
