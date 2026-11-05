// Real native spell fanout, clone AI, owner/scaling, timed removal and capture.
// Earlier admission/pose and source Unit+AI clock are controlled endpoints.
#define DEVOURER_CAPTURE_SUPPORT_ONLY
#include "DevourerCaptureNative.cpp"
#ifdef DEVOURER_CLONE_REFUSAL
static uint32 refusalCalls=0;
class zzzz_devourer_clone_refusal_probe : public SpellScriptLoader
{
public:
 zzzz_devourer_clone_refusal_probe():SpellScriptLoader("zzzz_devourer_clone_refusal_probe"){}
 class Hook:public SpellScript{PrepareSpellScript(Hook);SpellCastResult Reject(){++refusalCalls;return SPELL_FAILED_NOT_READY;}void Register()override{OnCheckCast+=SpellCheckCastFn(Hook::Reject);}};
 SpellScript* GetSpellScript()const override{return new Hook;}
};
#endif
static void NoCapture(Player& player,Scenario* scenario)
{
 Require(scenario->GetCurrentStep()==0&&!EscapeLeaf(scenario,47353)&&!player.HasPendingTeleport(),"Clone action granted capture progression");
}
int main(int argc,char**argv){try{
 InitializeEscapeNative(argc,argv);sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellProcs();sSpellMgr->LoadSpellLinked();sCreatureTextMgr->LoadCreatureTexts();uint32 cases=0;
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29781");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Capture has new native criteria conditions");
 auto extra=WorldDatabase.Query("SELECT (SELECT COUNT(*) FROM conditions WHERE SourceEntry IN(209231,209233,209234,209235,105609))+(SELECT COUNT(*) FROM pet_stats WHERE entry=105609)");Require(extra&&!extra->Fetch()[0].GetUInt32(),"Clone fixture needs new actual conditions/pet stats loaders");
#ifdef DEVOURER_CLONE_REFUSAL
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());Require(names.empty()||names.back()<"zzzz_devourer_clone_refusal_probe","Native refusal name must append without reindexing existing scripts");names.emplace_back("zzzz_devourer_clone_refusal_probe");auto id=sObjectMgr->GetScriptId("zzzz_devourer_clone_refusal_probe");Require(id,"Private refusal name absent");sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new zzzz_devourer_clone_refusal_probe;sScriptMgr->SwapScriptContext(true);Require(sScriptMgr->GetSpellScriptLoader(id),"Private refusal factory not registered");auto& bindings=sScriptDataStore->*Access(EscapeBindingsTag{});Require(bindings.count(209231)==0,"Refusal fixture must not replace a live Vanish hook");bindings.emplace(209231,std::make_pair(id,true));std::vector<uint32> modes{7};
#elif defined(DEVOURER_CLONE_RESET_BASELINE)
 std::vector<uint32> modes{8};
