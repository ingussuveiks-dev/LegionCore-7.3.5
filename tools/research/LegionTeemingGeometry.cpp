// Read-only physical survey of real DB anchors and reconstructed addon points.
// Uses native Map terrain/WMO/navmesh queries. Candidate NPCs stay unpublished;
// path probes publish isolated NullAI actors and the real shared ship collision.
#define CHALLENGE_DEATH_SUPPORT_ONLY
#include "../tests/LegionChallengeDeathAffixesNative.cpp"
#include "MMapManager.h"
#include "DetourNavMeshQuery.h"
#include "PathGenerator.h"
#include <fstream>
#include <sstream>

struct PhysicalSupportShip : GameObject
{
    explicit PhysicalSupportShip(Map& map)
    {
        auto row=WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask FROM gameobject WHERE map=1492 AND id=246919 AND (spawnMask&256)<>0 ORDER BY guid LIMIT 1");
        Require(bool(row),"Actual Maw ship DB placement absent");auto f=row->Fetch();
        Position pose{f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat(),f[4].GetFloat()};
        Require(Create(f[0].GetUInt64(),246919,&map,f[9].GetUInt32(),pose,G3D::Quat(f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat(),f[8].GetFloat()),255,GO_STATE_READY),"Actual ship model create failed");
        map.LoadGrid(pose.GetPositionX(),pose.GetPositionY());Require(map.AddToMap(static_cast<GameObject*>(this)),"Actual ship model publication failed");
        Require(m_model&&m_model->isCollisionEnabled(),"Actual ship collision unavailable");
    }
    ~PhysicalSupportShip(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
struct SurveyActor:Creature
{
    void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Path survey actor AI setup failed");}
    ~SurveyActor(){if(GetMap()){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}}
};

static void Surface(Map& map,char kind,uint64 id,uint32 entry,float x,float y,float z,Creature* actor,PhysicalSupportShip const* ship)
{
    uint32 flags=0;int32 adt=0,root=0,group=0;
    bool area=map.GetAreaInfo(x,y,z+0.5f,flags,adt,root,group);
    auto query=MMAP::MMapFactory::createOrGetMMapManager()->GetNavMeshQuery(map.GetId(),map.GetInstanceId());
    dtQueryFilter filter;filter.setIncludeFlags(0xffff);filter.setExcludeFlags(0);
    float point[3]={y,z,x},extent[3]={3.f,5.f,3.f},nearest[3]={};dtPolyRef poly=0;
    bool nav=query&&dtStatusSucceed(query->findNearestPoly(point,extent,&filter,&poly,nearest))&&poly;
    unsigned short polyFlags=0;
    if(nav)MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(map.GetId())->getPolyFlags(poly,&polyFlags);
    // Combat reach is an attack distance, not the native model footprint.
    float height=actor?actor->GetCollisionHeight():0.f,radius=actor?actor->GetBoundingRadius():0.f;
    bool body=true;
    bool center=true,foot=true;float footDelta=0.f;
    if(actor)
    {
        center=map.isInLineOfSight(x,y,z+0.15f,x,y,z+height,actor->GetPhases(),VMAP::ModelIgnoreFlags::Nothing,nullptr,actor);
        for(auto offset:std::array<std::pair<float,float>,5>{{{0.f,0.f},{radius,0.f},{-radius,0.f},{0.f,radius},{0.f,-radius}}})
        {
            body&=map.isInLineOfSight(x+offset.first,y+offset.second,z+0.15f,x+offset.first,y+offset.second,z+height,actor->GetPhases(),VMAP::ModelIgnoreFlags::Nothing,nullptr,actor);
            float local=map.GetHeight(actor->GetPhases(),x+offset.first,y+offset.second,z,true,50.f,nullptr,actor);
            footDelta=std::max(footDelta,std::fabs(local-z));float start=std::max(z+0.15f,local+0.15f);
            foot&=std::fabs(local-z)<=1.5f&&start<z+height&&map.isInLineOfSight(x+offset.first,y+offset.second,start,x+offset.first,y+offset.second,z+height,actor->GetPhases(),VMAP::ModelIgnoreFlags::Nothing,nullptr,actor);
        }
    }
    DynamicTreeCallback support;std::set<uint32> phases;
    float receipt=map.GetHeight(actor?actor->GetPhases():phases,x,y,z,true,50.f,&support,actor);
    uint32 supportEntry=support.go&&std::fabs(receipt-z)<3.f?support.go->GetEntry():0;
    auto modelQuery=ship&&map.GetId()==1492?MMAP::MMapFactory::createOrGetMMapManager()->GetModelNavMeshQuery(ship->GetDisplayId()):nullptr;
    bool modelNav=false;G3D::Vector3 modelNearest=G3D::Vector3::zero();unsigned short modelFlags=0;
    if(modelQuery)
    {
        auto local=ship->m_model->WorldToModel({x,y,z});float at[3]={local.y,local.z,local.x},found[3];dtPolyRef ref=0;
        if(dtStatusSucceed(modelQuery->findNearestPoly(at,extent,&filter,&ref,found))&&ref)
        {
            modelNearest=ship->m_model->ModelToWorld({found[2],found[0],found[1]});modelQuery->getAttachedNavMesh()->getPolyFlags(ref,&modelFlags);
            float direct=ship->m_model->getHeight(x,y,z,3.f,actor?actor->GetPhases():ship->GetPhases(),actor&&actor->IsUnitOwnedByPlayer());
            modelNav=std::fabs(modelNearest.z-z)<0.8f&&std::fabs(direct-z)<0.01f&&(modelFlags&1);
        }
    }
    std::cout<<"SURFACE\t"<<kind<<'\t'<<id<<'\t'<<map.GetId()<<'\t'<<entry<<'\t'<<x<<'\t'<<y<<'\t'<<z<<'\t'<<area<<'\t'<<adt<<'\t'<<root<<'\t'<<group<<'\t'<<flags<<'\t'<<bool(query)<<'\t'<<nav<<'\t'<<nearest[2]<<'\t'<<nearest[0]<<'\t'<<nearest[1]<<'\t'<<polyFlags<<'\t'<<height<<'\t'<<radius<<'\t'<<body<<'\t'<<center<<'\t'<<foot<<'\t'<<footDelta<<'\t'<<supportEntry<<'\t'<<bool(modelQuery)<<'\t'<<modelNav<<'\t'<<modelNearest.x<<'\t'<<modelNearest.y<<'\t'<<modelNearest.z<<'\t'<<modelFlags<<'\n';
}

#ifndef LEGION_TEEMING_GEOMETRY_SUPPORT_ONLY
int main(int argc,char** argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);auto path=std::getenv("LEGION_TEEMING_POINTS");Require(path,"Physical survey input missing");
        if(auto data=std::getenv("LEGION_MAW_MODEL_DATA"))for(uint32 id:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(id,std::string(data)+'/'),"Explicit staged model navigation unavailable");
        std::ifstream input(path);Require(bool(input),"Physical survey input unreadable");std::string line;
        // Also admit explicitly requested native world maps when investigating
        // a path-ID collision shared by dungeon and outdoor actors.
        std::unordered_map<uint32,std::vector<uint32>> physicalMaps;
        for(auto entry:sMapStore)if(entry->ExpansionID==6&&entry->InstanceType==MAP_INSTANCE)physicalMaps.emplace(entry->ID,std::vector<uint32>{});
        {std::ifstream scan(path);std::string record;while(std::getline(scan,record)){std::istringstream fields(record);char kind;uint64 id;uint32 mapId;Require(bool(fields>>kind>>id>>mapId)&&sMapStore.LookupEntry(mapId),"Physical survey map absent in matching client");physicalMaps.emplace(mapId,std::vector<uint32>{});}}
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(physicalMaps);MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(physicalMaps);
        std::map<uint32,std::unique_ptr<MapInstanced>> parents;std::map<uint32,std::unique_ptr<Map>> maps;
        std::unique_ptr<PhysicalSupportShip> ship;
        std::cout.precision(10);unsigned candidates=0,anchors=0;
        while(std::getline(input,line))
        {
            std::istringstream row(line);char kind;uint64 id;uint32 mapId,entry;float x,y,z;
            Require(bool(row>>kind>>id>>mapId>>entry>>x>>y>>z),"Physical survey row shape changed");
            if(!maps.count(mapId))
            {
                if(sMapStore.LookupEntry(mapId)->Instanceable())
                {
                    parents[mapId]=std::make_unique<MapInstanced>(mapId,300);
                    auto instance=std::make_unique<InstanceMap>(mapId,300,9947000+uint32(maps.size()),DIFFICULTY_MYTHIC_KEYSTONE,parents[mapId].get());instance->CreateInstanceData(nullptr);maps[mapId]=std::move(instance);
                }
                else maps[mapId]=std::make_unique<Map>(mapId,300,0,DIFFICULTY_NONE);
                if(mapId==1492)ship=std::make_unique<PhysicalSupportShip>(*maps[mapId]);
            }
            auto& map=*maps[mapId];map.LoadGrid(x,y);
            if(kind=='N')
            {
                Surface(map,kind,id,entry,x,y,z,nullptr,ship.get());++anchors;continue;
            }
            SurveyActor actor;Require(actor.Create(180800+candidates,&map,1,entry,0,ALLIANCE,x,y,z,0),"Native candidate model/create failed");
            if(kind=='T')
            {
                float dx,dy,dz;Require(bool(row>>dx>>dy>>dz),"Native path endpoint missing");
                map.LoadGrid(dx,dy);
                Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native path survey publication failed");
                PathGenerator path(&actor);bool accepted=path.CalculatePath(dx,dy,dz,false);
                std::cout<<"PATH\t"<<id<<'\t'<<accepted<<'\t'<<uint32(path.GetPathType())<<'\t'<<path.GetPath().size()<<'\t'<<actor.CanFly()<<'\t'<<actor.CanSwim()<<'\n';
            }
            else if(kind=='P')
            {
                float ground=map.GetHeight(actor.GetPhases(),x,y,z,true,50.f,nullptr,&actor);
                if(ground>INVALID_HEIGHT)Surface(map,'C',id,entry,x,y,ground,&actor,ship.get());
            }
            else
            {
            float previous=MAX_HEIGHT;
            for(unsigned level=0;level<24;++level)
            {
                float ground=map.GetHeight(actor.GetPhases(),x,y,previous,true,200000.f,nullptr,&actor);
                if(ground<=INVALID_HEIGHT||ground>=previous+0.01f)break;
                Surface(map,kind,id,entry,x,y,ground,&actor,ship.get());previous=ground-2.25f;
            }
            }
            actor.CombatStop(true);actor.CastStop();actor.RemoveAllAuras();actor.m_Events.KillAllEvents(true);if(actor.IsInWorld())map.RemoveFromMap(static_cast<Creature*>(&actor),false);actor.ResetMap();++candidates;
        }
        ship.reset();maps.clear();parents.clear();std::cout<<"PASS physical survey anchors="<<anchors<<" candidates="<<candidates<<";native surfaces are observations,not accepted placement or whole-chain assertions\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
#endif
