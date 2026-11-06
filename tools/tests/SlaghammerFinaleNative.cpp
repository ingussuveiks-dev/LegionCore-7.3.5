// Native SmartAI finale: earlier quest/stage, poses and arrivals are controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "GameObjectModel.h"
struct FinaleEventsTag{using Type=SmartAIEventList SmartScript::*;friend Type Access(FinaleEventsTag);};
template struct FixtureAccess<FinaleEventsTag,&SmartScript::mEvents>;
struct FinaleSceneTag{using Type=std::unordered_map<uint32,uint32> Player::*;friend Type Access(FinaleSceneTag);};
template struct FixtureAccess<FinaleSceneTag,&Player::m_sceneInstanceID>;
struct FinaleInvokerTag{using Type=ObjectGuid SmartScript::*;friend Type Access(FinaleInvokerTag);};
template struct FixtureAccess<FinaleInvokerTag,&SmartScript::mLastInvoker>;
static bool Consumed(SmartAI* ai,uint32 id){for(auto const& e:ai->GetScript()->*Access(FinaleEventsTag{}))if(e.event_id==id)return e.runOnce;throw std::runtime_error("Native finale event missing");}
static uint32 Scenes(Player const& p){uint32 n=0;for(auto const& v:p.*Access(FinaleSceneTag{}))if(v.second==1182)++n;return n;}
struct FinaleDoor:GameObject{~FinaleDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
static void Project(){
#ifdef SLAGHAMMER_FINALE_BASELINE
 WorldDatabase.DirectExecute("UPDATE smart_scripts SET action_type=85,action_param3=0,target_type=21,target_param1=50 WHERE id=9 AND entryorguid=101899");
 WorldDatabase.DirectExecute("DELETE FROM conditions WHERE SourceEntry=101899 AND SourceGroup=9 AND ConditionTypeOrReference<>44");
#elif !defined(SLAGHAMMER_FINALE_LIVE)
 WorldDatabase.DirectExecute("UPDATE smart_scripts SET action_type=86,action_param3=23,target_type=23,target_param1=0 WHERE id=9 AND entryorguid=101899");
 for(auto values:{"31,0,4,0,0,'Slaghammer finale: actual Player'","36,0,0,0,0,'Slaghammer finale: live Player'","9,0,40710,0,0,'Slaghammer finale: active quest'","15,0,16,0,0,'Slaghammer finale: Priest'","33,1,0,6,0,'Slaghammer finale: actual summoner'","22,1,1539,0,0,'Slaghammer finale: native map'"}){
  auto command=std::string("INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,Comment) SELECT 22,9,101899,")+values+" WHERE NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=9 AND SourceEntry=101899 AND ConditionTypeOrReference="+std::string(values).substr(0,std::string(values).find(','))+")";
  WorldDatabase.DirectExecute(command.c_str());
 }
#endif
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();sObjectMgr->LoadCreatureTemplateAddons();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("smart_scripts","entryorguid IN(101899,101461) AND source_type IN(0,9)");Shadow("conditions","SourceTypeOrReferenceId=22 AND SourceEntry IN(101899,101461)");Shadow("creature_text","CreatureID=101899");Project();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sCreatureTextMgr->LoadCreatureTexts();sSpellMgr->LoadSpellScene();Require(sSpellMgr->GetSpellScene(1182)&&sSpellMgr->GetSpellScene(1182)->SceneScriptPackageID==1551,"Actual scene1182/1551 missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});
 auto properties=sSummonPropertiesStore.LookupEntry(3747);Require(properties,"Native properties missing");uint32 cases=0;
#ifdef SLAGHAMMER_FINALE_BASELINE
 uint32 modes=4;
#else
 uint32 modes=23;
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1539,300);InstanceMap map(1539,300,9927000+cases,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,991);scenario->SetCurrentStep(3);auto first=Session(),second=Session();
  {Position pose{2020.16f,2337.81f,75.2808f,0},ownerPose=pose;ownerPose.m_positionX+=2;NativePlayer owner(first,map,101,ownerPose),visitor(second,map,102,pose);
   for(auto p:{&owner,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);p->setFactionForRace(race);p->SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40710);}visitor.SetQuestStatus(40710,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);
   auto companion=map.SummonCreature(101899,pose,properties,30000,&owner,{},201004);Require(companion,"Native companion missing");auto ai=dynamic_cast<SmartAI*>(companion->AI());Require(ai,"Native SmartAI missing");scenario->SetCurrentStep(7);
#ifndef SLAGHAMMER_FINALE_BASELINE
   Require(companion->HasQuestObjectScope()&&!companion->IsQuestObjectExpired(),"Real factory did not bind current40710 attempt before AI");
#endif
   std::unique_ptr<CreditPet> pet;if(mode==2){pet=std::make_unique<CreditPet>(owner,map);pet->Relocate(pose);}
   std::unique_ptr<CreditGuardian> guardian;if(mode==20){guardian=std::make_unique<CreditGuardian>(owner,map);guardian->Relocate(pose);}
   std::unique_ptr<Actor> friendly;if(mode==6){friendly=std::make_unique<Actor>(&map,101899,31001,pose);Require(map.AddToMap(static_cast<Creature*>(friendly.get())),"Native friendly actor missing");}
   std::unique_ptr<CreditParty> party;if(mode==5){visitor.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);visitor.SetQuestSlot(0,40710);party=std::make_unique<CreditParty>(&owner,&visitor);}
   std::unique_ptr<Actor> ward;std::unique_ptr<FinaleDoor> gate;
   if(mode==16){auto unrelated=map.SummonCreature(101899,pose,properties,30000,&owner,{},0);Require(unrelated&&!unrelated->HasQuestObjectScope(),"Unrelated summon inherited Slaghammer binding");unrelated->UnSummon();map.RemoveAllObjectsInRemoveList();}
   if(mode!=16){Position wardPose=pose;wardPose.m_positionY+=8;ward=std::make_unique<Actor>(&map,101461,31002,wardPose);Require(map.AddToMap(static_cast<Creature*>(ward.get()))&&ward->AIM_Initialize(new SmartAI(ward.get())),"Native ward missing");Require(ward->HasAura(200984),"Original ward aura missing");Position doorPose=pose;doorPose.m_positionX-=5;gate=std::make_unique<FinaleDoor>();Require(gate->Create(31003,251349,&map,1,doorPose,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(gate.get())),"Native barrier missing");}
   if(mode==3)owner.ApplySpellImmune(0,IMMUNITY_ID,201321,true);
   if(mode==4){visitor.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);visitor.SetQuestSlot(0,40710);}
   if(mode==7)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
   if(mode==8)owner.SetQuestStatus(40710,QUEST_STATUS_COMPLETE);
   if(mode==9)owner.Unit::setDeathState(JUST_DIED);
   if(mode==10){owner.SetPhaseMask(2,false);companion->SetPhaseMask(1,false);Require(!companion->InSamePhase(&owner),"Controlled phase mismatch missing");}
   if(mode==11)owner.Relocate(pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);
   if(mode==12)scenario->SetCurrentStep(6);
   if(mode==21)owner.SetQuestStatus(40710,QUEST_STATUS_NONE);
   if(mode==22)owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
   Unit* who=(mode==1||mode==4||mode==5||mode==7)?static_cast<Unit*>(&visitor):mode==2?static_cast<Unit*>(pet.get()):mode==6?static_cast<Unit*>(friendly.get()):mode==20?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&owner);
   ai->GetScript()->OnUpdate(10); // Authored LOS union initializes native1..5ms admission timer.
   ai->MoveInLineOfSight_Safe(who);