#else
 std::vector<uint32> modes{0,1,2,3,4,5,6,8,9,10,11};
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode:modes){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9871000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);auto first=Session(),second=Session();
  {NativePlayer player(first,map,101,Position{1854.16f,1278.57f,91.8f,0}),member(second,map,102,player.GetPosition());for(auto p:{&player,&member}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);}std::unique_ptr<CreditParty> party;if(mode==3)party=std::make_unique<CreditParty>(&player,&member);
   auto owned=std::make_unique<Actor>(&map,105536,18601+cases,Position{1854.16f,1275.57f,91.8f,1.56409f});auto& source=*owned;Require(map.AddToMap(static_cast<Creature*>(&source)),"Native Akaari missing");auto ai=new npc_devourer_capture_probe::npc_akaari_shadowgoreAI(&source);Require(source.AIM_Initialize(ai),"Whole production capture AI missing");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;if(mode==1||mode==5)pet=std::make_unique<CreditPet>(player,map);if(mode==2||mode==6)guardian=std::make_unique<CreditGuardian>(player,map);Unit* attacker=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&player);
   source.Attack(attacker,true);ai->EnterCombat(attacker);attacker->DealDamage(&source,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(source.IsAlive()&&ai->capturePlayer==player.GetGUID(),"Native owner/scaled one-hit capture guard failed");
   CaptureTick(player,source,11000);Require(CaptureClones(source).empty(),"Clones spawned before original12s event");NoCapture(player,scenario);if(mode==7){auto result=source.CastSpell(&source,209231,false);std::cout<<"CONTROL refused209231 result="<<uint32(result)<<" expected="<<uint32(SPELL_FAILED_DONT_REPORT)<<" bindingCount="<<(sScriptDataStore->*Access(EscapeBindingsTag{})).count(209231)<<std::endl;Require(result==SPELL_FAILED_DONT_REPORT,"Native injected CheckCast refusal not reached");}CaptureTick(player,source,2000);auto clones=CaptureClones(source);Require(clones.size()==(mode==7?0:3),"Native209231 fanout must create exactly3 actual105609 actors");NoCapture(player,scenario);
   std::set<uint32> producers;for(auto guid:clones){auto clone=ObjectAccessor::GetCreature(source,guid);auto summon=clone?clone->ToTempSummon():nullptr;Require(summon&&summon->IsAlive()&&summon->GetSummonerGUID()==source.GetGUID()&&summon->GetOwnerGUID()==source.GetGUID()&&summon->GetCreatorGUID()==source.GetGUID()&&summon->GetNPCAIName()=="AnyPetAI"&&summon->GetSummonType()==TEMPSUMMON_TIMED_DESPAWN&&summon->GetTimer()>=8500&&summon->GetTimer()<=10000&&summon->getFaction()==source.getFaction()&&summon->GetHealth(attacker)>0,"Native clone ownership/AI/scaling/timer differs");producers.insert(summon->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL));Require(summon->getVictim()&&(summon->getVictim()==&player||summon->getVictim()==attacker||summon->getVictim()==&member),"Native clone AI did not acquire an eligible combat target");}
   if(mode!=7)Require(producers==std::set<uint32>{209233,209234,209235}&&source.tempSummonList[105609].size()==3&&source.HasAura(209231),"Native clone producers/owner list/Vanish differ");
   if(mode>=4&&mode<=6){uint32 killed=0;for(auto guid:clones){auto clone=ObjectAccessor::GetCreature(source,guid);attacker->DealDamage(clone,UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(!clone->IsAlive(),"Actual clone one-hit kill failed");++killed;if(mode==4)break;}Require(killed==(mode==4?1:3),"Wrong native death count");NoCapture(player,scenario);}
   std::unique_ptr<Actor> foreign;std::vector<ObjectGuid> foreignClones;ObjectGuid scopeMarker;
   if(mode==10){auto pos=source.GetPosition();pos.m_positionX+=15;foreign=std::make_unique<Actor>(&map,105536,19601+cases,pos);Require(map.AddToMap(static_cast<Creature*>(foreign.get())),"Native foreign source missing");foreign->CastSpell(foreign.get(),209233,true);for(uint32 t=0;t<1000;t+=100)foreign->Unit::Update(100);foreignClones=CaptureClones(*foreign);Require(foreignClones.size()==3,"Native foreign clone control missing");scopeMarker=foreignClones.front();source.tempSummonList[105609].push_back(scopeMarker);}
   if(mode==11){auto extraClone=source.SummonCreature(105609,source.GetPosition(),player.GetGUID(),TEMPSUMMON_TIMED_DESPAWN,5000,0,sSummonPropertiesStore.LookupEntry(61));Require(extraClone&&extraClone->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL)==0,"Native other-producer control missing");scopeMarker=extraClone->GetGUID();}
   if(mode==9){map.RemoveFromMap(static_cast<Creature*>(&source),false);owned.reset();for(uint32 t=0;t<11000;t+=100){for(auto guid:clones)if(auto c=ObjectAccessor::GetCreature(player,guid))c->Update(100);player.Unit::Update(100);map.RemoveAllObjectsInRemoveList();}for(auto guid:clones)Require(!ObjectAccessor::GetCreature(player,guid),"Clone survived native timer after source destruction");NoCapture(player,scenario);}
   else{
    if(mode==8||mode==10||mode==11){ai->EnterEvadeMode();
     if(mode==10||mode==11){auto retained=ObjectAccessor::GetCreature(player,scopeMarker);Require(retained&&retained->IsInWorld()&&!retained->IsPreDelete()&&!retained->IsDespawn(),"Reset retired a foreign-source or other-producer clone");if(mode==10)source.tempSummonList[105609].remove(scopeMarker);else retained->ToTempSummon()->UnSummon();map.RemoveAllObjectsInRemoveList();}
     CaptureTick(player,source,11000);Require(CaptureClones(source).empty()&&!ai->capturePending&&!ai->captureComplete,"Evade clone expiry changed capture");NoCapture(player,scenario);
#ifdef DEVOURER_CLONE_RESET_BASELINE
     Require(source.isInCombat()&&!ai->capturePlayer.IsEmpty(),"Pinned clones no longer reaggro their reset owner");CaptureTick(player,source,23000);Require(ai->captureComplete&&scenario->GetCurrentStep()==1&&EscapeLeaf(scenario,47353)==1,"Pinned unintended capture counterfactual changed");auto captured=ObjectAccessor::GetPlayer(player,ai->capturePlayer);Require(captured&&captured->HasPendingTeleport(),"Pinned unintended captured player missing");CaptureAck(*captured);source.CombatStop(true);source.CastStop();source.RemoveAllAuras();source.m_Events.KillAllEvents(true);
#else
     Require(!source.isInCombat()&&ai->capturePlayer.IsEmpty(),"Reset mirror images reaggroed their owner without a fresh attack");source.GetMotionMaster()->MoveIdle();source.ClearUnitState(UNIT_STATE_EVADE);source.SetInCombatWith(attacker);attacker->SetInCombatWith(&source);source.Attack(attacker,true);ai->EnterCombat(attacker);CaptureTick(player,source,23000);
#endif
    }
    else CaptureTick(player,source,10000);
#ifndef DEVOURER_CLONE_RESET_BASELINE
    std::cout<<"CONTROL mode="<<mode<<" clones="<<CaptureClones(source).size()<<" ownerlist="<<source.tempSummonList[105609].size()<<" vanish="<<source.HasAura(209231)<<" pending="<<ai->capturePending<<" complete="<<ai->captureComplete<<" combat="<<source.isInCombat()<<std::endl;
    Require(CaptureClones(source).empty()&&source.tempSummonList[105609].empty()&&!source.HasAura(209231)&&ai->capturePending&&!ai->captureComplete,"Native10s clone/Vanish cleanup must precede capture");for(auto guid:clones)Require(!ObjectAccessor::GetCreature(source,guid),"Expired clone remains in actual accessor");NoCapture(player,scenario);
    CaptureTick(player,source,10000);Require(ai->captureComplete&&scenario->GetCurrentStep()==1&&EscapeLeaf(scenario,47353)==1&&player.HasAura(211684)&&player.HasAura(211686),"Actual transfer/50261 after clone lifecycle failed");CaptureAck(player);CaptureTick(player,source,1000);Require(EscapeLeaf(scenario,47353)==1,"Clone lifecycle replayed capture credit");source.CombatStop(true);source.CastStop();source.RemoveAllAuras();source.m_Events.KillAllEvents(true);
#endif
   }
   if(foreign){Require(CaptureClones(*foreign).size()==3,"Reset affected another actor native images");for(auto guid:foreignClones)if(auto c=ObjectAccessor::GetCreature(player,guid))c->ToTempSummon()->UnSummon();map.RemoveAllObjectsInRemoveList();foreign.reset();}
   CleanStories(map);player.CastStop();player.RemoveAllAuras();player.m_Events.KillAllEvents(true);std::cout<<"CASE race="<<race<<" mode="<<mode<<" native clone fanout/AI/timer/death/cancel lifecycle\n";
  }first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_CLONE_REFUSAL
 Require(refusalCalls>=4,"Both explicit and actual AI Vanish attempts must reach native injected CheckCast refusal");std::cout<<"CONTROL "<<cases<<" explicit native CheckCast refusal contexts, private test hook only; failed optional209231 creates no clones, original23s capture/10s transfer still once. Not an ordinary production binding or gameplay failure\n";
#elif defined(DEVOURER_CLONE_RESET_BASELINE)
 std::cout<<"COUNTERFACTUAL "<<cases<<" pinned native clone reset contexts: live105609 AnyPetAI images reaggro a returned-home owner after native evade, bind a nearby participant and complete50261 without a fresh player attack. Pair with repaired ordinary run; not a successful ordinary scenario\n";
#else
 std::cout<<"PASS "<<cases<<" native105609 clone lifecycle contexts: original209231/209233..235,3 actual AnyPetAI actors, native owner/scaling/kills/pet/guardian/group/factions,10s cleanup, evade/retry, foreign/fullGUID-stale-index and other-producer controls, destroyed source; no clone progression, original capture once. Earlier state/pose/source clock and returned-home endpoint controlled; client traversal/rendering separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
