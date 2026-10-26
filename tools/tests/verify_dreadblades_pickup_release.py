"""Select the registered Dreadblades GO factory/callback and actual pending AI."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p = runpy.run_path(str(Path(__file__).with_name('verify_dreadblades_death_release.py')))
n, instructions = p['native'], p['instructions']
def values(prefix):
    return {o.imm for i in instructions(prefix) for o in i.operands if o.type == CS_OP_IMM}
gate='?CanTake@DreadbladesAcquisition@@'
hello='?OnGossipHello@go_scenario_dreadblades@@'
loot='?OnLootStateChanged@go_scenario_dreadblades@@'
factory='?GetAI@go_scenario_dreadblades@@'
update='?UpdateAI@AI@go_scenario_dreadblades@@'
begin='?SetGUID@AI@go_scenario_dreadblades@@'
assert {1545,254087,40849,1012,4,46707} <= values(gate)
assert n['symbol'](gate) in n['calls'](loot)
assert n['symbol'](gate) in n['calls'](hello)
assert 202040 in values(begin) and 15000 in values(update)
assert 48553 in values(update)
assert n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](update)
for method in (hello,loot,update,begin):
    assert n['symbol']('?AddItem@Player@@') not in n['calls'](method), 'Pickup still grants a replacement'
    if method != gate:
        assert 1012 not in values(method), 'Explicit premature scenario reward retained'
def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if ins.mnemonic=='lea' and op.type==CS_OP_MEM and op.mem.base==X86_REG_RIP:
                yield n['offset'](ins.address+ins.size+op.mem.disp)
tables=[at for at in references('?AddSC_Dreadblades@@') if struct.unpack_from('<Q',n['binary'],at+16)[0]==n['symbol'](hello)]
assert len(tables)==1
at=tables[0]
assert struct.unpack_from('<Q',n['binary'],at+0x50)[0]==n['symbol'](loot)
assert struct.unpack_from('<Q',n['binary'],at+0x60)[0]==n['symbol'](factory)
assert any(struct.unpack_from('<Q',n['binary'],at+8)[0]==n['symbol'](update) for at in references(factory)), 'Factory misses actual pending pickup AI'
print('PASS selected registered Dreadblades script/AI: native admission/death leaf, actual loot end, native202040, bounded real equip confirmation and48553; no replacement grant or explicit1012 reward. Client traversal pending.')
