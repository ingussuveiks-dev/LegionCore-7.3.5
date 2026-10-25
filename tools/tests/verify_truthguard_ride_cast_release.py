"""Check selected ride rejection cleanup through native summon and deferred-despawn bridges."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_endpoint_release.py')))
native, instructions = previous['native'], previous['instructions']
suffix = previous['suffix']
spawn = '?IsSummonedBy' + suffix
body = instructions(spawn)

def find(entries, operand, mnemonic=None):
    return next(index for index, entry in enumerate(entries) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))

def call(entries, prefix):
    target = native['symbol'](prefix)
    return next(index for index, entry in enumerate(entries) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == target)

cast = call(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert [entry.op_str for entry in body[cast - 4:cast]] == ['r9b, 1', 'r8d, 0xcca7', 'rdx, qword ptr [rdi]', 'rcx, r15']
assert body[cast + 1].op_str == 'r13d, eax' and body[cast + 2].op_str == 'eax, eax' and body[cast + 3].mnemonic == 'je'
accepted = body[cast + 3].operands[0].imm
missing = find(body, 'r15, r15', 'test')
assert missing < cast and body[missing + 1].mnemonic == 'je' and body[missing + 1].operands[0].imm == accepted
summoner = call(body, '?GetSummoner@TempSummon@@')
for operand in ('byte ptr [rcx + 0x30e8], 1', 'byte ptr [r15 + 0x16], 4'):
    assert find(body, operand) < summoner
for gate, operand, condition in ((find(body, 'byte ptr [rcx + 0x30e8], 1', 'test'), 'rcx, rbx', 'cmove'),
                                 (find(body, 'byte ptr [r15 + 0x16], 4', 'cmp'), 'r14, rbx', 'cmovne')):
    assert body[gate + 1].op_str == operand and body[gate + 1].mnemonic == condition
    assert body[gate + 3].mnemonic == 'je' and body[gate + 3].operands[0].imm == accepted
assert body[summoner + 1].op_str == 'rax, r15' and body[summoner + 2].mnemonic == 'jne' and body[summoner + 2].operands[0].imm == accepted
for operand in ('eax, 0x19d9e', 'dword ptr [rax + 0xe0], 0x4c4', 'eax, 0x19edd', 'dword ptr [rax + 0xe0], 0x5d7'):
    assert find(body, operand, 'cmp') > summoner
outgoing = find(body, 'dword ptr [rax + 0xe0], 0x4c4', 'cmp')
returning = find(body, 'dword ptr [rax + 0xe0], 0x5d7', 'cmp')
kit = find(body, 'rdi, qword ptr [rax + 0x30e0]', 'mov')
assert body[outgoing + 1].mnemonic == 'je' and body[outgoing + 1].operands[0].imm == body[kit].address
for gate in (find(body, 'eax, 0x19edd', 'cmp'), returning):
    assert body[gate + 1].mnemonic == 'jne' and body[gate + 1].operands[0].imm == accepted
rider = call(body, '?GetRider' + suffix)
seat = call(body, '?HasEmptySeat@Vehicle@@')
cleanup = call(body, '?DespawnOrUnsummon@Creature@@')
assert cast < summoner < rider < seat < cleanup
assert body[rider + 1].op_str == 'rax, rax' and body[rider + 2].mnemonic == 'jne' and body[rider + 2].operands[0].imm == accepted
assert body[seat - 2].op_str == 'edx, edx' and body[seat - 2].mnemonic == 'xor' and body[seat - 1].op_str == 'rcx, rdi'
assert body[seat + 1].op_str == 'al, al' and body[seat + 2].mnemonic == 'je' and body[seat + 2].operands[0].imm == accepted
assert body[cleanup - 2].op_str == 'edx, 1' and body[cleanup - 1].op_str == 'rcx, qword ptr [rsi + 0x20]'
assert body[cleanup + 1].address == accepted and not any(entry.mnemonic.startswith('j') for entry in body[cleanup + 1:]), 'Accepted cast can enter cleanup'
assert b'Truthguard travel: ride cast 52391 rejected with result %u for bird %u' in native['binary']
assert previous['slots'][0x148 // 8] == native['symbol'](spawn)
init = instructions('?InitSummon@TempSummon@@')
dispatch = next(index for index, entry in enumerate(init) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x148]')
assert [entry.op_str for entry in init[dispatch - 3:dispatch]] == ['rcx, qword ptr [rdi + 0x9c0]', 'rdx, rbx', 'rax, qword ptr [rcx]']
lookup = call(init, '?GetUnit@ObjectAccessor@@')
assert lookup < dispatch and init[lookup + 2].op_str == 'rax, rax' and init[lookup + 3].mnemonic == 'je'
assert init[lookup + 3].operands[0].imm > init[dispatch].address

decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
def leaf(prefix, limit):
    address = native['symbol'](prefix)
    output = []
    for entry in decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + limit], address):
        if entry.mnemonic == 'int3':
            break
        output.append(entry)
    return output

empty = leaf('?HasEmptySeat@Vehicle@@', 160)
assert empty[0].op_str == 'r8, qword ptr [rcx + 8]' and find(empty, 'dl, byte ptr [rcx + 0x20]', 'cmp') > 0
tail = next(index for index, entry in enumerate(empty) if entry.mnemonic == 'jmp' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol']('??7ObjectGuid@@'))
assert empty[tail - 1].op_str == 'rcx, 0x30' and empty[tail + 1].op_str == 'al, al' and empty[tail + 1].mnemonic == 'xor'
assert empty[tail + 2].mnemonic == 'ret', 'Missing seat is not conservative false'
despawn = instructions('?DespawnOrUnsummon@Creature@@')
assert find(despawn, 'byte ptr [rcx + 0x39c5], 0', 'cmp') < call(despawn, '?IsInWorld@Object@@')
virtual = next(index for index, entry in enumerate(despawn) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x2b8]')
assert find(despawn, 'byte ptr [rsi + 0x30e8], 1', 'test') < virtual and despawn[virtual - 2].op_str == 'edx, ebp'
unsummon = native['symbol']('?UnSummon@TempSummon@@')
tables = []
constructor = instructions('??0TempSummon@@')
for index, entry in enumerate(constructor):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP and constructor[index + 1].op_str == 'qword ptr [rdi], rax':
            slots = struct.unpack_from('<100Q', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))
            if unsummon in slots:
                tables.append(slots)
assert len(tables) == 1 and tables[0][0x2b8 // 8] == unsummon
delay = instructions('?UnSummon@TempSummon@@')
timer = find(delay, 'edx, edx', 'test')
kill = call(delay, '?KillAllEvents@EventProcessor@@')
clock = call(delay, '?CalculateTime@EventProcessor@@')
queue = call(delay, '?AddEvent@EventProcessor@@')
assert timer < kill < clock < queue and delay[timer + 1].mnemonic == 'je' and delay[timer + 1].operands[0].imm > delay[queue].address
assert delay[queue + 1].mnemonic == 'jmp' and delay[queue + 1].operands[0].imm > delay[queue + 1].address
event = native['symbol']('?Execute@ForcedUnsummonDelayEvent@@')
tables = []
for entry in delay[kill:queue]:
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            slots = struct.unpack_from('<8Q', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))
            if event in slots:
                tables.append(slots)
assert len(tables) == 1 and tables[0][1] == event
execute = instructions('?Execute@ForcedUnsummonDelayEvent@@')
assert execute[1].op_str == 'rcx, qword ptr [rcx + 0x20]' and execute[2].op_str == 'edx, edx' and execute[2].mnemonic == 'xor'
assert execute[4].mnemonic == 'call' and execute[4].op_str == 'qword ptr [rax + 0x2b8]'
print('PASS: selected native InitSummon->AI0x148 checks real52391 result, original Player/TempSummon/route and rider/empty seat0 before1ms cleanup; accepted or occupied branches skip it. Native despawn guards/TempSummon2B8, event deadline/queue and selected delayed Execute retained. Actual actor allocation/vehicle/client remain separate.')
