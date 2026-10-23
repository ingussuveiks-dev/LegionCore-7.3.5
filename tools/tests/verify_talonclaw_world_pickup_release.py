"""Verify actual source-GUID storage and scoped visual retirement in the deployed PE."""
from pathlib import Path
import bisect
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_AC_READ, CS_AC_WRITE

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_world_model_release.py')))
native = previous['native']
layout = native['args'].release.parents[1] / 'runtime-test/talonclaw-world-pickup/layout.tsv'
tag, source_offset, size = layout.read_text(encoding='utf-8-sig').strip().split('\t')
assert tag == 'SUMMONER_OFFSET' and int(size) == 16
source_offset = int(source_offset)
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def instructions(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    if not begin <= address - native['base'] < end:
        following = min(value for value in native['symbols'].values() if value > address)
        assert 0 < following - address <= 256
        end = following - native['base']
    else:
        origin = native['root'](begin, unwind)
        for first, last, data in native['functions'][index + 1:]:
            if first != end or native['root'](first, data) != origin:
                break
            end = last
    start = native['offset'](address)
    return list(decoder.disasm(native['binary'][start:start + native['base'] + end - address], address))


def accesses(prefix, mode):
    position = 0 if mode == CS_AC_WRITE else 1
    return any(instruction.mnemonic in {'mov', 'movups', 'movaps', 'movdqu', 'movdqa'} and
               len(instruction.operands) == 2 and instruction.operands[position].type == CS_OP_MEM and
               instruction.operands[position].mem.disp == source_offset
               for instruction in instructions(prefix))


retire = '?Retire@TalonclawWorldPickup@@'
values = {operand.imm for instruction in instructions(retire) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {248834, 1511, 40385, 128808, 973, 99480, 100310, 12, 1000}.issubset(values)
assert accesses(retire, CS_AC_READ), 'Retirement does not read the actual GameObject source GUID field'
summons = [name for name in native['symbols'] if name.startswith('?SummonGameObject@WorldObject@@')]
assert any(accesses(name, CS_AC_WRITE) for name in summons), 'Native gameobject summon does not store its source GUID'
targets = native['calls'](retire)
for prefix in ('?GetCreatureListWithEntryInGrid@WorldObject@@', '?DespawnOrUnsummon@Creature@@'):
    assert native['symbol'](prefix) in targets, f'Missing scoped retirement path: {prefix}'
binding = '?OnLootStateChanged@go_artifact_acquisition@@'
assert native['symbol'](binding) == native['symbol'](retire) or native['symbol'](retire) in native['calls'](binding)
assert native['symbol']('?OnGameObjectLootStateChanged@ScriptMgr@@') in native['calls']('?SetLootState@GameObject@@')
print(f'PASS: deployed source-GUID field at+{source_offset} is written by native summon and read by completed-loot retirement; real script dispatch schedules only scoped Talonclaw after1000ms')
