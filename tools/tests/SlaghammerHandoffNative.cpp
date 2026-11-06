// Registered whole1539 instance plus actual derived SmartAI/native summon.
// Earlier accepted intro is explicitly controlled; unknown LINK2 remains OPEN.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "PlayerDefines.h"
#include "GameObjectModel.h"
#include "../../src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/silver_hand_first_fight.h"
#include "../../src/server/scripts/Scenario/Artifacts/Priest/slaghammer_handoff.h"
#include "SilverHandInstance.inc"
struct HandoffCleanupTag { using Type=std::atomic<bool> Unit::*;friend Type Access(HandoffCleanupTag); };
template struct FixtureAccess<HandoffCleanupTag,&Unit::m_cleanupDone>;
struct HandoffDoor : GameObject { ~HandoffDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();} };

static uint32 Companions(Player& player)
{
    uint32 count = 0;
    for (auto const& guid : *player.GetSummonList(101899))
        if (auto creature = ObjectAccessor::GetCreature(player, guid))
            if (creature->IsInWorld() && !creature->IsPreDelete()) ++count;
    return count;
}
int main(int argc, char** argv) { try {
    InitializeAltheaGuardsNative(argc, argv);
    sObjectMgr->LoadPageTexts(); sObjectMgr->LoadGameObjectTemplate();sObjectMgr->LoadCreatureTemplateAddons();
    LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("smart_scripts", "entryorguid IN(101122,101430,101899,101461) AND source_type IN(0,9)");
    Shadow("conditions", "SourceTypeOrReferenceId=22 AND SourceEntry IN(101122,101430,101899,101461)");
    Shadow("creature_text", "CreatureID IN(101430,101899)"); Shadow("waypoints", "entry=101430");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSmartWaypointMgr->LoadFromDB(); sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions(); sCreatureTextMgr->LoadCreatureTexts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    new instance_silver_hand; sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});
    uint32 cases=0;
