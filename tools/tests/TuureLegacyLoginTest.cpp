#include <cstring>
static unsigned normalized=0,refused=0;
template<class... T>static void Observe(char const*,char const* text,T...){if(std::strstr(text,"normalized legacy Tuure"))++normalized;if(std::strstr(text,"refusing unsafe legacy Tuure"))++refused;}
#define TC_LOG_INFO(...) Observe(__VA_ARGS__)
#define main SharedResumeUnusedMain
#include "ScenarioResumeTest.cpp"
#undef main
static ScenarioResume Legacy(){return {1489,1085,42,999,-828.85f,4372.0f,738.64f,1.86768f};}
static void Setup(Player& p,Scenario& live,unsigned team,int status){
 normalized=refused=0;p=Player{};p.cls=CLASS_PRIEST;p.team=team;p.questStatus=status;p.acceptedQuest=41993;p.map.entry.ID=1489;p.m_scenarioResume=Legacy();live={&p.map};live.id=1085;live.step=3;
 scenarios.entries.clear();scenarios.entries[42]=&live;scenarios.missingSteps=false;sMapStore.value.ID=1489;sMapStore.missing=sScenarioStore.missing=false;sDB2Manager.missing=false;
 objects.missing=false;objects.data={1085,0,5,12,1243,1489};sWorldSafeLocsStore={};
 lfgManager.missing=lfgManager.shared=lfgManager.fallbackAlways=false;lfgManager.data={};lfgManager.data.dbc=&lfgManager.data.entry;lfgManager.data.entry.ScenarioID=1085;lfgManager.data.map=1489;lfgManager.data.id=1243;
 lfgManager.data.x=-43.1441f;lfgManager.data.y=1160.34f;lfgManager.data.z=-74.3536f;lfgManager.data.o=0.483893f;
}
#ifndef TUURE_LEGACY_SOURCE_BASELINE
static bool Correct(ScenarioResume const& r){auto& d=lfgManager.data;return std::fabs(r.x-d.x)<0.001f&&std::fabs(r.y-d.y)<0.001f&&std::fabs(r.z-d.z)<0.001f&&std::fabs(r.o-d.o)<0.001f;}
static bool AtEntry(Player const& p){auto& d=lfgManager.data;return std::fabs(p.pos.x-d.x)<0.001f&&std::fabs(p.pos.y-d.y)<0.001f&&std::fabs(p.pos.z-d.z)<0.001f;}
#endif
int main(){unsigned cases=0;
 for(unsigned team:{0u,1u})for(int status:{3,1})for(unsigned mode=0;mode<14;++mode){Player p;Scenario live{};Setup(p,live,team,status);
  if(mode==1||mode==3||mode==4||mode==5)p.m_scenarioResume.boot=1000;
  if(mode==3||mode==4)p.pos={-828.85f,4372.0f,mode==4?-100.0f:738.64f,0.2f};
  if(mode==5)live.completed=true;
  if(mode==9){auto& d=lfgManager.data;p.m_scenarioResume={1489,1085,42,999,d.x,d.y,d.z,d.o};}
  if(mode==10)p.m_scenarioResume.x+=0.1f;if(mode==11)p.m_scenarioResume.o+=0.1f;
  auto before=p.m_scenarioResume;auto pose=p.pos;
  if(mode==12||mode==13){p.m_atLoginFlags=mode==12?AT_LOGIN_UNLOCK:0;p.validPosition=mode!=13;assert(!p.earlyWarrior());assert(p.m_scenarioResume.Write()==before.Write()&&!p.relocated&&!p.m_scenarioLoginId&&!normalized&&!refused);}
  else{
   p.PrepareScenarioLogin(1489,mode==2?99:42);
   if(mode==5){assert(!p.m_scenarioResume.scenario&&!p.m_scenarioLoginId&&!p.selector&&!p.relocated);}
   else{
#ifdef TUURE_LEGACY_SOURCE_BASELINE
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
    if(mode==6){p.PrepareScenarioLogin(1489,42);assert(normalized==1&&Correct(p.m_scenarioResume));}
    if(mode==7){p.RecordScenarioEntrance();assert(Correct(p.m_scenarioResume)&&p.m_scenarioResume.boot==1000&&p.m_scenarioResume.instance==42);}
    if(mode==8){ScenarioResume read;assert(read.Read(p.m_scenarioResume.Write())&&read.Write()==p.m_scenarioResume.Write());}
#endif
   }
  }
  assert(p.questStatus==status&&!p.claim&&live.step==3);++cases;
 }
#ifndef TUURE_LEGACY_SOURCE_BASELINE
 for(unsigned team:{0u,1u})for(unsigned fault=0;fault<22;++fault){Player p;Scenario live{};Setup(p,live,team,3);auto& d=lfgManager.data;auto& safe=sWorldSafeLocsStore.value;
  if(fault==0)sWorldSafeLocsStore.missing=true;if(fault==1)safe.MapID=1220;if(fault==2)safe.ID=5487;if(fault==3)safe.Loc.X+=0.1f;if(fault==4)safe.Loc.O+=1;
  if(fault==5)lfgManager.missing=true;if(fault==6)d.entry.ScenarioID=972;if(fault==7)d.id=1059;if(fault==8)d.map=1511;if(fault==9)d.difficulty=1;if(fault==10)d.entry.ID=1059;if(fault==11)d.entry.MapID=1511;if(fault==12)d.entry.Flags=0;if(fault==13)d.x+=1;if(fault==14)d.o+=1;if(fault==15)d.x=std::numeric_limits<float>::quiet_NaN();if(fault==16)d.o=std::numeric_limits<float>::quiet_NaN();if(fault==17)p.m_scenarioResume.instance=0;if(fault==18)p.m_scenarioResume.boot=0;if(fault==19)scenarios.missingSteps=true;if(fault==20)d.entry.DifficultyID=1;if(fault==21)d.z=0;
  p.PrepareScenarioLogin(1489,42);assert(!normalized&&refused==1&&!p.m_scenarioResume.scenario&&!p.m_scenarioLoginId&&!p.m_scenarioLoginInstance&&!p.selector&&!p.relocated&&p.questStatus==3&&!p.claim&&live.step==3);++cases;
 }
#endif
 for(unsigned team:{0u,1u})for(unsigned mode=0;mode<9;++mode){Player p;Scenario live{};Setup(p,live,team,3);
  if(mode==0)p.cls=CLASS_HUNTER;if(mode==1)p.m_scenarioResume.map=1539;if(mode==2){p.m_scenarioResume.scenario=lfgManager.data.entry.ScenarioID=940;}
  if(mode==3){p.map.entry.ID=sMapStore.value.ID=p.m_scenarioResume.map=lfgManager.data.map=1539;}if(mode==4)p.m_scenarioResume={};
  if(mode==5)p.m_scenarioResume.x+=0.01f;if(mode==6)p.m_scenarioResume.y+=0.01f;if(mode==7)p.m_scenarioResume.z+=0.01f;if(mode==8)p.m_scenarioResume.o+=0.01f;
  auto before=p.m_scenarioResume;p.PrepareScenarioLogin(p.GetMapId(),42);assert(!normalized&&!refused);if(mode>=2&&mode!=4)assert(p.m_scenarioResume.Write()==before.Write());assert(p.questStatus==3&&!p.claim&&live.step==3);++cases;
 }
#ifdef TUURE_LEGACY_SOURCE_BASELINE
 assert(cases==74);std::cout<<"COUNTERFACTUAL pinnedfdb24afb8 Tuure login74 ASAN:known legacy fresh/live/falling pose remains wrong;current native-source host and unrelated/completion/unlock controls retained;not ordinary gameplay success\n";
#else
 assert(cases==118);std::cout<<"PASS extracted Tuure legacy login118 ASAN:exact class/map/scenario/full-tuple proof,22 source/provenance refusals each faction,live valid pose preserved/live bad XY+fall recovered,fresh boot/numeric mismatch,serialization/idempotence/completion/unlock and custom/foreign controls;no quest/item/criterion grant. Source endpoints controlled;native/full client separate\n";
#endif
}
