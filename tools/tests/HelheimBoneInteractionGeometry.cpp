#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include "G3D/g3dmath.h"
#include <fstream>
#include <cmath>
static DB2Storage<GameObjectDisplayInfoEntry> boneDisplays("GameObjectDisplayInfo.db2",GameobjectDisplayInfoLoadInfo::Instance());
struct BoneInteractionObject
{
    struct Info {uint32 displayId=6328;} info;
    Info const* m_goInfo=&info;
    float x=0,y=0,z=0,o=0;
    float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}
    bool IsWithinDist3d(float,float,float,float)const{return false;}
    bool IsInRange(float,float,float,float)const;
} bone;
#define sGameObjectDisplayInfoStore boneDisplays
#include "NativeBoneRange.inc"
#undef sGameObjectDisplayInfoStore
static bool InitBone(std::string const& root,char const* input)
{
    if(!boneDisplays.Load((root+"/dbc/enUS/").c_str(),0)||!boneDisplays.LookupEntry(6328))return false;
    std::ifstream file(input);float ignored;for(unsigned i=0;i<6;++i)if(!(file>>ignored))return false;
    return bool(file>>bone.o);
}
static bool BoneRange(std::string const&,float const* center,float const* point)
{
    bone.x=center[2];bone.y=center[0];bone.z=center[1];
    return bone.IsInRange(point[2],point[0],point[1],5);
}
#define NAV_TARGET_INTERACTION(root,center,point) BoneRange(root,center,point)
#define NAV_INTERACTION_INIT(root,input) InitBone(root,input)
#define NAV_INTERACTION_RADIUS 8
#define NAV_INTERACTION_ANGLES 32
#include "StormheimStemTideGeometry.cpp"
