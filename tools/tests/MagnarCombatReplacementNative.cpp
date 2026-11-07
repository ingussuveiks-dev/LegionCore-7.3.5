// Real10 native deaths/46427, then actual AI replacement in combat phase.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct CombatPlayerLink{Player& p;CombatPlayerLink(Player& a,Map&m):p(a){p.GetMapRef().link(&m,&p);}~CombatPlayerLink(){p.GetMapRef().unlink();}};
static uint32 CombatStories(Map& map){uint32 n=0;for(auto s:*Access(StoriesTag{}))if(s&&s->GetMap()==&map&&s->GetEntry()==504)++n;return n;}
// Interpose only a callback after the real native scenario transition; all
// actual shared-instance behavior and the typed production bridge stay intact.
class CombatInstance final:public ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript{
 public:using instance_shields_rest_InstanceMapScript::instance_shields_rest_InstanceMapScript;std::function<void(unsigned)> afterStep;
 void onScenarionNextStep(uint32 step)override{instance_shields_rest_InstanceMapScript::onScenarionNextStep(step);if(afterStep)afterStep(step);}
};
class CombatFactory final:public ShieldsFixture::instance_shields_rest{
 public:InstanceScript*GetInstanceScript(InstanceMap*map)const override{return new CombatInstance(map);}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("smart_scripts","source_type=0 AND entryorguid=94813");Shadow("creature_template_addon","entry IN(96034,96468,97123,94707,94813)");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSmartScriptMgr->LoadSmartAIFromDB();sObjectMgr->LoadCreatureTemplateAddons();
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto original=lootName;lootName+=" WHERE Entry IN(108218,94707,94813) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=original;Require(loaded&&LootTemplates_Creature.HaveLootFor(94707)&&LootTemplates_Creature.HaveLootFor(94813),"Native roster loot missing");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new CombatFactory;new npc_magnar_icebreaker;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 kind=0;kind<3;++kind)for(uint32 mode=0;mode<10;++mode){
#ifdef MAGNAR_COMBAT_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9999100+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<CombatInstance*>(map.GetInstanceScript());Require(script,"Shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4715,346,-37.7f,0});CombatPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor companion(&map,96468,16202,{4717,349,-37.7f,0});companion.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&companion)),"Companion admission failed");companion.SetPhaseId({6999},false);script->OnCreatureCreate(&companion);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;
   MagnarActor boss(&map,96034,16201,{4714.82f,349.11f,-37.77f,0});boss.*Access(AltheaGuardDbTag{})=361400;Require(map.AddToMap(static_cast<Creature*>(&boss)),"Boss admission failed");boss.SetPhaseId({6999},false);script->OnCreatureCreate(&boss);
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit*killer=&player;if(kind==1){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(kind==2){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}
   bool reentered=false;if(mode==8)script->afterStep=[&](unsigned step){if(step==4){Require(boss.AIM_Initialize(),"Native phase-credit callback replacement failed");reentered=true;}};
   boss.Brain().DoAction(1);Require(boss.Brain().reserveGUIDs[11],"Native reserve missing");auto roster=boss.Brain().reserveGUIDs;boss.Brain().stop_combat=true;boss.Brain().LaunchEventWave(boss.Brain().wave++);uint32 deaths=0;
   while(mode!=7&&boss.Brain().first_phase&&deaths<10){auto active=boss.Brain().activeWave;Require(!active.empty(),"Actual wave stuck");for(auto guid:active){auto actor=map.GetCreature(guid);Require(actor&&actor->IsAlive(),"Actual active actor missing");actor->SetLootRecipient(&player);actor->LowerPlayerDamageReq(uint32(actor->GetMaxHealth()));killer->Kill(actor,false);killer->Kill(actor,false);++deaths;}}
   if(mode==7)scenario->SetCurrentStep(4);else Require(deaths==10&&!boss.Brain().first_phase&&scenario->GetCurrentStep()==4&&WaveCredit(scenario)==1,"Native ten deaths did not earn real phase4 once");Require(boss.Brain().GetFinaleHandoffDelay()>0,"Native504 data missing");uint32 auxiliaries=0;for(auto guid:roster)if(auto actor=map.GetCreature(guid))if(actor->IsAlive())++auxiliaries;Require(auxiliaries==(mode==7?12u:2u),"Actual surviving final-wave auxiliaries differ");
#ifndef MAGNAR_COMBAT_BASELINE
   if(mode==2||mode==3){killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(boss.Brain().finaleStarted&&CombatStories(map)==1,"Native pre-replacement504 missing");if(mode==3){boss.Brain().Reset();map.RemoveAllObjectsInRemoveList();Require(!CombatStories(map)&&!boss.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NOT_ATTACKABLE_1),"Ordinary defeat Reset retained504/player surrender flags");}}
   if(mode==5){script->Initialize();map.RemoveAllObjectsInRemoveList();}
   std::unique_ptr<MagnarActor> other;
   if(mode==6){other=std::make_unique<MagnarActor>(&map,96034,16203,Position{4714.82f,349.11f,-37.77f,0});(*other).*Access(AltheaGuardDbTag{})=361400;Require(map.AddToMap(static_cast<Creature*>(other.get())),"Different fullGUID admission failed");other->SetPhaseId({6999},false);script->OnCreatureCreate(other.get());map.RemoveAllObjectsInRemoveList();Require(!other->Brain().finaleStarted&&other->Brain().first_phase,"Different boss inherited phase");}
#endif
   if(mode==9){Require(boss.Attack(&player,true),"Native active duel attack refused");boss.SetInCombatWith(&player);Require(boss.isInCombat()&&boss.getVictim()==&player,"Native active duel context missing");}
   auto pose=boss.GetPosition();auto guid=boss.GetGUID();auto savedHealth=boss.GetHealth(killer);Require(mode==1?boss.AIM_Initialize():boss.AIM_Initialize(new MagnarAI(&boss)),"Actual phase4 AI replacement failed");Require(boss.GetGUID()==guid&&boss.GetExactDist(pose)<0.01f&&boss.GetHealth(killer)==savedHealth,"Replacement moved/recreated boss");map.RemoveAllObjectsInRemoveList();
#ifndef MAGNAR_COMBAT_BASELINE
   if(mode==4){Require(boss.AIM_Initialize(),"Repeated native factory replacement failed");map.RemoveAllObjectsInRemoveList();}
   bool denied=mode>=5&&mode<=7;
   if(mode==2||mode==3)Require(boss.HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IMMUNE_TO_NPC),"Interrupted finale recovery changed native NPC immunity");
   if(mode==9)Require(boss.isInCombat()&&boss.getVictim()==&player&&boss.Brain().events.HasEvent(EVENT_1)&&boss.Brain().events.HasEvent(EVENT_2)&&boss.Brain().events.HasEvent(EVENT_3)&&!boss.Brain().events.HasEvent(EVENT_4),"Actual active duel replacement lost victim/combat timers or restarted wave timer");
   if(!denied){if(mode==8)Require(reentered,"Actual native phase transition callback did not replace AI");if(boss.isInCombat()&&boss.getVictim())Require(boss.Brain().events.HasEvent(EVENT_1)&&boss.Brain().events.HasEvent(EVENT_2)&&boss.Brain().events.HasEvent(EVENT_3)&&!boss.Brain().events.HasEvent(EVENT_4),"Restored live duel lost spell timers or restarted waves");Require(!boss.Brain().first_phase&&boss.Brain().wave==7&&boss.Brain().reserveGUIDs==roster&&!CombatStories(map),"Earned phase/roster/owned504 cleanup lost on replacement");std::set<ObjectGuid> unique;for(auto id:boss.Brain().summons)Require(unique.insert(id).second,"Replacement duplicated summon ownership");for(auto id:roster)if(auto actor=map.GetCreature(id))Require(unique.count(id)&&actor->ToTempSummon()&&actor->ToTempSummon()->GetSummonerGUID()==boss.GetGUID(),"Existing native owned auxiliary not adopted");}
   else Require(boss.Brain().first_phase&&!CombatStories(map),"Epoch/identity/unearned step adopted a phase");
