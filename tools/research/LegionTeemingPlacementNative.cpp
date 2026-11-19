// Native geometry admission for explicitly reconstructed Legion extra spawns.
// Input/output are research data; this tool never writes persistent World data.
#define LEGION_TEEMING_GEOMETRY_SUPPORT_ONLY
#include "LegionTeemingGeometry.cpp"
#include "DB2LoadInfo.h"
#include <iomanip>

static bool Fits(Map& map,Creature& actor,float x,float y,float z,float hover)
{
    float height=actor.GetCollisionHeight(),radius=actor.GetBoundingRadius();
    // Some native nonblocking caster models deliberately have radius zero.
    // Their actual point footprint still needs ground and body clearance.
    if(height<=0.f||radius<0.f)return false;
    for(auto offset:std::array<std::pair<float,float>,9>{{{0,0},{radius,0},{-radius,0},{0,radius},{0,-radius},{radius*.7071f,radius*.7071f},{radius*.7071f,-radius*.7071f},{-radius*.7071f,radius*.7071f},{-radius*.7071f,-radius*.7071f}}})
    {
        float px=x+offset.first,py=y+offset.second;
        float ground=map.GetHeight(actor.GetPhases(),px,py,z,true,5.f,nullptr,&actor);
        if(ground<=INVALID_HEIGHT||std::fabs(ground-z)>1.5f)return false;
        float body=z+hover,start=std::max(body+.15f,ground+.15f);
        if(start>=body+height||!map.isInLineOfSight(px,py,start,px,py,body+height,actor.GetPhases(),VMAP::ModelIgnoreFlags::Nothing,nullptr,&actor))return false;
    }
    return true;
}

