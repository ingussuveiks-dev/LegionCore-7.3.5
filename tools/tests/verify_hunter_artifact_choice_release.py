"""Check the linked final Vigfus death callback, owner bridge and selected vtable."""
from pathlib import Path
import bisect
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

native = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
stamp = re.search(r'Timestamp is ([0-9a-fA-F]+)', (native['args'].release / 'worldserver.map').read_text(errors='replace'))
assert stamp and int(stamp.group(1), 16) == struct.unpack_from('<I', native['binary'], native['pe'] + 8)[0], 'Canonical PE/map link timestamps differ'
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


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



for method in ('?SendDisplayPlayerChoice@Player@@', '?HandleChoiceResponse@WorldSession@@'):
    body = instructions(method)
    values = {op.imm for ins in body for op in ins.operands if op.type == CS_OP_IMM}
    assert {240, 245, 32, 33} <= values, f'Missing Hunter/Warlock choice and response scopes: {method}'
    targets = native['calls'](method)
    for dependency in ('?GetConditionsForNotGroupedEntry@ConditionMgr@@', '?IsObjectMeetToConditions@ConditionMgr@@'):
        matches = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(dependency)}
        assert matches & targets, f'Missing native condition consumer: {method}, {dependency}'
    # Actual compiler shape: Warlock comparison jumps to the same condition
    # block that Hunter reaches by falling through the following jne.
    gates = []
    for index, ins in enumerate(body[:-4]):
        if ins.mnemonic == 'cmp' and ins.operands[-1].type == CS_OP_IMM and ins.operands[-1].imm == 245:
            hunter, warlock_jump, skip = body[index + 2], body[index + 1], body[index + 3]
            if hunter.mnemonic == 'cmp' and hunter.operands[-1].type == CS_OP_IMM and hunter.operands[-1].imm == 240:
                assert warlock_jump.mnemonic == 'je' and skip.mnemonic == 'jne'
                assert warlock_jump.operands[0].imm == body[index + 4].address
                assert skip.operands[0].imm > body[index + 4].address
                gates.append((body[index + 4].address, skip.operands[0].imm))
    assert len(gates) == (2 if method.startswith('?SendDisplay') else 1), f'Missing selected Hunter guards: {method}'
    if method.startswith('?SendDisplay'):
        # Empty Hunter response vector branches away before Write/Send.
        last = next(i for i in range(len(body)-1, -1, -1) if body[i].mnemonic == 'cmp' and body[i].operands[-1].type == CS_OP_IMM and body[i].operands[-1].imm == 240)
        assert [x.mnemonic for x in body[last:last+5]] == ['cmp','jne','mov','cmp','je']
        assert body[last+4].operands[0].imm > body[last+1].operands[0].imm
print('PASS actual selected display/source32, response filtering/source33, stale-response rechecks and empty Hunter frame guard in matching Release PE/map; Warlock245 retained.')
