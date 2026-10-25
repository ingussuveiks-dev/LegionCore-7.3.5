"""Verify point6/7 admission dominates real home-position writes."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_offer_release.py')))
instructions = previous['instructions']
positions = previous['positions']
body = instructions('?MovementInform@npc_hrutnirAI@npc_hrutnir@@')
homes = positions(body, '?SetHomePosition@Creature@@QEAAXMMMM')
assert len(homes) == 3, 'Expected one legacy and two admitted home writes'
legacy, offer, arrival = homes
first = legacy - 6
assert body[first].op_str == 'rcx, qword ptr [rcx + 0x20]'
gate = body[first - 5:first]
assert [(entry.mnemonic, entry.op_str) for entry in (gate[0], gate[2], gate[3])] == [
    ('cmp', 'edx, 8'), ('lea', 'eax, [r8 - 6]'), ('cmp', 'eax, 1')]
assert gate[1].mnemonic == 'jne' and gate[1].operands[0].imm == body[first].address
assert gate[4].mnemonic == 'jbe' and body[legacy].address < gate[4].operands[0].imm < body[offer].address
dispatch = next(index for index, entry in enumerate(body) if entry.address == gate[4].operands[0].imm)
assert body[dispatch].mnemonic == 'dec' and body[dispatch].op_str == 'ebx', 'Only point6/7 may skip legacy home before native switch'
assert body[dispatch + 1].op_str == 'ebx, 6' and body[dispatch + 2].mnemonic == 'ja'
for index in homes:
    expected_operands = [
        'dword ptr [rsp + 0x20], xmm0', 'xmm3, dword ptr [rcx + 0xd8]',
        'xmm2, dword ptr [rcx + 0xd4]', 'xmm1, dword ptr [rcx + 0xd0]'] if index == legacy else [
        'xmm0, dword ptr [rcx + 0xdc]', 'dword ptr [rsp + 0x20], xmm0',
        'xmm3, dword ptr [rcx + 0xd8]', 'xmm2, dword ptr [rcx + 0xd4]']
    assert [entry.op_str for entry in body[index - 4:index]] == expected_operands
    if index == legacy:
        assert body[index - 5].op_str == 'xmm0, dword ptr [rcx + 0xdc]'
    assert all(entry.mnemonic == 'movss' for entry in body[index - 4:index])
contexts = positions(body, '?IsMagnarOfferContext@npc_hrutnirAI@npc_hrutnir@@')
assert len(contexts) == 4 and legacy < contexts[0] < offer < contexts[1] < arrival < contexts[2] < contexts[3]
for context, home in zip(contexts[:2], (offer, arrival)):
    comparison = next(index for index, entry in enumerate(body) if index > context and entry.mnemonic == 'comiss')
    assert body[comparison + 1].mnemonic == 'jb' and home == comparison + 6
    assert body[comparison + 1].operands[0].imm > body[arrival].address, 'Rejected endpoint can still write home'
setter = instructions('?SetHomePosition@Creature@@QEAAXMMMM')
for operand in ('dword ptr [rcx + 0x3ac8], xmm1', 'dword ptr [rcx + 0x3acc], xmm2',
                'dword ptr [rcx + 0x3ad0], xmm3', 'dword ptr [rbx + 0x3ad4], xmm0'):
    assert any(entry.mnemonic == 'movss' and entry.op_str == operand for entry in setter), 'Actual creature home field store changed'
assert len(positions(setter, '?NormalizeOrientation@Position@@')) == 1
print('PASS: installed full Hruthnir MovementInform skips legacy home for actual POINT8 IDs6/7 only; both context/ordered3D gates precede their current XYZ/orientation home writes and claims. Actual Creature home fields/normalization checked. Other notifications retain legacy update; no automatic evade/path recovery or client certificate.')
