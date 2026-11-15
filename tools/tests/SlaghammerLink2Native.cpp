// Actual SmartScript LINK dispatcher/conditions/AttackStart and registered
// instance-owned handoff. Controlled accepted quest/poses/clock; no client save.
#define main RetainedHandoffMain
#include "SlaghammerHandoffNative.cpp"
#undef main
#include "SlaghammerLink2Migration.inc"
int main(int argc,char** argv){try{
    InitializeAltheaGuardsNative(argc,argv);sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();sObjectMgr->LoadCreatureTemplateAddons();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    Shadow("smart_scripts","entryorguid IN(101122,101430,101899,101461) AND source_type IN(0,9)");
    Shadow("conditions","SourceTypeOrReferenceId=22 AND SourceEntry IN(101122,101430,101899,101461)");
    Shadow("creature_text","CreatureID IN(101430,101899)");Shadow("waypoints","entry=101430");
    // Only these TEMP shadow tables are mutable; other statements are guarded
    // reads and session variables. Never mutate persistent World tables.
    for(auto sql:SlaghammerMigration)WorldDatabase.DirectExecute(sql);
#ifdef SLAGHAMMER_LINK2_BASELINE
    // The guarded migration first rejects foreign/custom rows. Reconstruct
    // only the known absent-node counterfactual in these TEMP tables, making
    // the regression reusable after the real updater has installed the fix.
    WorldDatabase.DirectExecute("DELETE FROM smart_scripts WHERE @slaghammer_link2_safe AND entryorguid=101430 AND source_type=0 AND id=2");
    WorldDatabase.DirectExecute("DELETE FROM conditions WHERE @slaghammer_link2_safe AND SourceTypeOrReferenceId=22 AND SourceEntry=101430 AND SourceGroup=3");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSmartWaypointMgr->LoadFromDB();sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();sCreatureTextMgr->LoadCreatureTexts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_silver_hand;sScriptMgr->SwapScriptContext(true);
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1539,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1539,{}}});
    // Full native Kill requires the actual guard's authored loot input too.
    auto guardInfo=sObjectMgr->GetCreatureTemplate(101122);Require(guardInfo,"Guard template missing");
    auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto oldName=lootName;lootName+=" WHERE Entry IN(108218,101122) ORDER BY Entry,Item";(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=oldName;Require(!guardInfo->lootid||LootTemplates_Creature.HaveLootFor(guardInfo->lootid),"Actual guard loot not loaded");
    uint32 cases=0;
    for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<9;++mode){
#ifdef SLAGHAMMER_LINK2_BASELINE
        if(mode)continue;
#endif
        MapInstanced parent(1539,300);InstanceMap map(1539,300,9953000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
        auto instance=dynamic_cast<instance_silver_hand::instance_silver_hand_InstanceMapScript*>(map.GetInstanceScript());auto scenario=sScenarioMgr->AddScenario(&map,mode==6?1092:991);scenario->SetCurrentStep(1);
        auto session=Session();{
            Position pose{2044.88f,2334.23f,68.8537f,0},ownerPose=pose;ownerPose.m_positionX+=1;
            NativePlayer owner(session,map,201,ownerPose);owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,mode==5?CLASS_WARRIOR:mode==6?CLASS_PALADIN:CLASS_PRIEST);owner.SetQuestStatus(40710,mode==8?QUEST_STATUS_NONE:QUEST_STATUS_INCOMPLETE);if(mode!=8)owner.SetQuestSlot(0,40710);owner.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PVP_ATTACKABLE);
            Actor actor(&map,101430,46001,pose);Require(map.AddToMap(static_cast<Creature*>(&actor))&&actor.AIM_Initialize(new SlaghammerHandoff::AI(&actor)),"Actual Slaghammer AI admission failed");auto ai=dynamic_cast<SlaghammerHandoff::AI*>(actor.AI());
            std::unique_ptr<Actor> guard;
            if(mode!=4){guard=std::make_unique<Actor>(&map,101122,46002,Position{2021.01f,2330.53f,75.2914f,0});Require(map.AddToMap(static_cast<Creature*>(guard.get()))&&guard->AIM_Initialize(new SmartAI(guard.get())),"Actual guard SmartAI missing");}
            CreditPet pet(owner,map);CreditGuardian guardian(owner,map);
            ai->GetScript()->OnUpdate(71);ai->MoveInLineOfSight_Safe(&pet);Require(scenario->GetCurrentStep()==1,"Pet consumed player-only initial admission");
            if(mode==7)owner.setDeathState(JUST_DIED);
            ai->MoveInLineOfSight_Safe(&owner);owner.Unit::Update(250);
            bool admitted=mode<5;
            Require(scenario->GetCurrentStep()==(admitted?2:1),"Native intro admitted an ineligible context");
#ifdef SLAGHAMMER_LINK2_BASELINE
            Require(!actor.getVictim(),"Missing-action counterfactual unexpectedly starts combat");
#else
            Require(actor.getVictim()==(admitted&&guard?guard.get():nullptr),"Native LINK2 failed exact guard attack or attacked an ineligible context");
            if(admitted)Require(instance->slaghammer.owner==owner.GetGUID()&&instance->slaghammer.phase==SlaghammerHandoff::State::Waiting,"Original admitted owner lost");
            ai->MoveInLineOfSight_Safe(&owner);Require(!SlaghammerHandoff::HasEvent(scenario,48160)&&!Companions(owner),"Intro/repeat manufactured handoff credit or companion");
            if(admitted&&guard){
                Unit* killer=mode==1?static_cast<Unit*>(&pet):mode==2?static_cast<Unit*>(&guardian):mode==3?static_cast<Unit*>(&actor):static_cast<Unit*>(&owner);
                guard->LowerPlayerDamageReq(uint32(guard->GetMaxHealth()));guard->SetLootRecipient(&owner);killer->Kill(guard.get(),false);
                Require(!guard->IsAlive()&&instance->slaghammer.pending,"Real guard death did not request owner handoff");
                // Native combat cleanup precedes waypoint admission; do not
                // force CombatStop or synthesize a death/criterion callback.
                for(unsigned i=0;i<100&&scenario->GetCurrentStep()==2;++i){actor.Update(100);map.MoveAllCreaturesInMoveList();}
                Require(scenario->GetCurrentStep()==3&&SlaghammerHandoff::HasEvent(scenario,48160),"Native combat prevented real-death handoff");
                ai->UpdateLifecycle(8000);ai->UpdateLifecycle(10000);owner.Unit::Update(250);ai->UpdateLifecycle(5000);
                Require(Companions(owner)==1&&FollowCredit(scenario)==1&&instance->slaghammer.phase==SlaghammerHandoff::State::Finished,"Post-combat native companion/acknowledgement failed");
                ai->SetData(1,1);ai->UpdateLifecycle(30000);Require(Companions(owner)==1&&FollowCredit(scenario)==1,"Repeated real-death handoff duplicated progress");
            }
#endif
            std::cout<<"CASE reconstructed LINK2 race="<<race<<" mode="<<mode<<" stage="<<uint32(scenario->GetCurrentStep())<<'\n';CleanStories(map);
        }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
    }
#ifdef SLAGHAMMER_LINK2_BASELINE
    Require(cases==2,"Expected both baseline races");std::cout<<"COUNTERFACTUAL2 missing node: no native guard combat\n";
#else
    Require(cases==18,"Expected18 repaired contexts");std::cout<<"PASS18 reconstructed LINK2 native guard combat and real-death handoff; original payload/client fidelity unproven\n";
#endif
    CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
