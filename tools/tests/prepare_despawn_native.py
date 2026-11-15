"""Run the real linked helper, AI destruction, Unit queue and Creature despawn."""
from pathlib import Path
import sys,runpy
from layered_ten_ten import ROOT
from prepare_despawn_on_respawn import verify
out=Path(sys.argv[2]);saved=sys.argv[:];verify()
sys.argv=['prepare_pending_ai_native.py','loot',str(out),sys.argv[3]]
runpy.run_path(str(ROOT/'tools/tests/prepare_pending_ai_native.py'),run_name='__main__');sys.argv=saved
s=(out/'LayeredNative.cpp').read_text(encoding='utf-8');s=s[:s.index('struct Observe')]
s+=r'''
struct QueueTag{using Type=FunctionProcessor Unit::*;friend Type Access(QueueTag);};template struct Accessor<QueueTag,&Unit::m_Functions>;
static void Pump(Creature& c,uint32 ms){(c.*Access(QueueTag{})).Update(ms);}
int main(int argc,char** argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{Init(argc,argv);sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();HashMapHolder<Creature>::SetSize(20000);
 uint32 cases=0;for(uint32 respawn:{0u,3u,5u})for(uint32 mode:{0u,1u,2u,3u}){
  Map map(1220,300,9999940+cases,DIFFICULTY_NONE);Actor actor(&map,299,19000+cases,{5478.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Actual actor map admission failed");Require(!actor.GetDBTableGUIDLow()&&actor.IsAlive(),"Private nonpersistent alive actor required");auto before=actor.GetRespawnTime();actor.AI()->DespawnOnRespawn(respawn);
  UnitAI* suspended=nullptr;bool enabled=actor.IsAIEnabled;
  if(mode==1)Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Actual source AI replacement failed");
  if(mode==2){actor.KillAllDelayedEvents();Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Cancelled AI replacement failed");}
  if(mode==3){suspended=actor.GetAI();actor.SetAI(nullptr);actor.IsAIEnabled=false;}
  Pump(actor,99);Require(actor.IsAlive()&&actor.GetRespawnTime()==before,"Native100ms deadline changed");Pump(actor,1);
  if(suspended){actor.SetAI(suspended);actor.IsAIEnabled=enabled;}
  if(mode==2)Require(actor.IsAlive()&&actor.GetRespawnTime()==before,"Cancelled native request fired");
  else Require(actor.getDeathState()==DEAD&&actor.GetRespawnTime()==(respawn?GameTime::GetGameTime()+respawn:0),"Actual Creature despawn/respawn argument differs");
  auto state=actor.getDeathState();auto time=actor.GetRespawnTime();Pump(actor,60000);Require(actor.getDeathState()==state&&actor.GetRespawnTime()==time,"Native callback repeated");
  std::cout<<"CASE native respawn="<<respawn<<" mode="<<mode<<" PASS\n";++cases;
 }
 Require(cases==12,"Native matrix incomplete");LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS12 native linked CreatureAI::DespawnOnRespawn,real AI replacement/Unit queue/Creature death-corpse removal and respawn state;zero DB spawn GUID/no Character writes,100ms and cancellation retained;full encounters/client pending\n";return 0;}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
(out/'LayeredNative.cpp').write_text(s,encoding='utf-8')
