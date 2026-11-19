// Native formation publication,waypoint dispatch and follower splines.
// Combat AI is deliberately unpumped;this is not a client/combat-spell test.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
#include "CreatureGroups.h"
#include "WaypointManager.h"
#include "MoveSpline.h"
#include <fstream>
#include <sstream>
#include <set>

struct FormationActor;
struct FormationObserver:NullCreatureAI
{
    explicit FormationObserver(FormationActor* actor);
    void MovementInform(uint32 type,uint32 id) override;
    FormationActor* Actor;
};

struct FormationActor:Creature
{
    std::set<uint32> Arrivals;
    unsigned PointArrivals=0;
    bool WrongWaypointArrival=false;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();
        SearchFormation();
        Require(AIM_Initialize(new FormationObserver(this)),"Native formation observer setup failed");
        if(auto script=GetInstanceScript())script->OnCreatureCreate(this);
    }
    ~FormationActor()
    {
        if(!GetMap())return;
        CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);
        if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);
        ResetMap();
    }
};
FormationObserver::FormationObserver(FormationActor* actor):NullCreatureAI(actor),Actor(actor){}
void FormationObserver::MovementInform(uint32 type,uint32 id)
{
    if(type==POINT_MOTION_TYPE){++Actor->PointArrivals;return;}
    if(type!=WAYPOINT_MOTION_TYPE)return;
    auto path=sWaypointMgr->GetPath(Actor->GetWaypointPath());
    if(!path||id>=path->size()){Actor->WrongWaypointArrival=true;return;}
    auto const& point=path->at(id);
    if(std::hypot(Actor->GetPositionX()-point.x,Actor->GetPositionY()-point.y)>1.5f||std::fabs(Actor->GetPositionZ()-point.z)>2.f)Actor->WrongWaypointArrival=true;
    Actor->Arrivals.insert(id);
}

static void ExecuteFixture()
{
    auto file=std::getenv("LEGION_TEEMING_FORMATION_SQL");Require(file,"Explicit formation shadow input missing");
    std::ifstream stream(file);Require(bool(stream),"Formation shadow input unreadable");std::string sql;
    while(std::getline(stream,sql))if(!sql.empty()&&sql.rfind("--",0)!=0)WorldDatabase.DirectExecute(sql.c_str());
    for(auto table:{"creature","creature_addon","waypoint_data","creature_formations"})
    {
        auto proof=WorldDatabase.Query(("SHOW CREATE TABLE "+std::string(table)).c_str());
        Require(proof&&proof->Fetch()[1].GetString().find("CREATE TEMPORARY TABLE")!=std::string::npos,"Formation loader target is not a temporary shadow");
    }
}

