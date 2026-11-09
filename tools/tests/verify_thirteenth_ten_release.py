"""Linked native selector and named callback lifetime/map/boarding guards."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM
p=runpy.run_path(str(Path(__file__).with_name('verify_twelfth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
def callback_names(cls,method):return [s for s in n['symbols']if s.startswith('?_Do_call@')and cls in s and method in s]
def expanded(name):
 rows=ins(name);result=list(rows)
 for x in rows:
  if x.mnemonic not in ('jmp','call')or not x.op_str.startswith('0x'):continue
  target=n['resolve'](int(x.op_str,16));matches=[s for s,a in n['symbols'].items()if a==target and s.startswith('??R<lambda')]
  if matches:result+=ins(matches[0])
 return result
def has_guard(rows,at):return any(x.mnemonic=='test'and x.op_str=='rax, rax' and rows[i+1].mnemonic in ('je','jne')for i,x in enumerate(rows)if at<i<min(at+5,len(rows)-1))
def lookup_callbacks(cls,method,lookup,count,direct=False):
 found=[]
 for name in callback_names(cls,method):
  rows=ins(name)if direct else expanded(name);calls=sites(rows,lookup)
  if not calls:continue
  assert any(has_guard(rows,at)for at in calls),(cls,method,'resolved-object guard');found.append(rows)
 assert len(found)==count,(cls,method,len(found),count)
 return found
r=ins('?FilterTargets@spell_failure_detection_pylon@@');assert any(x.mnemonic=='cmp'and x.op_str.endswith('+ 0x16], 4')for x in r),'Pylon player type guard';assert any(x.mnemonic=='test'and x.op_str=='rcx, rcx'for x in r)
lookup_callbacks('boss_demonic_inquisitionAI','UpdateAI','?GetUnit@ObjectAccessor@@',1)
lookup_callbacks('boss_guldanAI','JustSummoned','?GetCreature@ObjectAccessor@@',1)
bounds=lookup_callbacks('boss_guldanAI','SpellHitTarget','?GetUnit@ObjectAccessor@@',1)[0];assert 104252 in values(bounds),'original bounds summon entry retained'
lookup_callbacks('npc_atheris_voguesong','', '?GetPlayer@ObjectAccessor@@',9,True)
lookup_callbacks('npc_hearthstation','IsSummonedBy','?GetUnit@ObjectAccessor@@',1)
lookup_callbacks('npc_hearthstation','SpellFinishCast','?GetUnit@ObjectAccessor@@',1)
lookup_callbacks('npc_infant_spider','IsSummonedBy','?GetUnit@ObjectAccessor@@',1)
assert 1000 in values(ins('?GetAI@?$GenericCreatureScript@Unpc_hearthstation@@@@')),'initialized orphan timer in actual AI factory'
def map_guards(rows):
 assert sites(rows,'?IsInWorld@Object@@')
 offsets={o.mem.disp for x in rows if x.mnemonic=='cmp'for o in x.operands if o.type==CS_OP_MEM}
 assert {0xe0,0x3b8}<=offsets,'origin map and instance compared before delayed payload'
seat,=callback_names('instance_seat_of_the_triumvirate_InstanceMapScript','OnPlayerEnter');r=expanded(seat);map_guards(r);create,=sites(r,'?CreateConversation@Conversation@@');assert r[create+1].mnemonic=='test'and r[create+2].mnemonic=='je';assert 6054 in values(r)
timer=[expanded(s)for s in callback_names('instance_trial_of_style_InstanceMapScript','onScenarionNextStep')];timer=[r for r in timer if sites(r,'?IsInWorld@Object@@')];assert len(timer)==1;map_guards(timer[0]);assert 57838 in values(timer[0])
dragon=lookup_callbacks('npc_invasion_stormheim_dragonsAI','PassengerBoarded','?GetPlayer@ObjectAccessor@@',2)
for r in dragon:assert sites(r,'?GetVehicleBase@Unit@@')and op(r,'mov','rax, qword ptr [rdx + 0x40]')and op(r,'cmp','qword ptr [rcx + 0x20], rax'),'generation guard in both stages'
assert any(3000 in values(r)and sites(r,'?SetClientControl@Player@@')for r in dragon)
assert any(10993814 in values(r)and op(r,'setne','dl')and sites(r,'?MovePath@MotionMaster@@')for r in dragon),'original route variants'
r=ins('?PassengerBoarded@npc_invasion_stormheim_dragonsAI@');assert op(r,'test','r9b, r9b')and sites(r,'?SetClientControl@Player@@')and 2000 in values(r);assert op(r,'mov','r8b, 1')and sum(x.mnemonic=='inc'and x.op_str=='qword ptr [rbx + 0x40]'for x in r)==2,'remove restores control and invalidates generation'
print('PASS actual Release thirteenth ten:Pylon type/null admission;Quills GUID,Guldan owned Hellfire/Bounds lookups,nine greeting callbacks,two Hearthstation lookups plus initialized1000ms timer,Spider lookup,Seat/Style map-instance guards and successful-create commit,actual dragon two-stage generation/vehicle guards and control restore/original2s+3s/routes. Prior127 selected paths retained;payload/client scopes remain separate.')