#ifndef SLAGHAMMER_FINALE_BASELINE
   if(mode==1||mode==2||mode==4||mode==5||mode==6||mode==7||mode==8||mode==9||mode==10||mode==11||mode==12||mode==20||mode==21||mode==22){
    Require(!Consumed(ai,8)&&!Scenes(owner)&&!Scenes(visitor),"Ineligible arrival consumed finale");owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.Unit::setDeathState(ALIVE);owner.SetHealth(owner.GetMaxHealth());owner.SetPhaseMask(1,false);owner.Relocate(ownerPose);scenario->SetCurrentStep(7);
    if(mode==9||mode==21){
     auto retired=companion->GetGUID();Require(companion->IsDespawn()&&!companion->IsInWorld(),"Native death/cancellation no longer removes personal companion");map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetCreature(owner,retired),"Native death/cancellation retained old actor");
     Require(ward->HasAura(200984)&&gate->GetGoState()==GO_STATE_READY,"Death/cancellation unexpectedly completed shared finale");
     companion=map.SummonCreature(101899,pose,properties,30000,&owner,{},201004);Require(companion&&companion->GetGUID()!=retired,"Controlled native201004 recovery summon failed");ai=dynamic_cast<SmartAI*>(companion->AI());ai->GetScript()->OnUpdate(10);
    }
    ai->MoveInLineOfSight_Safe(&owner);
   }
