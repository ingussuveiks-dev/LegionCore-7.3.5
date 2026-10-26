"""Verify native effect157 uses spell-ID loot rather than a fake chest link."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_pickup_completion_release.py')))
native, instructions = previous['native'], previous['instructions']
effect = '?EffectCreateItem2@Spell@@'
mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')


def data_symbol(prefix):
    matches = re.findall(r'\s' + re.escape(prefix) + r'\S+\s+([0-9a-fA-F]{16})\s+game:', mapping)
    assert len(matches) == 1
    return int(matches[0], 16)


table = native['offset'](data_symbol('?SpellEffects@@'))
assert struct.unpack_from('<Q', native['binary'], table + 157 * 8)[0] == native['symbol'](effect)
body = instructions(effect)


def sites(decoded, prefix):
    targets = {address for name, address in native['symbols'].items() if name.startswith(prefix)}
    return [index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) in targets]


assert any(entry.op_str == 'dword ptr [rcx + 0x400], 3' for entry in body)
assert any(entry.op_str == 'byte ptr [rax + 0x16], 4' for entry in body)
crafting, = sites(body, '?IsLootCrafting@SpellInfo@@')
stores = sites(body, '?AutoStoreLoot@Player@@')
assert len(stores) == 2 and crafting < stores[0] < stores[1]
zero = next(index for index, entry in enumerate(body[crafting:], crafting) if entry.op_str == 'esi, esi' and entry.mnemonic == 'test')
assert body[zero + 1].mnemonic == 'je' and body[zero + 1].operands[0].imm == body[stores[1] - 8].address
for position in stores:
    window = body[position - 9:position]
    assert all(any(entry.op_str == operand for entry in window) for operand in ('r9d, dword ptr [r9]', 'r8b, 0xff', 'edx, edx', 'byte ptr [rsp + 0x28], 1'))
    references = [entry for entry in window if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
    assert any(entry.address + entry.size + entry.operands[1].mem.disp == data_symbol('?LootTemplates_Spell@@') for entry in references)
stored = instructions('?AutoStoreLoot@Player@@')
fill, = sites(stored, '?FillLoot@Loot@@')
slot, = sites(stored, '?LootItemInSlot@Loot@@')
allowed, = sites(stored, '?AllowedForPlayer@Loot@@')
capacity = sites(stored, '?CanStoreItem@Player@@')
created, = sites(stored, '?StoreNewItem@Player@@')
notify, = sites(stored, '?SendNewItem@Player@@')
assert fill < slot < allowed < min(capacity) < max(capacity) < created < notify
assert len(capacity) == 3
assert any(entry.mnemonic == 'setne' and entry.op_str == 'bl' for entry in stored[notify:])
print('PASS installed SpellEffects[157] selects native CreateItem2; zero ItemType reaches AutoStoreLoot using actual spell ID/LootTemplates_Spell, bag0/slot255/broadcast. Native fill/slot/condition/capacity/store/notify order retained. AutoStore result is slot-count based, NOT proof of stored item; enclosing full-bag/failed-store progression/retry and client remain OPEN. Live source12 row/rules verified separately.')
