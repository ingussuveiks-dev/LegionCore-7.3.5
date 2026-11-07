#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "../../src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_start.h"
#include "../../src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h"
#include "../../src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_combat.h"
#include "MagnarNative.inc"
using MagnarAI=npc_magnar_icebreaker::npc_magnar_icebreakerAI;
struct MagnarActor:Actor{
 using Actor::Actor;
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new MagnarAI(this)),"Native Magnar AI initialization failed");}
 MagnarAI& Brain(){return *static_cast<MagnarAI*>(AI());}
};
struct PausedAI{
 Creature& actor;UnitAI* original;bool enabled,restored=false;
 PausedAI(Creature& a,bool null):actor(a),original(a.GetAI()),enabled(a.IsAIEnabled){Require(original&&enabled,"Native actor starts without its ordinary AI");if(null)actor.SetAI(nullptr);else actor.IsAIEnabled=false;}
 void Restore(){if(!restored){actor.SetAI(original);actor.IsAIEnabled=enabled;restored=true;}}
 ~PausedAI(){Restore();}
};
static uint64 WaveCredit(Scenario* scenario){uint64 result=0;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()){auto tree=sCriteriaTreeStore.LookupEntry(row.first);auto c=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;if(c&&c->Type==92&&c->Asset==46427)result+=row.second.Counter;}return result;}
#ifndef MAGNAR_READINESS_SUPPORT_ONLY
// The scoped earned-combat fixture uses the actual shared-instance bridge.
// This original instance-free probe cannot own a recovery receipt.
void MagnarCombat::Remember(Creature* c, CreatureAI const*, std::array<ObjectGuid,12> const&, std::array<bool,12> const&) { Require(!c->GetInstanceScript(),"Unexpected combat receipt in instance-free fixture"); }
bool MagnarCombat::Restore(Creature* c, CreatureAI const*, SummonList&, std::array<ObjectGuid,12>&, std::array<bool,12>&) { Require(!c->GetInstanceScript(),"Unexpected combat restore in instance-free fixture");return false; }
// This older wave fixture deliberately has no shared InstanceScript/Hruthnir
// controller. The reset-to-offer integration uses MagnarResetNative instead.
void MagnarStart::ResetOffer(Creature*, Creature*) { Require(false,"Unexpected reset bridge in instance-free wave fixture"); }
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("smart_scripts","source_type=0 AND entryorguid=94813");Shadow("creature_template_addon","entry IN(96034,96468,97123,94707,94813)");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 auto readonly=WorldDatabase.Query("SELECT @@session.transaction_read_only");Require(readonly&&readonly->Fetch()[0].GetBool(),"Native World shadow did not return to readonly");
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=94813");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Native windshaper needs additional conditions");
 std::string criteria;
 for(auto criterion:sCriteriaStore)if(criterion->Type==92&&criterion->Asset==46427){if(!criteria.empty())criteria+=',';criteria+=std::to_string(criterion->ID);}
 Require(!criteria.empty(),"Native Magnar event46427 has no criterion");
 rules=WorldDatabase.Query(("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN("+criteria+")").c_str());Require(rules&&!rules->Fetch()[0].GetUInt32(),"Native Magnar criterion needs additional World rules");
 sSmartScriptMgr->LoadSmartAIFromDB();sObjectMgr->LoadCreatureTemplateAddons();
 auto& name=LootTemplates_Creature.*Access(LootNameTag{});auto original=name;name+=" WHERE Entry IN(108218,94707,94813) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();name=original;Require(loaded&&LootTemplates_Creature.HaveLootFor(94707)&&LootTemplates_Creature.HaveLootFor(94813),"Native roster loot inputs missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 killerKind=0;killerKind<3;++killerKind)for(uint32 mode=0;mode<4;++mode){
#ifdef MAGNAR_READINESS_BASELINE
 if(mode>=2||killerKind)continue;
#endif
 MapInstanced parent(1495,300);InstanceMap map(1495,300,9987200+cases++,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
 {
 NativePlayer player(session,map,901,{4715,346,-37.7f,0});player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);
 Actor companion(&map,96468,12002,{4717,349,-37.7f,0});Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native companion admission failed");
 MagnarActor boss(&map,96034,12001,{4714.82f,349.11f,-37.77f,0});Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Magnar admission failed");
 auto& brain=boss.Brain();brain.DoAction(1);Require(brain.encounterStarted&&brain.reserveGUIDs[11],"Native Magnar start/12-slot reserve failed");auto roster=brain.reserveGUIDs;
 auto selected=map.GetCreature(roster[0]);Require(selected&&selected->IsAlive(),"Native selected reserve missing");
 {
 PausedAI pause(mode%2?companion:*selected,mode>=2);brain.stop_combat=true;brain.reserveRetry=1000;brain.LaunchEventWave(brain.wave++);
#ifdef MAGNAR_READINESS_BASELINE
 Require(!selected->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC)&&brain.activeWave.size()==2,"Pinned disabled-AI release no longer reproduces");
 std::cout<<"COUNTERFACTUAL actual native Creature/AI: pinned fb01c9a6a releases selected opponents while "<<(mode?"companion":"opponent")<<" AI is disabled; not a passing ordinary run\n";
#else
 Require(brain.pendingWave==1&&brain.activeWave.empty()&&selected->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC)&&!WaveCredit(scenario),"Unavailable native AI consumed the wave");
 pause.Restore();brain.UpdateReserve(999);Require(brain.pendingWave==1,"Native1s readiness retry ran early");brain.UpdateReserve(1);
 Require(!brain.pendingWave&&brain.pendingActivation.empty()&&brain.activeWave.size()==2&&brain.reserveGUIDs==roster,"Native readiness retry lost/duplicated its actual roster");
#endif
 }
#ifndef MAGNAR_READINESS_BASELINE
 std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit*killer=&player;if(killerKind==1){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(killerKind==2){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}
 uint32 deaths=0;
 while(brain.first_phase&&deaths<10){auto active=brain.activeWave;Require(!active.empty(),"Native wave stuck with no selected opponent");for(auto guid:active){auto actor=map.GetCreature(guid);Require(actor&&actor->IsAlive(),"Native active opponent disappeared");actor->SetLootRecipient(&player);actor->LowerPlayerDamageReq(uint32(actor->GetMaxHealth()));killer->Kill(actor,false);killer->Kill(actor,false);++deaths;}}
 Require(deaths==10&&WaveCredit(scenario)==1&&scenario->GetCurrentStep()==4&&!brain.first_phase&&brain.pendingActivation.empty(),"Native deaths/owner credit did not advance3->4 exactly once");brain.UpdateAI(0);Require(WaveCredit(scenario)==1,"Native repeat tick duplicated46427");
 std::cout<<"PASS native Magnar race="<<race<<" killer="<<killerKind<<" fault="<<mode<<": exact same12-slot readiness retry,10 real Unit::Kill deaths plus duplicate kills, native46427 once/scenario909 stage3->4. Earlier stage/wave clock and companion NullAI controlled; not client combat\n";
#endif
 boss.CombatStop(true);companion.CombatStop(true);for(auto guid:roster)if(auto actor=map.GetCreature(guid)){actor->CombatStop(true);actor->DespawnOrUnsummon();}map.RemoveAllObjectsInRemoveList();CleanStories(map);
 }
 session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Magnar completed cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