#endif
   if(!Consumed(ai,8)){
    std::cerr<<"Finale admission race="<<race<<" mode="<<mode<<" nativeSummonedBy="<<companion->GetSummonedByGUID().ToString()<<" tempSummoner="<<companion->GetSummonerGUID().ToString()<<" owner="<<owner.GetGUID().ToString()<<'\n';
    std::cerr<<"actor alive="<<companion->IsAlive()<<" despawn="<<companion->IsDespawn()<<" expired="<<companion->IsQuestObjectExpired()<<" combat="<<companion->isInCombat()<<" range="<<companion->IsWithinDistInMap(&owner,5)<<" los="<<companion->IsWithinLOSInMap(&owner)<<" hostile="<<companion->IsHostileTo(&owner)<<" inWorld="<<companion->IsInWorld()<<'\n';
    for(auto const&e:ai->GetScript()->*Access(FinaleEventsTag{}))if(e.event_id==8)std::cerr<<"event active="<<e.active<<" timer="<<e.timer<<" once="<<e.runOnce<<'\n';
    for(auto c:sConditionMgr->GetConditionsForSmartEvent(101899,8,0)){ConditionSourceInfo info(&owner,companion);std::cerr<<"condition="<<c->ConditionType<<" target="<<uint32(c->ConditionTarget)<<" values="<<c->ConditionValue1<<','<<c->ConditionValue2<<','<<c->ConditionValue3<<" result="<<c->Meets(info)<<'\n';}
   }
   Require(Consumed(ai,8),"Qualified finale not started");
#ifndef SLAGHAMMER_FINALE_BASELINE
   if(mode==13){
    auto predicates=sConditionMgr->GetConditionsForSmartEvent(101899,8,0);
    for(auto c:predicates)if(c->ConditionType==CONDITION_RELATION_TO){
     ConditionSourceInfo actual(&owner,companion),foreign(&visitor,companion);
     Require(c->ConditionValue2==RELATION_SUMMONED_BY&&c->Meets(actual)&&!c->Meets(foreign),"Actual TempSummon relation failed");
     Condition legacy=*c;legacy.ConditionValue2=RELATION_OWNED_BY;Require(!legacy.Meets(actual),"Legacy field ownership changed");
     Actor ordinary(&map,101899,31101,pose);ordinary.SetGuidValue(UNIT_FIELD_SUMMONED_BY,owner.GetGUID());ConditionSourceInfo field(&owner,&ordinary);
     Require(legacy.Meets(field)&&!c->Meets(field),"Legacy ordinary-unit field relation or TempSummon type guard changed");
    }
    Actor strict(&map,101899,31100,pose);owner.BindQuestObject(40710,&strict);
    Require(strict.HasQuestObjectScope()&&strict.CanTargetQuestObject(&owner)&&!strict.CanTargetQuestObject(ward.get()),"Default quest scope no longer restricts unrelated targets");
    auto child=map.SummonCreature(101899,pose,properties,30000,companion,{},0);
    Require(child&&child->HasQuestObjectScope()&&child->CanTargetQuestObject(ward.get()),"Native inherited shared-target option lost");
   }
