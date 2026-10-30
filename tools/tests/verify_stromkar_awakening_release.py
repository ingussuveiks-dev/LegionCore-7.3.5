"""Verify the selected awakening AI, recoil hooks and native arrival consumers."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP,X86_REG_BPL,X86_REG_R8B,X86_REG_RSP
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
assert {64833,2000} <= values(methods[1]),'Native transient ground kit receipt missing'
calls_prefix(methods[1],'?SendPlaySpellVisualKit@Unit@@')
calls_prefix(methods[0],'?CancelSpellVisualKit@Unit@@')
calls_prefix('?Abort@'+ai,'?CancelSpellVisualKit@Unit@@')
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
# The second angle is landing->source, after the existing outward fallback.
# Normalized orientation is then copied with the destination and consumed by
# the native MoveJump(Position) wrapper. Merely storing it without enabling
# facing would still leave the participant facing away along the flight path.
angle=n['symbol']('?GetAngle@Position@@QEBAMPEBU1@@Z')
angle_calls=[i for i,x in enumerate(ins(launch)) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and x.operands[0].imm==angle]
assert len(angle_calls)==2,'Landing-to-body facing calculation missing'
jump=list(ins('?EffectJumpDest@Spell@@'))
def operand_is_register(x,reg):return x.type==CS_OP_REG and x.reg==reg
set_recoil=[i for i,x in enumerate(jump) if x.mnemonic=='mov' and operand_is_register(x.operands[0],X86_REG_BPL) and x.operands[1].type==CS_OP_IMM and x.operands[1].imm==1]
assert len(set_recoil)==1
assert {1539,206360,206361}<={o.imm for x in jump[max(0,set_recoil[0]-13):set_recoil[0]] for o in x.operands if o.type==CS_OP_IMM},'Facing flag not scoped to native recoil pair/map'
facing=[jump[i+1].operands[0].imm for i,x in enumerate(jump[:-1]) if x.mnemonic=='test' and operand_is_register(x.operands[0],X86_REG_BPL) and jump[i+1].mnemonic=='jne']
assert len(facing)==1
enabled=next(x for x in jump if x.address==facing[0])
assert enabled.mnemonic=='mov' and operand_is_register(enabled.operands[0],X86_REG_R8B) and enabled.operands[1].type==CS_OP_IMM and enabled.operands[1].imm==1,'Scoped recoil does not enable spline facing'
assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.base==X86_REG_RSP and x.operands[0].mem.disp==0x40 and operand_is_register(x.operands[1],X86_REG_R8B) for x in jump),'Facing flag not passed to actual MoveJump ABI'
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
