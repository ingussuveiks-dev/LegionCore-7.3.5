"""Exercise cancellation/rearm through actual Unit queues and Creature helper."""
from pathlib import Path
import sys,runpy
from layered_ten_ten import ROOT
out=Path(sys.argv[2]);runpy.run_path(str(ROOT/'tools/tests/prepare_despawn_native.py'),run_name='__main__')
p=out/'LayeredNative.cpp';s=p.read_text(encoding='utf-8');marker=' Require(cases==12,"Native matrix incomplete");'
extra=r'''
 struct CombatQueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(CombatQueueTag);};
'''
# Accessor must be instantiated at namespace scope, not inside main.
s=s.replace('int main(', 'struct CombatQueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(CombatQueueTag);};template struct Accessor<CombatQueueTag,&Unit::m_CombatFunctions>;\nint main(',1)
extra=r'''
 for(uint32 mode=0;mode<6;++mode){
  Map map(1220,300,9999920+mode,DIFFICULTY_NONE);Actor actor(&map,299,19100+mode,{5478.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Actual cancellation actor admission failed");Require(!actor.GetDBTableGUIDLow(),"Nonpersistent fixture required");uint32 stale=0,fresh=0;
  if(mode<2){actor.AddDelayedEvent(100,[&]{++stale;});if(mode==1)Pump(actor,1);actor.KillAllDelayedEvents();actor.AI()->DespawnOnRespawn(3);Pump(actor,99);Require(actor.IsAlive()&&!stale,"Native rearm deadline changed");Pump(actor,1);Require(actor.getDeathState()==DEAD&&actor.GetRespawnTime()==GameTime::GetGameTime()+3&&!stale,"Native cancel lost fresh despawn generation");}
  if(mode==2){actor.AddDelayedEvent(0,[&]{actor.KillAllDelayedEvents();});actor.AI()->DespawnOnRespawn(3);Pump(actor,100);Require(actor.IsAlive(),"Old due despawn ran after native callback cancellation");}
  if(mode==3){actor.AddDelayedEvent(0,[&]{actor.KillAllDelayedEvents();actor.AI()->DespawnOnRespawn(3);});actor.AddDelayedEvent(1,[&]{++stale;});Pump(actor,10);Require(actor.IsAlive()&&!stale,"Native stale callback survived cancellation");Pump(actor,99);Require(actor.IsAlive(),"Native rearm delay shortened");Pump(actor,1);Require(actor.getDeathState()==DEAD&&actor.GetRespawnTime()==GameTime::GetGameTime()+3,"Native callback rearm lost fresh generation");}
  if(mode==4){actor.AddDelayedEvent(0,[&]{++fresh;Require(fresh==1,"Native currently executing callback replayed");Pump(actor,0);});Pump(actor,0);Require(fresh==1,"Native reentrant dispatch differs");}
  if(mode==5){actor.AddDelayedCombat(0,[&]{++fresh;});actor.AddDelayedEvent(0,[&]{++stale;});actor.KillAllDelayedEvents();Pump(actor,0);(actor.*Access(CombatQueueTag{})).Update(0);Require(fresh==1&&!stale,"Ordinary cancellation changed independent combat queue");}
  std::cout<<"CASE native cancel="<<mode<<" PASS\n";
 }
'''
assert marker in s;s=s.replace(marker,extra+marker,1).replace('PASS12 native linked CreatureAI::DespawnOnRespawn','PASS18 native linked helper and FunctionProcessor cancellation/rearm');p.write_text(s,encoding='utf-8')
