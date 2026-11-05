// Actual native Use ->211689/211687 ->criteria/aura timer ->50536 door.
// Earlier quest, stage2, poses and owned Wardstone are controlled endpoints.
#include "SpellScript.h"
#define DEVOURER_DOOR_SUPPORT_ONLY
#include "DevourerDoorNative.cpp"
struct EscapeBindingsTag{using Type=SpellScriptsContainer ScriptDataStoreMgr::*;friend Type Access(EscapeBindingsTag);};
template struct FixtureAccess<EscapeBindingsTag,&ScriptDataStoreMgr::_spellScriptsStore>;
struct EscapeCheckTag{using Type=SpellCastResult(Spell::*)(bool);friend Type Access(EscapeCheckTag);};
template struct FixtureAccess<EscapeCheckTag,&Spell::CheckCast>;
static uint64 EscapeLeaf(Scenario* scenario,uint32 id){auto p=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto f=p->find(id);return f==p->end()?0:f->second.Counter;}
static void EscapeTick(Player& player,uint32 ms){for(uint32 t=0;t<ms;t+=100)player.Unit::Update(std::min(uint32(100),ms-t));}
static void EscapeRetry(Player& player,GameObject& cache){player.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(211687));cache.Use(&player);EscapeTick(player,2500);}
static void InitializeEscapeNative(int argc,char**argv){
 InitializeDoorNative(argc,argv);
#ifndef DEVOURER_ESCAPE_BASELINE
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());
#ifndef DEVOURER_ESCAPE_LIVE
 if(!sObjectMgr->GetScriptId("spell_devourer_escape")){names.emplace_back("spell_devourer_escape");std::sort(names.begin(),names.end());}
#endif
 auto scriptId=sObjectMgr->GetScriptId("spell_devourer_escape");Require(scriptId,"Escape script name unavailable");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new spell_devourer_escape;sScriptMgr->SwapScriptContext(true);
 sScriptDataStore->LoadSpellScriptNames();auto& bindings=sScriptDataStore->*Access(EscapeBindingsTag{});
 for(uint32 spell:{211687u,211689u}){auto range=bindings.equal_range(spell);
#ifdef DEVOURER_ESCAPE_LIVE
 Require(std::distance(range.first,range.second)==1&&range.first->second.first==scriptId,"Actual live native escape binding differs");
#else
 Require(range.first==range.second,"Pending escape refuses another spell binding");bindings.emplace(spell,std::make_pair(scriptId,true));
#endif
 }
#endif
}
#ifndef DEVOURER_ESCAPE_SUPPORT_ONLY
int main(int argc,char**argv){try{
 InitializeEscapeNative(argc,argv);
 auto reclaim=sSpellMgr->GetSpellInfo(211687);Require(reclaim&&reclaim->CalcCastTime()==2000,"Native two-second reclaim time changed");
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<23;++mode){
 MapInstanced parent(1607,300);InstanceMap map(1607,300,9911000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(mode==4?1:2);auto session=Session(),otherSession=Session();
 {Position cagePose{1847.93f,1259.83f,57.0953f,0},cachePose{1870.09f,1272.14f,57.0953f,0},doorPose{1854.51f,1296.27f,62.1306f,0};NativePlayer player(session,map,101,cagePose),member(otherSession,map,102,cagePose);
 player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==2?CLASS_WARRIOR:CLASS_ROGUE);player.setFactionForRace(race);if(mode!=1){player.SetQuestStatus(41924,mode==12?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);}
 std::unique_ptr<DoorKey> key;if(mode!=3)key=std::make_unique<DoorKey>(player,cases);
 std::unique_ptr<CreditParty> party;if(mode==11){member.SetByteValue(UNIT_FIELD_BYTES_0,0,race);member.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);member.setFactionForRace(race);member.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);member.SetQuestSlot(0,41924);party=std::make_unique<CreditParty>(&player,&member);}
 player.AddAura(211684,&player);player.AddAura(211686,&player);Require(player.HasAura(211684)&&player.HasAura(211686),"Actual infinite chain auras missing");
 DoorObject cage,cache,door,barrier;uint32 guid=7500+cases*4;
 auto create=[&](DoorObject& go,uint32 entry,Position const& p){Require(go.Create(guid++,entry,&map,1,p,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&go)),"Native escape actor create failed");go.SetLootState(GO_READY);};
 create(cage,249809,cagePose);if(mode!=18)create(cache,248992,cachePose);create(door,249390,doorPose);create(barrier,249391,Position{1855.38f,1292.18f,62.5271f,0});
 if(mode==13){CreditPet pet(player,map);CreditGuardian guardian(player,map);for(Unit* actor:{static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)}){actor->CastSpell(actor,211689,true);actor->CastSpell(actor,211687,true);}Require(!EscapeLeaf(scenario,48002)&&!EscapeLeaf(scenario,48005),"Nonplayer cast advanced player progression");}
 if(mode==14)player.Unit::setDeathState(CORPSE);if(mode==15)player.SetGameMaster(true);if(mode==16)player.Relocate(cagePose.GetPositionX()+12,cagePose.GetPositionY(),cagePose.GetPositionZ(),0);if(mode==17)player.SetPhaseMask(2,false);
 bool const stunBefore=player.HasAura(211684);if(mode==5||mode==6){cage.AI()->GossipHello(&player,mode==6);cache.AI()->GossipHello(&player,mode==6);}else cage.Use(&player);
 EscapeTick(player,100);
 bool invalid=mode==1||mode==2||mode==3||mode==4||mode==5||mode==6||mode==14||mode==15||mode==16||mode==17;
