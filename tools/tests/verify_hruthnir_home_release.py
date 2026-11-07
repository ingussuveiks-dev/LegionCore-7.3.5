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
cold_route = previous['cold_route']
primary_approach = previous['primary_approach']
if cold_route:
    type_gate = next(i for i,entry in enumerate(body[:first]) if entry.mnemonic=='cmp' and entry.op_str=='edx, 8')
    assert body[type_gate+1].mnemonic=='jne' and body[type_gate+1].operands[0].imm==body[first].address
    if primary_approach:
        six=next(i for i,e in enumerate(body[:first]) if e.mnemonic=='cmp' and e.op_str=='edi, 6')
        seven=next(i for i,e in enumerate(body[:first]) if e.mnemonic=='cmp' and e.op_str=='edi, 7')
        assert body[six+1].mnemonic=='jne' and body[six+1].operands[0].imm==body[seven].address
        branch=body[seven+1]
    else:
        gate=body[first-3:first]
        assert [(entry.mnemonic,entry.op_str) for entry in gate[:2]]==[('lea','eax, [r8 - 6]'),('cmp','eax, 1')]
        branch=gate[2]
else:
    gate = body[first - 5:first]
    assert [(entry.mnemonic, entry.op_str) for entry in (gate[0], gate[2], gate[3])] == [('cmp', 'edx, 8'), ('lea', 'eax, [r8 - 6]'), ('cmp', 'eax, 1')]
    assert gate[1].mnemonic == 'jne' and gate[1].operands[0].imm == body[first].address
    branch=gate[4]
assert branch.mnemonic==('je' if primary_approach else 'jbe') and body[legacy].address<branch.operands[0].imm<body[offer].address
dispatch=next(i for i,entry in enumerate(body) if entry.address==branch.operands[0].imm)
id_register='ebx' if cold_route and not primary_approach else 'edi'
assert body[dispatch].mnemonic=='dec' and body[dispatch].op_str==id_register
assert body[dispatch+1].op_str==id_register+', 6' and body[dispatch+2].mnemonic=='ja'
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
contexts = [index for index in positions(body, '?IsMagnarOfferContext@npc_hrutnirAI@npc_hrutnir@@') if index > legacy]
assert len(contexts) == (2 if previous['current_start'] else 3) and legacy < contexts[0] < offer < contexts[1] < arrival
if not previous['current_start']:
    assert arrival < contexts[2]
assert len(positions(instructions('?StartMagnarConversation@npc_hrutnirAI@npc_hrutnir@@'), '?IsMagnarOfferContext@npc_hrutnirAI@npc_hrutnir@@')) == 1
for context, home in zip(contexts[:2], (offer, arrival)):
    comparison = next(index for index, entry in enumerate(body) if index > context and entry.mnemonic == 'comiss')
    assert body[comparison + 1].mnemonic == 'jb' and home == comparison + 6
    assert body[comparison + 1].operands[0].imm > body[arrival].address, 'Rejected endpoint can still write home'
setter = instructions('?SetHomePosition@Creature@@QEAAXMMMM')
for operand in ('dword ptr [rcx + 0x3ac8], xmm1', 'dword ptr [rcx + 0x3acc], xmm2',
                'dword ptr [rcx + 0x3ad0], xmm3', 'dword ptr [rbx + 0x3ad4], xmm0'):
    assert any(entry.mnemonic == 'movss' and entry.op_str == operand for entry in setter), 'Actual creature home field store changed'
assert len(positions(setter, '?NormalizeOrientation@Position@@')) == 1
print('PASS: installed ordinary Hruthnir POINT IDs6/7 require context/ordered3D endpoint before current XYZ/orientation home writes and claims; actual Creature home fields/normalization checked. Ordinary other notifications retain legacy home; additional scoped cold POINT8 arrival is certified separately when present. No client traversal certificate.')
if cold_route:
    runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cold_route_release.py')))['check'](previous['native'],instructions)
