"""Verify the selected native sparse loader and repaired string allocation in PE."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']


def references(method):
    return {x.address + x.size + op.mem.disp for x in ins(method) for op in x.operands
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}


def find(body, mnemonic, operand):
    return next(i for i, x in enumerate(body) if x.mnemonic == mnemonic and x.op_str == operand)


text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
tables = re.findall(r'\s+\w+:\w+\s+\?\?_7DB2FileLoaderSparseImpl@@6B@\s+([0-9a-fA-F]{16})', text)
assert len(tables) == 1
vtable = int(tables[0], 16)
slots = struct.unpack_from('<24Q', n['binary'], n['offset'](vtable))
strings = '?AutoProduceStrings@DB2FileLoaderSparseImpl@@'
data = '?AutoProduceData@DB2FileLoaderSparseImpl@@'
assert slots[5] == n['symbol'](strings) and slots[4] == n['symbol'](data)
assert vtable in references('?Load@DB2FileLoader@@'), 'Actual file load does not construct the sparse implementation'
dispatch = ins('?AutoProduceStrings@DB2FileLoader@@')
assert any(x.mnemonic == 'jmp' and x.op_str == 'qword ptr [rax + 0x28]' for x in dispatch)
assert n['symbol']('?AutoProduceStrings@DB2FileLoader@@') in n['calls']('?Load@DB2StorageBase@@')

body = ins(strings)
count = find(body, 'movzx', 'eax, word ptr [rdx + 4]')
assert body[count - 2].op_str == 'dword ptr [rdx], 0' and body[count - 1].mnemonic == 'je'
assert [(x.mnemonic, x.op_str) for x in body[count + 1:count + 7]] == [
    ('test', 'ax, ax'), ('je', hex(body[count + 5].address)), ('inc', 'esi'),
    ('add', 'rdi, rax'), ('add', 'rdx, 6'), ('sub', 'r8, 1')]
# Both localized 's' and unlocalized 'S' are variable-size serialized fields.
kind = find(body, 'cmp', 'al, 0x73')
assert body[kind + 2].op_str == 'al, 0x53' and body[kind + 4].op_str == 'ebx'
allocation = find(body, 'sub', 'rdi, rcx')
assert count < kind < allocation
assert [(x.mnemonic, x.op_str) for x in body[allocation - 5:allocation + 2]] == [
    ('mov', 'eax, ebx'), ('shl', 'rax, 3'), ('sub', 'rcx, rax'),
    ('mov', 'eax, esi'), ('imul', 'rcx, rax'), ('sub', 'rdi, rcx'), ('mov', 'rcx, rdi')]
assert body[allocation + 2].mnemonic == 'call'
# Check a missing base-locale ID branches to the same next-row path as an
# out-of-range ID, before dereferencing the localized holder.
lookup = find(body, 'mov', 'rax, qword ptr [rax + rcx*8]')
assert body[lookup - 3].mnemonic == 'jae'
skip = body[lookup - 3].operands[0].imm
assert body[lookup + 2].mnemonic == 'test' and body[lookup + 2].op_str == 'rax, rax'
assert body[lookup + 3].mnemonic == 'je' and body[lookup + 3].operands[0].imm == skip

# Resolve the actual 'S' switch target in AutoProduceData, rather than finding
# an unrelated pointer store elsewhere in the method.
materialize = ins(data)
switch = find(materialize, 'add', 'eax, -0x53')
byte_table = next(x.operands[1].mem.disp for x in materialize[switch:switch + 12]
                  if x.mnemonic == 'movzx' and len(x.operands) == 2 and x.operands[1].type == CS_OP_MEM)
jump_table = next(x.operands[1].mem.disp for x in materialize[switch:switch + 12]
                  if x.mnemonic == 'mov' and len(x.operands) == 2 and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.scale == 4)
case = n['binary'][n['offset'](n['base'] + byte_table)]
target = n['base'] + struct.unpack_from('<I', n['binary'], n['offset'](n['base'] + jump_table + 4 * case))[0]
entry = next(x for x in materialize if x.address == target)
assert entry.mnemonic == 'mov' and entry.op_str == 'qword ptr [rbp + r14], rsi', 'Unlocalized field does not store the actual string cursor directly'
print('PASS selected sparse loader/vtable/storage dispatch: 64-bit sum over all logical catalog records drives the string allocation, both string kinds excluded from fixed bytes, missing locale IDs skipped, actual S switch stores a direct string pointer. Native ASAN/full-file differentials remain separate from client quest traversal.')
