"""Verify the selected Grash final-heal, criteria and raw gossip paths."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

old=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_companion_healing_release.py')))
n=old['native'];instructions=n['instructions'];table=old['previous']['selected_tables']['npc_grash']
suffix='@npc_grashAI@npc_grash@@'
for method,slot in [('OnHealApplied',0x288),('UpdateLifecycle',0x110),('Reset',0x28)]:
    assert struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?'+method+suffix)
assert struct.unpack_from('<Q',n['binary'],table+0xb0)[0]==n['symbol']('?HealReceived@UnitAI@@')
def sites(body,prefix):
    address=n['symbol'](prefix)
    return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==address]
def floats(body):
    values=set()
    for x in body:
        if x.mnemonic in ('movss','comiss'):
            for operand in x.operands:
                if operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                    values.add(struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+operand.mem.disp))[0])
    return values
def immediates(body):
    return {operand.imm for x in body for operand in x.operands if operand.type==CS_OP_IMM}
def events(body):
    result=[]
    for site in sites(body,'?UpdateAchievementCriteria@Player@@'):
        asset=[x.operands[1].imm for x in body[max(0,site-10):site] if x.mnemonic=='mov' and x.op_str.startswith('r8d, ') and x.operands[1].type==CS_OP_IMM]
        assert asset
        result.append(asset[-1])
    return result
heal=instructions('?OnHealApplied'+suffix)
assert [(x.mnemonic,x.op_str) for x in heal[8:11]]==[
    ('cmp','byte ptr [rcx + 0x5a], 0'),('je',heal[9].op_str),('mov','byte ptr [rcx + 0x5b], 1')]
owner,=sites(heal,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
eligible,=sites(heal,'?Eligible@SharasdalGrash@@')
lease,=sites(heal,'?AddQuestDelayedEvent@Player@@')
assert owner<eligible<lease and {50.0,90.0,795.0}<=floats(heal)
assert 40341 in immediates(heal) and not events(heal)
complete=instructions('?CompleteIntroHeal'+suffix)
assert events(complete)==[49815,49900]
criterion_sites=sites(complete,'?UpdateAchievementCriteria@Player@@')
assert any(criterion_sites[0]<site<criterion_sites[1] for site in sites(complete,'?GetPlayer@ObjectAccessor@@'))
assert any(criterion_sites[0]<site<criterion_sites[1] for site in sites(complete,'?getQuestStatus@Player@@'))
assert any(criterion_sites[0]<site<criterion_sites[1] for site in sites(complete,'?Eligible@SharasdalGrash@@'))
assert {50.0,90.0,795.0}<=floats(complete)
assert {47121,47180,1000,30,3000}<=immediates(complete)
assert sites(complete,'?GetPlayer@ObjectAccessor@@') and sites(complete,'?getQuestStatus@Player@@')
assert sites(complete,'?Eligible@SharasdalGrash@@') and sites(complete,'?Receipt@SharasdalGrash@@')
talk,=sites(complete,'?Talk@CreatureAI@@QEAAXEVObjectGuid@@');queued,=sites(complete,'?AddFunction@FunctionProcessor@@')
assert talk<queued
assert b'Sharasdal Grash heal criteria rejected after thirty attempts; retaining real-heal retry' in n['binary']
life=instructions('?UpdateLifecycle'+suffix)
recover,=sites(life,'?Recover@SharasdalCompanion@@');clock,=sites(life,'?Update@FunctionProcessor@@');finish,=sites(life,'?CompleteIntroHeal'+suffix)
assert recover<clock<finish
reset=instructions('?Reset'+suffix)
assert sites(reset,'?CancelIntroHeal'+suffix) and sites(reset,'?KillAllFunctions@FunctionProcessor@@')
select=instructions('?sGossipSelect'+suffix)
assert events(select)==[50161]
assert sites(select,"?UpperWallScenario"+suffix) and 40341 in immediates(select) # inlined upper bind; receipt belongs to its arrival helper
assert 47303 in immediates(select)
if any(name.startswith('?Recruitable@SharasdalGrash@@') for name in n['symbols']):
    recruitment=instructions('?Recruitable@SharasdalGrash@@')
    assert {47121,47180,47161,2}<=immediates(recruitment)
    assert sites(select,'?Recruitable@SharasdalGrash@@')
else:
    assert {47121,47180}<=immediates(select)
assert sites(select,'?GetItem@GossipMenu@@') and sites(select,'?GetMenuId@GossipMenu@@')
assert sites(select,'?Eligible@SharasdalGrash@@') and sites(select,'?RemoveFlag@Object@@')
assert sites(select,'?ClearMenus@PlayerMenu@@') and sites(select,'?SendCloseGossip@PlayerMenu@@')
gossip_slots=[slot for slot in range(0,0x290,8) if struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?sGossipSelect'+suffix)]
assert len(gossip_slots)==1
packet=instructions('?HandleGossipSelectOption@WorldSession@@')
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==gossip_slots[0] for x in packet)
hello=instructions('?OnGossipHello@npc_grash@@')
assert sites(hello,'?SetMenuId@GossipMenu@@') and sites(hello,'?ClearMenus@PlayerMenu@@')
data_slots=[slot for slot in range(0,0x290,8) if struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?GetData'+suffix)]
assert len(data_slots)==1
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==data_slots[0] for x in hello)
legacy=instructions('?OnGossipSelect@npc_grash@@')
assert [(x.mnemonic,x.op_str) for x in legacy[:2]]==[('mov','al, 1'),('ret','')]
print('PASS selected Grash applied-heal0x288/empty pre-heal0xb0, native50/90/795 guards, persistent owner/quest lease, only49815+49900 acknowledged receipts/finite retry, AI-owned3s reply and lifecycle clock order; actual raw packet gossip dispatch installs native menu/option admission, separate50161 recruitment and issued upper-wall helper without early49819, role-only removal and inert legacy callback. Original route/final damage source certificate is separate; full client sequence remains pending')