#ifdef DEVOURER_ESCAPE_BASELINE
 bool cageCredited=mode!=4&&mode!=5&&mode!=15;Require(EscapeLeaf(scenario,48002)==uint64(cageCredited),"Pinned native cage endpoint differs");
 if(mode==6)Require(cage.GetPhaseMask()==4&&cache.GetPhaseMask()==4&&player.HasAura(211686)&&!EscapeLeaf(scenario,48005),"Pinned premature Hello no longer hides uncompleted cache");
#else
 Require(EscapeLeaf(scenario,48002)==uint64(!invalid)&&player.HasAura(211684)==(invalid&&stunBefore)&&cage.GetPhaseMask()==1&&cache.GetPhaseMask()==1,"Cage admission/native aura release differs");
#endif
 if(!invalid){
  player.Relocate(cachePose);if(mode==18)player.CastSpell(&player,211687,false);else cache.Use(&player);
  if(mode!=18)Require(player.GetCurrentSpell(CURRENT_GENERIC_SPELL),"Native nontriggered reclaim not pending");
  EscapeTick(player,1000);Require(!EscapeLeaf(scenario,48005)&&player.HasAura(211686),"Reclaim completed before native two seconds");
  auto const playerGuid=player.GetGUID(),cacheGuid=mode==18?ObjectGuid::Empty:cache.GetGUID();if(mode==20){auto guid=playerGuid;guid.SetRawValue(guid.GetHighPart()^1,guid.GetLowPart());player.SetGuidValue(OBJECT_FIELD_GUID,guid);}if(mode==21){auto guid=cacheGuid;guid.SetRawValue(guid.GetHighPart()^1,guid.GetLowPart());cache.SetGuidValue(OBJECT_FIELD_GUID,guid);}if(mode==22)cache.RemoveFromWorld();if(mode==7){player.SetQuestStatus(41924,QUEST_STATUS_NONE);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);}if(mode==8)cache.SetPhaseMask(2,false);if(mode==9)player.Relocate(cagePose);if(mode==10)player.CastStop();if(mode==19){auto status=player.getQuestStatus(41924);status->ScriptObjects=std::make_shared<QuestObjectScope>();status->ScriptObjects->Owner=player.GetGUID();}
  if(mode==20){auto active=player.GetCurrentSpell(CURRENT_GENERIC_SPELL);Require(active,"Native pending identity endpoint missing");auto result=(active->*Access(EscapeCheckTag{}))(false);
#ifdef DEVOURER_ESCAPE_BASELINE
  Require(result==SPELL_CAST_OK,"Pinned fullGUID check no longer admits changed identity");
#else
  Require(result!=SPELL_CAST_OK,"Current fullGUID check admitted changed identity");
#endif
  // A live Player GUID is also the native aura application key: restore it
  // before updating those unrelated aura maps. This is a CheckCast endpoint.
  player.SetGuidValue(OBJECT_FIELD_GUID,playerGuid);}
  EscapeTick(player,1500);
#ifdef DEVOURER_ESCAPE_BASELINE
  bool reclaimed=mode!=10;
#else
  bool reclaimed=mode!=7&&mode!=8&&mode!=9&&mode!=10&&mode!=18&&mode!=19&&mode!=21&&mode!=22;
#endif
  Require(EscapeLeaf(scenario,48005)==uint64(reclaimed)&&player.HasAura(211686)==!reclaimed,"Native timed reclaim/cancel/source receipt differs");
  player.SetGuidValue(OBJECT_FIELD_GUID,playerGuid);if(mode!=18)cache.SetGuidValue(OBJECT_FIELD_GUID,cacheGuid);if(mode==22)cache.AddToWorld();if(!reclaimed&&mode!=18){player.Relocate(cachePose);cache.SetPhaseMask(1,false);EscapeRetry(player,cache);Require(EscapeLeaf(scenario,48005)==1&&!player.HasAura(211686),"Fresh actual reclaim retry failed");reclaimed=true;}
  if(reclaimed){Require(player.HasItemCount(136802,1)&&scenario->GetCurrentStep()==2,"Release/reclaim consumed key or skipped door");player.Relocate(doorPose);door.Use(&player);Require(DoorCredit(scenario)==1&&scenario->GetCurrentStep()==3&&door.GetPhaseMask()==4&&barrier.GetPhaseMask()==4&&!player.HasItemCount(136802,1),"Actual cage/cache/door sequence differs");door.Use(&player);Require(DoorCredit(scenario)==1,"Repeated door progress changed");}
 }
 player.CastStop();player.RemoveAllAuras();player.m_Events.KillAllEvents(true);std::cout<<"CASE race="<<race<<" mode="<<mode<<" cage="<<EscapeLeaf(scenario,48002)<<" cache="<<EscapeLeaf(scenario,48005)<<" door="<<DoorCredit(scenario)<<std::endl;
 }session->SetPlayer(nullptr);otherSession->SetPlayer(nullptr);delete session;delete otherSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef DEVOURER_ESCAPE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native escape contexts: ordinary unqualified Use, premature Hello hides uncompleted cache, timed native reclaim can survive canceled/replaced quest or lost source. Earlier quest/key/stage/poses seeded; client separate\n";
#else
 std::cout<<"PASS "<<cases<<" native escape contexts: qualified actual Use, native211689/211687/two-second cast/10ms aura timers, no Hello progress, real receipt/source recheck, interruption/fresh retry, group/pet/guardian and complete50536 door sequence once. Earlier quest/key/stage/poses seeded; client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}

#endif
