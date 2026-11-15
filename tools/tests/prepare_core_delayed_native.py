"""Real helper/action dispatch, actual AI deletion and Unit queues, no characters."""
from pathlib import Path
import sys,runpy
from prepare_core_delayed_owners import verify,ROOT
verify();out=Path(sys.argv[2]);runpy.run_path(str(ROOT/'tools/tests/prepare_despawn_native.py'),run_name='__main__')
s=(out/'LayeredNative.cpp').read_text();s=s[:s.index('int main(')]
s=s.replace('{{1220,{}}}','{{1220,{}},{1630,{}}}').replace('{{1220, {}}}','{{1220, {}}, {1630, {}}}')
s+=r'''
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "SmartScript.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
struct CombatTag{using Type=FunctionProcessor Unit::*;friend Type Access(CombatTag);};template struct Accessor<CombatTag,&Unit::m_CombatFunctions>;
static void CombatPump(Creature&c,uint32 ms){(c.*Access(CombatTag{})).Update(ms);}
struct Recorder:NullCreatureAI{using NullCreatureAI::NullCreatureAI;static Creature*last;void JustSummoned(Creature*c)override{Require(!last,"Duplicate actual summon");last=c;}};Creature*Recorder::last=nullptr;
struct SummonSource:ScriptedAI{using ScriptedAI::ScriptedAI;void JustSummoned(Creature*c)override{Require(!Recorder::last,"Duplicate source summon");Recorder::last=c;}};
struct InstanceTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(InstanceTag);};template struct Accessor<InstanceTag,&InstanceMap::i_data>;
struct RejectedInstance:InstanceScript{mutable uint32 checks=0;explicit RejectedInstance(InstanceMap*m):InstanceScript(m){SetBossNumber(1);}bool CheckRequiredBosses(uint32,uint32,Player const* =nullptr)const override{++checks;return false;}};
struct RejectedBoss:BossAI{uint32 evades=0;explicit RejectedBoss(Creature*c):BossAI(c,0){}using BossAI::_EnterCombat;void EnterEvadeMode()override{++evades;}};
int main(int argc,char**argv){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(CrashTrace);try{Init(argc,argv);sObjectMgr->LoadCreatureModelInfo();sObjectMgr->LoadWDBCreatureTemplates();sObjectMgr->LoadCreatureTemplates();sObjectMgr->LoadCreatureClassLevelStats();sObjectMgr->LoadCreatureScalingData();sObjectMgr->LoadCreatureDifficultyStat();HashMapHolder<Creature>::SetSize(20000);
 uint32 cases=0;
 for(uint32 kind=0;kind<3;++kind)for(uint32 mode=0;mode<3;++mode){
  Map map(1220,300,9999800+cases,DIFFICULTY_NONE);Actor actor(&map,299,19500+cases,{5478.f,470.f,150.f,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native actor admission");Require(!actor.GetDBTableGUIDLow(),"Private nonpersistent actor required");uint32 delay=kind==0?123:kind==1?127:2000;
  if(kind==0){auto ai=new SummonSource(&actor);Require(actor.AIM_Initialize(ai),"Native ScriptedAI install");Position pos;pos.Relocate(5479.f,470.f,150.f,1.f);ai->SummonCreatureDelay(delay,299,pos,TEMPSUMMON_TIMED_DESPAWN,5000);pos.m_positionX=999.f;}
  if(kind==1){auto ai=new SmartAI(&actor);Require(actor.AIM_Initialize(ai),"Native SmartAI install");actor.SetReactState(REACT_AGGRESSIVE);SmartScriptHolder action;action.event.type=SMART_EVENT_LINK;action.event.event_chance=100;action.action.type=SMART_ACTION_SET_REACT_STATE;action.action.react.state=REACT_PASSIVE;action.action.react.returnStateTimer=delay;action.action.react.stopAttack=1;ai->GetScript()->ProcessAction(action);Require(actor.GetReactState()==REACT_PASSIVE,"Actual SmartScript immediate state");}
  if(kind==2){actor.SetDefaultMovementType(IDLE_MOTION_TYPE);Require(actor.AIM_Initialize(new CritterAI(&actor)),"Native CritterAI install");}
  if(mode){Require(actor.AIM_Initialize(new Recorder(&actor)),"Actual AI/embedded SmartScript destruction");if(kind==1)actor.SetReactState(REACT_PASSIVE);}
  if(mode==2){if(kind==0)actor.KillAllDelayedCombats();else actor.KillAllDelayedEvents();}
  auto pump=[&](uint32 n){if(kind==0)CombatPump(actor,n);else Pump(actor,n);};pump(delay-1);
  if(kind==0)Require(!Recorder::last,"Summon deadline changed");if(kind==1)Require(actor.GetReactState()==REACT_PASSIVE,"React deadline changed");if(kind==2)Require(actor.GetMotionMaster()->GetCurrentMovementGeneratorType()!=RANDOM_MOTION_TYPE,"Movement deadline changed");
  pump(1);
  if(kind==0){   Require(bool(Recorder::last)==(mode!=2),"Actual summon lost after AI replacement/cancel");if(auto spawned=Recorder::last){Require(spawned->GetEntry()==299&&spawned->ToTempSummon()->GetSummonerGUID()==actor.GetGUID()&&spawned->GetPositionX()==5479.f,"Actual summon source/arguments changed");map.RemoveFromMap(spawned,true);Recorder::last=nullptr;}}
  if(kind==1)Require(actor.GetReactState()==(mode==2?REACT_PASSIVE:REACT_AGGRESSIVE),"Actual original react state not restored");
  if(kind==2)Require((actor.GetMotionMaster()->GetCurrentMovementGeneratorType()==RANDOM_MOTION_TYPE)==(mode!=2),"Actual random movement request lost");
  pump(60000);Require(!Recorder::last,"Actual summon repeated");std::cout<<"CASE native core="<<kind<<" mode="<<mode<<" PASS\n";++cases;
 }
 for(uint32 mode=0;mode<3;++mode){
  InstanceMap map(1630,300,9999850+mode,DIFFICULTY_N_SCENARIO,nullptr);auto instance=new RejectedInstance(&map);map.*Access(InstanceTag{})=instance;Actor actor(&map,299,19550+mode,{-854.89f,1188.86f,114.29f,3.1683f});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Actual instance actor admission");auto ai=new RejectedBoss(&actor);Require(actor.AIM_Initialize(ai),"Native BossAI install");ai->_EnterCombat();Require(instance->checks==1&&ai->evades==1,"Actual admission rejection changed");
  if(mode)Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"Actual BossAI retirement");if(mode==2)actor.KillAllDelayedEvents();Pump(actor,199);Require((actor.*Access(QueueTag{})).SizeQueue()+(actor.*Access(QueueTag{})).Size()==(mode==2?0u:1u),"Actual200ms request not pending");Pump(actor,1);Pump(actor,60000);Require((actor.*Access(QueueTag{})).Empty()&&(actor.*Access(QueueTag{})).SizeQueue()==0,"Actual rejection callback did not settle");std::cout<<"CASE native core=3 mode="<<mode<<" PASS\n";++cases;
 }
 Require(cases==12,"Native matrix incomplete");LoginDatabase.Close();HotfixDatabase.Close();WorldDatabase.Close();std::cout<<"PASS12 native real ScriptedAI summons/SmartScript state/Critter motion/Boss admission callbacks,AI destruction/Unit queues,private actual InstanceMap with empty player list;no Character writes;client/graveyard traversal pending\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
(out/'LayeredNative.cpp').write_text(s,encoding='utf-8')