int main(int argc,char** argv)
{
    try
    {
        InitializeDungeon(argc,argv,true);
        DB2Storage<DungeonMapChunkEntry> chunks("DungeonMapChunk.db2",DungeonMapChunkLoadInfo::Instance());
        Require(chunks.Load(argv[1],0),"Native DungeonMapChunk absent");chunks.LoadFromDB();
        if(auto data=std::getenv("LEGION_MAW_MODEL_DATA"))for(uint32 id:{31852u,31853u})Require(MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(id,std::string(data)+'/'),"Maw model navigation absent");
        auto inputPath=std::getenv("LEGION_TEEMING_POINTS");Require(inputPath,"Placement input absent");std::ifstream input(inputPath);Require(bool(input),"Placement input unreadable");
        std::map<uint32,std::unique_ptr<MapInstanced>> parents;std::map<uint32,std::unique_ptr<InstanceMap>> maps;std::unique_ptr<PhysicalSupportShip> ship;
        std::string line;unsigned accepted=0,rejected=0;std::cout<<std::setprecision(10);
        while(std::getline(input,line))
        {
            std::istringstream row(line);uint64 id;uint32 mapId,entry,floor;float x,y,z,ax,ay,az,limit;
            Require(bool(row>>id>>mapId>>entry>>floor>>x>>y>>z>>ax>>ay>>az>>limit),"Placement row shape changed");
            float observedScale=0,observedHover=-1;
            if(row>>observedScale)Require(bool(row>>observedHover)&&std::isfinite(observedScale)&&observedScale>0&&observedScale<100&&std::isfinite(observedHover)&&observedHover>=0&&observedHover<100,"Native body receipt shape changed");
            if(!maps.count(mapId)){parents[mapId]=std::make_unique<MapInstanced>(mapId,300);maps[mapId]=std::make_unique<InstanceMap>(mapId,300,9949000+uint32(maps.size()),DIFFICULTY_MYTHIC_KEYSTONE,parents[mapId].get());maps[mapId]->CreateInstanceData(nullptr);if(mapId==1492)ship=std::make_unique<PhysicalSupportShip>(*maps[mapId]);}
            auto& map=*maps[mapId];map.LoadGrid(x,y);map.LoadGrid(ax,ay);
            SurveyActor actor;Require(actor.Create(200800+accepted+rejected,&map,1,entry,0,ALLIANCE,x,y,z,0),"Placement actor create failed");Require(map.AddToMap(static_cast<Creature*>(&actor)),"Placement actor publication failed");
            if(observedScale>0)actor.SetObjectScale(observedScale);
            float hover=observedHover>=0?observedHover:(actor.HasUnitMovementFlag(MOVEMENTFLAG_HOVER)?actor.GetFloatValue(UNIT_FIELD_HOVER_HEIGHT):0.f);
            std::set<uint32> displays;for(int32 model:actor.GetCreatureTemplate()->Modelid)if(model>0){displays.insert(uint32(model));if(auto info=sObjectMgr->GetCreatureModelInfo(model))if(info->displayId_other_gender)displays.insert(info->displayId_other_gender);}
            Require(!displays.empty(),"Placement template has no native model");
            auto onFloor=[&](float px,float py,float pz)
            {
                uint32 flags=0;int32 adt=0,root=0,group=0;bool area=map.GetAreaInfo(px,py,pz+.5f,flags,adt,root,group);
                float minZ=-std::numeric_limits<float>::infinity();std::set<uint32> floors;
                if(area)for(auto chunk:chunks)if(chunk->MapID==mapId&&chunk->WmoGroupID==group&&pz>=chunk->MinZ){if(chunk->MinZ>minZ){minZ=chunk->MinZ;floors.clear();}if(chunk->MinZ==minZ)floors.insert(chunk->DungeonMapID);}
                return floor?floors.count(floor)!=0:floors.empty();
            };
            struct Offset{float x,y,d;};std::vector<Offset> offsets{{0,0,0}};
            for(float dx=-limit;dx<=limit+.01f;dx+=.5f)for(float dy=-limit;dy<=limit+.01f;dy+=.5f){float d=dx*dx+dy*dy;if(d>.001f&&d<=limit*limit)offsets.push_back({dx,dy,d});}
            std::stable_sort(offsets.begin(),offsets.end(),[](Offset const&a,Offset const&b){return a.d<b.d;});
            bool found=false;float fx=0,fy=0,fz=0;uint32 pathType=0,pathPoints=0;unsigned tries=0,fit=0,corridor=0;
            for(auto offset:offsets)
            {
                ++tries;float px=x+offset.x,py=y+offset.y;
                float ground=map.GetHeight(actor.GetPhases(),px,py,z+.5f,true,5.f,nullptr,&actor);
                if(ground<=INVALID_HEIGHT||std::fabs(ground-z)>3.f||!onFloor(px,py,ground))continue;
                bool clear=true;for(uint32 display:displays){actor.SetDisplayId(display);actor.SetNativeDisplayId(display);if(!Fits(map,actor,px,py,ground,hover)){clear=false;break;}}if(!clear)continue;++fit;
                map.CreatureRelocation(&actor,px,py,ground,0);
                PathGenerator path(&actor);bool calculated=path.CalculatePath(ax,ay,az,false);
                if(fit==1){auto end=path.GetPath().empty()?G3D::Vector3::zero():path.GetPath().back();std::cout<<"CORRIDOR\t"<<id<<'\t'<<calculated<<'\t'<<uint32(path.GetPathType())<<'\t'<<end.x<<'\t'<<end.y<<'\t'<<end.z<<'\t'<<map.GetHeight(actor.GetPhases(),ax,ay,az,true,100.f,nullptr,&actor)<<'\n';}
                if(!calculated||path.GetPathType()!=PATHFIND_NORMAL||path.GetPath().empty())continue;
                auto end=path.GetPath().back();if(std::hypot(end.x-ax,end.y-ay)>1.f||std::fabs(end.z-az)>2.f)continue;++corridor;
                found=true;fx=px;fy=py;fz=ground;pathType=path.GetPathType();pathPoints=uint32(path.GetPath().size());break;
            }
            uint32 zone=0,area=0;
            if(found)map.GetZoneAndAreaId(zone,area,fx,fy,fz);
            std::cout<<"PLACEMENT\t"<<id<<'\t'<<mapId<<'\t'<<entry<<'\t'<<found<<'\t'<<fx<<'\t'<<fy<<'\t'<<fz<<'\t'<<displays.size()<<'\t'<<tries<<'\t'<<fit<<'\t'<<corridor<<'\t'<<pathType<<'\t'<<pathPoints<<'\t'<<actor.CanFly()<<'\t'<<actor.CanSwim()<<'\t'<<zone<<'\t'<<area<<'\n';
            if(found)++accepted;else ++rejected;
        }
        ship.reset();maps.clear();parents.clear();std::cout<<"PASS native placement observations accepted="<<accepted<<" rejected="<<rejected<<";all native display variants,real floor/9-point body-foot checks and complete native corridor;reconstruction not original sniff/client traversal\n";
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&error){std::cerr<<error.what()<<'\n';return 1;}
}
