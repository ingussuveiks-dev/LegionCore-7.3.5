"""Verify the registered Frost scene gate and native path record in linked PE."""
import bisect
from pathlib import Path
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP, X86_REG_R9D, X86_REG_R9B

n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
def ins(prefix):
    address=n['symbol'](prefix);index=bisect.bisect_right(n['starts'],address-n['base'])-1
    begin,end,unwind=n['functions'][index]
    if not begin<=address-n['base']<end:
        end=min(v for v in n['symbols'].values() if v>address)-n['base']
        assert end+n['base']-address<=128
    else:
        origin=n['root'](begin,unwind)
        for start,stop,info in n['functions'][index+1:]:
            if start!=end or n['root'](start,info)!=origin:break
            end=stop
    at=n['offset'](address)
    return list(decoder.disasm(n['binary'][at:at+end+n['base']-address],address))
def refs(prefix):
    return {i.address+i.size+o.mem.disp for i in ins(prefix) for o in i.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
data_symbols={}
for line in (n['args'].release/'worldserver.map').read_text(errors='replace').splitlines():
    m=re.match(r'\s+\w+:\w+\s+(\?\S+)\s+([0-9A-Fa-f]{16})\s',line)
    if m:data_symbols[m[1]]=int(m[2],16)
def data_symbol(prefix):
    matches=[v for k,v in data_symbols.items() if k.startswith(prefix)]
    assert len(matches)==1,(prefix,len(matches))
    return matches[0]
handler='?Denied@FrostArtifactScene@@';wrapper='?OnTrigger@scene_frost_artifact_equipment@@'
constructor='??0scene_frost_artifact_equipment@@';registration='?AddSC_artifact_acquisition@@'
vtable=data_symbol('??_7scene_frost_artifact_equipment@@6B@')
entries=struct.unpack_from('<QQ',n['binary'],n['offset'](vtable))
assert n['resolve'](entries[1])==n['symbol'](wrapper),'Registered virtual scene callback differs'
assert vtable in refs(constructor),'Constructor does not install the tested scene vtable'
assert n['symbol'](constructor) in n['calls'](registration) or vtable in refs(registration),'Scene gate is not constructed by AddSC'
assert n['symbol'](handler) in n['calls'](wrapper) or n['symbol'](wrapper)==n['symbol'](handler)
paths=data_symbol('?Pickups@FrostArtifactScene@@')
assert struct.unpack_from('<7I',n['binary'],n['offset'](paths))==(1071,1466,1480,901,6,38990,128292)
assert any(paths<=v<paths+28 for v in refs(handler)),'Gate does not consume its native path record'
calls=n['calls'](handler)
for target in ('?HasSceneStatus@Player@@','?HasItemCount@Player@@','?GetScenario@ScenarioMgr@@'):
    assert n['symbol'](target) in calls,('Missing actual admission consumer',target)
quest_calls=set(calls)
if n['symbol']('?Active@ArtifactAcquisition@@') in calls:quest_calls|=n['calls']('?Active@ArtifactAcquisition@@')
assert n['symbol']('?GetQuestStatus@Player@@') in quest_calls
for prefix in ('?CastSpell@Unit@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?TeleportTo@Player@@'):
    assert not any(n['resolve'](value) in calls for name,value in n['symbols'].items() if name.startswith(prefix)),prefix
body=ins(handler)
assert any(o.type==CS_OP_IMM and o.imm==6 for i in body for o in i.operands),'Frost class admission lost'
item_call=n['symbol']('?HasItemCount@Player@@')
sites=[index for index,i in enumerate(body) if i.mnemonic=='call' and i.operands[0].type==CS_OP_IMM and n['resolve'](i.operands[0].imm)==item_call]
assert len(sites)==1
window=body[max(0,sites[0]-20):sites[0]]
assert any(i.mnemonic=='xor' and len(i.operands)==2 and all(o.type==CS_OP_REG and o.reg==X86_REG_R9D for o in i.operands) or i.mnemonic=='mov' and len(i.operands)==2 and i.operands[0].type==CS_OP_REG and i.operands[0].reg in (X86_REG_R9D,X86_REG_R9B) and i.operands[1].type==CS_OP_IMM and i.operands[1].imm==0 for i in window),'Inventory check no longer excludes bank-only copies'
assert b'scene_frost_artifact_equipment' in n['binary']
print('PASS actual registered Frost scene constructor/vtable/OnTrigger consumes the exact path record, real completed-scene/quest/scenario and inventory-only item checks; gate itself casts/grants/transfers nothing. Native host verifies invalid/stale/context branches; SQL callback and client behavior are separate checks.')
