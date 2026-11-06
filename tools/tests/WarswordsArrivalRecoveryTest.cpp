#include <cassert>
#include <cstdint>
#include <vector>
#include <set>
#include <memory>
#include <algorithm>
#include <iostream>
using uint32=uint32_t; using uint64=uint64_t;
enum {CLASS_WARRIOR=1,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,UNIT_STATE_ROOT=1};
enum SpellCastResult {SPELL_CAST_OK,FAILED};
using TriggerCastFlags=uint32;constexpr uint32 TRIGGERED_FULL_MASK=0xffff,TRIGGERED_DONT_REPORT_CAST_ERROR=0x1000;
constexpr float INVALID_HEIGHT=-100000;
struct ObjectGuid {uint64 high=1,low=1;bool operator<(ObjectGuid const&o)const{return high<o.high||(high==o.high&&low<o.low);}bool operator==(ObjectGuid const&o)const{return high==o.high&&low==o.low;}};
#include "Scope.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) ++warnings
struct Session{bool loading=false,logout=false;bool PlayerLoading(){return loading;}bool PlayerLogout(){return logout;}};
struct Script{uint32 ready=0;void SetData(uint32 id,uint32 value){assert(id==0x574152&&value==1);++ready;}};
struct Map{bool unloading=false;float floor=50;bool IsMapUnload(){return unloading;}float GetHeight(std::set<uint32>const&,float,float,float,bool,float){return floor;}};
struct Scenario{uint32 id=944,step=0;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}} scenario;
struct Manager{Scenario* GetScenario(uint32){return &scenario;}}manager;auto sScenarioMgr=&manager;
struct Spline{struct Vector{float x=0,y=0,z=0;} destination;uint32 id=1;bool done=true;uint32 GetId(){return id;}bool Finalized(){return done;}Vector FinalDestination(){return destination;}};
struct Player;
struct SpellCastTargets{Player* caster=nullptr;Player* target=nullptr;float x=0,y=0,z=0;void SetCaster(Player*p){caster=p;}void SetUnitTarget(Player*p){target=p;}void SetDst(float a,float b,float c,float){x=a;y=b;z=c;}};
struct SpellInfo{};struct SpellMgr{SpellInfo info;SpellInfo const*GetSpellInfo(uint32 id){assert(id==205744);return &info;}}spellMgr;auto sSpellMgr=&spellMgr;
struct Player{
 Session session;Session* m_session=&session;Map worldMap;Map* map=&worldMap;Script script;Script* attached=&script;
 ObjectGuid guid;std::shared_ptr<QuestObjectScope> scope=std::make_shared<QuestObjectScope>();std::set<uint32> phases{1},auras;
 bool world=true,alive=true,changing=false,deleted=false,teleport=false,rewarded=false,root=false,protectedArrival=false,fly=false,levitate=false,vehicle=false,transport=false;
 bool moveFails=false,finishFails=false,finishNoMotion=false,finishKeepsAura=false,presentationFails=false;
 uint32 klass=1,mapId=1511,instance=42,status=3,holds=0,moveRequests=0,jumps=0,finishes=0,conversations=0,presentations=0;
 float x=3349.74f,y=1355.64f,z=182.79f;Spline spline;Spline* movespline=&spline;
 bool IsDelete(){return deleted;}bool IsChangeMap(){return changing;}bool IsInWorld(){return world;}bool CanContact();bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}
 Map* GetMap(){return map;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}Script* GetInstanceScript(){return attached;}uint32 getClass(){return klass;}
 bool GetQuestRewardStatus(uint32 q){assert(q==40043);return rewarded;}uint32 GetQuestStatus(uint32 q){assert(q==40043);return status;}ObjectGuid GetGUID(){return guid;}
 std::shared_ptr<QuestCancellationReceipt const> GetQuestCancellationReceipt(uint32 q){assert(q==40043);return scope->CancellationReceipt();}
 float GetPositionX(){return x;}float GetPositionY(){return y;}float GetPositionZ(){return z;}float GetOrientation(){return 0.7f;}
 std::set<uint32> const& GetPhases(){return phases;}void* GetTransport(){return transport?this:nullptr;}void* GetVehicle(){return vehicle?this:nullptr;}bool CanFly(){return fly;}bool IsLevitating(){return levitate;}
 void BeginMapObjectArrival(bool notify){assert(notify==world);++holds;root=protectedArrival=true;}bool IsMapObjectArrivalProtected(){return protectedArrival;}bool HasUnitState(uint32){return root;}
 bool HasAura(uint32 id){return auras.count(id)!=0;}SpellCastResult CastSpell(Player*p,uint32 id,bool trigger){assert(p==this&&trigger&&(id==205534||id==189695));++presentations;if(presentationFails)return FAILED;auras.insert(id);return SPELL_CAST_OK;}
 Player* GetMotionMaster(){return this;}void MoveJump(float a,float b,float c,float o,float speed,float vz){assert(a==3386.46f&&b==1406.94f&&c==68.94f&&o==0.7f&&speed==3&&vz==3);++moveRequests;if(moveFails)return;++jumps;++spline.id;spline.done=false;spline.destination={a,b,c};}
 SpellCastResult CastSpell(float a,float b,float c,uint32 id,bool trigger){assert(id==205744&&trigger&&a==3386.46f&&b==1406.94f&&c==68.94f);++finishes;if(finishFails)return FAILED;if(!finishNoMotion){++spline.id;spline.done=false;spline.destination={a,b,c};}if(!finishKeepsAura)auras.erase(189695);return SPELL_CAST_OK;}
 SpellCastResult CastSpell(SpellCastTargets const&t,SpellInfo const* info,void*,TriggerCastFlags flags){assert(t.caster==this&&t.target==this&&info==&spellMgr.info&&flags==(TRIGGERED_FULL_MASK&~TRIGGERED_DONT_REPORT_CAST_ERROR));return CastSpell(t.x,t.y,t.z,205744,true);}
 void CreateConversation(uint32 id){assert(id==838);++conversations;}void ReleaseHold(){root=protectedArrival=false;}
};
#include "Contact.inc"
struct PlayerScript{explicit PlayerScript(char const*){}virtual void OnBeforeLogout(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}virtual void OnUpdate(Player*,uint32){}};
#include "Recovery.inc"
player_warswords_arrival_recovery observer;
struct Case{Player p;Case(){scenario={};warnings=0;WarswordsArrivalRecovery::entries.clear();}void Enter(){WarswordsArrivalRecovery::Enter(&p);}void Tick(uint32 diff){observer.OnUpdate(&p,diff);observer.OnUpdate(&p,0);}void Start(){Enter();p.ReleaseHold();Tick(3000);assert(p.jumps==1);}bool Pending(){return WarswordsArrivalRecovery::entries.count(p.guid)!=0;}};
int main(){uint32 cases=0;
 {Case c;c.p.world=false;c.p.session.loading=true;c.Enter();assert(c.p.holds==1&&c.p.root&&c.p.protectedArrival);c.p.world=true;c.Tick(3000);c.Tick(12000);assert(!c.p.CanContact()&&!c.p.jumps&&c.Pending());c.p.session.loading=false;c.Tick(1);assert(!c.p.jumps);c.p.ReleaseHold();c.Tick(1);assert(c.p.jumps==1&&c.p.presentations==2);c.Tick(4999);assert(!c.p.finishes);c.Tick(1);assert(c.p.finishes==1&&c.p.conversations==1&&!c.Pending());c.Tick(30000);assert(c.p.finishes==1);++cases;}
 for(uint32 invalid=0;invalid<11;++invalid){Case c;switch(invalid){case 0:c.p.map=nullptr;break;case 1:c.p.mapId=1220;break;case 2:c.p.worldMap.unloading=true;break;case 3:c.p.attached=nullptr;break;case 4:c.p.klass=3;break;case 5:c.p.rewarded=true;break;case 6:c.p.status=0;break;case 7:c.p.status=5;break;case 8:scenario.id=973;break;case 9:c.p.status=6;break;case 10:c.p.status=2;break;}c.Enter();c.Tick(3000);assert(!c.Pending()&&!c.p.jumps&&!c.p.holds);++cases;}
 for(uint32 invalid=0;invalid<11;++invalid){Case c;c.Enter();switch(invalid){case 0:c.p.world=false;break;case 1:c.p.session.loading=true;break;case 2:c.p.changing=true;break;case 3:c.p.deleted=true;break;case 4:c.p.session.logout=true;break;case 5:c.p.teleport=true;break;case 6:c.p.alive=false;break;case 7:c.p.root=true;break;case 8:c.p.vehicle=true;break;case 9:c.p.transport=true;break;case 10:c.p.m_session=nullptr;break;}c.Tick(3000);assert(!c.p.jumps&&!c.p.finishes&&!c.p.conversations);++cases;}
 for(uint32 grounded=0;grounded<8;++grounded){Case c;switch(grounded){case 0:c.p.z=68.94f;break;case 1:c.p.worldMap.floor=c.p.z;break;case 2:c.p.x=3400;c.p.worldMap.floor=c.p.z;break;case 3:c.p.x=3500;break;case 4:c.p.y=1600;break;case 5:c.p.fly=true;break;case 6:c.p.levitate=true;break;case 7:c.p.z=220;break;}c.Enter();c.Tick(3000);assert(!c.p.holds&&!c.p.jumps&&!c.Pending()&&c.p.script.ready);++cases;}
 {Case c;c.Enter();c.p.ReleaseHold();c.p.moveFails=true;c.Tick(3000);assert(c.Pending()&&!c.p.jumps&&warnings==1);c.Tick(500);assert(c.p.moveRequests==1);c.p.moveFails=false;c.Tick(500);assert(c.p.jumps==1&&c.p.presentations==2);c.Tick(5000);assert(c.p.conversations==1);++cases;}
 for(uint32 fault=0;fault<3;++fault){Case c;c.Start();if(fault==0)c.p.finishFails=true;if(fault==1)c.p.finishNoMotion=true;if(fault==2)c.p.finishKeepsAura=true;c.Tick(5000);if(fault)c.Tick(2000);assert(c.Pending()&&!c.p.conversations&&warnings==1);c.p.finishFails=c.p.finishNoMotion=c.p.finishKeepsAura=false;c.Tick(1000);assert(c.p.conversations==1&&!c.Pending());++cases;}
 {Case c;c.Start();c.p.finishNoMotion=true;c.Tick(5000);assert(c.Pending()&&!c.p.conversations);++c.p.spline.id;c.p.spline.destination={3386.46f,1406.94f,68.94f};c.p.auras.erase(189695);c.Tick(1);assert(c.p.conversations==1&&!c.Pending()&&!warnings);++cases;}
 {Case c;c.Start();c.p.spline.done=true;c.p.x=3386.46f;c.p.y=1406.94f;c.p.z=68.94f;c.p.finishNoMotion=true;c.Tick(5000);assert(c.p.conversations==1&&!c.Pending()&&!warnings);++cases;}
 for(uint32 stale=0;stale<5;++stale){Case c;c.Start();switch(stale){case 0:++c.p.spline.id;break;case 1:scenario.step=1;break;case 2:c.p.instance=43;break;case 3:c.p.scope->Cancel();c.p.scope=std::make_shared<QuestObjectScope>();break;case 4:c.p.rewarded=true;break;}c.Tick(5000);assert(!c.Pending()&&!c.p.finishes&&!c.p.conversations);++cases;}
 for(uint32 leave=0;leave<4;++leave){Case c;c.Enter();if(leave==0)observer.OnBeforeLogout(&c.p);if(leave==1)observer.OnLogout(&c.p);if(leave==2){c.p.mapId=1220;observer.OnMapChanged(&c.p);}if(leave==3)WarswordsArrivalRecovery::Forget(&c.p);c.Tick(30000);assert(!c.Pending()&&!c.p.jumps);++cases;}
 {Case c;c.Enter();observer.OnMapChanged(&c.p);assert(c.Pending());c.p.ReleaseHold();c.Tick(3000);assert(c.p.jumps==1);++cases;}
 {Case c;c.p.alive=false;c.Enter();c.Tick(30000);assert(!c.p.holds&&!c.p.jumps);c.p.alive=true;c.Tick(1);assert(c.p.holds==1&&!c.p.jumps);c.p.ReleaseHold();c.Tick(1);assert(c.p.jumps==1);++cases;}
 {Case c;c.Enter();auto old=WarswordsArrivalRecovery::entries.at(c.p.guid);c.Tick(1000);WarswordsArrivalRecovery::Forget(&c.p);c.Enter();assert(old!=WarswordsArrivalRecovery::entries.at(c.p.guid));c.p.ReleaseHold();c.Tick(2000);assert(!c.p.jumps);c.Tick(1000);assert(c.p.jumps==1);++cases;}
 {Case c;Player other;other.guid.high=2;other.worldMap.floor=other.z;c.Enter();WarswordsArrivalRecovery::Enter(&other);observer.OnBeforeLogout(&other);assert(c.Pending()&&WarswordsArrivalRecovery::entries.size()==1);++cases;}
 std::cout<<"PASS "<<cases<<" ASAN actual Fury arrival state/native CanContact/original quest receipt: pre-create bounded hold endpoint, loading longer than3s retains entry, per-entry3s+5s clocks, real spline/finish ACK, grounded/flying/foreign rejection, cast/motion retry, external movement, death/revive, fullGUID, cancellation, logout/re-entry. Map/packet/spell/spline endpoints controlled; actual native/core/client tests separate\n";
}
