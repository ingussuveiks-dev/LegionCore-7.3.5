"""Selected shipped brewing factories/hooks and native receipt/cleanup paths."""
from pathlib import Path
import hashlib
import json
import re
import runpy
import struct
from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import floats,fields

prior=runpy.run_path(str(Path(__file__).with_name('verify_fuzan_wave_entry_release.py')))
n,require,sites,values=prior['n'],prior['require'],prior['sites'],prior['values']
references=prior['prior']['prior']['references']
registration=n['instructions']('?AddSC_instance_temple_of_the_jade_serpent@@')
installed=prior['installed']
suffix='@Actor@FuZanBrewing@@'
finder=require('?Find@FuZanBrewing@@',('?GetCreature@Map@@','?GetUInt32Value@Object@@'),(217304,))
fire=require('?GossipUse@go_fuzan_brewing_fire@@',('?Contact@FuZanBrewing@@','?Find@FuZanBrewing@@',
    '?Begin'+suffix,'?CastSpell@Unit@@','?KillCreditGO@Player@@','?HasAuraType@Unit@@'),(251490,128074,217297))
begin=require('?Begin'+suffix,('?Contact@FuZanBrewing@@','?AddQuestDelayedEvent@Player@@',
    '?GetContactPoint@WorldObject@@','?UpdateGroundPositionZ@WorldObject@@','?ShouldHideFor@WorldObject@@',
    '?MovePoint@MotionMaster@@'),(42868,217302,217321,217332,221291,15000))
current=require('?Current'+suffix,('?getQuestStatus@Player@@','?Active@FuZanBrewing@@','??8ObjectGuid@@'),(42868,217302))
ticket=require('?Ticket'+suffix,('?Current'+suffix,'?Contact@FuZanBrewing@@','?GetGameObject@Map@@'))
release=require('?Release@spell_fuzan_brewing_ingredient@@',('?Ticket'+suffix,'?Failure'+suffix,'?CastSpell@Unit@@'),(2500,))
delivery=require('?Delivery'+suffix,('?Current'+suffix,'?CanContact@Player@@','?IsWithinDistInMap@WorldObject@@'),(5,))
hit=require('?Hit@spell_fuzan_brewing_receipt@@',('?GetHitCreature@SpellScript@@','?Delivery'+suffix,
    '?GetPower@Unit@@','?CastSpell@Unit@@','?GetQuestStatus@Player@@','?Stop'+suffix),(217335,42868,115359,5515))
assert len(sites(n,hit,'?CastSpell@Unit@@'))==1,'One actual selected-owner217335 energize'
assert 60.0 in floats(n,delivery) and {0xa18,0x6848,0x6849,0xe8}<=fields(delivery),'Final receipt retains native alive/teleport/distance/nonce guards'
energize=sites(n,hit,'?CastSpell@Unit@@')[0]
alive=[i for i,op in enumerate(delivery) if op.mnemonic=='cmp' and '0xa18]' in op.op_str and op.op_str.endswith(', 0')]
assert len(alive)==1 and delivery[alive[0]+1].mnemonic=='jne'
rejected=next(i for i,op in enumerate(delivery) if op.address==delivery[alive[0]+1].operands[0].imm)
assert ('xor','al, al') in [(op.mnemonic,op.op_str) for op in delivery[rejected:rejected+4]],'Dead-owner helper returns false'
gate=sites(n,hit,'?Delivery'+suffix)
assert len(gate)==1 and gate[0]<energize and hit[gate[0]+1].op_str=='al, al' and hit[gate[0]+2].mnemonic=='je' and hit[gate[0]+2].operands[0].imm>hit[energize].address,'Rejected delivery skips the actual native energize'
load=require('?Load@spell_fuzan_brewing_delivery@@',('?Current'+suffix,'??8ObjectGuid@@'))
before=require('?Before@spell_fuzan_brewing_delivery@@',('?Delivery'+suffix,'??8ObjectGuid@@','?PreventHitEffect@SpellScript@@'))
launch=require('?Launch@spell_fuzan_brewing_delivery@@',('?Delivery'+suffix,'?PreventHitEffect@SpellScript@@'))
restore=require('?Restore'+suffix,('?GetCreature@Map@@',),(109187,366254))
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
creature_table=re.findall(r'\s+\w+:\w+\s+\?\?_7Creature@@6BUnit@@@\s+([0-9a-fA-F]{16})',maptext)
assert len(creature_table)==1
creature_slots=struct.unpack_from('<128Q',n['binary'],n['offset'](int(creature_table[0],16)))
visibility=n['symbol']('?UpdateObjectVisibility@Unit@@')
visibility_slots=[i for i,address in enumerate(creature_slots) if n['base']<=address<n['base']+0x10000000 and n['resolve'](address)==visibility]
assert len(visibility_slots)==1 and any(op.mnemonic in ('call','jmp') and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==visibility_slots[0]*8 for op in restore),'Native Creature/Unit visibility virtual dispatch'
stop=require('?Stop'+suffix,('?Current'+suffix,'?Restore'+suffix,'?RemoveChildren@FuZanBrewing@@','?RemoveAurasDueToSpell@Unit@@'),(217302,))
before_remove=require('?BeforeRemove@aura_fuzan_brewing_controller@@',('?GetUnitOwner@AuraScript@@','?Restore'+suffix),(217302,))
leave=require('?Leave@FuZanBrewing@@',('?Find@FuZanBrewing@@','?Stop'+suffix,'?RemoveChildren@FuZanBrewing@@'),(870,10,217302))
speech=require('?Say@FuZanBrewing@@',('?IsPlayerMeetingCondition@ConditionMgr@@','?PlayDirectSound@WorldObject@@'))
assert len(sites(n,speech,'?PlayDirectSound@WorldObject@@'))==1,'One owner-targeted native sound'

