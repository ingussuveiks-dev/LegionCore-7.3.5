#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
using uint32=uint32_t;
namespace G3D {
struct Vector3 {
    float x,y,z;
    Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
    static Vector3 zero(){return {};}
    Vector3 operator+(Vector3 const& b)const{return {x+b.x,y+b.y,z+b.z};}
    Vector3 operator*(float n)const{return {x*n,y*n,z*n};}
};
}
struct Position {
    float x=0,y=0,z=0;
    float GetPositionX()const{return x;} float GetPositionY()const{return y;} float GetPositionZ()const{return z;}
    float GetExactDist(Position const* p)const{return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}
};
struct GameObjectModel {
    bool enabled=true,gap=false,wall=false;
    float step=0,width=2,sideCurb=0;
    bool isCollisionEnabled()const{return enabled;}
    float getHeight(float x,float y,float z,float length,std::set<uint32>const& phases,bool playerRules)const {
        assert(!playerRules);
        if(!enabled||!phases.count(1)||std::fabs(y)>width||(gap&&x>=5&&x<=7))return -std::numeric_limits<float>::infinity();
        float floor=x>=6?step:0;
        if(y>.2f)floor+=sideCurb;
        float ray=z+.5f; // Actual GameObjectModel::getHeight convention.
        return ray>=floor&&ray-floor<=length?floor:-std::numeric_limits<float>::infinity();
    }
    bool isInLineOfSight(G3D::Vector3 const& a,G3D::Vector3 const& b,std::set<uint32>const&,bool)const {
        return !(wall&&a.x<=6&&b.x>=6);
    }
};
struct Transport {GameObjectModel* m_model;std::set<uint32> phases{1};auto const& GetPhases()const{return phases;}};
namespace Movement {using PointsArray=std::vector<G3D::Vector3>;}
#include "TransportGroundPath.h"
namespace PriorCharge {using Movement::PointsArray;}
#include "PriorTransportGroundPath.inc"
int main(){
    GameObjectModel model;Transport ship{&model};Movement::PointsArray points;
    Position start{},end{12,0,0};
    auto check=[&](Movement::TransportGroundResult expected){
        auto result=Movement::BuildTransportGroundPath(&ship,start,end,2,74,points);
        assert(result==expected);
        if(expected!=Movement::TransportGroundResult::Complete)assert(points.empty());
        else assert(points.size()>=2&&points.front().z==0&&points.back().z==model.step);
    };
    for(float z:{0.f,.1f,.49f,.55f,-.55f}){
        start.z=z;
        for(unsigned attempt=0;attempt<8;++attempt)check(Movement::TransportGroundResult::Complete);
    }
    start.z=.55f;
    assert(PriorCharge::BuildTransportGroundPath(&ship,start,end,2,74,points)==PriorCharge::TransportGroundResult::NoPath);
    check(Movement::TransportGroundResult::Complete);
    start.z=.7f;check(Movement::TransportGroundResult::NoPath);start.z=0;
    end.z=.55f;check(Movement::TransportGroundResult::Complete);
    end.z=.7f;check(Movement::TransportGroundResult::NoPath);end.z=0;
    for(float height:{.4f,.55f,-.4f,-.55f}){
        model.step=end.z=height;check(Movement::TransportGroundResult::Complete);
    }
    model.step=end.z=.7f;check(Movement::TransportGroundResult::NoPath);model.step=end.z=0;
    model.step=end.z=-.9f;check(Movement::TransportGroundResult::Complete);
    assert(points.size()==6); // Preserve both sides of the curb in the native path.
    bool curbPair=false;
    for(size_t i=1;i<points.size();++i)
        if(points[i-1].x<6&&points[i-1].z==0&&points[i].x==6&&points[i].z==-.9f)curbPair=true;
    assert(curbPair);
    assert(Movement::BuildTransportGroundPath(&ship,start,end,2,5,points)==Movement::TransportGroundResult::TooLong&&points.empty());
    model.step=end.z=-1.1f;check(Movement::TransportGroundResult::NoPath);model.step=end.z=0;
    model.sideCurb=.8f;check(Movement::TransportGroundResult::Complete);
    model.sideCurb=1.1f;check(Movement::TransportGroundResult::NoPath);model.sideCurb=0;
    model.gap=true;check(Movement::TransportGroundResult::NoPath);model.gap=false;
    model.wall=true;check(Movement::TransportGroundResult::NoPath);model.wall=false;
    model.width=.3f;check(Movement::TransportGroundResult::NoPath);model.width=2;
    ship.phases={2};check(Movement::TransportGroundResult::NoPath);ship.phases={1};
    model.enabled=false;check(Movement::TransportGroundResult::NoPath);model.enabled=true;
    assert(Movement::BuildTransportGroundPath(&ship,start,end,2,2,points)==Movement::TransportGroundResult::TooLong&&points.empty());
    start.x=std::numeric_limits<float>::quiet_NaN();check(Movement::TransportGroundResult::NoPath);
    std::cout<<"PASS actual ground helper: repeatable first admission at bounded source/destination height offsets, ascending/descending0.55 steps, native-ray counterfactual, gap/wall/radius/phase/collision/height/point guards. Model boundaries mocked; both real ship models tested separately.\n";
}
