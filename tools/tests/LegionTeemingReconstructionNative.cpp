// Actual loaders/admission,earned native kill receipts and Skjal instance ledger.
// World SQL and respawn writes stay in connection-local temporary shadows.
// NPC combat AI is controlled in kill cases;full combat/client traversal is separate.
#define LEGION_TEEMING_CRITERIA_SUPPORT_ONLY
#include "LegionTeemingCriteriaNative.cpp"
#include "CreatureTextMgr.h"
#include "WaypointManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>

struct ReconstructionRow { uint32 Candidate,Route,Entry,Map; uint64 Guid; Position Pose; };

static void ExecuteFile(char const* path)
{
    Require(path,"Explicit SQL fixture missing");std::ifstream stream(path);Require(bool(stream),"SQL fixture unreadable");
    std::string line;
    while(std::getline(stream,line))if(!line.empty()&&line.rfind("--",0)!=0)WorldDatabase.DirectExecute(line.c_str());
}

struct ReconstructionActor:Creature
{
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)),"Controlled reconstruction kill AI unavailable");
        GetInstanceScript()->OnCreatureCreate(this);
    }
    ~ReconstructionActor(){if(GetMap()){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}}
};

static uint64 Counter(Scenario& scenario,uint32 tree)
{
    auto const& map=scenario.GetAchievementMgr().*Access(TeemingProgressTag{});
    auto it=map.find(tree);return it==map.end()?0:it->second.Counter;
}

