// Actual 7.3.5 DB2, GameObject::Create and production IsInRange; no quest grants.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "GameObject.h"
#include <G3D/CoordinateFrame.h>
#include <filesystem>
#include <cstdlib>
struct RangeObject:GameObject {
 GameObjectTemplate fallback;
 void MissingDisplay(){fallback=*m_goInfo;fallback.displayId=0;m_goInfo=&fallback;}
 ~RangeObject(){ResetMap();}
};
int main(int argc,char**argv){try{
 SetUnhandledExceptionFilter(CrashTrace);Require(argc==2,"Expected dbc path");auto config=std::getenv("LEGION_DIALOGUE_CONFIG");Require(config,"Missing private config");
 auto dir=std::filesystem::path(argv[1]).lexically_normal();if(dir.filename().empty())dir=dir.parent_path();std::string runtime=dir.parent_path().parent_path().string()+'/';char* args[]={argv[0],config,runtime.data()};InitializeNative(3,args);
 sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1469,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1469,{}}});
 Map map(1469,300,9996997,DIFFICULTY_NONE);RangeObject object;
 Require(object.Create(2601,245122,&map,1,{887.451f,1106.41f,49.4598f,.237051f},G3D::Quat(0,0,0,1),255,GO_STATE_READY),"Native rock create failed");
 auto info=sGameObjectDisplayInfoStore.LookupEntry(2230);Require(info,"Matching2230 DB2 missing");
 std::cout<<"DB2 2230 min="<<info->GeoBoxMin.X<<','<<info->GeoBoxMin.Y<<','<<info->GeoBoxMin.Z<<" max="<<info->GeoBoxMax.X<<','<<info->GeoBoxMax.Y<<','<<info->GeoBoxMax.Z<<" scale="<<object.GetObjectScale()<<'\n';
 bool face=object.IsInRange(873.333f,1109.87f,48.1834f,5.0f),vertical=object.IsInRange(887.451f,1106.41f,249.4598f,5.0f);
 unsigned cases=0,mismatches=0;
 // Independent forward G3D transform, with inside/outside points in the scaled
 // native DB2 box. Strict boundary behavior is retained; use .05 margins to
 // avoid exact-edge float round-off on translated world coordinates.
 for(float scale:{.5f,1.f,1.75f,2.5f})for(float yaw:{0.f,.237051f,1.5707963f,3.1415926f,4.3f})for(float radius:{0.f,5.f}){
  object.SetObjectScale(scale);object.Relocate(887.451f,1106.41f,49.4598f,yaw);
  G3D::CoordinateFrame frame(G3D::Matrix3::fromEulerAnglesZYX(yaw,0,0),{object.GetPositionX(),object.GetPositionY(),object.GetPositionZ()});
  G3D::Vector3 lo(info->GeoBoxMin.X*scale-radius,info->GeoBoxMin.Y*scale-radius,info->GeoBoxMin.Z*scale-radius),hi(info->GeoBoxMax.X*scale+radius,info->GeoBoxMax.Y*scale+radius,info->GeoBoxMax.Z*scale+radius);
  auto middle=(lo+hi)*.5f;
  for(unsigned axis=0;axis<3;++axis)for(bool upper:{false,true})for(bool inside:{false,true}){
   auto local=middle;local[axis]=(upper?hi[axis]:lo[axis])+(upper?1.f:-1.f)*(inside?-.05f:.05f);auto world=frame.pointToWorldSpace(local);
   bool actual=object.IsInRange(world.x,world.y,world.z,radius);if(actual!=inside)++mismatches;++cases;
  }
 }
#ifdef EXPECT_BASELINE
 Require(!face&&vertical&&mismatches>0,"Expected pinned pre-fix native range failures no longer reproduced");
 std::cout<<"COUNTERFACTUAL "<<cases<<" native scaled/rotated DB2 boundary cases, mismatches="<<mismatches<<"; actual rock face rejected and same-XY remote height admitted. Expected broken baseline, not passing ordinary behavior\n";
#else
 Require(face&&!vertical&&!mismatches,"Native DB2 interaction face/vertical/scaled rotated boundaries differ");
 object.MissingDisplay();object.SetObjectScale(1);object.Relocate(10,20,30,0);
 Require(object.IsInRange(10,20,30,5)&&!object.IsInRange(10,20,230,5),"Missing-display native spherical fallback changed");
 std::cout<<"PASS "<<cases+4<<" actual native GameObject interaction cases: matching2230 DB2, Ring face, same-XY remote height, 4 scales, 5 yaw rotations, 2 radii, 6 independent box faces and missing-display fallback; no range enlargement, phase or quest grants\n";
#endif
 std::cout.flush();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
