// OPEN observational reproduction; cached accepted-intro endpoint/earlier state controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "../tests/SlaghammerCompanionNative.cpp"
struct HandoffInvokerTag{using Type=ObjectGuid SmartScript::*;friend Type Access(HandoffInvokerTag);};
template struct FixtureAccess<HandoffInvokerTag,&SmartScript::mLastInvoker>;
struct HandoffEventsTag{using Type=SmartAIEventList SmartScript::*;friend Type Access(HandoffEventsTag);};
template struct FixtureAccess<HandoffEventsTag,&SmartScript::mEvents>;
static uint64 Credit(Scenario* scenario,uint32 asset){uint64 n=0;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()){auto tree=sCriteriaTreeStore.LookupEntry(row.first);auto c=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;if(c&&c->Type==92&&c->Asset==asset)n+=row.second.Counter;}return n;}
static uint32 Summons(Player& p){uint32 n=0;for(auto const& guid:p.tempSummonList[101899])if(auto c=ObjectAccessor::GetCreature(p,guid))if(c->IsInWorld())++n;return n;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);auto original=sObjectMgr->GetCreatureTemplate(101430);Require(original&&original->AIName=="SmartAI"&&!original->ScriptID,"Observational probe requires original unbound101430;do not bypass a future owner controller");sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("smart_scripts","entryorguid IN(101430,101899,101461) AND source_type=0");Shadow("conditions","SourceTypeOrReferenceId=22 AND SourceEntry IN(101430,101899,101461)");Shadow("creature_text","CreatureID IN(101430,101899)");Shadow("waypoints","entry=101430");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sSmartWaypointMgr->LoadFromDB();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sCreatureTextMgr->LoadCreatureTexts();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<5;++mode){MapInstanced parent(1539,300);InstanceMap map(1539,300,9941000+cases,DIFFICULTY_N_SCENARIO,&parent);auto scenario=sScenarioMgr->AddScenario(&map,991);scenario->SetCurrentStep(2);auto first=Session(),second=Session();{
  Position pose{2020.16f,2337.81f,75.2808f,0},ownerPose=pose,visitorPose=pose;ownerPose.m_positionX+=(mode==0?1:mode==3?100:15);visitorPose.m_positionX+=(mode==0?15:1);NativePlayer owner(first,map,101,ownerPose),visitor(second,map,102,visitorPose);
  for(auto p:{&owner,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);p->setFactionForRace(race);p->SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,40710);}if(mode!=2){visitor.SetQuestStatus(40710,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);}std::unique_ptr<CreditParty> party;if(mode==2)party=std::make_unique<CreditParty>(&owner,&visitor);
  Actor actor(&map,101430,32001,pose);Require(map.AddToMap(static_cast<Creature*>(&actor))&&actor.AIM_Initialize(new SmartAI(&actor)),"Actual native static actor failed");auto ai=dynamic_cast<SmartAI*>(actor.AI());ai->GetScript()->*Access(HandoffInvokerTag{})=mode==4?ObjectGuid::Empty:owner.GetGUID(); // controlled accepted-intro endpoint; unknown LINK2 not invoked.
  uint32 target=0;for(auto const& e:ai->GetScript()->*Access(HandoffEventsTag{}))if(e.event_id==6){std::unique_ptr<ObjectList> selected(ai->GetScript()->GetTargets(e));if(selected&&!selected->empty())target=selected->front()->GetGUIDLow();}
  ai->SetData(1,1);auto stage=scenario->GetCurrentStep();auto credit=Credit(scenario,48160);ai->GetScript()->OnUpdate(8001);ai->GetScript()->OnUpdate(10001);owner.Unit::Update(250);visitor.Unit::Update(250);
  std::cout<<"CASE handoff race="<<race<<" mode="<<mode<<" criterionTarget="<<target<<" credit48160="<<credit<<" stage="<<stage<<" ownerSummons="<<Summons(owner)<<" visitorSummons="<<Summons(visitor)<<" invoker="<<(ai->GetScript()->*Access(HandoffInvokerTag{})).GetCounter()<<'\n';CleanStories(map);party.reset();
 }first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;}
 std::cout<<"PASS research "<<cases<<" controlled late handoff observations; earlier intro/unknownLINK2 not traversed,no full client claim\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
