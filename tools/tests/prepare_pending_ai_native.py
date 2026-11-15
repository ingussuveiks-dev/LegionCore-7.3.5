"""Actual AI replacement and Unit queue engine; controlled admission payload."""
from pathlib import Path
import sys,runpy
from layered_ten_ten import ROOT
out=Path(sys.argv[2]);sql=sys.argv[3]
# Reuse the private read-only pools/native catalog bootstrap. Its six GO shadow
# inputs remain connection-local and it never opens a Character database.
saved=sys.argv[:];sys.argv=['prepare_tail_loot_native.py','loot',str(out),sql]
runpy.run_path(str(ROOT/'tools/tests/prepare_tail_loot_native.py'),run_name='__main__')
sys.argv=['prepare_pending_ai.py',str(out)];runpy.run_path(str(ROOT/'tools/tests/prepare_pending_ai.py'),run_name='__main__');sys.argv=saved
s=(out/'LayeredNative.cpp').read_text(encoding='utf-8');s=s[:s.index('int main(')]
raw=(ROOT/'tools/tests/RockawayChannelNative.cpp').read_text(encoding='utf-8');a=raw.index('struct Actor : Creature');b=raw.index('\n};',a)+3
actor=raw[a:b].replace('GetEntry() == 115133 ? static_cast<CreatureAI*>(new npc_rockaway_vision(this)) : new NullCreatureAI(this)','new NullCreatureAI(this)')
body=r'''
struct Observe{static uint32 calls;static Unit* actor;static void Mark(Unit* u,uint32){++calls;actor=u;}};uint32 Observe::calls=0;Unit* Observe::actor=nullptr;
struct QueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(QueueTag);};template struct Accessor<QueueTag,&Unit::m_Functions>;
static void Pump(Creature& c,uint32 ms){(c.*Access(QueueTag{})).Update(ms);}
#include "PendingAIScripts.inc"
template<class F>static void Check(Creature& actor,uint32 index){
 auto queue=[&](){auto frame=new F(&actor,&actor);Require(actor.AIM_Initialize(frame),"Actual AI installation failed");frame->Queue(index);};
 Observe::calls=0;queue();Pump(actor,122);Require(!Observe::calls,"Controlled native admission deadline");Pump(actor,1);Require(Observe::calls==1&&Observe::actor==&actor,"Native live admission lost");Pump(actor,60000);Require(Observe::calls==1,"Duplicate admitted callback");
 Observe::calls=0;queue();Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Native retirement failed");Pump(actor,123);Require(!Observe::calls,"Retired AI entered pending payload");
 queue();actor.KillAllDelayedEvents();Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Native cancellation replacement failed");Pump(actor,60000);Require(!Observe::calls,"Cancelled native queue fired");
 queue();auto prior=actor.GetAI();bool enabled=actor.IsAIEnabled;actor.SetAI(nullptr);actor.IsAIEnabled=false;Pump(actor,123);actor.SetAI(prior);actor.IsAIEnabled=enabled;Require(Observe::calls==1,"Temporarily inactive surviving AI lost admission");
 Observe::calls=0;actor.Unit::setDeathState(CORPSE);queue();Pump(actor,123);Require(Observe::calls==1,"Living AI on corpse lost its outro admission");actor.Unit::setDeathState(ALIVE);actor.SetFullHealth();Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Final native AI cleanup failed");
}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{Init(argc,argv);sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();HashMapHolder<Creature>::SetSize(20000);
 {Map map(1220,300,9999966,DIFFICULTY_NONE);Actor actor(&map,299,19001,{5478.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Actor map admission failed");
'''
import json
plan=json.loads((ROOT/'tools/tests/PendingAILifetime20261115.json').read_text())
for e in plan:
 body+='for(uint32 i=0;i<'+str(len(e['callbacks']))+';++i)Check<PendingAI::'+e['key']+'::Frame>(actor,i);std::cout<<"CASE AI '+e['cls']+' PASS\\n";\n'
body+='''}LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS2 native actual Creature::AIM_Initialize destruction and Unit queues,4 selected admissions/live/retired/cancelled/inactive/corpse; controlled payload only, original encounter bodies not executed or client-certified\\n";return 0;}catch(std::exception const& e){std::cerr<<e.what()<<'\\n';return 1;}}'''
(out/'LayeredNative.cpp').write_text(s+'\n'+actor+'\n'+body,encoding='utf-8')