static void Leaves(CriteriaTree const* tree,uint32 entry,std::vector<uint32>& ids)
{
    if(tree->Criteria&&tree->Criteria->Entry->Type==CRITERIA_TYPE_KILL_CREATURE&&tree->Criteria->Entry->Asset==entry)ids.push_back(tree->ID);
    for(auto child:tree->Children)Leaves(child,entry,ids);
}

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();
        auto models=std::getenv("LEGION_MAW_MODEL_DATA");Require(models,"Explicit model data directory missing");
        for(uint32 display:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(display,std::string(models)+'/'),"Matching Maw model navmesh missing");
        sAchievementMgr->LoadCriteriaList();sCreatureTextMgr->LoadCreatureTexts();sObjectMgr->LoadEquipmentTemplates();sObjectMgr->LoadCreatureTemplateAddons();
        // Real deaths dispatch native proc lookups. Initialise their complete
        // catalogs before exercising Unit::Kill;geometry-only helpers need not.
        sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellPrcoCheck();sSpellMgr->LoadSpellBonusess();
        for(uint32 id:{123978u,191450u})
        {
            bool found=false;
            for(unsigned index=0;index<MAX_SPELL_EFFECTS;++index)
            {
                auto effect=sSpellMgr->GetSpellInfo(id)->GetEffect(static_cast<SpellEffIndex>(index),8);
                if(!effect||(effect->ApplyAuraName!=SPELL_AURA_MOD_SCALE&&effect->ApplyAuraName!=SPELL_AURA_MOD_SCALE_2))continue;
                Require(effect->RealPointsPerLevel==0&&effect->Scaling.Coefficient==0,"Review changed native random-size scaling before geometry bounds");
                std::cout<<"SCALE_BOUND\t"<<id<<'\t'<<index<<'\t'<<effect->BasePoints<<'\t'<<effect->DieSides<<'\n';found=true;
            }
            Require(found,"Native size aura has no reviewed scale effect");
        }
        (sQuestDataStore->*Access(QuestVectorTag{})).resize(1,nullptr);sQuestDataStore->*Access(QuestMaxTag{})=0;
        auto skjalRow=WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=99307 AND map=1492 ORDER BY guid LIMIT 1");
        Require(bool(skjalRow),"Native Skjal pose missing");auto sf=skjalRow->Fetch();Position skjalPose{sf[0].GetFloat(),sf[1].GetFloat(),sf[2].GetFloat(),sf[3].GetFloat()};
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        ExecuteFile(std::getenv("LEGION_TEEMING_SHADOW_SQL"));
        for(auto table:{"creature","creature_addon","waypoint_data","creature_formations"})
        {
            auto proof=WorldDatabase.Query(("SHOW CREATE TABLE "+std::string(table)).c_str());
            Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"World mutation target is not a temporary shadow");
        }
        ExecuteFile(std::getenv("LEGION_TEEMING_MIGRATION_SQL"));ExecuteFile(std::getenv("LEGION_TEEMING_MIGRATION_SQL"));
        auto count=WorldDatabase.Query("SELECT COUNT(*) FROM creature WHERE guid BETWEEN 900902000 AND 900902214 AND spawnMask=256 AND isTeemingSpawn=1");
        Check(count&&count->Fetch()[0].GetUInt32()==215,"Exact SQL migration/idempotent shadow count differs");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sWaypointMgr->Load();sObjectMgr->LoadCreatures();sObjectMgr->LoadCreatureAddons();
        // Retain the actual loader/cache,but isolate explicit publication from
        // incidental neighbouring grids during controlled admission/kill cases.
        for(uint64 guid=900902000;guid<=900902214;++guid){auto data=sObjectMgr->GetCreatureData(guid);Require(data,"Native loader lost reconstructed row");sObjectMgr->RemoveCreatureFromGrid(guid,data);}
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
        for(auto table:{"creature_respawn","gameobject_respawn"})
        {
            std::string seed="CREATE TEMPORARY TABLE reconstruction_"+std::string(table)+" LIKE "+table;
            std::string shadow="CREATE TEMPORARY TABLE "+std::string(table)+" LIKE reconstruction_"+table;
            CharacterDatabase.DirectExecute(seed.c_str());CharacterDatabase.Execute(seed.c_str());CharacterDatabase.DirectExecute(shadow.c_str());CharacterDatabase.Execute(shadow.c_str());
        }
        CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");CharacterDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        std::ifstream input(std::getenv("LEGION_TEEMING_NATIVE_ROWS"));Require(bool(input),"Native reconstruction row input missing");
        std::vector<ReconstructionRow> rows;ReconstructionRow row;float x,y,z,o;
        while(input>>row.Candidate>>row.Route>>row.Guid>>row.Entry>>row.Map>>x>>y>>z>>o){row.Pose={x,y,z,o};rows.push_back(row);}
        Check(rows.size()==215,"Native reconstruction fixture scope differs");
        std::set<uint32> killEntries;for(auto const& r:rows)killEntries.insert(r.Entry);
        for(auto const& spawn:MawSkjalCrew::Spawns)killEntries.insert(spawn.Entry);
        std::string criteriaIds;
        for(auto criterion:sCriteriaStore)if(criterion->Type==CRITERIA_TYPE_KILL_CREATURE&&killEntries.count(criterion->Asset))
        {if(!criteriaIds.empty())criteriaIds+=',';criteriaIds+=std::to_string(criterion->ID);}
        Require(!criteriaIds.empty(),"Native kill criteria scope empty");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
        WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE reconstruction_criteria_seed AS SELECT * FROM achievement_criteria_data WHERE criteria_id IN("+criteriaIds+")").c_str());
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE achievement_criteria_data LIKE reconstruction_criteria_seed");
        WorldDatabase.DirectExecute("INSERT INTO achievement_criteria_data SELECT * FROM reconstruction_criteria_seed");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sAchievementMgr->LoadAchievementCriteriaData();
        std::cout<<std::setprecision(10);
        for(uint32 path=1467062000;path<1467062010;++path)
        {
            auto points=sWaypointMgr->GetPath(path);Require(points,"Native reconstructed waypoint path missing");
            for(auto const& point:*points)std::cout<<"WIRE_WAYPOINT\t"<<path<<'\t'<<point.id<<'\t'<<point.x<<'\t'<<point.y<<'\t'<<point.z<<'\n';
        }
        for(auto& r:rows)
        {
            auto data=sObjectMgr->GetCreatureData(r.Guid);Require(data,"Native row cache missing");
            std::cout<<"WIRE_POSE\t"<<r.Candidate<<'\t'<<data->posX<<'\t'<<data->posY<<'\t'<<data->posZ<<'\t'<<data->orientation<<'\n';
            r.Pose={data->posX,data->posY,data->posZ,data->orientation};
        }
        unsigned instance=9960000;
        for(Difficulty difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})
        for(unsigned profile=0;profile<3;++profile)
        for(uint32 routeId:{197u,198u,199u,200u,206u,207u,208u,209u,210u,227u,233u,234u,239u})
        {
            auto route=sMapChallengeModeStore.LookupEntry(routeId);Require(route,"Native reconstruction route absent");
            auto first=std::find_if(rows.begin(),rows.end(),[&](ReconstructionRow const& r){return r.Route==routeId;});Require(first!=rows.end(),"Route has no reconstructed DB row");
            MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,instance++,difficulty,&parent);map.CreateInstanceData(nullptr);
            LifecyclePlayer owner(map,901,first->Pose);auto key=owner.Key(*route);owner.m_challengeKeyInfo.Affix=profile==2?Affixes::Teeming:0;
            Scenario* scenario=nullptr;
            if(profile){scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Registered scenario factory failed");scenario->CreateChallenge(&owner);}
            for(auto const& r:rows)if(r.Route==routeId)
            {
                ReconstructionActor actor;bool accepted=actor.LoadCreatureFromDB(r.Guid,&map,false);
                Check(accepted==(difficulty==DIFFICULTY_MYTHIC_KEYSTONE&&profile==2),"Reconstructed DB row escaped exact native Teeming admission");
                if(accepted){Check(actor.GetEntry()==r.Entry&&actor.GetDBTableGUIDLow()==r.Guid,"Native reconstructed identity changed");
                    Check(actor.IsAlive()&&actor.GetHealth()==actor.GetMaxHealth(),"Reconstructed native spawn is dead or starts partly damaged");
                    std::cout<<"NATIVE_BODY\t"<<r.Candidate<<'\t'<<actor.GetObjectScale()<<'\t'<<(actor.HasUnitMovementFlag(MOVEMENTFLAG_HOVER)?actor.GetFloatValue(UNIT_FIELD_HOVER_HEIGHT):0.f)<<'\t'<<actor.GetNativeDisplayId()<<'\t'<<actor.GetDisplayId()<<'\n';
                    auto expected=r.Pose;if(actor.HasUnitMovementFlag(MOVEMENTFLAG_HOVER))expected.m_positionZ+=actor.GetFloatValue(UNIT_FIELD_HOVER_HEIGHT);
                    if(actor.GetExactDist(expected)>=0.01f)std::cout<<"POSE_MISMATCH candidate="<<r.Candidate<<" entry="<<r.Entry<<" requested="<<r.Pose.GetPositionX()<<','<<r.Pose.GetPositionY()<<','<<r.Pose.GetPositionZ()<<" expected="<<expected.GetPositionZ()<<" actual="<<actor.GetPositionX()<<','<<actor.GetPositionY()<<','<<actor.GetPositionZ()<<" cache="<<actor.GetCreatureData()->posX<<','<<actor.GetCreatureData()->posY<<','<<actor.GetCreatureData()->posZ<<'\n';
                    Check(actor.GetExactDist(expected)<0.01f,"Native reconstructed pose/metadata hover changed");}
            }
            if(scenario){map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());}
        }
        std::cout<<"PASS native SQL shadow and215 DB row admission across13 routes/4 modes/3 scenario-affix profiles\n";
        unsigned earnedKills=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
        for(bool grouped:{false,true})
        for(unsigned killerKind=0;killerKind<3;++killerKind)
        for(uint32 routeId:{197u,198u,199u,200u,206u,207u,208u,209u,210u,227u,233u,234u,239u})
        {
            auto route=sMapChallengeModeStore.LookupEntry(routeId);auto first=std::find_if(rows.begin(),rows.end(),[&](ReconstructionRow const& r){return r.Route==routeId;});
            MapInstanced parent(route->MapID,300);InstanceMap map(route->MapID,300,instance++,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
            std::unique_ptr<DungeonShip> ship;if(route->MapID==1492)ship=std::make_unique<DungeonShip>(map);
            LifecyclePlayer owner(map,911,first->Pose);LifecyclePlayer member(map,912,first->Pose);
            for(auto player:{&owner,&member}){player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->setFactionForRace(race);}
            auto key=owner.Key(*route);owner.m_challengeKeyInfo.Affix=Affixes::Teeming;
            LifecycleGroup group(owner,grouped?&member:nullptr,*route,*key);if(grouped)group.group.m_affixes={Affixes::Teeming,0,0};
            auto scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Earned-kill scenario missing");scenario->CreateChallenge(&owner);
            auto steps=sScenarioMgr->GetScenarioSteps(scenario->GetScenarioId(),true);Require(steps&&!steps->empty(),"Native Teeming steps missing");
            auto tree=sAchievementMgr->GetCriteriaTree(steps->front()->Criteriatreeid);Require(tree,"Native kill criterion graph missing");
            DungeonPet pet(owner,map);DungeonGuardian guardian(owner,map);
            // SetOwnerGUID alone is insufficient for real damage admission.
            // Register through native SetMinion,which marks player control.
            for(Minion* minion:{static_cast<Minion*>(&pet),static_cast<Minion*>(&guardian)})
            {minion->SetOwnerGUID(ObjectGuid::Empty);owner.SetMinion(minion,true);minion->SetOwnerGUID(owner.GetGUID());Check(minion->IsControlledByPlayer(),"Native minion registration lost player damage ownership");}
            Unit* killer=killerKind==0?static_cast<Unit*>(&owner):killerKind==1?static_cast<Unit*>(&pet):static_cast<Unit*>(&guardian);
            for(auto const& r:rows)if(r.Route==routeId)
            {
                owner.Relocate(r.Pose);member.Relocate(r.Pose);pet.Relocate(r.Pose);guardian.Relocate(r.Pose);
                ReconstructionActor actor;Require(actor.LoadCreatureFromDB(r.Guid,&map,true),"Actual DB actor publication failed in earned-kill case");
                std::vector<uint32> leaves;Leaves(tree,r.Entry,leaves);Require(!leaves.empty(),"Reconstructed NPC has no matching native kill leaf");
                std::vector<uint64> before;for(auto leaf:leaves)before.push_back(Counter(*scenario,leaf));
                killer->DealDamage(&actor,actor.GetHealth(),nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
                Check(actor.isDead(),"One-hit native player/pet/guardian damage did not kill actual DB actor");
                for(size_t i=0;i<leaves.size();++i){if(Counter(*scenario,leaves[i])!=before[i]+1)std::cout<<"KILL_MISMATCH route="<<routeId<<" candidate="<<r.Candidate<<" entry="<<r.Entry<<" killer="<<killerKind<<" grouped="<<grouped<<" race="<<uint32(race)<<" leaf="<<leaves[i]<<" before="<<before[i]<<" after="<<Counter(*scenario,leaves[i])<<" damage="<<actor.m_PlayerDamageReq<<" canUpdate="<<scenario->CanUpdateCriteria(leaves[i])<<'\n';Check(Counter(*scenario,leaves[i])==before[i]+1,"Earned native kill receipt was lost or duplicated by killer/group resolution");}
                killer->Kill(&actor);
                for(size_t i=0;i<leaves.size();++i)Check(Counter(*scenario,leaves[i])==before[i]+1,"Repeated real death dispatch duplicated native criterion receipt");
                ++earnedKills;
            }
            map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());
        }
        std::cout<<"PASS earned native kills="<<earnedKills<<";215 DB actors/player+pet+guardian/solo+eligible group/both factions/repeated-death guards\n";
        // Actual Skjal AI action,instance-owned publication/reset/re-entry and
        // real native death dispatch. No progression is awarded by recovery.
        unsigned scriptKills=0;
        for(uint8 race:{uint8(RACE_HUMAN),uint8(RACE_ORC)})
        for(bool grouped:{false,true})
        for(unsigned killerKind=0;killerKind<3;++killerKind)
        for(Difficulty difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})
        for(unsigned profile=0;profile<3;++profile)
        {
            MapInstanced parent(1492,300);InstanceMap map(1492,300,instance++,difficulty,&parent);map.CreateInstanceData(nullptr);DungeonShip ship(map);
            LifecyclePlayer owner(map,902,skjalPose);owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.setFactionForRace(race);
            LifecyclePlayer member(map,903,skjalPose);member.SetByteValue(UNIT_FIELD_BYTES_0,0,race);member.setFactionForRace(race);
            auto key=owner.Key(*sMapChallengeModeStore.LookupEntry(208));owner.m_challengeKeyInfo.Affix=profile==2?Affixes::Teeming:0;
            LifecycleGroup group(owner,grouped?&member:nullptr,*sMapChallengeModeStore.LookupEntry(208),*key);if(grouped)group.group.m_affixes={profile==2?Affixes::Teeming:0u,0,0};
            Scenario* scenario=nullptr;if(profile){scenario=sScenarioMgr->AddScenario(&map,1175);scenario->CreateChallenge(&owner);}
            auto script=dynamic_cast<instance_maw_of_souls::instance_maw_of_souls_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Native Maw instance type missing");
            auto skjal=map.SummonCreature(NPC_SKJAL,skjalPose);Require(skjal&&skjal->AI(),"Actual registered Skjal AI missing");
            skjal->AI()->DoAction(ACTION_1);Check(!script->GetData(MawSkjalCrew::Request),"Skjal action bypassed real Harbaron prerequisite");
            script->SetBossState(DATA_HARBARON,DONE);skjal->AI()->DoAction(ACTION_1);
            size_t expected=difficulty==DIFFICULTY_MYTHIC_KEYSTONE&&profile==2?23:16;
            std::array<ObjectGuid,23> ids;
            for(size_t i=0;i<ids.size();++i){ids[i]=script->GetGuidData(MawSkjalCrew::GuidFirst+uint32(i));Check(bool(map.GetCreature(ids[i]))==(i<expected),"Skjal native normal/Teeming crew count differs");}
            skjal->AI()->DoAction(ACTION_1);script->Update(1000);
            for(size_t i=0;i<expected;++i)Check(script->GetGuidData(MawSkjalCrew::GuidFirst+uint32(i))==ids[i],"Repeated Skjal action duplicated actual crew");
            if(expected==23)
            {
                std::array<bool,8> visited{};
                for(unsigned elapsed=0;elapsed<180000;elapsed+=100)
                {
                    for(size_t i=0;i<expected;++i)if(auto actor=map.GetCreature(ids[i]))actor->Unit::Update(100);
                    map.MoveAllCreaturesInMoveList();script->Update(100);
                    if(auto guard=map.GetCreature(ids[22]))for(size_t i=0;i<visited.size();++i)if(guard->GetExactDist(MawSkjalCrew::GuardPatrol[i])<1.5f)visited[i]=true;
                }
                for(size_t i=0;i<22;++i){auto actor=map.GetCreature(ids[i]);Check(actor&&actor->GetExactDist(MawSkjalCrew::Spawns[i].Home)<1.5f,"Native Skjal crew movement did not reach actual home");}
                for(bool value:visited)Check(value,"Native Skjal guard did not traverse every reconstructed patrol waypoint");
                Check(!script->skjalCrew[22].PatrolPending,"Guard patrol bypassed or lost actual home-arrival receipt");
                DungeonPet pet(owner,map);DungeonGuardian guardian(owner,map);
                for(Minion* minion:{static_cast<Minion*>(&pet),static_cast<Minion*>(&guardian)}){minion->SetOwnerGUID(ObjectGuid::Empty);owner.SetMinion(minion,true);minion->SetOwnerGUID(owner.GetGUID());}
                Unit* killer=killerKind==0?static_cast<Unit*>(&owner):killerKind==1?static_cast<Unit*>(&pet):static_cast<Unit*>(&guardian);
                auto steps=sScenarioMgr->GetScenarioSteps(scenario->GetScenarioId(),true);auto tree=sAchievementMgr->GetCriteriaTree(steps->front()->Criteriatreeid);
                for(size_t i=16;i<23;++i)
                {
                    auto actor=map.GetCreature(ids[i]);Require(actor&&actor->IsAlive(),"Actual Skjal extra missing before earned kill");
                    owner.Relocate(actor->GetPosition());member.Relocate(actor->GetPosition());pet.Relocate(actor->GetPosition());guardian.Relocate(actor->GetPosition());
                    std::vector<uint32> leaves;Leaves(tree,actor->GetEntry(),leaves);Require(!leaves.empty(),"Skjal extra has no native force receipt");
                    std::vector<uint64> before;for(auto leaf:leaves)before.push_back(Counter(*scenario,leaf));
                    killer->DealDamage(actor,actor->GetHealth(),nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Check(actor->isDead(),"Actual Skjal extra survived native one-hit damage");
                    for(size_t n=0;n<leaves.size();++n)Check(Counter(*scenario,leaves[n])==before[n]+1,"Actual Skjal extra lost or duplicated native player/pet/guardian/group forces");
                    killer->Kill(actor);script->Update(1000);Check(script->GetData(MawSkjalCrew::DeadFirst+uint32(i))==1,"Actual extra death lost instance receipt");
                    for(size_t n=0;n<leaves.size();++n)Check(Counter(*scenario,leaves[n])==before[n]+1,"Repeated Skjal extra death duplicated forces");++scriptKills;
                }
            }
            auto victim=map.GetCreature(ids[0]);Require(victim,"Normal crew victim missing");owner.Kill(victim);script->Update(1000);
            Check(script->GetData(MawSkjalCrew::DeadFirst)&&script->GetGuidData(MawSkjalCrew::GuidFirst)==ids[0],"Native crew death lost exactly-once receipt");
            auto missing=map.GetCreature(ids[1]);missing->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();script->Update(1000);
            Check(script->GetGuidData(MawSkjalCrew::GuidFirst+1)!=ids[1]&&map.GetCreature(script->GetGuidData(MawSkjalCrew::GuidFirst+1)),"Missing alive crew did not recover through instance ledger");
            script->SetBossState(DATA_SKJAL,DONE);skjal->AI()->Reset();Check(script->GetBossState(DATA_SKJAL)==DONE,"Skjal reset erased real completed encounter");
            script->SetBossState(DATA_HARBARON,NOT_STARTED);map.RemoveAllObjectsInRemoveList();Check(!script->GetData(MawSkjalCrew::Request),"New attempt retained stale Skjal crew request");
            for(size_t i=0;i<expected;++i)Check(script->GetGuidData(MawSkjalCrew::GuidFirst+uint32(i)).IsEmpty(),"Reset retained stale crew GUID");
            skjal->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
            if(scenario){map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());}
        }
        std::cout<<"PASS Skjal earned kills="<<scriptKills<<";seven actual instance-owned additions/all3 killers/solo+eligible group/both factions\n";
        std::cout<<"PASS "<<checked<<" reconstruction checks;controlled native server checks,not client traversal\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