#endif
   auto health=boss.GetHealth(killer);killer->DealDamage(&boss,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
#ifdef MAGNAR_COMBAT_BASELINE
   Require(boss.Brain().first_phase&&boss.IsAlive()&&boss.GetHealth(killer)==health&&!boss.Brain().finaleStarted&&!CombatStories(map)&&WaveCredit(scenario)==1,"Pinned phase4 immortal replacement did not reproduce");std::cout<<"COUNTERFACTUAL combat replacement race="<<race<<" killer="<<kind<<": actual10 deaths/46427/step4,then native AI replacement resets first_phase and discards lethal hit; no504/finale,2 living auxiliaries lose AI roster ownership; expected broken,not gameplay success\n";
#else
   if(mode>=5&&mode<=7){Require(boss.Brain().first_phase&&boss.IsAlive()&&boss.GetHealth(killer)==health&&!boss.Brain().finaleStarted&&!CombatStories(map)&&WaveCredit(scenario)==(mode==7?0u:1u),"Refused native phase claim nevertheless completed");if(mode==5||mode==6)for(auto id:roster)Require(!map.GetCreature(id),"Retired instance/boss retained owned reserve");}
   else {
    Require(!boss.Brain().first_phase&&boss.IsAlive()&&boss.Brain().finaleStarted&&CombatStories(map)==1&&WaveCredit(scenario)==1,"Real earned phase4 replacement did not retain native surrender/504");
    auto delay=boss.Brain().GetFinaleHandoffDelay();boss.Brain().UpdateFinale(delay-1);Require(scenario->GetCurrentStep()==4,"Native504 handoff ran early");boss.Brain().UpdateFinale(1);Require(scenario->GetCurrentStep()==5&&boss.Brain().finaleDeparting&&WaveCredit(scenario)==1,"Actual45784 native outro handoff missing");
    script->OnCreatureRemove(&boss);map.RemoveAllObjectsInRemoveList();for(auto id:roster)Require(!map.GetCreature(id),"Actual instance removal retained owned native summon");
   }
   std::cout<<"PASS combat replacement race="<<race<<" killer="<<kind<<" mode="<<mode<<": real waves/phase4/AI replacement/owned auxiliaries/504 cleanup and actual handoff5 or epoch/fullGUID/unearned-step refusal; guard receipt and wave clock controlled,client combat separate\n";
#endif
   script->afterStep={};boss.CombatStop(true);companion.CombatStop(true);for(auto id:roster)if(auto actor=map.GetCreature(id)){actor->CombatStop(true);actor->DespawnOrUnsummon();}map.RemoveAllObjectsInRemoveList();CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native combat replacement cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
