"""Select the registered Devourer GO factory/callback and actual pending AI."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p = runpy.run_path(str(Path(__file__).with_name('verify_devourer_death_release.py')))
n, instructions = p['native'], p['instructions']
def values(prefix):
    return {o.imm for i in instructions(prefix) for o in i.operands if o.type == CS_OP_IMM}
gate='?CanTake@DevourerAcquisition@@'
hello='?OnGossipHello@go_art_devourer@@'
loot='?OnLootStateChanged@go_art_devourer@@'
factory='?GetAI@go_art_devourer@@'
update='?UpdateAI@AI@go_art_devourer@@'
begin='?SetGUID@AI@go_art_devourer@@'
assert {1607,249347,41924,1078,6,47369} <= values(gate)
assert n['symbol'](gate) in n['calls'](loot)
assert n['symbol'](gate) in n['calls'](hello)
assert 209816 in values(begin) and 15000 in values(update)
assert 50301 in values(update)
assert n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](update)
for method in (hello,loot,update,begin):
    assert n['symbol']('?AddItem@Player@@') not in n['calls'](method), 'Pickup still grants a replacement'
    if method != gate:
        assert 1078 not in values(method), 'Explicit premature scenario reward retained'
def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
                yield n['offset'](ins.address+ins.size+op.mem.disp)
tables=[at for at in references('?AddSC_Devourer@@') if struct.unpack_from('<Q',n['binary'],at+16)[0]==n['symbol'](hello)]
assert len(tables)==1
at=tables[0]
assert struct.unpack_from('<Q',n['binary'],at+0x50)[0]==n['symbol'](loot)
assert struct.unpack_from('<Q',n['binary'],at+0x60)[0]==n['symbol'](factory)
assert any(struct.unpack_from('<Q',n['binary'],at+8)[0]==n['symbol'](update) for at in references(factory)), 'Factory misses actual pending pickup AI'
# MSVC inlines the small pair helper into this actual selected update.
assert {128476,128479} <= values(update)
assert n['symbol']('?GetItemByEntry@Player@@') in n['calls'](update)
body=instructions(update)
child=next(i for i,x in enumerate(body) if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm==128479 for o in x.operands))
assert body[child+1].mnemonic in ('je','jne'), 'Missing native offhand identity branch'
print('PASS selected registered Devourer script/AI: native admission/death leaf, actual loot end, native209816, bounded real equip confirmation and50301; no replacement grant or explicit1078 reward. Client traversal pending.')
