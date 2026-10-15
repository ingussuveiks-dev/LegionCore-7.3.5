#include <cassert>
#include <cmath>
#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <iostream>
using uint32=uint32_t;using uint16=uint16_t;
constexpr int CLASS_WARLOCK=9,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
constexpr int TEMPSUMMON_MANUAL_DESPAWN=1,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_QUESTGIVER=2;
constexpr int SPELL_AURA_HOVER=1,MOVEMENTFLAG_FALLING=4;
#define TC_LOG_INFO(...) ((void)0)
struct Position{float x,y,z,o;};
enum class CreatureGroundMovementType{None,Run,Hover};
struct Movement{CreatureGroundMovementType Ground=CreatureGroundMovementType::None;int flight=1;
 bool IsGroundAllowed()const{return Ground!=CreatureGroundMovementType::None;}bool IsFlightAllowed()const{return flight!=0;}
 bool CanFly()const{return flight==2;}bool IsGravityDisabled()const{return flight==1;}};
namespace G3D{bool fuzzyGt(float x,float y){return x>y;}bool fuzzyLt(float x,float y){return x<y;}}
struct HeightMap{float ground=200;float GetHeight(std::set<uint32> const&,float,float,float){return ground;}};
struct Creature;using TempSummon=Creature;
struct Creature{
 bool alive=true,bound=false,expired=false,temporary=true;int owner=0,npcflags=0;Position pos{};
 bool m_playerMovingMe=false,fly=false,gravity=false,hover=false,falling=false;Movement movement;HeightMap map;
 bool IsAlive(){return alive;}bool HasQuestObjectScope(){return bound;}bool IsQuestObjectExpired(){return expired;}
 TempSummon* ToTempSummon(){return temporary?this:nullptr;}int GetSummonerGUID(){return owner;}
 void DespawnOrUnsummon(){alive=false;expired=true;}
 void SetPhaseId(std::set<uint32> const&,bool){}void SetUInt32Value(int,int value){npcflags=value;}
 HeightMap* GetMap(){return &map;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZMinusOffset(){return pos.z;}
 std::set<uint32> const& GetPhases(){static std::set<uint32> s{6930};return s;}Movement const& GetMovementTemplate(){return movement;}
 bool IsFalling(){return falling;}bool HasAuraType(int){return false;}bool CanHover(){return false;}
 void SetCanFly(bool x){fly=x;}void SetDisableGravity(bool x){gravity=x;}void SetHover(bool x){hover=x;}
 void RemoveUnitMovementFlag(int){falling=false;}bool CanSwim(){return false;}bool IsInWater(){return false;}void SetSwim(bool){}
 void UpdateMovementFlags();
};
#include "Movement.inc"
struct Quest{};
struct QuestStore{Quest q;Quest const* GetQuestTemplate(uint16 id){return id?&q:nullptr;}} store;
auto sQuestDataStore=&store;
std::list<Creature> actors;
struct Player{
 int owner=1,cls=9,status=3,map=1107,binds=0,credits=0;bool rewarded=false,world=true,tele=false,summon=true;
 uint16 m_adventure_questID=0;bool m_AdventureQuestChanged=false;
 Position pos{3038.36f,887.68f,247.333f,0};
 int getClass(){return cls;}int GetQuestStatus(int id){assert(id==40824);return status;}
 bool GetQuestRewardStatus(int){return rewarded;}bool IsInWorld(){return world;}bool IsBeingTeleported(){return tele;}
 int GetMapId(){return map;}int GetGUID(){return owner;}int GetGUIDLow(){return owner;}
 std::set<uint32> const& GetPhases(){static std::set<uint32>s{6930,6932};return s;}
 float GetExactDist(Position const* p){return std::sqrt(std::pow(pos.x-p->x,2)+std::pow(pos.y-p->y,2)+std::pow(pos.z-p->z,2));}
 void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,int entry,float radius){assert(entry==101921);for(auto& c:actors)if(GetExactDist(&c.pos)<=radius)out.push_back(&c);}
 Creature* SummonCreature(int entry,Position const& p,int,int,int,int guid){assert(entry==101921);if(!summon)return nullptr;actors.emplace_back();auto& c=actors.back();c.pos=p;c.owner=guid;return &c;}
 void BindQuestObject(int q,Creature* c){assert(q==40824);c->bound=true;++binds;}
 void KilledMonsterCredit(int id){assert(id==97067);credits=1;}
 void setAdventureQuestID(uint16 questID);
};
#include "Adventure.inc"
#include "Plans.inc"
int main(){
 Creature board;board.pos.z=247.333f;board.movement.Ground=CreatureGroundMovementType::Run;board.movement.flight=2;
 board.UpdateMovementFlags();assert(board.fly&&!board.gravity); // actual old native branch
 // A template reload requires a fresh spawn/server restart, not changing
 // a running creature's template beneath its current movement flags.
 for(float ground:{200.f,247.333f,300.f}){Creature fixed;fixed.pos.z=247.333f;fixed.map.ground=ground;fixed.UpdateMovementFlags();assert(fixed.gravity&&!fixed.fly&&!fixed.hover);}
 Player p;for(int i=0;i<10000;++i)DreadscarPlans::Recover(&p);assert(p.binds==1&&actors.size()==1&&actors.front().npcflags==2&&p.credits==0);
 p.setAdventureQuestID(0);assert(!p.credits);p.setAdventureQuestID(39718);assert(p.credits==1&&p.m_AdventureQuestChanged);p.setAdventureQuestID(39718);assert(p.credits==1);
 auto duplicate=actors.front();actors.push_back(duplicate);DreadscarPlans::Recover(&p);assert(!actors.back().alive&&actors.front().alive);
 Player other;other.owner=2;other.status=QUEST_STATUS_COMPLETE;DreadscarPlans::Recover(&other);assert(other.binds==1&&p.binds==1);
 for(int mode=0;mode<7;++mode){Player q;q.owner=10+mode;if(mode==0)q.cls=1;if(mode==1)q.status=0;if(mode==2)q.rewarded=true;if(mode==3)q.world=false;if(mode==4)q.tele=true;if(mode==5)q.map=1220;if(mode==6)q.pos.z-=50;DreadscarPlans::Recover(&q);assert(!q.binds);}
 Player retry;retry.owner=30;retry.summon=false;DreadscarPlans::Recover(&retry);assert(!retry.binds);retry.summon=true;DreadscarPlans::Recover(&retry);assert(retry.binds==1);
 actors.back().expired=true;DreadscarPlans::Recover(&retry);assert(retry.binds==2);
 std::cout<<"PASS: extracted native movement, actual adventure quest credit, personal recovery 10,000 updates, retries and owner/stage/map boundaries\n";
}
