#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#include "PathGenerator.h"
#include "MawShipGroundPath.h"
#include "MoveSpline.h"
#include <thread>
#ifdef MAW_SHIP_PATH_BASELINE
#include "PathGeneratorActual.inc"
#endif

static void ClearActor(Creature& actor)
{
    actor.CombatStop(true);actor.CastStop();actor.RemoveAllAuras();actor.m_Events.KillAllEvents(true);
    if(actor.IsInWorld())actor.GetMap()->RemoveFromMap(static_cast<Creature*>(&actor),false);actor.ResetMap();
}
struct NavigationActor:Creature
{
    // Publish through native Map/Unit lookup and collision. AI combat events
    // are independent of this movement regression and remain unpumped.
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Navigation actor AI setup failed");}
    ~NavigationActor(){if(GetMap())ClearActor(*this);}
};

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv);auto data=std::getenv("LEGION_MAW_MODEL_DATA");Require(data,"Explicit staged model folder required");
        auto manager=MMAP::MMapFactory::createOrGetMMapManager();
        for(uint32 id:{31852u,31853u})Check(manager->loadGameObject(id,std::string(data)+'/'),"Matching ship model mesh failed to load");
        auto query=manager->GetModelNavMeshQuery(31852);Check(query&&query==manager->GetModelNavMeshQuery(31852),"Model query missing or not retained per thread");
        dtNavMeshQuery const* other=nullptr;
        std::thread worker([&]{other=manager->GetModelNavMeshQuery(31852);Check(other&&other!=query&&other->getAttachedNavMesh()==query->getAttachedNavMesh(),"Model query shared mutable state across threads");});worker.join();
        for(Difficulty difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON,DIFFICULTY_MYTHIC_KEYSTONE})
        {
        MapInstanced parent(1492,300);InstanceMap map(1492,300,9948200+uint32(difficulty),difficulty,&parent);map.CreateInstanceData(nullptr);
        std::cout<<"MODE\t"<<uint32(difficulty)<<'\n';
        {
            DungeonShip ship(map);
            struct Sample{uint32 entry;G3D::Vector3 start,end;};
            std::array<Sample,3> cases={{{97365,{2906.87f,831.485f,515.16f},{2907.11f,826.473f,515.16f}},
                {97182,{2951.01f,783.438f,515.276f},{2947.95f,758.559f,515.243f}},
                {96754,{2934.49f,715.757f,554.286f},{2938.49f,715.757f,554.286f}}}};
            uint32 serial=0;
            for(uint32 display:{31852u,31853u})
            {
            ship.SetDisplayId(display);ship.EnableCollision(true);
            std::cout<<"MODEL_STATE\t"<<display<<'\n';
            for(auto const& sample:cases)
            {
                NavigationActor actor;Check(actor.Create(189000+serial,&map,1,sample.entry,0,ALLIANCE,sample.start.x,sample.start.y,sample.start.z,0),"Native walking actor create failed");
                map.LoadGrid(sample.start.x,sample.start.y);Check(map.AddToMap(static_cast<Creature*>(&actor)),"Native walking actor publication failed");
                auto local=ship.m_model->WorldToModel(sample.start);Check((ship.m_model->ModelToWorld(local)-sample.start).length()<0.001f,"Actual ship transform roundtrip changed pose");
                PathGenerator path(&actor);bool ok=path.CalculatePath(sample.end.x,sample.end.y,sample.end.z,false);
                std::cout<<"PATH\t"<<serial<<'\t'<<ok<<'\t'<<uint32(path.GetPathType())<<'\t'<<path.GetPath().size()<<'\n';
#ifdef MAW_SHIP_PATH_BASELINE
                Check(path.GetPathType()!=PATHFIND_NORMAL,"Original global mesh unexpectedly provides a complete native ship path");
#else
                Check(ok&&path.GetPathType()==PATHFIND_NORMAL&&path.GetPath().size()>=2,"Model-local native ship path incomplete");
                Check((path.GetPath().back()-sample.end).length()<0.6f,"Ship path did not reach actual destination");
                for(size_t i=1;i<path.GetPath().size();++i)Check((path.GetPath()[i]-path.GetPath()[i-1]).length()>=0.1f,"Model path contains vertices rejected by native spline admission");
                PathGenerator repeated(&actor);Check(repeated.CalculatePath(sample.end.x,sample.end.y,sample.end.z,false)&&repeated.GetPathType()==PATHFIND_NORMAL,"Repeated model path changed outcome");
                PathGenerator raycast(&actor);raycast.SetUseRaycast(true);raycast.CalculatePath(sample.end.x,sample.end.y,sample.end.z,false);Check(raycast.GetPathType()!=PATHFIND_NORMAL,"Native raycast query was replaced by a routed model path");
                actor.AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);PathGenerator ignored(&actor);Check(ignored.CalculatePath(sample.end.x,sample.end.y,sample.end.z,false)&&(ignored.GetPathType()&PATHFIND_NOT_USING_PATH),"Explicit ignore-pathfinding behavior changed");actor.ClearUnitState(UNIT_STATE_IGNORE_PATHFINDING);
                Movement::PointsArray points;Check(Movement::BuildMawShipGroundPath(&actor,sample.start,sample.end,1,points)==Movement::MawShipPathResult::NoPath&&points.empty(),"Native point bound bypassed");
                for(auto flag:{MOVEMENTFLAG_FLYING,MOVEMENTFLAG_DISABLE_GRAVITY,MOVEMENTFLAG_FALLING,MOVEMENTFLAG_SWIMMING,MOVEMENTFLAG_HOVER})
                {actor.AddUnitMovementFlag(flag);Check(Movement::BuildMawShipGroundPath(&actor,sample.start,sample.end,74,points)==Movement::MawShipPathResult::NotApplicable,"Airborne/swimming/hover movement was replaced by a ground route");actor.RemoveUnitMovementFlag(flag);}
                auto unsupported=sample.end+G3D::Vector3(0,0,8.f);Check(Movement::BuildMawShipGroundPath(&actor,sample.start,unsupported,74,points)==Movement::MawShipPathResult::NotApplicable,"Airborne endpoint treated as model-supported ground");
                ship.EnableCollision(false);Check(Movement::BuildMawShipGroundPath(&actor,sample.start,sample.end,74,points)==Movement::MawShipPathResult::NotApplicable,"Disabled ship collision still admitted model path");ship.EnableCollision(true);
                actor.GetMotionMaster()->MovePoint(201,sample.end.x,sample.end.y,sample.end.z,true);
                for(unsigned elapsed=0;elapsed<15000;elapsed+=10){actor.Unit::Update(10);map.MoveAllCreaturesInMoveList();}
                std::cout<<"MOVE\t"<<serial<<'\t'<<actor.GetPositionX()<<'\t'<<actor.GetPositionY()<<'\t'<<actor.GetPositionZ()<<'\t'<<actor.GetSpeed(MOVE_RUN)<<'\t'<<actor.HasUnitState(UNIT_STATE_NOT_MOVE)<<'\t'<<actor.movespline->Finalized()<<'\n';
                Check(actor.GetExactDist(sample.end.x,sample.end.y,sample.end.z)<0.6f,"Native NPC point movement did not arrive through the generated path");
#endif
                ClearActor(actor);++serial;
            }
#ifndef MAW_SHIP_PATH_BASELINE
            {
                DungeonPlayer owner(map,RACE_HUMAN);owner.Relocate(2906.87f,831.485f,515.16f,0);
                DungeonPet pet(owner,map);DungeonGuardian guardian(owner,map);
                for(Unit* companion:{static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian)})
                {companion->Relocate(owner.GetPosition());PathGenerator path(companion);Check(path.CalculatePath(2907.11f,826.473f,515.16f,false)&&path.GetPathType()==PATHFIND_NORMAL,"Native pet/guardian hold movement failed");
                 companion->GetMotionMaster()->MovePoint(202,2907.11f,826.473f,515.16f,true);for(unsigned elapsed=0;elapsed<5000;elapsed+=10){companion->Unit::Update(10);map.MoveAllCreaturesInMoveList();}
                 Check(companion->GetExactDist(2907.11f,826.473f,515.16f)<0.6f,"Native pet/guardian spline did not reach the hold destination");}
                Movement::PointsArray points;Check(Movement::BuildMawShipGroundPath(&owner,{owner.GetPositionX(),owner.GetPositionY(),owner.GetPositionZ()},{2907.11f,826.473f,515.16f},74,points)==Movement::MawShipPathResult::NotApplicable,"NPC ship path changed Player movement");
            }
#endif
            }
        }
        }
        std::cout<<"PASS "<<checked<<" actual native ship navigation checks;no combat credit or client traversal claimed\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
}