#ifndef LEGION_TEEMING_FORMATION_SUPPORT_ONLY
int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadEquipmentTemplates();sObjectMgr->LoadCreatureTemplateAddons();sObjectMgr->LoadScenarioData();
        sAchievementMgr->LoadCriteriaList();sAchievementMgr->LoadAchievementCriteriaData();
        sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellPrcoCheck();sSpellMgr->LoadSpellBonusess();
        auto modelData=std::getenv("LEGION_MAW_MODEL_DATA");Require(modelData,"Explicit model folder missing");
        for(uint32 display:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(display,std::string(modelData)+'/'),"Matching ship navigation missing");
        WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");ExecuteFixture();WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        sWaypointMgr->Load();sObjectMgr->LoadCreatures();sObjectMgr->LoadCreatureAddons();sFormationMgr->LoadCreatureFormations();
        Check(sFormationMgr->CreatureGroupMap.size()==284,"Reviewed266 formation rows plus18 retained guards changed");
        auto rows=WorldDatabase.Query("SELECT guid FROM creature ORDER BY guid");Require(bool(rows),"Native formation creature closure empty");
        do{uint64 guid=rows->Fetch()[0].GetUInt64();auto data=sObjectMgr->GetCreatureData(guid);Require(data,"Formation native cache closure incomplete");sObjectMgr->RemoveCreatureFromGrid(guid,data);}while(rows->NextRow());
        auto groups=WorldDatabase.Query("SELECT DISTINCT f.leaderGUID,c.map FROM creature_formations f JOIN creature c ON c.guid=f.leaderGUID ORDER BY f.leaderGUID");Require(bool(groups),"Native formation group scope empty");
        unsigned contexts=0,moving=0,followers=0,extraFollowers=0,ordinary=0;
        do
        {
            auto fields=groups->Fetch();uint64 leaderGuid=fields[0].GetUInt64();uint32 mapId=fields[1].GetUInt32();
            std::vector<uint64> members;for(auto const& pair:sFormationMgr->CreatureGroupMap)if(pair.second->leaderGUID==leaderGuid)members.push_back(pair.first);
            std::sort(members.begin(),members.end());Require(!members.empty(),"Native formation member scope missing");
            auto leaderData=sObjectMgr->GetCreatureData(leaderGuid);Require(leaderData,"Actual native leader data missing");
            for(unsigned profile=0;profile<3;++profile)
            {
                Difficulty difficulty=profile?DIFFICULTY_MYTHIC_KEYSTONE:DIFFICULTY_MYTHIC_DUNGEON;
                MapInstanced parent(mapId,300);InstanceMap map(mapId,300,9986000+contexts,difficulty,&parent);map.CreateInstanceData(nullptr);
                // The controlled owner is stationary; load the real movement grids
                // that normal nearby player traversal would keep available.
                auto loadMovementGrids=[&](float x,float y){for(float dx:{-32.f,0.f,32.f})for(float dy:{-32.f,0.f,32.f})map.LoadGrid(x+dx,y+dy);};
                for(uint64 guid:members)
                {
                    auto data=sObjectMgr->GetCreatureData(guid);Require(data,"Movement grid creature missing");loadMovementGrids(data->posX,data->posY);
                    auto addon=sObjectMgr->GetCreatureAddon(guid);auto templateAddon=sObjectMgr->GetCreatureTemplateAddon(data->id);
                    uint32 pathId=addon?addon->path_id:templateAddon?templateAddon->path_id:0;
                    if(auto path=sWaypointMgr->GetPath(pathId))for(auto const& point:*path)loadMovementGrids(point.x,point.y);
                }
                std::unique_ptr<DungeonShip> ship;if(mapId==1492)ship=std::make_unique<DungeonShip>(map);
                LifecyclePlayer owner(map,931,Position{leaderData->posX,leaderData->posY,leaderData->posZ,leaderData->orientation});
                Scenario* scenario=nullptr;
                if(profile)
                {
                    MapChallengeModeEntry const* route=nullptr;for(auto value:sMapChallengeModeStore)if(value->MapID==mapId){route=value;break;}
                    Require(route,"Native formation map has no challenge route");auto key=owner.Key(*route);owner.m_challengeKeyInfo.Affix=profile==2?Affixes::Teeming:0;
                    scenario=sScenarioMgr->AddScenario(&map,1175);Require(scenario,"Native formation scenario registration failed");scenario->CreateChallenge(&owner);
                }
                {
                    std::vector<std::unique_ptr<FormationActor>> actors;FormationActor* leader=nullptr;size_t expected=0;
                    for(uint64 guid:members)
                    {
                        auto data=sObjectMgr->GetCreatureData(guid);Require(data&&data->mapid==mapId,"Native formation crosses map boundaries");
                        if(!(data->spawnMask&(uint64(1)<<difficulty)))continue;
                        bool allowed=!data->isTeemingSpawn||profile==2;
                        auto actor=std::make_unique<FormationActor>();bool loaded=actor->LoadCreatureFromDB(guid,&map,true);
                        Check(loaded==allowed,"Native formation publication escaped actual mask/Teeming rules");
                        if(!loaded)continue;
                        ++expected;Check(actor->GetFormation()&&actor->GetFormation()->GetId()==leaderGuid,"Native SearchFormation lost membership");
                        if(guid==leaderGuid)leader=actor.get();actors.push_back(std::move(actor));
                    }
                    auto found=map.CreatureGroupHolder.find(leaderGuid);
                    Check((found!=map.CreatureGroupHolder.end())==bool(expected),"Actual native map group publication count differs");
                    if(expected)Check(found->second->GetMembers().size()==expected&&found->second->getLeader()==leader,"Native live formation leader/member identity differs");
                    if(leader&&leaderData->movementType==WAYPOINT_MOTION_TYPE)
                    {
                        auto path=sWaypointMgr->GetPath(leader->GetWaypointPath());Require(path&&!path->empty(),"Actual moving formation leader path absent");
                        for(unsigned elapsed=0;elapsed<600000&&leader->Arrivals.size()<path->size();elapsed+=100)
                        {
                            for(auto const& actor:actors)actor->Unit::Update(100);
                            map.MoveAllCreaturesInMoveList();
                        }
                        std::cout<<"FORMATION leader="<<leaderGuid<<" profile="<<profile<<" members="<<actors.size()<<" nodes="<<path->size()<<" arrived="<<leader->Arrivals.size()<<" bad="<<leader->WrongWaypointArrival<<" motion="<<unsigned(leader->GetMotionMaster()->GetCurrentMovementGeneratorType())<<" state="<<leader->GetUnitState()<<" alive="<<leader->IsAlive()<<" pose="<<leader->GetPositionX()<<','<<leader->GetPositionY()<<','<<leader->GetPositionZ()<<" spline="<<leader->movespline->Finalized()<<'\n';
                        Check(!leader->WrongWaypointArrival&&leader->Arrivals.size()==path->size(),"Actual formation leader did not arrive at every native waypoint");
                        leader->GetMotionMaster()->MoveIdle();
                        for(unsigned elapsed=0;elapsed<60000;elapsed+=100){for(auto const& actor:actors)actor->Unit::Update(100);map.MoveAllCreaturesInMoveList();}
                        for(auto const& actor:actors)if(actor.get()!=leader&&sFormationMgr->CreatureGroupMap.at(actor->GetDBTableGUIDLow())->groupAI&&sFormationMgr->CreatureGroupMap.at(actor->GetDBTableGUIDLow())->groupAI!=3)
                        {
                            auto home=actor->GetHomePosition();std::cout<<"FOLLOWER guid="<<actor->GetDBTableGUIDLow()<<" arrivals="<<actor->PointArrivals<<" homeDistance="<<actor->GetExactDist(home)<<'\n';
                            Check(actor->PointArrivals&&actor->GetExactDist(home)<1.5f,"Native formation follower did not settle at the actual issued target");++followers;
                            if(actor->GetCreatureData()->isTeemingSpawn)++extraFollowers;
                        }
                        ++moving;
                    }
                    else ++ordinary;
                }
                Check(map.CreatureGroupHolder.empty(),"Native formation cleanup retained deleted actor pointers");
                if(scenario){map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());}
                ++contexts;
            }
        }while(groups->NextRow());
        Check(contexts==312&&moving==25&&followers==51&&extraFollowers==18&&ordinary==287,"Reviewed native formation coverage changed");
        std::cout<<"PASS "<<checked<<" native formation checks;contexts="<<contexts<<" moving="<<moving<<" follower arrivals="<<followers<<" Teeming followers="<<extraFollowers<<" static/absent="<<ordinary<<";no full combat AI or client traversal claim\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif
