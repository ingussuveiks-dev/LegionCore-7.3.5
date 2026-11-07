// Actual early loader/Prepare/CanResume and binding guard with controlled
// source/quest endpoints. The owned-claim exit has its own native regression.
#define main WarriorSourceUnusedMain
#include "ScenarioResumeTest.cpp"
#undef main
#ifndef SURVIVAL_LOGIN_SOURCE_BASELINE
static void Binding(Player& p){
 auto map=p.GetMap();auto mapId=p.GetMapId();auto& m_scenarioResume=p.m_scenarioResume;auto& m_scenarioLoginId=p.m_scenarioLoginId;auto& m_scenarioLoginInstance=p.m_scenarioLoginInstance;
 auto getClass=[&]{return p.getClass();};auto SetScenarioId=[&](uint16 id){p.SetScenarioId(id);};
#include "SurvivalBinding.inc"
 (void)map;(void)mapId;(void)m_scenarioResume;(void)m_scenarioLoginId;(void)m_scenarioLoginInstance;(void)getClass;(void)SetScenarioId;
}
#endif
int main(){unsigned cases=0;
 for(unsigned team:{0u,1u})for(unsigned kind=0;kind<34;++kind){
  Player p;p.cls=CLASS_HUNTER;p.team=team;p.acceptedQuest=40385;p.map.entry.ID=1511;p.map.instance=p.instance;
  Scenario live{&p.map};live.id=973;live.step=3;scenarios.entries.clear();scenarios.entries[p.instance]=&live;scenarios.missingSteps=false;
  sMapStore.value.ID=1511;sMapStore.missing=sScenarioStore.missing=false;sDB2Manager.missing=false;
  objects.missing=false;objects.data={};objects.data.ScenarioID=973;objects.data.Class=CLASS_HUNTER;objects.data.Team=0;objects.data.LfgDungeonID=1178;
  lfgManager.missing=false;lfgManager.shared=false;lfgManager.fallbackAlways=true;lfgManager.data={};lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.data.entry.ScenarioID=973;lfgManager.data.map=1511;lfgManager.data.id=1178;
  spellManager.targets.clear();SpellTargetPosition target{1511,3364.77f,1919.54f,0.092994f,5.69981f};spellManager.targets[198950]=target;
  bool existing=kind==5||kind==6||kind==7||kind==24||kind==25||kind==26||kind==27||kind==29;
  if(existing)p.m_scenarioResume={1511,973,p.instance,1000,101,202,30,1};
  if(kind==1)assert(!p.m_scenarioResume.Read("malformed"));if(kind==2)p.m_scenarioResume={1511,944,p.instance,1000,1,2,3,0};if(kind==3)p.m_scenarioResume={1539,973,p.instance,1000,1,2,3,0};if(kind==4)p.m_scenarioResume={1511,973,p.instance,1000,std::numeric_limits<float>::quiet_NaN(),2,3,0};
  if(kind==6||kind==25)p.m_scenarioResume.boot=999;
  if(kind==7||kind==8)p.holder.status=QUEST_STATUS_COMPLETE;if(kind==9)p.holder.status=0;if(kind==10)p.cls=CLASS_WARLOCK;if(kind==11){p.map.entry.ID=sMapStore.value.ID=1539;}
  if(kind==12)p.m_atLoginFlags=AT_LOGIN_UNLOCK;if(kind==13)p.validPosition=false;if(kind==14)spellManager.targets.clear();if(kind==15)spellManager.targets[198950].target_mapId=1495;if(kind==16){auto& t=spellManager.targets[198950];t.target_X=t.target_Y=t.target_Z=0;}if(kind==17)spellManager.targets[198950].target_X=std::numeric_limits<float>::quiet_NaN();if(kind==18)spellManager.targets[198950].target_Orientation=std::numeric_limits<float>::infinity();if(kind==19)spellManager.targets[198950].target_X=20000;
  if(kind==20)p.acceptedQuest=40043;if(kind==30)scenarios.entries.clear();if(kind==31)lfgManager.missing=true;if(kind==32)scenarios.missingSteps=true;if(kind==33)sScenarioStore.missing=true;
  auto loaded=p.earlyWarrior();p.normalQuest(loaded);assert(p.loads==1);
  if(kind==21||kind==26)p.rewarded=true;if(kind==22||kind==27)p.questStatus=0;if(kind==23||kind==24||kind==25)p.claim=1;
  bool seeded=kind<=4||kind==21||kind==22||kind==23||kind==28||kind==30;
#ifdef SURVIVAL_LOGIN_SOURCE_BASELINE
  if(seeded)assert(!p.m_scenarioLoginId&&!loaded);
  if(existing)assert(p.m_scenarioLoginId==973);
  bool allowed=existing;
#else
  if(seeded)assert(p.m_scenarioLoginId==973&&loaded&&p.m_scenarioResume.instance==0&&p.m_scenarioResume.boot==0&&p.pos.x==target.target_X&&p.pos.y==target.target_Y&&p.pos.z==target.target_Z);
  if(existing)assert(p.m_scenarioLoginId==973&&!loaded);
  if(kind==28||kind==29||kind==30)Binding(p);
  if(kind==28)assert(!p.m_scenarioLoginId&&!p.m_scenarioResume.scenario&&!p.selector);
  if(kind==29)assert(p.m_scenarioLoginId==973&&p.m_scenarioLoginInstance==p.instance);
  if(kind==30)assert(p.m_scenarioLoginId==973&&!p.m_scenarioLoginInstance);
  bool allowed=(seeded||existing)&&kind!=21&&kind!=22&&kind!=23&&kind!=25&&kind!=26&&kind!=27&&kind!=28;
#endif
  if(!seeded&&!existing)assert(!p.m_scenarioLoginId);
  assert(p.CanResumeScenarioLogin(973)==allowed&&live.step==3);
  if(existing)assert(p.m_scenarioResume.x==101);
  ++cases;
 }
 assert(cases==68);
#ifdef SURVIVAL_LOGIN_SOURCE_BASELINE
 std::cout<<"COUNTERFACTUAL pinned0af584162 source Hunter68:missing checkpoints not seeded,claim/reward/abandonment admission remains old;controlled services,not ordinary gameplay"<<std::endl;
#else
 std::cout<<"PASS extracted Hunter early login68 ASAN:both teams,missing/malformed/foreign receipt,active native source,one quest load,valid live/fresh/complete preservation,source/unlock/map/class refusals,late reward/abandon/claim and live-binding safeguards;services controlled"<<std::endl;
#endif
 return 0;
}
