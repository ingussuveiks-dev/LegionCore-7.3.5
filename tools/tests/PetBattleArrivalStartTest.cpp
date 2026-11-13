// Exact production PlayerScript class; only clock, player and battle endpoints
// are controlled here. Full-core Begin integration is checked separately.
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
using uint32 = uint32_t;
using ObjectGuid = uint32;
static uint32 clockNow;
static uint32 getMSTime() { return clockNow; }
enum { POINT_MOTION_TYPE=1, PETBATTLE_ENTER_MOVE_SPLINE_ID=42, PETBATTLE_STATUS_CREATION=0, PETBATTLE_STATUS_RUNNING=1, PETBATTLE_STATUS_FINISHED=2, UNIT_FIELD_FLAGS=3, UNIT_FLAG_PACIFIED=4, UNIT_FLAG_IMMUNE_TO_NPC=8, UNIT_STATE_ROOT=16 };
static void Require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
struct Player {
    ObjectGuid guid=11, _petBattleId=101; bool world=true, deleted=false, dead=false, rooted=false; uint32 flags=0, flagCalls=0;
    ObjectGuid GetGUID() const { return guid; } bool IsInWorld() const { return world; } bool IsDelete() const { return deleted; } bool isDead() const { return dead; }
    void SetFlag(uint32 field,uint32 value) { Require(field==UNIT_FIELD_FLAGS,"Wrong field"); flags|=value; ++flagCalls; }
    void SetControlled(bool value,uint32 state) { Require(state==UNIT_STATE_ROOT,"Wrong root"); rooted=value; }
};
struct Team { ObjectGuid PlayerGuid=0, OwnerGuid=0; };
struct PetBattle {
    uint32 BattleStatus=PETBATTLE_STATUS_CREATION, starts=0; Team teams[2]; std::array<Team*,2> Teams={&teams[0],&teams[1]}; std::function<void()> callback;
    void Begin() { ++starts; BattleStatus=PETBATTLE_STATUS_RUNNING; if(callback)callback(); }
};
struct System { std::map<ObjectGuid,PetBattle*> battles; PetBattle* GetBattle(ObjectGuid id) { auto i=battles.find(id);return i==battles.end()?nullptr:i->second; } } systemObject;
static System* sPetBattleSystem=&systemObject;
struct PlayerScript { explicit PlayerScript(char const*){} virtual ~PlayerScript()=default; virtual void OnMovementInform(Player*,uint32,uint32){} virtual void OnUpdate(Player*,uint32){} virtual void OnBeforeLogout(Player*){} };
#include "PetBattleArrivalScript.inc"
struct Fixture {
    Player player; PetBattle battle, replacement; PlayerScriptPetBattle script;
    Fixture(uint32 team,uint32 start) { clockNow=start;systemObject.battles.clear();systemObject.battles[101]=&battle;systemObject.battles[102]=&replacement;for(auto b:{&battle,&replacement})b->teams[team].PlayerGuid=b->teams[team].OwnerGuid=player.guid; }
    void Arrive(){script.OnMovementInform(&player,POINT_MOTION_TYPE,PETBATTLE_ENTER_MOVE_SPLINE_ID);}
    void Tick(uint32 at){clockNow=at;script.OnUpdate(&player,0);}
};
static uint32 count=0;
static void Case(uint32 team,uint32 start,std::string const& name) {
    Fixture f(team,start); auto& p=f.player; auto& b=f.battle;
    if(name=="early") { f.Arrive();for(uint32 elapsed:{0u,1u,999u}){f.Tick(start+elapsed);Require(!b.starts&&!p.rooted&&!p.flagCalls,"Arrival must not Begin before the original1000ms delay");}f.Tick(start+1000);Require(b.starts==1,"Due arrival must Begin exactly at1000ms"); }
    else if(name=="late") { f.Arrive();f.Tick(start+5000);Require(b.starts==1&&f.script.m_DelayedPetBattleStart.empty(),"First update after the deadline must consume and Begin the arrival"); }
    else if(name=="repeat") { f.Arrive();f.Tick(start+1000);for(uint32 i=0;i<4;++i){f.Arrive();f.Tick(start+2000+i*1000);}Require(b.starts==1&&p.flagCalls==1&&f.script.m_DelayedPetBattleStart.empty(),"Repeated callback/update cannot reset a running battle"); }
    else if(name=="duplicate") { f.Arrive();clockNow=start+900;f.Arrive();f.Tick(start+1000);Require(b.starts==1,"Duplicate movement cannot postpone the original deadline"); }
    else if(name=="othermotion") { f.script.OnMovementInform(&p,99,PETBATTLE_ENTER_MOVE_SPLINE_ID);f.script.OnMovementInform(&p,POINT_MOTION_TYPE,99);f.Tick(start+2000);Require(!b.starts&&f.script.m_DelayedPetBattleStart.empty(),"Other motion callbacks must remain unrelated"); }
    else if(name=="replacement") { f.Arrive();p._petBattleId=102;f.Tick(start+1000);Require(!b.starts&&!f.replacement.starts&&!p.flagCalls&&f.script.m_DelayedPetBattleStart.empty(),"Stale arrival cannot start a replacement battle"); }
    else if(name=="newarrival") { f.Arrive();clockNow=start+500;p._petBattleId=102;f.Arrive();f.Tick(start+1000);Require(!f.replacement.starts,"New battle arrival needs its own delay");f.Tick(start+1500);Require(f.replacement.starts==1&&!b.starts,"Only the new actual arrival may start its own battle"); }
    else if(name=="missing") { f.Arrive();systemObject.battles.erase(101);f.Tick(start+1000);Require(!b.starts&&!p.flagCalls&&f.script.m_DelayedPetBattleStart.empty(),"Removed battle must consume stale pending entry without flags"); }
    else if(name=="finished") { f.Arrive();b.BattleStatus=PETBATTLE_STATUS_FINISHED;f.Tick(start+1000);Require(!b.starts&&!p.flagCalls,"Finished battle must not restart"); }
    else if(name=="running") { b.BattleStatus=PETBATTLE_STATUS_RUNNING;f.Arrive();f.Tick(start+1000);Require(!b.starts&&f.script.m_DelayedPetBattleStart.empty(),"Immediate wild/PvP starts must not be rescheduled"); }
    else if(name=="wrongowner") { b.teams[team].OwnerGuid=77;f.Arrive();f.Tick(start+1000);Require(!b.starts&&f.script.m_DelayedPetBattleStart.empty(),"Foreign battle cannot schedule an arrival"); }
    else if(name=="changedowner") { f.Arrive();b.teams[team].PlayerGuid=77;f.Tick(start+1000);Require(!b.starts&&!p.flagCalls,"Changed participant must fail at dispatch"); }
    else if(name=="dead") { f.Arrive();p.dead=true;f.Tick(start+1000);Require(!b.starts&&!p.flagCalls,"Dead player cannot Begin from a delayed arrival"); }
    else if(name=="deleted") { f.Arrive();p.deleted=true;f.Tick(start+1000);Require(!b.starts&&!p.flagCalls,"Deleting player cannot Begin from a delayed arrival"); }
    else if(name=="outofworld") { f.Arrive();p.world=false;f.Tick(start+1000);Require(!b.starts&&!p.flagCalls,"Removed player cannot Begin from a delayed arrival"); }
    else if(name=="logout") { f.Arrive();f.script.OnBeforeLogout(&p);f.Tick(start+1000);Require(!b.starts&&f.script.m_DelayedPetBattleStart.empty(),"Logout must discard pending arrival before player deletion"); }
    else if(name=="reentrant") { f.Arrive();b.callback=[&](){f.Arrive();f.script.OnUpdate(&p,0);f.script.OnBeforeLogout(&p);};f.Tick(start+1000);Require(b.starts==1&&f.script.m_DelayedPetBattleStart.empty(),"Begin callbacks must run without holding the pending map mutex"); }
    else if(name=="null") { f.script.OnMovementInform(nullptr,POINT_MOTION_TYPE,PETBATTLE_ENTER_MOVE_SPLINE_ID);f.script.OnUpdate(nullptr,0);f.script.OnBeforeLogout(nullptr);Require(f.script.m_DelayedPetBattleStart.empty(),"Null callbacks must be harmless"); }
    else if(name=="shared") { Player other;other.guid=12;other._petBattleId=102;f.replacement.teams[team].OwnerGuid=f.replacement.teams[team].PlayerGuid=12;f.Arrive();clockNow=start+400;f.script.OnMovementInform(&other,POINT_MOTION_TYPE,PETBATTLE_ENTER_MOVE_SPLINE_ID);f.Tick(start+1000);Require(b.starts==1&&!f.replacement.starts,"One player's deadline cannot start another");clockNow=start+1400;f.script.OnUpdate(&other,0);Require(f.replacement.starts==1&&f.script.m_DelayedPetBattleStart.empty(),"Independent player deadline must complete"); }
    else throw std::runtime_error("Unknown arrival case");
    ++count;
}
int main(int argc,char** argv) { try { std::string only=argc>1?argv[1]:"";for(uint32 team:{0u,1u})for(uint32 start:{100u,0xfffffc17u,0xffffff00u})for(auto name:{"early","late","repeat","duplicate","othermotion","replacement","newarrival","missing","finished","running","wrongowner","changedowner","dead","deleted","outofworld","logout","reentrant","null","shared"})if(only.empty()||only==name)Case(team,start,name);Require(count>0,"No arrival cases selected");std::cout<<"PASS exact production arrival class cases="<<count<<"; controlled endpoints; no client traversal claim\n";return 0;}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;} }
