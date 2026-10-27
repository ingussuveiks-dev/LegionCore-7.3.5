"""Check selected Kingslayers script/AI and quest-owned GUID outro consumers."""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']
def values(method):return {o.imm for x in ins(method) for o in x.operands if o.type==CS_OP_IMM}
def needs(method,*dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method),(method,dependency)
def refs(method):
    return {x.address+x.size+o.mem.disp for x in ins(method) for o in x.operands
            if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
def table(address,count):return struct.unpack_from('<'+str(count)+'Q',n['binary'],n['offset'](address))
gate='?CanTake@KingslayersAcquisition@@'
hello='?OnGossipHello@go_the_kingslayers@@'
loot='?OnLootStateChanged@go_the_kingslayers@@'
factory='?GetAI@go_the_kingslayers@@'
update='?UpdateAI@AI@go_the_kingslayers@@'
begin='?SetGUID@AI@go_the_kingslayers@@'
assert {1620,251107,1123,48589,48591,8,9}<=values(gate)
assert {42504,42627}<=values('?Quest@KingslayersAcquisition@@')
needs(hello,gate);needs(loot,gate,'?SetLootState@GameObject@@')
needs(begin,gate,'?getQuestStatus@Player@@','?GetPlayer@ObjectAccessor@@')
assert 215112 in values(begin)
needs(update,gate,'?GetItemByEntry@Player@@','?UpdateAchievementCriteria@Player@@','?StartOutro@KingslayersAcquisition@@')
assert {51616,48591,15000,2000,9}<=values(update)
pair='?Equipped@KingslayersAcquisition@@'
pair=pair if any(name.startswith(pair) for name in n['symbols']) else update
assert {128870,128869}<=values(pair)
for method in (hello,loot,begin,update):
    assert n['symbol']('?AddItem@Player@@') not in n['calls'](method)
    assert 1123 not in values(method),'Explicit premature scenario reward retained'
registered=[a for a in refs('?AddSC_Kingslayers@@') if table(a,13)[2]==n['symbol'](hello)]
assert len(registered)==1
script=table(registered[0],13)
assert script[10]==n['symbol'](loot) and script[12]==n['symbol'](factory)
assert any(table(a,30)[1]==n['symbol'](update) and n['symbol'](begin) in table(a,30) for a in refs(factory))
outro='?StartOutro@KingslayersAcquisition@@'
needs(outro,'?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@','?Outro@KingslayersAcquisition@@')
assert {108367,7000,13000,27000}<=values(outro)
resolver='?Outro@KingslayersAcquisition@@'
needs(resolver,'?getQuestStatus@Player@@','?GetCreature@ObjectAccessor@@')
assert {1620,1123,108367,48591,9}<=values(resolver)
lambdas=[name for name in n['symbols'] if 'StartOutro@KingslayersAcquisition' in name and name.startswith('?_Do_call@')]
assert len(lambdas)==3
personal=[method for method in lambdas if n['symbol'](resolver) in n['calls'](method)]
assert len(personal)==2
for method in personal:needs(method,'?FindPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVObjectGuid@@',resolver)
world_callbacks=[method for method in lambdas if method not in personal]
assert len(world_callbacks)==1
world='?WorldOutro@KingslayersAcquisition@@'
if any(name.startswith(world) for name in n['symbols']):
    needs(world_callbacks[0],world)
else:
    # The no-unwind std::function wrapper tail-calls the lambda operator.
    number=re.search(r'<lambda_\d+>',world_callbacks[0]).group()
    world='??R'+number+'@?1??StartOutro@KingslayersAcquisition@@'
    needs(world_callbacks[0],world)
needs(world,'?FindMap@MapManager@@','?GetCreature@Map@@')
assert {1620,1123,108367,48591,9}<=values(world)
print('PASS actual registered Kingslayers GO script/AI, native scope/quest lease/equipped pair/51616 acceptance,2s recovery and quest-owned7/13s and actor-owned27s world cleanup callbacks resolving full GUIDs. No replacement item or explicit scenario reward. Native portal215151/51617 and actual client traversal remain separate checks.')
