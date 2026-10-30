"""Follow registered native Stromkar return producer, owned AI and area admission."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_defeat_pose_release.py')))
n, ins, values = (p[k] for k in ('n','ins','values'))

def needs(caller, prefix):
    assert any(n['resolve'](a) in n['calls'](caller) for name,a in n['symbols'].items() if name.startswith(prefix)), (caller,prefix)

def tables(method, count=80):
    for x in ins(method):
        for operand in x.operands:
            if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                try:
                    at=n['offset'](x.address+x.size+operand.mem.disp)
                    yield struct.unpack_from('<'+str(count)+'Q',n['binary'],at)
                except (AssertionError,ValueError,struct.error):
                    pass

factory='?GetAI@npc_stromkar_valkyr@@'
condition='?OnConditionCheck@condition_stromkar_return@@'
registration=list(tables('?AddSC_artifact_acquisition@@'))
assert any(n['symbol'](factory) in t for t in registration), 'Actual creature script registration missing'
assert any(n['symbol'](condition) in t for t in registration), 'Actual condition script registration missing'
needs(factory,'?Owner@StromkarReturn@@')
needs(factory,'??0SmartAI@@')
selected=[t for t in tables(factory) if all(n['symbol']('?'+method+'@ValkyrAI@StromkarReturn@@') in t for method in ('Reset','UpdateAI'))]
assert selected, 'Owned AI is not installed by actual factory'
source='?Source@StromkarReturn@@'
eligible='?Eligible@StromkarReturn@@'
participant='?Participant@StromkarReturn@@'
owner='?Owner@StromkarReturn@@'
update='?Update@StromkarReturn@@'
signal='?SetData@npc_stromkar_thoradin@@' # Signal is fully inlined here.
area_update='?UpdateAI@ValkyrAI@StromkarReturn@@'
clear='?ClearArea@ValkyrAI@StromkarReturn@@'
assert {104307,1539,1037,10} <= values(source)
assert {41105,103114,128910,1539,1} <= values(participant)
needs(participant,'?GetQuestObjectiveData@Player@@QEBAHII@Z')
needs(eligible,participant)
needs(eligible,'?CanContact@Player@@')
# Source's small scope predicate is inlined in Owner by this Release build.
assert {93819,104307,1539,1037,10} <= values(owner)
needs(owner,'?GetSummoner@TempSummon@@')
needs(owner,'?GetCurrentStep@Scenario@@')
needs(signal,'?GetCurrentStep@Scenario@@')
assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM and x.operands[0].size == 1
           and x.operands[0].mem.disp == 0x208 and x.operands[-1].imm == 1 for x in ins(signal)), 'Actual final-speech state latch missing'
needs('?UpdateAI@npc_stromkar_thoradin@@',update)
needs('?UpdateAI@npc_stromkar_thoradin@@','?UpdateAI@SmartAI@@')
assert {3,1,9,10,104307,1539,1037} <= values(signal)
for call in (source,owner,participant,'?CanContact@Player@@','?MovePositionToFirstCollision@WorldObject@@','?SummonCreature@WorldObject@@'):
    needs(update,call)
assert {93819,8,1000} <= values(update)
for call in (owner,clear,'?CreateAreaTrigger@AreaTrigger@@','?SetDuration@AreaTrigger@@'):
    needs(area_update,call)
assert {11869,60000,1000} <= values(area_update)
assert {-1,0xffffffff} & values(area_update), 'Owned area does not use native infinite duration'
needs(area_update,'?CanContact@Player@@') # Small Eligible wrapper is inlined here and in Update.
needs(area_update,participant)
for call in ('?IsUnitAffected@AreaTrigger@@','?IsInArea@AreaTrigger@@','?AffectUnit@AreaTrigger@@'):
    needs(area_update,call)
needs(area_update,'?HasAura@Unit@@')
assert {215997,1} <= values(area_update), 'Missing real enter-action retry/aura gate'
needs(clear,'?Remove@AreaTrigger@@')
needs(condition,eligible)
# This64-byte tail-call leaf has no x64 unwind entry. Bound it by the next
# actual mapped symbol instead of reading a neighbouring function's pdata.
leaf=n['symbol'](condition)
leaf_end=min(a for a in n['symbols'].values() if a>leaf)
assert 0 < leaf_end-leaf <= 256
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
leaf_at=n['offset'](leaf)
leaf_code=list(decoder.disasm(n['binary'][leaf_at:leaf_at+leaf_end-leaf],leaf))
assert {26,11869,41,1} <= {op.imm for x in leaf_code for op in x.operands if op.type==CS_OP_IMM}
for method in (update,signal,area_update):
    for prefix in ('?KilledMonsterCredit@Player@@','?RewardPlayerAndGroupAtEvent@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
        assert not any(n['resolve'](a) in n['calls'](method) for name,a in n['symbols'].items() if name.startswith(prefix)), (method,prefix)
print('PASS registered Val kyr factory/Thoradin bridges, completed1037 quest/item participant versus contact-ready admission, native tracked/exact-area enter retry for missing215997, manual/infinite lifetime and60s absent-participant cleanup. No direct item/credit/teleport calls; native lifetime/area actions/arrival and client rendering are separate checks.')