#ifdef SLAGHAMMER_HANDOFF_OBSERVE_INTRO
    for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)}){
        MapInstanced parent(1539,300);InstanceMap map(1539,300,9963000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
        auto instance=dynamic_cast<instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());auto scenario=sScenarioMgr->AddScenario(&map,991);scenario->SetCurrentStep(1);
        auto session=Session();{
            Position pose{2020.16f,2337.81f,75.2808f,0},ownerPose=pose;ownerPose.m_positionX+=1;
            NativePlayer owner(session,map,101,ownerPose);owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);
            Actor actor(&map,101430,36001,pose);Require(map.AddToMap(static_cast<Creature*>(&actor))&&actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Intro actor missing");auto ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());
            CreditPet pet(owner,map);ai->GetScript()->OnUpdate(71);ai->MoveInLineOfSight_Safe(&pet);Require(instance->slaghammer.phase==SlaghammerHandoff::State::Unclaimed&&scenario->GetCurrentStep()==1,"Pet consumed original-owner admission");
            ai->MoveInLineOfSight_Safe(&owner);owner.Unit::Update(250);
            std::cout<<"OBSERVE intro near="<<SlaghammerHandoff::Near(&owner,&actor,70)<<" los="<<actor.IsWithinLOSInMap(&owner)<<" hostile="<<actor.IsHostileTo(&owner)<<" stage="<<uint32(scenario->GetCurrentStep())<<" root="<<ai->GetScript()->HasRunEvent(0,SMART_EVENT_OOC_LOS)<<'\n';
            Require(instance->slaghammer.phase==SlaghammerHandoff::State::Waiting&&instance->slaghammer.owner==owner.GetGUID()&&scenario->GetCurrentStep()==2&&SlaghammerHandoff::HasEvent(scenario,48121),"Actual root0 acceptance did not record original owner");
            std::cout<<"OPEN intro race="<<race<<" root0/48121 accepted,original owner retained;root1->missingLINK2 is deliberately unresolved and must remain in DBErrors\n";CleanStories(map);
        }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
    std::cout<<"OPEN "<<cases<<" initial native admissions captured;not a clean ordinary quest test\n";CloseAltheaNative();return 0;
#endif
    for (uint32 race : {uint32(RACE_HUMAN),uint32(RACE_BLOODELF)}) for (uint32 mode=0;mode<17;++mode) {
#ifdef SLAGHAMMER_HANDOFF_CAST_FAILURE
        if(mode!=9) continue;
#else
        if(mode==9) continue;
#endif
        MapInstanced parent(1539,300); InstanceMap map(1539,300,9961000+cases,DIFFICULTY_N_SCENARIO,&parent);
        map.CreateInstanceData(nullptr);
        auto instance=dynamic_cast<instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());
        Require(instance,"Actual shared1539 factory missing");
        auto scenario=sScenarioMgr->AddScenario(&map,991); scenario->SetCurrentStep(2);
        auto first=Session(),second=Session(); {
            Position pose{2020.16f,2337.81f,75.2808f,0},ownerPose=pose;
            ownerPose.m_positionX+=15;
            NativePlayer owner(first,map,101,ownerPose),visitor(second,map,102,pose);
            for(auto player:{&owner,&visitor}) {
                player->SetByteValue(UNIT_FIELD_BYTES_0,0,race); player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);
                player->setFactionForRace(race); player->SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE); player->SetQuestSlot(0,40710);
            }
            std::unique_ptr<CreditParty> party;
            if(mode==1) party=std::make_unique<CreditParty>(&owner,&visitor);
            else {visitor.SetQuestStatus(40710,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);}
            Actor actor(&map,101430,34001,pose);
            Require(map.AddToMap(static_cast<Creature*>(&actor)) && actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Actual derived SmartAI failed");
            auto ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI()); auto& state=instance->slaghammer;
            std::unique_ptr<Actor> ward;
            std::vector<std::unique_ptr<HandoffDoor>> doors;
            if(mode!=11) {
                Position at=pose;at.m_positionY+=8;ward=std::make_unique<Actor>(&map,101461,34002,at);
                Require(map.AddToMap(static_cast<Creature*>(ward.get()))&&ward->AIM_Initialize(new SmartAI(ward.get()))&&ward->HasAura(200984),"Native ward missing");
                for(uint32 number=0;number<2;++number){at=pose;at.m_positionX-=5+number;auto door=std::make_unique<HandoffDoor>();Require(door->Create(34003+number,251349,&map,1,at,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(door.get())),"Native barrier missing");doors.push_back(std::move(door));}
            }
            if(mode!=2) state.Claim(&actor,&owner,owner.GetQuestCancellationReceipt(40710)); // controlled admitted root0 endpoint, not missingLINK2.
            if(mode==3) owner.Relocate(pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);
            if(mode==4) owner.setDeathState(JUST_DIED);
            if(mode==5) owner.SetPhaseMask(2,false);
            if(mode==6) {owner.SetQuestStatus(40710,QUEST_STATUS_NONE);owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);}
            std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;std::unique_ptr<Actor> cultist;
            if(mode>=12&&mode<=14){
                if(mode==13)pet=std::make_unique<CreditPet>(owner,map);
                if(mode==14)guardian=std::make_unique<CreditGuardian>(owner,map);
                cultist=std::make_unique<Actor>(&map,101122,34005,pose);Require(map.AddToMap(static_cast<Creature*>(cultist.get()))&&cultist->AIM_Initialize(new SmartAI(cultist.get())),"Native cultist missing");
                cultist->AI()->JustDied(pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&owner));
                Require(state.pending,"Actual cultist death did not deliver DATA1/1");
            }else ai->SetData(1,1);
            ai->SetData(1,1);ai->UpdateLifecycle(1);
            bool paused=mode>=2 && mode<=6;
            Require(scenario->GetCurrentStep()==(paused?2:3),"Wrong initial handoff admission");
            if(mode==2||mode==6) {
                ai->UpdateLifecycle(30000);Require(!SlaghammerHandoff::HasEvent(scenario,48160)&&!Companions(owner)&&!Companions(visitor),"Unclaimed/cancelled attempt progressed");
            } else {
                if(mode==3) owner.Relocate(ownerPose);
                if(mode==4) owner.setDeathState(ALIVE);
                if(mode==5) owner.SetPhaseMask(1,false);
                if(paused)ai->UpdateLifecycle(1);
                Require(scenario->GetCurrentStep()==3 && SlaghammerHandoff::HasEvent(scenario,48160),"Recovered owner did not receive48160");
                if(mode==7) {ai->Reset();Require(actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"AI replacement failed");ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());}
                if(mode==8) {owner.Relocate(pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);ai->UpdateLifecycle(30000);Require(state.phase==SlaghammerHandoff::State::FirstText,"Absent owner consumed text checkpoint");owner.Relocate(ownerPose);}
                ai->UpdateLifecycle(7999);Require(state.phase==SlaghammerHandoff::State::FirstText,"First speech shortened");
                ai->UpdateLifecycle(1);Require(state.phase==SlaghammerHandoff::State::SecondText,"Native8s first speech missing");
                ai->UpdateLifecycle(9999);Require(!Companions(owner),"Early companion before10s speech");
                if(mode==9) owner.*Access(HandoffCleanupTag{})=true; // controlled native Unit::CastSpell early rejection,not real logout.
                ai->UpdateLifecycle(1);owner.Unit::Update(250);
                if(mode==9) {
                    Require(!Companions(owner)&&state.phase==SlaghammerHandoff::State::Summon&&actor.IsInWorld(),"Failed native summon retired source");
                    std::cout<<"FAULT native cast rejection="<<uint32(SPELL_FAILED_ERROR)<<"; expected warning,paired cleared-state retry follows\n";
                    owner.*Access(HandoffCleanupTag{})=false;ai->UpdateLifecycle(5000);owner.Unit::Update(250);
                }
                if(mode==10) {ai->Reset();Require(actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Post-summon AI replacement failed");ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());}
                Require(Companions(owner)==1 && !Companions(visitor) && FollowCredit(scenario)==1,"Native owner companion/48306 missing or duplicated");
                ai->UpdateLifecycle(5000);Require(state.phase==SlaghammerHandoff::State::Finished,"Actual owned summon not acknowledged");
                if(mode==15){Require(actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Retiring AI replacement failed");ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());ai->UpdateLifecycle(1);}
                if(mode==16){ai->Reset();ai->UpdateLifecycle(1);}
                ai->SetData(1,1);ai->UpdateLifecycle(30000);Require(Companions(owner)==1&&FollowCredit(scenario)==1,"Repeated handoff duplicated companion/credit");
                if(ward)Require(!ward->HasAura(200984),"Shared ward cleanup lost");
                for(auto const& door:doors)Require(door->GetGoState()==GO_STATE_ACTIVE,"Native distance target did not open every barrier");
                ai->UpdateAI(2);Require(!actor.IsVisible(),"Acknowledged native retirement did not hide source");
                ai->UpdateLifecycle(5001);ai->UpdateAI(5001);Require(actor.IsDespawn()&&!actor.IsAlive(),"Native5s static-source despawn not completed");
            }
            std::cout<<"CASE handoff race="<<race<<" mode="<<mode<<" phase="<<state.phase<<" companions="<<Companions(owner)<<" foreign="<<Companions(visitor)<<'\n';
            CleanStories(map);party.reset();
        }first->SetPlayer(nullptr);second->SetPlayer(nullptr);delete first;delete second;
        sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
#ifndef SLAGHAMMER_HANDOFF_CAST_FAILURE
    for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 abandon=0;abandon<2;++abandon){
        MapInstanced parent(1539,300);InstanceMap map(1539,300,9962000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
        auto instance=dynamic_cast<instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());
        auto scenario=sScenarioMgr->AddScenario(&map,991);scenario->SetCurrentStep(2);Position pose{2020.16f,2337.81f,75.2808f,0};
        Actor actor(&map,101430,35001,pose);Require(map.AddToMap(static_cast<Creature*>(&actor))&&actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Re-entry actor missing");
        auto ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());auto first=Session();{
            NativePlayer owner(first,map,101,pose);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);
            instance->slaghammer.Claim(&actor,&owner,owner.GetQuestCancellationReceipt(40710));ai->SetData(1,1);
        }first->SetPlayer(nullptr);delete first;
        ai->UpdateLifecycle(30000);Require(scenario->GetCurrentStep()==2,"Disconnected owner advanced");
        auto second=Session();{
            NativePlayer owner(second,map,101,pose);owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);
            auto local=owner.GetQuestCancellationReceipt(40710);Require(local!=instance->slaghammer.attempt,"Player replacement did not create fresh local receipt");instance->OnPlayerEnter(&owner);Require(owner.GetQuestCancellationReceipt(40710)==instance->slaghammer.attempt,"Instance did not reattach original receipt");
            if(abandon){owner.SetQuestStatus(40710,QUEST_STATUS_NONE);owner.SetQuestStatus(40710,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,40710);Require(local->load()&&instance->slaghammer.Cancelled(),"Re-entry abandon failed to mirror cancellation");}
            ai->UpdateLifecycle(1);Require(scenario->GetCurrentStep()==(abandon?2:3),"Re-entry/retake admission wrong");
            std::cout<<"CASE replacement race="<<race<<" abandon="<<abandon<<" stage="<<uint32(scenario->GetCurrentStep())<<'\n';CleanStories(map);
        }second->SetPlayer(nullptr);delete second;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
#endif
    std::cout<<"PASS "<<cases<<" native handoff contexts; earlier intro/unknownLINK2 controlled,not client traversal\n";
    CloseAltheaNative();return 0;
}catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}}
