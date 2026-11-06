"""Select the installed per-entry Fury loading/descent/finish controller."""
from pathlib import Path
import runpy,struct,re
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native,instructions=n['native'],n['instructions']
if not any(name.startswith('?Tick@State@WarswordsArrivalRecovery@@') for name in native['symbols']):
    historical=runpy.run_path(str(Path(__file__).with_name('verify_warswords_arrival_legacy_release.py')))
    latch=historical['latch']
else:
    def needs(caller,dependency):
        assert any(name.startswith(dependency) and native['resolve'](address) in native['calls'](caller)
                   for name,address in native['symbols'].items()),caller+' missing '+dependency
    suffix='@instance_warswords_InstanceMapScript@instance_warswords@@'
    tick='?Tick@State@WarswordsArrivalRecovery@@'
    for caller,dependencies in (
        ('?OnPlayerEnter'+suffix,('?Enter@WarswordsArrivalRecovery@@',)),
        ('?OnPlayerLeave'+suffix,('?Forget@WarswordsArrivalRecovery@@',)),
        ('?OnUpdate@player_warswords_arrival_recovery@@',(tick,)),
        (tick,('?Valid@State@WarswordsArrivalRecovery@@','?CanContact@Player@@','?IsMapObjectArrivalProtected@Player@@','?CastSpell@Unit@@','?MoveJump@MotionMaster@@QEAAXMMMMMM','?CreateConversation@Unit@@','?SetUnitTarget@SpellCastTargets@@')),
        ('?Enter@WarswordsArrivalRecovery@@',('?Context@WarswordsArrivalRecovery@@','?BeginMapObjectArrival@Player@@')),
        ('?Valid@State@WarswordsArrivalRecovery@@',('?Context@WarswordsArrivalRecovery@@','?GetQuestCancellationReceipt@Player@@')),
        ('?AirEntrance@WarswordsArrivalRecovery@@',('?GetHeight@Map@@',)),
        ('?OnBeforeLogout@player_warswords_arrival_recovery@@',('?Forget@WarswordsArrivalRecovery@@',))):
        for dependency in dependencies:needs(caller,dependency)
    calls=native['calls'](tick)
    for forbidden in ('?TeleportTo@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@'):
        assert not any(name.startswith(forbidden) and native['resolve'](address) in calls for name,address in native['symbols'].items())
    values={v.imm for x in instructions(tick) for v in x.operands if v.type==CS_OP_IMM}
    assert {205534,189695,205744,838,5000,2000,1000,0x574152} <= values
    ready=instructions('?SetData'+suffix)
    writes=[x for x in ready if x.mnemonic=='mov' and len(x.operands)==2 and x.operands[0].type==CS_OP_MEM
            and x.operands[0].size==1 and x.operands[0].mem.disp>=0x200]
    assert len(writes)==1 and any(x.op_str=='r8d, 1' and x.mnemonic=='cmp' for x in ready)
    assert any(x.op_str=='edx, 0x574152' and x.mnemonic=='cmp' for x in ready)
    latch=writes[0].operands[0].mem.disp
    needs('?SetData'+suffix,'?Start@State@WarswordsWaves@@')
    step=instructions('?onScenarionNextStep'+suffix)
    assert any(x.mnemonic=='cmp' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==latch for x in step)
    selected=[]
    for x in instructions('?GetInstanceScript@instance_warswords@@'):
        for v in x.operands:
            if x.mnemonic=='lea' and v.type==CS_OP_MEM and v.mem.base==X86_REG_RIP:
                offset=native['offset'](x.address+x.size+v.mem.disp);table=struct.unpack_from('<90Q',native['binary'],offset)
                if all(native['symbol']('?'+method+suffix) in table for method in ('OnPlayerEnter','OnPlayerLeave','SetData','Update')):selected.append(table)
    assert len(selected)==1
    maptext=(native['args'].release/'worldserver.map').read_text(errors='replace')
    # Player::CanFly is a virtual call; Unit's declaration is pure virtual.
    flight=native['symbol']('?CanFly@Player@@')
    flight_slots=[]
    for address in re.findall(r'\s+\w+:\w+\s+\?\?_7Player@@\S+\s+([0-9a-fA-F]{16})',maptext):
        offset=native['offset'](int(address,16));table=struct.unpack_from('<128Q',native['binary'],offset)
        flight_slots += [i*8 for i,value in enumerate(table) if native['resolve'](value)==flight]
    assert flight in native['calls']('?AirEntrance@WarswordsArrivalRecovery@@') or any(
        x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp in flight_slots
        for x in instructions('?AirEntrance@WarswordsArrivalRecovery@@'))
    tables=re.findall(r'\s+\w+:\w+\s+\?\?_7player_warswords_arrival_recovery@@6B@\s+([0-9a-fA-F]{16})',maptext)
    assert len(tables)==1
    offset=native['offset'](int(tables[0],16));slots=struct.unpack_from('<45Q',native['binary'],offset)
    assert native['resolve'](slots[44])==native['symbol']('?OnBeforeLogout@player_warswords_arrival_recovery@@')
    assert native['symbol']('?OnUpdate@player_warswords_arrival_recovery@@') in slots
    print('PASS selected native Fury instance queues per-entry recovery, pre-create bounded native hold, real CanContact/receipt/spline/finish acknowledgement and explicit self+destination; registered PlayerScript updates/pre-save cleanup and late wave bootstrap retained. No teleport/progression/item grants. Source/native/client evidence separate.')