#endif

   if(mode==13||mode==14||mode==18||mode==19){auto old=companion->GetGUID();
    if(mode==19){Require(companion->AIM_Initialize(new SmartAI(companion)),"Native AI replacement failed");ai=dynamic_cast<SmartAI*>(companion->AI());}
    owner.CancelQuestObjects(40710);owner.SetQuestStatus(40710,QUEST_STATUS_NONE);owner.SetQuestSlot(0,0);
    if(mode==18){owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);Require(companion->AIM_Initialize(new SmartAI(companion)),"Cancelled AI replacement failed");ai=dynamic_cast<SmartAI*>(companion->AI());ai->GetScript()->OnUpdate(6000);ai->MoveInLineOfSight_Safe(&owner);Require(companion->IsQuestObjectExpired()&&!Scenes(owner),"Old actor adopted retaken quest through AI replacement");}
    map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetCreature(owner,old)&&!Scenes(owner),"Abandoned finale survived");if(mode==13){party.reset();++cases;goto finish;}
    owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);companion=map.SummonCreature(101899,pose,properties,30000,&owner,{},201004);Require(companion&&companion->GetGUID()!=old&&!companion->IsQuestObjectExpired(),"Fresh retake actor missing");ai=dynamic_cast<SmartAI*>(companion->AI());ai->GetScript()->OnUpdate(10);ai->MoveInLineOfSight_Safe(&owner);Require(Consumed(ai,8),"Fresh retake cannot start");}
   if(mode==17){ai->MoveInLineOfSight_Safe(&owner);ai->MoveInLineOfSight_Safe(&owner);Require(Consumed(ai,8),"Repeated arrival reset event");}
   if(mode==15)ai->GetScript()->*Access(FinaleInvokerTag{})=visitor.GetGUID(); // Explicit global-invoker replacement control.
   ai->GetScript()->OnUpdate(4999);Require(!Scenes(owner)&&!Scenes(visitor),"Scene before authored5s");ai->GetScript()->OnUpdate(2);owner.Unit::Update(250);visitor.Unit::Update(250);if(pet)pet->Unit::Update(250);
#ifdef SLAGHAMMER_FINALE_BASELINE
   if(mode==2){Require(!Scenes(owner)&&ObjectAccessor::GetCreature(owner,companion->GetGUID()),"Original pet stall changed");companion->UnSummon();map.RemoveAllObjectsInRemoveList();}
   else{
#endif
    auto guid=companion->GetGUID();ai->UpdateAI(2);ai->UpdateAI(5001);map.RemoveAllObjectsInRemoveList();Require(!ObjectAccessor::GetCreature(owner,guid),"Native hide/removal cleanup incomplete");
    if(ward)Require(!ward->HasAura(200984),"Native ward DATA cleanup missing");if(gate)Require(gate->GetGoState()==GO_STATE_ACTIVE,"Native barrier cleanup missing");
#ifdef SLAGHAMMER_FINALE_BASELINE
   }
   Require(Scenes(owner)==(mode==0?1u:0u)&&Scenes(visitor)==(mode==1?1u:0u),"Original recipient/refusal counterfactual changed");
#else
   Require(Scenes(owner)==(mode==3?0u:1u)&&!Scenes(visitor),"Scene not exclusively owned or optional refusal blocked cleanup");
#endif
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" ownerScene="<<Scenes(owner)<<" visitorScene="<<Scenes(visitor)<<" native cleanup\n";++cases;party.reset();CleanStories(map);
  }
 finish:first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
#ifdef SLAGHAMMER_FINALE_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native original-data finale contexts:foreign Player gets scene1182,pet consumes root and stalls;owner and optional-refusal controls retain native cleanup. Current lifetime binding is retained; earlier state/arrivals/clocks controlled,no client claim\n";
#else
 std::cout<<"PASS "<<cases<<" native finale contexts:owner-only admission,actual1182/1551 publication,original5s talk/native ward+barrier/hide/removal,independent cosmetic refusal,group/pet/foreign guards,current-attempt cancellation/retake,actual death cleanup with controlled recovery summon,missing actors and replaced invoker. No manual progress/items;client traversal remains separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}