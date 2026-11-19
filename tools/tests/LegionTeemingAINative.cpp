// Actual template AI initialization and formation combat-entry dispatch.
// No timed combat updates,damage,loot or whole-dungeon claim.
#define LEGION_TEEMING_FORMATION_SUPPORT_ONLY
#include "LegionTeemingFormationNative.cpp"
#include "SmartAI.h"
#include "CreatureTextMgr.h"
#include "ConditionMgr.h"
#include "ScriptsData.h"

struct NativeAIActor:FormationActor
{
    void AddToWorld() override { Creature::AddToWorld(); }
};

#ifndef LEGION_TEEMING_AI_SUPPORT_ONLY
int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);
        sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadEquipmentTemplates();sObjectMgr->LoadCreatureTemplateAddons();sObjectMgr->LoadScenarioData();
        sAchievementMgr->LoadCriteriaList();sAchievementMgr->LoadAchievementCriteriaData();
        sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellPrcoCheck();sSpellMgr->LoadSpellBonusess();
        sCreatureTextMgr->LoadCreatureTexts();sScriptDataStore->LoadSpellScriptNames();
        auto modelData=std::getenv("LEGION_MAW_MODEL_DATA");Require(modelData,"Explicit ship model folder missing");
        for(uint32 display:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(display,std::string(modelData)+'/'),"Matching ship navmesh missing");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");ExecuteFixture();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        for(auto table:{"smart_scripts","conditions"})
        {
            auto proof=WorldDatabase.Query(("SHOW CREATE TABLE "+std::string(table)).c_str());
            Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"AI loader target must be a temporary shadow");
        }
        sWaypointMgr->Load();sObjectMgr->LoadCreatures();sObjectMgr->LoadCreatureAddons();sFormationMgr->LoadCreatureFormations();
        sSmartScriptMgr->LoadSmartAIFromDB();sConditionMgr->LoadConditions();
        auto rows=WorldDatabase.Query("SELECT guid FROM creature ORDER BY guid");Require(bool(rows),"Native AI creature scope empty");
        do{auto guid=rows->Fetch()[0].GetUInt64();auto data=sObjectMgr->GetCreatureData(guid);Require(data,"Native AI cache missing");sObjectMgr->RemoveCreatureFromGrid(guid,data);}while(rows->NextRow());
        auto scope=WorldDatabase.Query("SELECT DISTINCT f.leaderGUID,c.map FROM creature_formations f JOIN creature c ON c.guid=f.leaderGUID UNION SELECT c.guid,c.map FROM creature c LEFT JOIN creature_formations f ON f.memberGUID=c.guid WHERE c.guid BETWEEN 900902000 AND 900902214 AND f.memberGUID IS NULL ORDER BY 1");
        Require(bool(scope),"Native AI group/solo scope empty");
        unsigned contexts=0,pulls=0,assists=0,published=0,extras=0;
        std::set<uint32> extraEntries;
        do
        {
            uint64 leaderGuid=scope->Fetch()[0].GetUInt64();uint32 mapId=scope->Fetch()[1].GetUInt32();
            std::vector<uint64> members;
            for(auto const& item:sFormationMgr->CreatureGroupMap)if(item.second->leaderGUID==leaderGuid)members.push_back(item.first);
            bool grouped=!members.empty();if(!grouped)members.push_back(leaderGuid);std::sort(members.begin(),members.end());
            auto leaderData=sObjectMgr->GetCreatureData(leaderGuid);Require(leaderData,"Actual AI leader cache absent");
            for(unsigned profile=0;profile<3;++profile)for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})for(unsigned targetKind=0;targetKind<3;++targetKind)
            {
                Difficulty difficulty=profile?DIFFICULTY_MYTHIC_KEYSTONE:DIFFICULTY_MYTHIC_DUNGEON;
                MapInstanced parent(mapId,300);InstanceMap map(mapId,300,9956000+contexts,difficulty,&parent);map.CreateInstanceData(nullptr);
                std::unique_ptr<DungeonShip> ship;if(mapId==1492)ship=std::make_unique<DungeonShip>(map);
                LifecyclePlayer owner(map,951,Position{leaderData->posX,leaderData->posY,leaderData->posZ,leaderData->orientation});
                owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);
                Scenario* scenario=nullptr;
                if(profile)
                {
                    MapChallengeModeEntry const* route=nullptr;for(auto row:sMapChallengeModeStore)if(row->MapID==mapId){route=row;break;}
                    Require(route,"Actual AI map route missing");owner.Key(*route);owner.m_challengeKeyInfo.Affix=profile==2?Affixes::Teeming:0;
                    scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Native AI challenge registration missing");scenario->CreateChallenge(&owner);
                }
                auto pet=targetKind==1?std::make_unique<DungeonPet>(owner,map):nullptr;
                auto guardian=targetKind==2?std::make_unique<DungeonGuardian>(owner,map):nullptr;
                Unit* target=pet?static_cast<Unit*>(pet.get()):guardian?static_cast<Unit*>(guardian.get()):static_cast<Unit*>(&owner);
                {
                    std::vector<std::unique_ptr<NativeAIActor>> actors;
                    for(uint64 guid:members)
                    {
                        auto data=sObjectMgr->GetCreatureData(guid);Require(data&&data->mapid==mapId,"AI group crosses maps");
                        if(!(data->spawnMask&(uint64(1)<<difficulty)))continue;
                        auto actor=std::make_unique<NativeAIActor>();bool loaded=actor->LoadCreatureFromDB(guid,&map,true);
                        Check(loaded==(!data->isTeemingSpawn||profile==2),"Real-AI publication escaped native admission");if(!loaded)continue;
                        Check(actor->AI()&&actor->IsAIEnabled,"Actual template AI did not initialize");
                        auto t=actor->GetCreatureTemplate();
                        if(t->ScriptID)Check(dynamic_cast<ScriptedAI*>(actor->AI()),"Native C++ template silently fell back to another AI");
                        else if(actor->GetAIName()=="SmartAI")Check(dynamic_cast<SmartAI*>(actor->AI()),"Native SmartAI factory binding lost");
                        if(data->isTeemingSpawn)
                        {
                            ++extras;
                            if(extraEntries.insert(actor->GetEntry()).second)std::cout<<"AI_BIND entry="<<actor->GetEntry()<<" type="<<typeid(*actor->AI()).name()<<'\n';
                        }
                        ++published;actors.push_back(std::move(actor));
                    }
                    Creature* seed=nullptr;
                    for(auto const& actor:actors)if(actor->canStartAttack(target,true)){seed=actor.get();break;}
                    if(seed)
                    {
                        std::vector<Creature*> eligible;
                        if(grouped)
                        {
                            auto info=sFormationMgr->CreatureGroupMap.at(seed->GetDBTableGUIDLow());
                            if(info->groupAI&&(info->groupAI!=1||seed->GetDBTableGUIDLow()==leaderGuid))
                                for(auto const& actor:actors)if(actor.get()!=seed&&actor->IsAlive()&&actor->IsInWorld()&&!actor->IsInEvadeMode()&&!actor->getVictim()&&actor->canStartAttack(target,true))eligible.push_back(actor.get());
                        }
                        seed->AI()->AttackStart(target);
                        Check(seed->getVictim()==target&&seed->isInCombat(),"Native initial attack/combat callback failed");++pulls;
                        for(auto member:eligible){Check(member->getVictim()==target&&member->isInCombat(),"Native eligible formation member did not assist actual attack");++assists;}
                        auto count=grouped?seed->GetFormation()->GetMembers().size():0;
                        seed->AI()->AttackStart(target);
                        Check(!grouped||seed->GetFormation()->GetMembers().size()==count,"Repeated native attack duplicated formation membership");
                        for(auto const& actor:actors){actor->CombatStop(true);actor->CastStop();}
                        for(auto const& actor:actors)Check(!actor->getVictim()&&!actor->isInCombat(),"Native combat stop retained target/combat state");
                    }
                    else if(!actors.empty()&&profile==2&&race==RACE_HUMAN&&!targetKind)
                    {
                        std::cout<<"NO_PULL leader="<<leaderGuid<<" map="<<mapId<<" members="<<actors.size();
                        for(auto const& actor:actors)std::cout<<" entry="<<actor->GetEntry()<<" react="<<unsigned(actor->GetReactState())<<" flags="<<actor->GetUInt32Value(UNIT_FIELD_FLAGS);
                        std::cout<<'\n';
                    }
                }
                Check(map.CreatureGroupHolder.empty(),"Actual AI cleanup retained deleted formation members");
                if(scenario){map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());}
                ++contexts;
            }
        }while(scope->NextRow());
        Check(contexts==2520&&published==3180&&extras==1290&&extraEntries.size()==103&&pulls==1964&&assists==1168,"Actual AI group/solo,215-extra or native eligible attack coverage changed");
        std::cout<<"PASS "<<checked<<" actual native AI checks;contexts="<<contexts<<" published="<<published<<" extra publications="<<extras<<" extra templates="<<extraEntries.size()<<" pulls="<<pulls<<" assists="<<assists<<";initialization/combat entry/cleanup only,no timed spell rotation or client traversal claim\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif
