#include "MMapManager.h"
#include "MapDefines.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>
#include "NavInteractionSurface.h"

// An audit query, not an override of the server's per-instance query budget.
// A partial corridor after node exhaustion cannot establish disconnected terrain.
static bool Complete(dtStatus status)
{
    return dtStatusSucceed(status) && !(status & DT_STATUS_DETAIL_MASK);
}

static float Distance(float const* a, float const* b)
{
    return std::sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2]));
}

#ifndef NAV_TARGET_INTERACTION
#define NAV_TARGET_INTERACTION(root,center,point) (Distance(center,point)<=5)
#define NAV_INTERACTION_INIT(root,input) true
#define NAV_INTERACTION_RADIUS 6
#define NAV_INTERACTION_ANGLES 16
#endif

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--self-test")
    {
        if (!Complete(DT_SUCCESS) || Complete(DT_FAILURE) || Complete(DT_SUCCESS | DT_PARTIAL_RESULT) ||
            Complete(DT_SUCCESS | DT_OUT_OF_NODES) || Complete(DT_SUCCESS | DT_BUFFER_TOO_SMALL)) return 1;
        std::cout << "PASS: failed, partial, exhausted and truncated queries cannot pass.\n";
        return 0;
    }
    if (argc != 4 && argc != 5 && argc != 6) return 2;
    int budget = std::stoi(argv[3]);
    if (budget < 1 || budget > 65535) return 2;
    uint32 map = argc >= 5 ? uint32(std::stoul(argv[4])) : 1220;
    bool swim = argc == 6;
    if (swim && std::string(argv[5]) != "walk-swim") return 2;
    MMAP::MMapManager mm;
    mm.InitializeThreadUnsafe({{map,{}}});
    std::string root = argv[1];
    if (!NAV_INTERACTION_INIT(root,argv[2])) return 4;
    NavInteractionSurface surfaces(map,root);
    std::ifstream input(argv[2]);
    float points[2][3];
    std::set<std::pair<int,int>> tiles;
    for (auto& point : points)
    {
        // Detour coordinates are world y,z,x. Retain the original endpoint/tile bounds.
        if (!(input >> point[2] >> point[0] >> point[1])) return 3;
        int tx = int(32-point[2]/533.33333f), ty = int(32-point[0]/533.33333f);
        for (int dx=-1; dx<=1; ++dx) for (int dy=-1; dy<=1; ++dy)
            if (tiles.insert({tx+dx,ty+dy}).second) mm.loadMap(root+"/",map,tx+dx,ty+dy);
    }
    auto mesh = mm.GetNavMesh(map);
    if (!mesh) return 4;
    dtNavMeshQuery query;
    if (!Complete(query.init(mesh,budget))) return 4;
    dtQueryFilter filter;
    // Player routes can cross ordinary water. Keep the default ground-only
    // diagnostic and never admit magma/slime to this walking/swimming audit.
    filter.setIncludeFlags(NAV_GROUND | (swim ? NAV_WATER : 0));
    struct Approach { dtPolyRef ref; float p[3]; };
    std::vector<Approach> choices[2];
    for (unsigned i=0; i<2; ++i)
    {
        std::set<dtPolyRef> seen;
        for (int radius=0; radius<=NAV_INTERACTION_RADIUS; ++radius) for (int angle=0; angle<NAV_INTERACTION_ANGLES; ++angle) for (unsigned terrain=0;terrain<(swim?2u:1u);++terrain)
        {
            float a=angle*6.283185307f/NAV_INTERACTION_ANGLES;
            float probe[3]={points[i][0]+radius*std::sin(a),points[i][1],points[i][2]+radius*std::cos(a)};
            float search[3]={1.5f,4,1.5f}, nearest[3]={};
            dtPolyRef ref=0;
            // Probe layers separately. An invalid liquid polygon below land
            // must not hide a valid ground approach at the same XY position.
            dtQueryFilter nearestFilter=filter;
            nearestFilter.setIncludeFlags(terrain?NAV_WATER:NAV_GROUND);
            auto status=query.findNearestPoly(probe,search,&nearestFilter,&ref,nearest);
            if (Complete(status) && ref && (i==0?Distance(nearest,points[i])<=5:NAV_TARGET_INTERACTION(root,points[i],nearest)) &&
                (!swim || i==0 || surfaces.Check(nearest,points[i],false)) && seen.insert(ref).second)
            {
                Approach approach;
                approach.ref=ref;
                std::copy(nearest,nearest+3,approach.p);
                choices[i].push_back(approach);
            }
        }
    }
    bool exhausted=false;
    for (auto const& start : choices[0]) for (auto const& end : choices[1])
    {
        dtPolyRef path[2048]; int count=0;
        auto status=query.findPath(start.ref,end.ref,start.p,end.p,&filter,path,&count,2048);
        if (!Complete(status) || !count || path[count-1]!=end.ref)
        {
            exhausted |= dtStatusDetail(status,DT_OUT_OF_NODES);
            std::cout << "INCOMPLETE: budget=" << budget << " status=0x" << std::hex << status << std::dec
                      << " polygons=" << count << " out_of_nodes=" << dtStatusDetail(status,DT_OUT_OF_NODES)
                      << " partial=" << dtStatusDetail(status,DT_PARTIAL_RESULT) << "\n";
            continue;
        }
        float straight[2048*3]; unsigned char flags[2048]; int corners=0;
        status=query.findStraightPath(start.p,end.p,path,count,straight,flags,nullptr,&corners,2048);
        if (!Complete(status) || corners<2 || !(flags[corners-1]&DT_STRAIGHTPATH_END) ||
            Distance(straight+3*(corners-1),end.p)>0.01f) return 5;
        float length=0;
        for (int i=1; i<corners; ++i) length+=Distance(straight+3*(i-1),straight+3*i);
        unsigned ground=0,water=0;
        for (int i=0; i<count; ++i)
        {
            dtMeshTile const* tile=nullptr;dtPoly const* poly=nullptr;
            if (!Complete(mesh->getTileAndPolyByRef(path[i],&tile,&poly)) || !poly ||
                (poly->flags & ~(NAV_GROUND | NAV_WATER)) || !(poly->flags & filter.getIncludeFlags())) return 5;
            if (poly->flags & NAV_GROUND) ++ground;
            if (poly->flags & NAV_WATER) ++water;
        }
        std::cout << "PASS: Source approach " << start.p[2] << "," << start.p[0] << "," << start.p[1]
                  << " -> target approach " << end.p[2] << "," << end.p[0] << "," << end.p[1]
                  << " through " << count << " native " << (swim ? "ground/water" : "ground") << " navmesh polygons; budget=" << budget
                  << ", complete corridor and endpoint, " << corners << " corners, length=" << length << "yd.\n";
        if (swim) std::cout << "terrain polygons ground=" << ground << " water=" << water << " magma/slime=0\n";
        if (swim) std::cout << "endpoint centerDistance=" << Distance(end.p,points[1]) << '\n';
        if (swim && !surfaces.Check(end.p,points[1],true)) return 5;
        for (int i=0; i<corners; ++i)
            std::cout << "corner " << i << " " << straight[3*i+2] << " " << straight[3*i] << " " << straight[3*i+1] << "\n";
        return 0;
    }
    if (swim && choices[1].empty()) for (unsigned flags:{unsigned(NAV_GROUND),unsigned(NAV_WATER)})
    {
        dtQueryFilter nearestFilter;nearestFilter.setIncludeFlags(flags);
        float ext[3]={1.5f,4,1.5f},nearest[3]={};dtPolyRef ref=0;
        if (Complete(query.findNearestPoly(points[1],ext,&nearestFilter,&ref,nearest)) && ref)
        {
            std::cout << "Rejected nearest " << (flags==NAV_GROUND?"ground":"water") << " diagnostic: ";
            surfaces.Check(nearest,points[1],true);
        }
    }
    std::cout << (exhausted ? "EXHAUSTED" : "FAIL") << ": approaches (" << choices[0].size() << "," << choices[1].size() << ").\n";
    return exhausted ? 7 : 6;
}
