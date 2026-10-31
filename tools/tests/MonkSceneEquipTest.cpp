#include "SceneLifecycleFixture.h"
#include <fstream>
enum {CLASS_MONK=10,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1};
struct Scenario {uint32 id=983,step=6;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioManager {Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
namespace ArtifactAcquisition {
#include "ActiveQuest.inc"
}
struct SceneTriggerScript {std::string name;explicit SceneTriggerScript(char const* value):name(value){}virtual bool OnTrigger(Player*,SpellScene const*,std::string)=0;};
#include "MonkSceneGuard.inc"
struct Path {uint32 scene,package,aura,equip,map,scenario,first,quest,item,flags=21;};
static uint32 BeginScene(Player& player,Path const& path,AuraEffect& aura){
 if(path.flags!=4)return player.Start(aura);
 //1192 is native effect198, not a persistent ACTIVATE_SCENE aura.
 Position position;player.SendSpellScene(path.scene,nullptr,true,&position);return player.sceneInstanceID;
}
static void Ready(Path const& path,Player& player){player.playerClass=10;player.map=path.map;player.expectedQuest=path.quest;player.expectedItem=path.item;player.quest=3;player.runtimeInstance=42;scenarioManager={{path.scenario,path.first},true};}
static void Bind(scene_monk_artifact_equipment& script){scriptManager.sceneCallback=[&script](Player* player,SpellScene const* scene,std::string event){return script.OnTrigger(player,scene,event);};}
static void Run(Path const& path,scene_monk_artifact_equipment& script,std::map<uint32,std::vector<SceneTriggerEvent>> const& mappings){
 spellManager.scene={path.flags,path.package,path.scene};spellManager.scenes={{path.scene,spellManager.scene},{1235,{21,1595,1235}}};spellManager.events.clear();scriptManager.sceneCallback={};
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};player.SceneCompleted(BeginScene(player,path,aura));assert(player.casts.empty()&&player.credits.empty());} // missing original callback
 spellManager.events=mappings;
 // Expected unguarded counterfactual: outside-map source17 use is native and
 // must stay intact; therefore the newly connected scene needs its own gate.
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);player.map=1;player.SceneCompleted(id);assert(player.casts==std::vector<uint32>{path.equip});}
 Bind(script);
 if(path.flags==4){Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);assert(player.effects.empty());WorldSession session;session._player=&player;WorldPackets::Scene::SceneTriggerEvent event{id,"sceneComplete"};session.HandleSceneTriggerEvent(event);assert(player.casts.empty());WorldPackets::Scene::SceneInstance complete{id};session.HandleScenePlaybackComplete(complete);assert(player.casts==std::vector<uint32>{path.equip}&&player.effects.empty());}
 for(bool canceled:{false,true}){Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);unsigned accepted=0;player.cast=[&](uint32 spell){assert(spell==path.equip&&player.HasSceneStatus(path.scene,SCENE_COMPLETE));++accepted;};WorldSession session;session._player=&player;WorldPackets::Scene::SceneInstance packet{id};if(canceled)session.HandleScenePlaybackCanceled(packet);else session.HandleScenePlaybackComplete(packet);session.HandleScenePlaybackComplete(packet);player.TrigerScene(id,"complete");assert(accepted==1&&player.casts==std::vector<uint32>{path.equip}&&player.credits.empty()&&player.effects.empty()&&player.m_sceneInstanceID.empty());}
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);player.TrigerScene(id,"complete");assert(player.casts.empty()&&!player.HasSceneStatus(path.scene,SCENE_COMPLETE));player.SceneCompleted(id);assert(player.casts==std::vector<uint32>{path.equip});}
 for(unsigned mode=0;mode<15;++mode){Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);switch(mode){case 0:player.map=1;break;case 1:player.playerClass=3;break;case 2:player.alive=false;break;case 3:player.world=false;break;case 4:player.contact=false;break;case 5:player.teleporting=true;break;case 6:player.quest=0;break;case 7:player.rewarded=true;break;case 8:player.owned=false;break;case 9:player.owned=false;player.bankOwned=true;break;case 10:scenarioManager.present=false;break;case 11:scenarioManager.scenario.id=900;break;case 12:scenarioManager.scenario.step=path.first-1;break;case 13:scenarioManager.scenario.step=path.first+2;break;case 14:player.runtimeInstance=43;break;}player.SceneCompleted(id);assert(player.casts.empty()&&player.credits.empty()&&player.effects.empty()&&player.m_sceneInstanceID.empty());}
 for(uint32 step=0;step<=12;++step)for(uint32 status:{0u,1u,3u,5u,6u}){Player player;Ready(path,player);player.quest=status;player.m_sceneStatus[path.scene]=SCENE_COMPLETE;scenarioManager.scenario.step=step;bool expected=(status==1||status==3)&&(step==path.first||step==path.first+1);assert(script.OnTrigger(&player,&spellManager.scene,"complete")==!expected);}
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura},other{1235,207560};auto old=BeginScene(player,path,aura),current=BeginScene(player,path,aura),separate=player.Start(other);player.SceneCompleted(old);player.SceneCompleted(999999);assert(player.casts.empty());player.SceneCompleted(current);assert(player.casts==std::vector<uint32>{path.equip}&&player.m_sceneInstanceID.count(separate)==1&&player.effects.size()==1);player.SceneCompleted(separate);assert(player.casts.size()==1&&player.effects.empty());}
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};auto id=BeginScene(player,path,aura);scriptManager.callback={};scriptManager.sceneCallback=[&](Player* owner,SpellScene const* scene,std::string event){owner->SceneCompleted(id);return script.OnTrigger(owner,scene,event);};player.SceneCompleted(id);assert(player.casts==std::vector<uint32>{path.equip});Bind(script);}
 {Player player;Ready(path,player);AuraEffect aura{path.scene,path.aura};player.castResult=1;player.SceneCompleted(BeginScene(player,path,aura));assert(player.casts==std::vector<uint32>{path.equip}&&player.effects.empty());player.castResult=0;player.SceneCompleted(BeginScene(player,path,aura));assert(player.casts==std::vector<uint32>({path.equip,path.equip})&&player.credits.empty());} // actual new scene request, no forced equip success/retry
 {Player player;Ready(path,player);player.m_sceneStatus[path.scene]=SCENE_COMPLETE;SpellScene foreign{21,1595,1235},wrong{21,1,path.scene};assert(!script.OnTrigger(&player,&foreign,"complete")&&!script.OnTrigger(&player,nullptr,"complete"));assert(script.OnTrigger(&player,&wrong,"complete")&&script.OnTrigger(nullptr,&spellManager.scene,"complete")&&script.OnTrigger(&player,&spellManager.scene,"Visual"));}
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);assert(input);
 std::map<uint32,std::vector<SceneTriggerEvent>> mappings;uint32 scene,spell,credit;std::string event;
 while(input>>scene>>event>>spell>>credit){assert(event=="complete"&&!credit);mappings[scene].push_back({event,spell,credit,scene});}
 scene_monk_artifact_equipment script;assert(script.name=="scene_monk_artifact_equipment");
