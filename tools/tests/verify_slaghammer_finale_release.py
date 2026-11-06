"""Verify linked Slaghammer attempt lifetime and explicit shared-target delegation."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1),16)==struct.unpack_from('<I',native['binary'],native['pe']+8)[0], 'PE/map mismatch'
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    assert begin <= address - native['base'] < end, f'Missing runtime bounds: {prefix}'
    origin = native['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in native['functions'][index + 1:]:
        if tail_begin != end or native['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    start = native['offset'](address)
    return list(decoder.disasm(native['binary'][start:start + native['base'] + end - address], address))


def call_index(body, prefix):
    target=native['symbol'](prefix)
    return [i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==target]

summon=instructions('?SummonCreature@Map@@')
values={o.imm for x in summon for o in x.operands if o.type==CS_OP_IMM}
assert {101899,201004,1539,991,40710}<=values, 'Missing native actor/spell/map/scenario/quest guards'
indices=call_index(summon,'?BindQuestObject@Player@@')
selected=[i for i in indices if any(x.mnemonic=='mov' and x.op_str=='edx, 0x9f06' for x in summon[max(0,i-8):i])]
assert len(selected)==1, 'Missing unique quest40710 scope binding'
assert any(x.mnemonic=='xor' and x.op_str=='r9d, r9d' for x in summon[selected[0]-8:selected[0]]), 'Shared-target option not explicitly false'
for prefix in ('?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?GetScenarioId@Scenario@@'):
    assert call_index(summon,prefix), 'Missing native admission call: '+prefix
assert native['symbol']('?BindQuestObjectScope@WorldObject@@') in native['calls']('?BindQuestObject@Player@@'), 'Player no longer binds actual object scope'
bind=instructions('?BindQuestObjectScope@WorldObject@@')
writes=[x.operands[0].mem.disp for x in bind if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[0].type==CS_OP_MEM and x.operands[1].type==CS_OP_REG and x.reg_name(x.operands[1].reg)=='r8b']
assert len(writes)==1, 'Target restriction flag not stored from caller'
flag=writes[0];target=instructions('?CanTargetQuestObject@WorldObject@@')
checks=[i for i,x in enumerate(target) if x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==flag for o in x.operands)]
assert len(checks)==1 and checks[0]>10, 'Missing independent target restriction decision after scope admission'
assert any(x.mnemonic=='test' for x in target[checks[0]-8:checks[0]]), 'Scope/cancellation guard missing before shared-target branch'
# MSVC inlines BindQuestObjectScope here. Verify the actual read/copy of
# the parent's mode, rather than requiring an out-of-line call.
inherit=instructions('?InheritQuestObjectScope@WorldObject@@')
reads=[i for i,x in enumerate(inherit) if x.mnemonic=='movzx' and any(o.type==CS_OP_MEM and o.mem.disp==flag for o in x.operands)]
assert len(reads)==1, 'Inherited target restriction mode not read'
source_register=inherit[reads[0]].reg_name(inherit[reads[0]].operands[0].reg)
low_register={'eax':'al','r8d':'r8b','edx':'dl','ecx':'cl'}.get(source_register)
assert low_register and any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==flag and x.operands[1].type==CS_OP_REG and x.reg_name(x.operands[1].reg)==low_register for x in inherit[reads[0]+1:reads[0]+5]), 'Child does not retain actual parent mode'

# Relation6 has its own switch arm: reject a non-TempSummon, load the full
# native summoner GUID, then reuse the existing full-GUID comparison tail.
condition=instructions('?Meets@Condition@@')
dispatch=[i for i,x in enumerate(condition[:-3]) if x.mnemonic=='cmp' and x.op_str=='eax, 6' and condition[i+1].mnemonic=='ja' and condition[i+2].mnemonic=='mov' and condition[i+2].operands[1].type==CS_OP_MEM]
assert len(dispatch)==1, 'Actual relation0..6 switch missing'
table=condition[dispatch[0]+2].operands[1].mem.disp
entries=struct.unpack_from('<7I',native['binary'],native['offset'](native['base']+table))
assert len({entries[i] for i in (3,5,6)})==3, 'Legacy field relations conflated with TempSummon source'
address=native['base']+entries[6]
arm=list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address)+32],address))
assert arm[0].mnemonic=='test' and arm[0].operands[0].type==CS_OP_MEM and arm[0].operands[0].size==1 and arm[0].operands[1].imm==1 and arm[1].mnemonic=='je', 'Missing actual isSummon type guard'
assert arm[2].mnemonic=='movups' and arm[2].operands[1].type==CS_OP_MEM and arm[2].operands[1].size==16 and arm[2].operands[1].mem.disp==0x3b64, 'Missing actual full TempSummon summoner GUID (verified x64 layout)'
assert arm[3].mnemonic=='jmp' and arm[3].operands[0].imm>native['base']+entries[3] and arm[3].operands[0].imm<native['base']+entries[5], 'TempSummon branch bypasses legacy full-GUID comparison tail'

print('PASS linked PE: scoped101899/201004/1539/991/40710 with explicit shared-target option; normal Player/object binding and inherited mode; cancellation gate and distinct actual-TempSummon relation6 retained. Native behavioral regression and client checks are separate.')
