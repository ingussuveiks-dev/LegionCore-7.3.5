#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32=uint32_t;
using int32=int32_t;
constexpr int QUEST_STATUS_INCOMPLETE=3,EFFECT_0=0;
#define TC_LOG_DEBUG(...) ((void)0)
struct Position {float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}};
struct WorldLocation:Position {WorldLocation()=default;WorldLocation(int,float a,float b,float c,float d){x=a;y=b;z=c;o=d;}};
struct SpellDestination {WorldLocation pos;explicit SpellDestination(WorldLocation p):pos(p){}};
struct Creature;
struct Unit {virtual Creature* ToCreature(){return nullptr;}};
struct Player:Unit {
 bool alive=true,near=true;int quest=3;
 bool IsAlive(){return alive;}int GetQuestStatus(int){return quest;}
 bool IsWithinDistInMap(Creature*,float){return near;}
 Position GetFirstCollisionPosition(float dist,float angle){assert(dist>=4 && dist<=8 && angle>=-1 && angle<=1);return {3100,980,257,0};}
};
struct Map {std::vector<Player*> players;template<class F>void ApplyOnEveryPlayer(F f){for(auto p:players)f(p);}};
struct Creature:Unit {uint32 mapId=1522,db=369458;Map map;Creature* ToCreature()override{return this;}uint32 GetMapId(){return mapId;}uint32 GetDBTableGUIDLow(){return db;}uint32 GetInstanceId(){return 1;}Map*GetMap(){return &map;}};
struct Scenario {uint32 id=962,step=5;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct Manager {Scenario*GetScenario(uint32){return &scenario;}} manager;
auto sScenarioMgr=&manager;
float frand(float a,float b){return (a+b)/2;}
enum {TARGET_CHECK_ENTRY,TARGET_OBJECT_TYPE_GOBJ,TARGET_OBJECT_TYPE_DEST,TARGET_OBJECT_TYPE_OBJ_AND_DEST,SPELL_EFFECT_TELEPORT_L,SPELL_EFFECT_TELEPORT};
struct TargetType {int GetCheckType(){return TARGET_CHECK_ENTRY;}int GetObjectType(){return TARGET_OBJECT_TYPE_OBJ_AND_DEST;}};
struct SpellTargetPosition {int target_mapId=1522;float target_X=3088.45f,target_Y=834.382f,target_Z=243.846f,target_Orientation=0;};
struct SpellMgr {SpellTargetPosition position;bool installed=false;SpellTargetPosition const*GetSpellTargetPosition(int){return installed?&position:nullptr;}} spellMgr;
auto sSpellMgr=&spellMgr;
struct Info {int Id=199215;struct {bool RequiresSpellFocus=false;} CastingReq;bool HasEffect(int){return false;}} info;
struct Targets {WorldLocation dst;bool set=false;Unit*GetUnitTarget(){return nullptr;}void SetDst(Unit&){set=true;}void SetDst(float x,float y,float z,float o=0,int=0){dst=WorldLocation(1522,x,y,z,o);set=true;}};
struct Spell {
 Creature* m_caster=nullptr;Info* m_spellInfo=&info;Targets m_targets;WorldLocation effectDestination;bool effectSet=false;int searches=0;
 void AddDestTarget(SpellDestination const& d,uint32){effectDestination=d.pos;effectSet=true;}
 void AddGOTarget(Unit*,int){}
 void ResolveNativeDestination(){TargetType targetType;std::vector<int>*condList=nullptr;Unit*focusObject=nullptr;int effMask=1;
#include "NearbyDestination.inc"
 ++searches;
 }
};
struct Hook {void operator+=(int){}};
struct SpellScript {Unit*caster=nullptr;Spell*spell=nullptr;Hook OnCast;virtual void Register(){}Unit*GetCaster(){return caster;}Spell*GetSpell(){return spell;}void SetExplTargetDest(WorldLocation& loc){spell->m_targets.SetDst(loc.x,loc.y,loc.z,loc.o);}};
#define PrepareSpellScript(name) public:
#define SpellCastFn(...) 0
#include "EscapeMeteor.inc"
int main(){
 Creature guide;Spell summon;summon.m_caster=&guide;
 summon.m_targets.SetDst(3088.45f,834.382f,243.846f);summon.ResolveNativeDestination();
 assert(summon.searches==1); // explicit SAI coordinates do not satisfy target 46
 spellMgr.installed=true;summon.ResolveNativeDestination();
 assert(summon.searches==1 && summon.m_targets.dst.y==834.382f);
 guide.mapId=1502;summon.m_targets.set=false;summon.ResolveNativeDestination();assert(!summon.m_targets.set);guide.mapId=1522;
 Player player;guide.map.players={&player};Spell meteor;
 spell_new_blood_escape_meteor script;script.caster=&guide;script.spell=&meteor;
 // A fresh native periodic cast has a fresh destination; exercise several ticks.
 for(int i=0;i<5;++i){meteor.m_targets.set=false;meteor.effectSet=false;script.TargetRoute();assert(meteor.m_targets.set && meteor.effectSet && meteor.m_targets.dst.y==980 && meteor.effectDestination.y==980);}
 for(int id:{998,1011}){scenario.id=id;meteor.m_targets.set=false;script.TargetRoute();assert(!meteor.m_targets.set);}scenario.id=962;
 for(int step:{0,4,6}){scenario.step=step;meteor.m_targets.set=false;script.TargetRoute();assert(!meteor.m_targets.set);}scenario.step=5;
 guide.db=369457;script.TargetRoute();assert(!meteor.m_targets.set);guide.db=369458;
 player.near=false;script.TargetRoute();assert(!meteor.m_targets.set);player.near=true;
 player.alive=false;script.TargetRoute();assert(!meteor.m_targets.set);player.alive=true;
 player.quest=0;script.TargetRoute();assert(!meteor.m_targets.set);
 std::cout<<"PASS: production native target-46 resolution and repeated escape meteor retargeting; unrelated scenario/spawn/stage/player isolation\n";
}