#ifdef SHEILUN_SCENE_TEST
 assert(mappings.size()==1&&mappings[1192][0].trigerSpell==202438);
 Run({1192,1557,202437,202438,1541,1007,4,41003,128937,4},script,mappings);
 std::cout<<"PASS native Sheilun flags4/effect198 scene packets/lifecycle and actual registered admission: original missing mapping, unguarded late-map counterfactual, sceneComplete cannot equip before terminal complete, ordinary terminal/cancel exactly once, forged/stale/replaced/reentrant callbacks,15 late-invalid cases,65 stage/status states, inventory/bank/quest/context and failed-cast/new-scene retry. Cast/equip endpoints controlled; no item/credit/return grant.\n";
#else
 assert(mappings.size()==2&&mappings[1160][0].trigerSpell==199499&&mappings[1342][0].trigerSpell==217813);
 Run({1160,1524,199502,199499,1528,983,6,40570,128940},script,mappings);
 Run({1342,1668,217814,217813,1625,1137,7,42765,128938},script,mappings);
 std::cout<<"PASS retained native Monk flags21 scene lifecycle/packet handlers and actual registered guard: original missing callback and unguarded late-map counterfactuals, complete/cancel once, stale/reentrant/context/inventory/failed-cast boundaries.\n";
#endif
}
