#define MARKSMANSHIP_PRESENTATION_TEST
#include "TuureFixture.h"
enum {CLASS_PRIEST=5,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SPELL_CAST_OK=0};
using SpellCastResult=uint32;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Progress{uint32 Counter=1;};
struct Scenario{uint32 id=1085,step=6;std::map<uint32,Progress> progress{{47656,{1}}};uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}
 Scenario& GetAchievementMgr(){return *this;}auto const* GetCriteriaProgressMap(){return &progress;}};
struct Manager{Scenario scenario;bool present=true;Scenario* GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}} manager;
auto sScenarioMgr=&manager;
struct Spell{uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* m_caster;SpellInfo* m_spellInfo;void EffectForceEquipItem(SpellEffIndex);};
#include "ForceEquip.inc"
struct SceneTriggerScript{explicit SceneTriggerScript(std::string name){assert(name=="scene_tuure_pickup");}virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;}};
#include "Presentation.inc"
struct Fixture{Player player;AuraEffect effect{1265,210733};SpellInfo info;Spell equip{SPELL_EFFECT_HANDLE_HIT_TARGET,0,&player,&info};scene_tuure_pickup script;uint32 instance;bool deferred=false;std::function<void()> queued;
 Fixture(){warnings=0;manager={};spellManager.scene={21,1565,1265};player.playerClass=5;player.map=1489;player.expectedQuest=41993;player.expectedItem=128825;
  player.forceEquip=[this](){if(deferred)queued=[this](){equip.EffectForceEquipItem(0);};else equip.EffectForceEquipItem(0);};
#ifdef BASELINE
  scriptManager.callback=[](Player*,std::string){return false;}; // Original registry has no scene handler for1265.
#else
  scriptManager.callback=[this](Player* p,std::string trigger){return script.OnTrigger(p,&spellManager.scene,trigger);};
#endif
  instance=player.Start(effect);}
 ~Fixture(){scriptManager.callback={};}
 void Finish(bool cancel=false){WorldSession session{&player};WorldPackets::Scene::SceneInstance packet{instance};if(cancel)session.HandleScenePlaybackCanceled(packet);else session.HandleScenePlaybackComplete(packet);}
};
int main(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
#ifdef BASELINE
 Fixture f;f.Finish();assert(f.player.HasSceneStatus(1265,SCENE_COMPLETE)&&f.player.casts.empty()&&!f.player.swaps&&f.player.mainhand==&f.player.previousWeapon);
 std::cout<<"PASS pinned44e7df34/unbound1265 counterfactual: real scene completes but never requests native210734; existing staff stays in bags. Cinematic clone equipment is not real inventory equip. Expected missing behavior.\n";
#else
 for(uint32 status:{1u,3u})for(bool cancel:{false,true})for(bool deferred:{false,true}){Fixture f;f.player.quest=status;f.deferred=deferred;
  f.player.TrigerScene(f.instance,"Smooth Phase Complete");f.player.TrigerScene(f.instance,"complete");assert(f.player.casts.empty());
  f.Finish(cancel);f.Finish(cancel);assert(f.player.casts==std::vector<uint32>({210734})&&f.player.credits.empty()&&!warnings);
  if(deferred){assert(!f.player.swaps&&f.queued);f.queued();}assert(f.player.swaps==1&&f.player.mainhand==&f.player.artifact&&f.player.previousWeapon.position==0xFF20);
  assert(f.player.effects.empty()&&f.player.m_sceneInstanceID.empty());}
 for(uint32 invalid=0;invalid<19;++invalid){Fixture f;switch(invalid){case 0:manager.scenario.step=5;break;case 1:manager.scenario.step=7;break;case 2:manager.scenario.id=972;break;case 3:manager.present=false;break;case 4:f.player.runtimeInstance=43;break;case 5:f.player.map=1220;break;case 6:f.player.playerClass=3;break;case 7:f.player.rewarded=true;break;case 8:f.player.quest=0;break;case 9:f.player.quest=5;break;case 10:f.player.alive=false;break;case 11:f.player.world=false;break;case 12:f.player.contact=false;break;case 13:f.player.teleporting=true;break;case 14:f.player.owned=false;break;case 15:f.player.bankOnly=true;break;case 16:manager.scenario.progress.clear();break;case 17:manager.scenario.progress[47656].Counter=0;break;case 18:f.player.m_bHasDelayedTeleport=true;break;}
  f.Finish();assert(f.player.casts.empty()&&!f.player.swaps&&f.player.credits.empty());}
 for(uint32 failure=0;failure<3;++failure){Fixture f;if(failure==0)f.player.castResult=1;if(failure==1)f.player.equipResult=EQUIP_ERR_BAG_FULL;if(failure==2)f.player.swapBlocked=true;
  f.Finish();assert(f.player.casts==std::vector<uint32>({210734})&&f.player.mainhand==&f.player.previousWeapon&&f.player.swaps==(failure==2?1u:0u)&&f.player.credits.empty());assert(warnings==(failure==0?1u:0u));assert(f.player.equipErrors==(failure==1?1u:0u));}
 {Fixture f;auto obsolete=f.instance;f.instance=f.player.Start(f.effect);f.player.SceneCompleted(obsolete);assert(f.player.casts.empty());f.Finish();assert(f.player.swaps==1);f.player.TrigerScene(obsolete,"complete");assert(f.player.casts.size()==1);}
 {Fixture f;f.player.RemoveAurasDueToSpell(210733);f.Finish();assert(f.player.casts.empty());SpellScene other{21,1565,1266};assert(!f.script.OnTrigger(&f.player,&other,"complete"));other={21,1,1265};assert(!f.script.OnTrigger(&f.player,&other,"complete"));assert(!f.script.OnTrigger(&f.player,nullptr,"complete"));assert(f.script.OnTrigger(nullptr,&spellManager.scene,"complete"));}
 {Fixture f;f.player.mainhand=&f.player.artifact;f.player.artifact.position=0xFF0F;f.Finish();assert(!f.player.swaps&&!warnings&&f.player.casts.size()==1);}
 std::cout<<"PASS native scene/packet completion/latest-instance/cancel dispatch and real ForceEquipItem for1265/1565/210734:8 status/cancel/deferred variants,19 invalid contexts/actual boss leaf, failure/native inventory error, stale/forged/repeated/removed aura and already-equipped. No item/progression grant. Deferred invocation uses a controlled scheduling adapter; live spell scheduling/client and native pickup/exit is separately regressed.\n";
#endif
}
