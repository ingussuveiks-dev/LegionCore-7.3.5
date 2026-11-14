"""Exclude both proved native Map summon RVA tables from instruction searches."""
import bisect
import struct
from capstone import CS_OP_IMM, CS_OP_MEM


def instructions(native):
    from five_ten_20261111_pe import ins, n
    assert native['binary'] == n['binary'] and native['base'] == n['base']
    rows = ins('?SummonCreature@Map@@')
    start, code_end = rows[0].address, rows[-1].address + rows[-1].size
    index = bisect.bisect_right(n['starts'], start - n['base']) - 1
    begin, end, unwind = n['functions'][index]
    origin = n['root'](begin, unwind)
    for b, e, u in n['functions'][index+1:]:
        if b != end or n['root'](b, u) != origin:
            break
        end = e
    boundaries = {x.address for x in rows}
    tables = []
    for i, row in enumerate(rows):
        for operand in row.operands:
            if operand.type != CS_OP_MEM or operand.mem.scale != 4 or not operand.mem.index:
                continue
            table = n['base'] + operand.mem.disp
            if not code_end <= table < end + n['base']:
                continue
            assert row.reg_name(operand.mem.index) == 'rax'
            bounds = [x for x in rows[max(0, i-6):i] if x.mnemonic == 'cmp'
                      and x.op_str.startswith('eax, ') and x.operands[1].type == CS_OP_IMM]
            assert len(bounds) == 1
            bound = bounds[0]
            at = rows.index(bound)
            branch = rows[at+1]
            assert branch.mnemonic == 'ja' and branch.operands[0].imm in boundaries
            # Only the image-base LEA or safe positive-index sign extension
            # may intervene between the checked index and its table load.
            count = bound.operands[1].imm + 1
            assert 0 < count <= 0x80000000
            assert all(x.mnemonic == 'cdqe' or x.mnemonic == 'lea' and
                       all(x.reg_name(reg) not in ('rax', 'eax', 'ax', 'al', 'ah')
                           for reg in x.regs_access()[1]) for x in rows[at+2:i])
            assert table + 4 * count <= end + n['base']
            targets = struct.unpack_from('<' + str(count) + 'I', n['binary'], n['offset'](table))
            assert all(n['base'] + target in boundaries for target in targets), 'Map summon RVA target is not code'
            tables.append((table, count))
    tables.sort()
    assert [count for _, count in tables] == [6, 17], 'Native category/type dispatch bounds changed'
    assert tables[0][0] == code_end and tables[1][0] == tables[0][0] + 4 * tables[0][1]
    return rows
