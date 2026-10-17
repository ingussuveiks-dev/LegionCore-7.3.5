#include <string>
#include <cassert>
#include "ShieldReach.inc"
namespace ShieldSurvey {
struct Transport {unsigned GetGUIDLow()const{return 0;}};
struct RangeObject {
 float x=0,y=0,z=0,size=0;Transport* m_transport=nullptr;
 struct {struct {struct {float m_positionX=0,m_positionY=0,m_positionZ=0;}Pos;}transport;}m_movementInfo;
 struct {struct {bool WithObjectSize=false;}info;auto const& GetAreaTriggerInfo()const{return info;}} area;
 bool IsAreaTrigger()const{return false;}bool IsUnit()const{return false;}bool IsPlayer()const{return false;}
 auto const* ToAreaTrigger()const{return &area;}auto const* ToUnit()const{return this;}float GetModelSize()const{return size;}
 Transport* GetTransport()const{return m_transport;}float GetObjectSize()const{return size;}
 float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}
 bool _IsWithinDist(RangeObject const*,float,bool,bool=false)const;
};
#include "ShieldNativeRange.inc"
bool Range(float const* center,float const* point){RangeObject go,player;go.x=center[2];go.y=center[0];go.z=center[1];go.size=ShieldObjectSize;player.x=point[2];player.y=point[0];player.z=point[1];player.size=ShieldPlayerReach;return go._IsWithinDist(&player,ShieldInteractionDistance,true);}
bool Init(){RangeObject go,player;go.size=ShieldObjectSize;player.size=ShieldPlayerReach;player.x=ShieldInteractionDistance+ShieldObjectSize+ShieldPlayerReach-.001f;assert(go._IsWithinDist(&player,ShieldInteractionDistance,true));player.x+=.002f;assert(!go._IsWithinDist(&player,ShieldInteractionDistance,true));return true;}
}
#define NAV_TARGET_INTERACTION(root,center,point) ShieldSurvey::Range(center,point)
#define NAV_INTERACTION_INIT(root,input) ShieldSurvey::Init()
#define NAV_INTERACTION_RADIUS 8
#define NAV_INTERACTION_ANGLES 64
#include "NavInteractionSurface.h"
// Native book GameObject::Use/report-use has a range gate, not a ray to the
// wall-mounted model's origin+2. Keep that extra gate in other surveys.
class ShieldNativeSurface:public NavInteractionSurface {
public:using NavInteractionSurface::NavInteractionSurface;
 bool Check(float const* p,float const* object,bool report){return NavInteractionSurface::Check(p,object,report,false);}
};
#define NavInteractionSurface ShieldNativeSurface
#include "StormheimStemTideGeometry.cpp"
