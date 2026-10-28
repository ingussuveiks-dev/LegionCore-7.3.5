"""Verify the selected awakening AI, recoil hooks and native arrival consumers."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_jump_release.py')))
n,ins,values=(p[k] for k in ('n','instructions','values'))
ai='npc_stromkar_awakening@@'
methods=['?'+name+'@'+ai for name in ('UpdateAI','SpellHit','SpellHitTarget','Reset')]
def calls_prefix(method,prefix):
    targets={n['resolve'](v) for k,v in n['symbols'].items() if k.startswith(prefix)}
    assert targets & n['calls'](method),(method,prefix)
for method,targets in (
    (methods[0],('?UpdateAI@SmartAI@@','?SummonCreature@WorldObject@@','?UpdateAchievementCriteria@Player@@','?CastSpell@Unit@@','?Abort@'+ai)),
    (methods[1],('?SpellHit@SmartAI@@','?GetGuidValue@Object@@','??8ObjectGuid@@')),
    (methods[2],('?SpellHitTarget@SmartAI@@',)),
    (methods[3],('?Abort@'+ai,'?Reset@SmartAI@@')),
):
    for target in targets:calls_prefix(method,target)
assert {104276,206360,206361,206390,49101,2000,60000} <= values(methods[0]), ({104276,206360,206361,206390,49101,2000,60000} - values(methods[0]))
assert {204771,206357,5000} <= values(methods[1])
# HasParticipant is inlined here: packed ObjectGuid16 plus three receipt bits.
assert 19 in values(methods[1]) and any(x.mnemonic.startswith('j') and x.operands and x.operands[0].type==CS_OP_IMM and x.operands[0].imm<x.address for x in ins(methods[1]))
assert 206357 in values(methods[2])
selected=[]
for x in ins('?GetAI@?$GenericCreatureScript@Unpc_stromkar_awakening@@@@'):
    for op in x.operands:
        if x.mnemonic!='lea' or op.type!=CS_OP_MEM or op.mem.base!=X86_REG_RIP:continue
        at=n['offset'](x.address+x.size+op.mem.disp)
        if at+640<=len(n['binary']):
            table=struct.unpack_from('<80Q',n['binary'],at)
            if all(n['symbol'](m) in table for m in methods):selected.append(table)
assert len(selected)==1,'Native initial actor factory does not install actual awakening overrides'
script='spell_stromkar_opening_recoil@@'
launch,check,register=('?'+name+'@'+script for name in ('Launch','Check','Register'))
for prefix in ('?GetFirstCollisionPosition@WorldObject@@','?GetHeight@WorldObject@@','?GetHitDest@SpellScript@@','?SetExplTargetDest@SpellScript@@','?PreventHitDefaultEffect@SpellScript@@'):
    calls_prefix(launch,prefix)
calls_prefix(check,'?Controller@'+script)
calls_prefix('?Controller@'+script,'?GetOriginalTarget@SpellScript@@')
def references(method):
    result=set()
    for x in ins(method):
        for op in x.operands:
            address=op.imm if op.type==CS_OP_IMM else (x.address+x.size+op.mem.disp if op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP else None)
            if address is not None:
                try:result.add(n['resolve'](address))
                except ValueError:pass # Stack sizes/effect constants are not PE pointers.
    return result
assert {n['symbol'](launch),n['symbol'](check)} <= references(register),'Recoil callback registration missing'
for method in ('?EffectJumpDest@Spell@@','?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@','?MovementInform@EffectMovementGenerator@@'):
    assert {206360,206361,1539} <= values(method),method
for token in (b'npc_stromkar_awakening\0',b'spell_stromkar_opening_recoil\0'):
    assert token in n['binary'],'Matching registration name missing'
print('PASS selected awakening factory Update/SpellHit/SpellHitTarget/Reset; actual spawn/channel/real49101 and retry/cleanup; selected recoil callbacks/collision-floor consumers; native self-target/failed-launch/completed-arrival206360->206361. Client sequence remains separate.')
