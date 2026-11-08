#include <cstring>
static unsigned normalized=0,refused=0;
template<class... T>static void Observe(char const*,char const* text,T...){if(std::strstr(text,"normalized legacy Sharasdal"))++normalized;if(std::strstr(text,"refusing unsafe legacy Sharasdal"))++refused;}
#define TC_LOG_INFO(...) Observe(__VA_ARGS__)
#define main SharedResumeUnusedMain
#include "ScenarioResumeTest.cpp"
#undef main
static ScenarioResume Legacy(){return {1600,1066,42,999,836.16f,1035.4f,48.61f,1.7935f};}
static void Setup(Player& p,Scenario& live,unsigned team,int status){
 normalized=refused=0;p=Player{};p.cls=CLASS_SHAMAN;p.team=team;p.questStatus=status;p.acceptedQuest=40341;p.map.entry.ID=1600;p.m_scenarioResume=Legacy();live={&p.map};live.id=1066;live.step=3;
 scenarios.entries.clear();scenarios.entries[42]=&live;scenarios.missingSteps=false;sMapStore.value.ID=1600;sMapStore.missing=sScenarioStore.missing=false;sDB2Manager.missing=false;
 objects.missing=false;objects.data={1066,0,7,12,1229,1600};sWorldSafeLocsStore={};
 lfgManager.missing=lfgManager.shared=false;lfgManager.fallbackAlways=true;lfgManager.data={};lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.data.entry.ScenarioID=0;lfgManager.data.map=1600;lfgManager.data.entry.ID=1229;lfgManager.data.entry.MapID=1600;lfgManager.data.id=1229;
 lfgManager.data.x=-629.42f;lfgManager.data.y=808.68f;lfgManager.data.z=247.67f;lfgManager.data.o=6.2797f;spellManager.targets.clear();spellManager.targets[207683]={1600,-629.42f,808.68f,247.67f,6.2797f};
}
#ifndef SHARASDAL_LEGACY_SOURCE_BASELINE
static bool Correct(ScenarioResume const& r){auto& d=lfgManager.data;return std::fabs(r.x-d.x)<0.001f&&std::fabs(r.y-d.y)<0.001f&&std::fabs(r.z-d.z)<0.001f&&std::fabs(r.o-d.o)<0.001f;}
static bool AtEntry(Player const& p){auto& d=lfgManager.data;return std::fabs(p.pos.x-d.x)<0.001f&&std::fabs(p.pos.y-d.y)<0.001f&&std::fabs(p.pos.z-d.z)<0.001f;}
#endif
int main(){unsigned cases=0;
 for(unsigned team:{0u,1u})for(int status:{3,1})for(unsigned mode=0;mode<14;++mode){Player p;Scenario live{};Setup(p,live,team,status);
  if(mode==1||mode==3||mode==4||mode==5)p.m_scenarioResume.boot=1000;
  if(mode==3||mode==4)p.pos={836.16f,1035.4f,mode==4?-100.0f:48.61f,0.2f};
  if(mode==5)live.completed=true;
  if(mode==9){auto& d=lfgManager.data;p.m_scenarioResume={1600,1066,42,999,d.x,d.y,d.z,d.o};}
  if(mode==10)p.m_scenarioResume.x+=0.1f;if(mode==11)p.m_scenarioResume.o+=0.1f;
  auto before=p.m_scenarioResume;auto pose=p.pos;
  if(mode==12||mode==13){p.m_atLoginFlags=mode==12?AT_LOGIN_UNLOCK:0;p.validPosition=mode!=13;assert(!p.earlyWarrior());assert(p.m_scenarioResume.Write()==before.Write()&&!p.relocated&&!p.m_scenarioLoginId&&!normalized&&!refused);}
  else{
   p.PrepareScenarioLogin(1600,mode==2?99:42);
   if(mode==5){assert(!p.m_scenarioResume.scenario&&!p.m_scenarioLoginId&&!p.selector&&!p.relocated);}
   else{
#ifdef SHARASDAL_LEGACY_SOURCE_BASELINE
    assert(p.m_scenarioResume.Write()==before.Write()&&!normalized&&!refused);
    if(mode==1||mode==3||mode==4)assert(!p.relocated&&p.pos.x==pose.x&&p.m_scenarioLoginInstance==42);else assert(p.relocated&&p.pos.x==before.x);
#else
    bool repair=mode<9;
    assert(normalized==unsigned(repair)&&!refused&&p.m_scenarioResume.map==before.map&&p.m_scenarioResume.scenario==before.scenario&&p.m_scenarioResume.instance==before.instance&&p.m_scenarioResume.boot==before.boot);
    if(repair)assert(Correct(p.m_scenarioResume));else assert(p.m_scenarioResume.Write()==before.Write());
    if(mode==1){assert(!p.relocated&&p.pos.x==pose.x&&p.m_scenarioLoginInstance==42);}
    else if(mode==3||mode==4){assert(p.relocated&&AtEntry(p)&&p.m_scenarioLoginInstance==42);}
    else if(repair||mode==9)assert(p.relocated&&AtEntry(p)&&!p.m_scenarioLoginInstance);
    else assert(p.relocated&&p.pos.x==before.x&&!p.m_scenarioLoginInstance);
    if(mode==6){p.PrepareScenarioLogin(1600,42);assert(normalized==1&&Correct(p.m_scenarioResume));}
    if(mode==7){p.RecordScenarioEntrance();assert(Correct(p.m_scenarioResume)&&p.m_scenarioResume.boot==1000&&p.m_scenarioResume.instance==42);}
    if(mode==8){ScenarioResume read;assert(read.Read(p.m_scenarioResume.Write())&&read.Write()==p.m_scenarioResume.Write());}
#endif
   }
  }
  assert(p.questStatus==status&&!p.claim&&live.step==3);++cases;
 }
#ifndef SHARASDAL_LEGACY_SOURCE_BASELINE
 for(unsigned team:{0u,1u})for(unsigned fault=0;fault<26;++fault){Player p;Scenario live{};Setup(p,live,team,3);auto& d=lfgManager.data;auto& target=spellManager.targets[207683];
  if(fault==0)spellManager.targets.erase(207683);if(fault==1)target.target_mapId=1220;if(fault==2)target.target_X+=.1f;if(fault==3)target.target_Y+=.1f;if(fault==4)target.target_Z+=.1f;if(fault==5)target.target_Orientation+=1;
  if(fault==6)lfgManager.missing=true;if(fault==7)d.entry.ScenarioID=972;if(fault==8)d.id=1059;if(fault==9)d.map=1511;if(fault==10)d.difficulty=1;if(fault==11)d.entry.ID=1059;if(fault==12)d.entry.MapID=1511;if(fault==13)d.entry.Flags=0;if(fault==14)d.x+=1;if(fault==15)d.o+=1;if(fault==16)d.x=std::numeric_limits<float>::quiet_NaN();if(fault==17)d.o=std::numeric_limits<float>::quiet_NaN();if(fault==18)p.m_scenarioResume.instance=0;if(fault==19)p.m_scenarioResume.boot=0;if(fault==20)scenarios.missingSteps=true;if(fault==21)d.entry.DifficultyID=1;if(fault==22)d.z=0;if(fault==23)target.target_X=std::numeric_limits<float>::quiet_NaN();if(fault==24)target.target_Orientation=std::numeric_limits<float>::infinity();if(fault==25)target.target_X=target.target_Y=target.target_Z=0;
  p.PrepareScenarioLogin(1600,42);assert(!normalized&&refused==1&&!p.m_scenarioResume.scenario&&!p.m_scenarioLoginId&&!p.m_scenarioLoginInstance&&!p.selector&&!p.relocated&&p.questStatus==3&&!p.claim&&live.step==3);++cases;
 }
#endif
 for(unsigned team:{0u,1u})for(unsigned mode=0;mode<9;++mode){Player p;Scenario live{};Setup(p,live,team,3);
  if(mode==0)p.cls=CLASS_HUNTER;if(mode==1)p.m_scenarioResume.map=1539;if(mode==2){p.m_scenarioResume.scenario=lfgManager.data.entry.ScenarioID=940;}
  if(mode==3){p.map.entry.ID=sMapStore.value.ID=p.m_scenarioResume.map=lfgManager.data.map=1539;}if(mode==4)p.m_scenarioResume={};
  if(mode==5)p.m_scenarioResume.x+=0.01f;if(mode==6)p.m_scenarioResume.y+=0.01f;if(mode==7)p.m_scenarioResume.z+=0.01f;if(mode==8)p.m_scenarioResume.o+=0.01f;
  auto before=p.m_scenarioResume;p.PrepareScenarioLogin(p.GetMapId(),42);assert(!normalized&&!refused);if(mode>=2&&mode!=4)assert(p.m_scenarioResume.Write()==before.Write());assert(p.questStatus==3&&!p.claim&&live.step==3);++cases;
 }
#ifdef SHARASDAL_LEGACY_SOURCE_BASELINE
 assert(cases==74);std::cout<<"COUNTERFACTUAL pinned677430a07 Sharasdal login74 ASAN:known legacy fresh/live/falling pose remains wrong;current native-source host and unrelated/completion/unlock controls retained;not ordinary gameplay success\n";
#else
 assert(cases==126);std::cout<<"PASS extracted Sharasdal legacy login126 ASAN:exact class/map/scenario/full-tuple proof,26 source/provenance refusals each faction,live valid pose preserved/live bad XY+fall recovered,fresh boot/numeric mismatch,serialization/idempotence/completion/unlock and custom/foreign controls;no quest/item/criterion grant. Source endpoints controlled;native/full client separate\n";
#endif
}