factories=[]
for category,name in (('Creature','npc_fuzan_brewing'),('Creature','npc_fuzan_brewing_questgiver'),
                      ('GameObject','go_fuzan_brewing_fire'),('GameObject','go_fuzan_brewing_ingredient')):
    prefix='?GetAI@?$Generic'+category+'Script@U'+name+'@@'
    assert n['symbol'](prefix) in installed,('Actual AI factory registration',name)
    factories.append(prefix)
hooks={
 'spell_fuzan_brewing_summon':['Destination'],
 'spell_fuzan_brewing_ingredient':['Check','Release'],
 'spell_fuzan_brewing_delivery':['Launch','Before','After'],
 'spell_fuzan_brewing_receipt':['Hit'],
 'aura_fuzan_brewing_controller':['Update','BeforeRemove','Remove']
}
for name,methods in hooks.items():
    aura=name.startswith('aura_')
    prefix=('?GetAuraScript@?$GenericAuraScriptLoader@V' if aura else '?GetSpellScript@?$GenericSpellScriptLoader@V')+name+'@@'
    assert n['symbol'](prefix) in installed,('Actual spell/aura factory registration',name)
    body=n['instructions'](prefix);register=n['symbol']('?Register@'+name+'@@')
    tables=[]
    for address in references(body):
        try:
            slots=struct.unpack_from('<16Q',n['binary'],n['offset'](address))
            if register in [n['resolve'](value) if n['base']<=value<n['base']+0x10000000 else value for value in slots]:tables.append(address)
        except (ValueError,struct.error):pass
    assert tables,('Constructed script vtable has no Register hook',name)
    callbacks=references(n['instructions']('?Register@'+name+'@@'))
    for method in methods:assert n['symbol']('?'+method+'@'+name+'@@') in callbacks,('Actual callback binding',name,method)
    factories.append(prefix)
for name,method in (('map_fuzan_brewing','OnPlayerLeave'),('player_fuzan_brewing','OnBeforeLogout')):
    tables=re.findall(r'\s+\w+:\w+\s+\?\?_7'+name+r'@@6B\S*\s+([0-9a-fA-F]{16})',maptext)
    target=n['symbol']('?'+method+'@'+name+'@@');matched=False
    for text in tables:
        address=int(text,16)
        if address not in references(registration):continue
        slots=struct.unpack_from('<96Q',n['binary'],n['offset'](address))
        if target in [n['resolve'](value) if n['base']<=value<n['base']+0x10000000 else value for value in slots]:matched=True
    assert matched,('Actual map/logout vtable registration',name)
for body in (finder,fire,begin,current,ticket,release,delivery,hit,load,before,launch,restore,stop,before_remove,leave):
    for target in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?SetPower@Unit@@','?SummonCreature@WorldObject@@','?AddItem@Player@@'):
        assert not sites(n,body,target),('No manufactured final progress, power or summon',target)
print('PASS linked brewing:9 actual factories/callbacks, map/logout vtables, scoped native starter and first GO receipt, owned request/delivery/receipt guards, selected-owner217335 only, native power/quest readback, personal speech, pre-removal visibility restoration and cleanup; no direct final credit/power/summon grants.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'brewing_factories':len(factories),'lifecycle_registrations':2}))
