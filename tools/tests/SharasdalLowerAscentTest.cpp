#include "AscentHost.inc"
#include "WaypointData.inc"
struct Waypoints{WaypointPath path;bool available=true;WaypointPath const*GetPathScript(uint32 id){assert(id==10993810);return available?&path:nullptr;}}waypoints;auto*sWaypointMgr=&waypoints;
struct AscentController:InstanceScript{
 ObjectGuid lowerGrash;
#include "sharasdal_lower_ascent.h"
 void Update(uint32 diff){UpdateAscent(diff);}void UpdateForScript(uint32){}
};
AscentController* activeAscent=nullptr;
void InstanceScript::SetGuidData(uint32 id,ObjectGuid guid){if(activeAscent)activeAscent->SetGuidData(id,guid);}
ObjectGuid InstanceScript::GetGuidData(uint32 id)const{return activeAscent?activeAscent->GetGuidData(id):ObjectGuid::Empty;}
SpellCastResult Player::CastSpell(Unit*target,uint32 id,bool triggered){assert(target==this&&id==208580&&!triggered);++cameraCalls;if(cameraResult==SPELL_CAST_OK&&makeCamera)auras.insert(id);if(onCamera)onCamera();return cameraResult;}
void Player::UpdateAchievementCriteria(uint32 type,uint32 asset,uint64 count){assert((type==73||type==92)&&asset==50336&&count==1);ascentEvents.push_back(type);if(type==92&&scenario.step==3){scenario.achievements.progress[47131].Counter=1;if(scenario.achievements.progress[47378].Counter)scenario.step=4;}}
struct InstanceMap:Map{AscentController*i_data=nullptr;void Update(uint32);};
#include "InstanceTick.inc"
enum{UNIT_STATE_ROAMING_MOVE=128};
bool roll_chance_i(uint32){assert(false);return false;}inline int sEventScripts=0,sWaypointScripts=0;
struct NativeWaypoint{
 WaypointPath const*_path=&waypoints.path;uint32&_currentNode;struct Wait{uint32 value=0;void Reset(uint32 n){value=n;}}_nextMoveTime;
 explicit NativeWaypoint(Creature&actor):_currentNode(actor.motion.path._currentNode){}
 void OnArrived(Creature&);void MovementInform(Creature&);
};
#include "WaypointMovementInform.inc"
#include "WaypointOnArrived.inc"
using WorldObject=Unit;using ZoneScript=InstanceScript;using SpellEffIndex=uint32;
enum{SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,CRITERIA_TYPE_SCRIPT_EVENT=73,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct NativeExitEvent{
 struct Effect{uint32 MiscValue=50336;uint32 GetProvidedTargetMask()const{return 1;}}effect;
 struct Info{uint32 Id=209992;Effect*value;Effect*GetEffect(uint32,uint32){return value;}}info{209992,&effect};
 Player*m_caster;Unit*unitTarget;Unit*gameObjTarget=nullptr,*focusObject=nullptr;uint32 effectHandleMode=1,m_diffMode=0;Info*m_spellInfo=&info;
 explicit NativeExitEvent(Player*p):m_caster(p),unitTarget(p){}void EffectSendEvent(SpellEffIndex);
};
#include "NativeAscentEvent.inc"
struct LowerRun:Run{
 AscentController controller;InstanceMap tick;int dx=0,dy=0;
 LowerRun(){liveMap.id=1600;scenario.achievements.progress.clear();scenario.step=actor.instance.step=2;scenario.achievements.progress[47127].Counter=1;waypoints.available=true;
  actor.db=362063;actor.z=258;actor.x=-374;actor.y=807;actor.m_waypointID=10;grash.db=362066;grash.z=245;adel.db=362078;adel.z=258;
  player.quest.ObjectiveData[0]=1;second.quest.ObjectiveData[0]=1;controller.lowerGrash=grash.guid;controller.lowerErunak=actor.guid;activeAscent=&controller;tick.i_data=&controller;
 }
 ~LowerRun(){activeAscent=nullptr;}
 void Tick(uint32 diff=0,std::function<void()> move={}){tick.onUpdate=[&](uint32 d){actor.Lifecycle(d);actor.functions.Update(d);player.functions.Update(d);if(move)move();};tick.Update(diff);tick.onUpdate={};}
 bool Bind(){controller.SetGuidData(208580,player.guid);return controller.GetGuidData(208580)==player.guid;}
 void Issue(){actor.motion.MovePath(10993810,false,dx,dy);actor.motion.path._currentNode=0;actor.finalized=false;controller.SetGuidData(10993810,actor.guid);}
 void Begin(){assert(Bind());Issue();assert(controller.ascentIssued&&!controller.ascentArrived);}
 void Node(uint32 node,bool remove=true,uint32 diff=50){Tick(diff,[&]{auto const&p=waypoints.path.at(node);actor.x=p.x+dx;actor.y=p.y+dy;actor.z=p.z;actor.motion.path._currentNode=node;actor.finalized=true;NativeWaypoint(actor).OnArrived(actor);if(node==5&&remove)actor.motion.path.id=0;});}
 void Reach(bool remove=true){for(uint32 i=0;i<6;++i)Node(i,remove);}
 void Victory(){scenario.achievements.progress[47330].Counter=1;if(scenario.achievements.progress[47127].Counter)scenario.step=actor.instance.step=3;}
 void Finish(Player*owner=nullptr){auto p=owner?owner:&player;p->auras.erase(208580);NativeExitEvent(p).EffectSendEvent(2);Tick();}
};
int main(int argc,char**argv){_set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==2);std::ifstream input(argv[1]);std::string line;
 while(std::getline(input,line)){std::istringstream row(line);WaypointData p;uint32 chance;assert(row>>p.id>>p.x>>p.y>>p.z>>p.orientation>>p.move_type>>p.speed>>p.delay>>p.event_id>>chance);assert(!p.event_id);p.event_chance=uint8(chance);waypoints.path.push_back(p);}assert(waypoints.path.size()==6);
 unsigned cases=0;
 for(uint32 status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(bool early:{false,true})for(uint32 mode=0;mode<4;++mode){LowerRun r;r.player.quest.Status=status;r.Begin();
  if(mode){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==1?&r.temporary:mode==2?nullptr:r.actor.i_disabledAI;}
  if(early)r.Victory();r.Reach();assert(r.controller.ascentArrived);if(!early){assert(!r.player.cameraCalls);r.Tick(60000);assert(!r.player.cameraCalls);r.Victory();r.Tick();}
  assert(r.player.cameraCalls==1&&r.player.HasAura(208580)&&r.player.ascentEvents.empty()&&r.controller.ascentBound);r.Tick(10000);assert(r.player.cameraCalls==1);r.Finish();assert(r.player.ascentEvents==std::vector<uint32>({73,92})&&!r.controller.ascentBound);r.Tick(10000);assert(r.player.cameraCalls==1&&r.actor.motion.paths==1);++cases;
 }
 for(int offset:{-5,0,5}){LowerRun r;r.dx=offset;r.dy=-offset;r.Begin();r.Reach();r.Victory();r.Tick();assert(r.player.cameraCalls==1);++cases;}
 for(uint32 denied=0;denied<17;++denied){LowerRun r;switch(denied){case 0:r.player.gm=true;break;case 1:r.player.alive=false;break;case 2:r.player.contact=false;break;case 3:r.player.teleport=true;break;case 4:r.player.classId=1;break;case 5:r.player.map=&otherMap;break;case 6:r.player.phase=false;break;case 7:r.player.detect=false;break;case 8:r.player.los=false;break;case 9:r.player.close=false;break;case 10:r.player.record=false;break;case 11:r.player.rewarded=true;break;case 12:r.player.quest.ObjectiveData[0]=0;break;case 13:r.grash.db=362092;break;case 14:scenario.step=3;break;case 15:scenario.id=1;break;case 16:scenarioManager.exists=false;break;}assert(!r.Bind()&&!r.controller.ascentBound&&!r.player.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.controller.SetGuidData(208580,r.second.guid);assert(!r.controller.ascentBindAccepted&&r.controller.ascentRecipient==r.player.guid);r.Reach();r.Victory();r.Tick();assert(r.player.cameraCalls==1&&!r.second.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.actor.motion.path.id=10993809;r.actor.motion.path._currentNode=5;r.actor.finalized=true;r.controller.SetGuidData(50336,r.actor.guid);r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.actor.motion.path._currentNode=5;r.actor.finalized=false;r.controller.SetGuidData(50336,r.actor.guid);r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.actor.motion.path._currentNode=5;r.actor.finalized=true;r.actor.x=0;r.controller.SetGuidData(50336,r.actor.guid);r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 for(uint32 reason=0;reason<3;++reason){LowerRun r;r.Begin();r.Reach();r.Victory();if(reason==0)r.player.cameraResult=SPELL_FAILED_ERROR;if(reason==1)r.player.makeCamera=false;if(reason==2)r.player.near=false;r.Tick();for(int i=1;i<30;++i)r.Tick(1000);assert(r.controller.ascentExhausted&&warnings==1&&r.player.ascentEvents.empty());auto calls=r.player.cameraCalls;r.Tick(10000);assert(r.player.cameraCalls==calls);r.player.cameraResult=SPELL_CAST_OK;r.player.makeCamera=true;r.controller.RearmAscent(&r.second);r.Tick();assert(r.player.cameraCalls==calls);r.player.near=true;r.controller.RearmAscent(&r.player);r.Tick();assert(r.player.HasAura(208580)&&r.player.ascentEvents.empty());++cases;}
 {LowerRun r;r.Begin();r.Reach();r.Victory();r.player.near=false;r.Tick();r.player.near=true;r.Tick(1000);assert(r.player.cameraCalls==1&&!r.second.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.Reach();r.Victory();r.player.onCamera=[&]{r.controller.UpdateAscent(50000);r.controller.SetGuidData(208580,r.second.guid);};r.Tick();assert(r.player.cameraCalls==1&&r.controller.ascentRecipient==r.player.guid);++cases;}
 {LowerRun r;r.Begin();r.Reach();r.player.quest.ScriptLifetime.reset();r.player.record=false;r.Victory();r.Tick();assert(!r.player.cameraCalls&&!r.controller.ascentBound);++cases;}
 {LowerRun r;r.Begin();auto held=r.player.quest.ScriptLifetime;r.Reach();r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Victory();r.Tick();assert(!r.player.cameraCalls&&!r.controller.ascentBound);++cases;}
 {LowerRun r;r.Begin();ObjectAccessor::players={&r.second};r.player.quest.ScriptLifetime.reset();r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=nullptr;r.Reach();r.Victory();r.Tick(60000);assert(r.controller.ascentArrived&&r.controller.ascentBound&&!r.player.cameraCalls&&!r.second.cameraCalls);Player reconnect;reconnect.guid=r.player.guid;reconnect.quest.ObjectiveData[0]=1;ObjectAccessor::players={&reconnect};r.controller.RearmAscent(&reconnect);reconnect.contact=false;r.Tick();assert(r.controller.ascentBound&&!reconnect.cameraCalls);reconnect.contact=true;r.Tick();assert(reconnect.cameraCalls==1);r.Finish(&reconnect);assert(!r.controller.ascentBound);++cases;}
 {LowerRun r;r.Begin();r.Reach();r.player.quest.ScriptLifetime.reset();r.player.quest.ObjectiveData[0]=0;r.controller.RearmAscent(&r.player);r.Victory();r.Tick();assert(!r.player.cameraCalls&&!r.controller.ascentBound&&!r.player.calls);++cases;}
 {LowerRun r;r.Begin();r.Reach();r.Victory();r.Tick();r.player.alive=false;r.player.auras.erase(208580);r.Tick();assert(r.player.cameraCalls==1);r.player.alive=true;r.Tick(1000);assert(r.player.cameraCalls==2&&r.player.HasAura(208580)&&r.player.ascentEvents.empty());++cases;}
 {LowerRun r;r.Begin();r.Reach();r.Victory();r.player.auras.insert(208580);r.Tick();assert(!r.player.cameraCalls&&r.controller.ascentScene);r.Finish();assert(!r.controller.ascentBound);++cases;}
 {LowerRun r;r.Begin();r.Reach();r.Victory();scenario.step=r.actor.instance.step=4;r.Tick();assert(!r.player.cameraCalls&&!r.controller.ascentBound);++cases;}
 for(uint32 denied=0;denied<12;++denied){LowerRun r;r.Begin();r.Reach();r.Victory();switch(denied){
  case 0:r.actor.alive=false;break;case 1:r.actor.world=false;break;case 2:r.actor.preDelete=true;break;case 3:r.actor.db=362093;break;
  case 4:r.player.phase=false;break;case 5:r.player.detect=false;break;case 6:r.player.los=false;break;case 7:r.player.alive=false;break;
  case 8:r.player.gm=true;break;case 9:r.player.teleport=true;break;case 10:r.player.map=&otherMap;break;case 11:r.player.rewarded=true;break;
 }r.Tick();assert(!r.player.cameraCalls&&r.player.ascentEvents.empty()&&!r.second.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.actor.cosmeticWorks=false;r.Reach();r.Victory();r.Tick();assert(r.player.cameraCalls==1&&r.player.HasAura(208580)&&r.player.ascentEvents.empty());++cases;}
 {LowerRun r;r.Begin();r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=nullptr;r.Node(0);r.Node(1);assert(r.controller.ascentProgress);r.actor.motion.path.id=10993809;r.Tick();assert(!r.controller.ascentIssued);r.actor.m_waypointID=5;r.actor.motion.path.id=0;auto const&p=waypoints.path.back();r.actor.x=p.x;r.actor.y=p.y;r.actor.z=p.z;r.actor.finalized=true;r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 {LowerRun r;r.Begin();r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=nullptr;auto const&p=waypoints.path.back();r.actor.x=p.x;r.actor.y=p.y;r.actor.z=p.z;r.actor.finalized=true;r.actor.m_waypointID=5;r.actor.motion.path.id=0;r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 {LowerRun r;r.Begin();waypoints.available=false;r.Reach();r.Victory();r.Tick();assert(!r.controller.ascentArrived&&!r.player.cameraCalls);++cases;}
 {LowerRun r;assert(r.Bind());r.actor.motion.path.id=10993809;r.controller.SetGuidData(10993810,r.actor.guid);assert(!r.controller.ascentIssued);r.actor.motion.MovePath(10993810,false);r.actor.motion.path._currentNode=1;r.controller.SetGuidData(10993810,r.actor.guid);assert(!r.controller.ascentIssued);++cases;}
 {LowerRun r;r.Begin();ObjectGuid alias=r.actor.guid;alias.high+=1;r.actor.motion.path._currentNode=5;r.actor.finalized=true;r.controller.SetGuidData(50336,alias);assert(!r.controller.ascentArrived);++cases;}
 {LowerRun r;r.Begin();r.Reach(false);r.Victory();r.Tick();for(int i=0;i<10;++i){r.controller.SetGuidData(50336,r.actor.guid);r.Tick(1000);}assert(r.player.cameraCalls==1&&r.player.ascentEvents.empty());r.Finish();r.controller.SetGuidData(50336,r.actor.guid);r.Tick();assert(r.player.cameraCalls==1&&r.player.ascentEvents.size()==2&&!r.controller.ascentBound);++cases;}
 std::cout<<"PASS "<<cases<<" current full Erunak/instance-owned actual ascent header/native OnArrived+zero-based dispatcher+InstanceMap scheduler/native quest lease+objective reader and native61 event contexts: early/late Kraliss, current/suspended/null/alias content, installed route/real ending/cached native marker after final removal, original full GUID/40341 witness and login re-entry, finite cast refusal/missing aura/absent visitor, abandonment/replacement/death/re-entry and actual50336 acknowledgement. Movement installation/spline/registry/criteria engine/scene/teleport services controlled; no actual camera/path traversal/full1066 claim.\n";
}
