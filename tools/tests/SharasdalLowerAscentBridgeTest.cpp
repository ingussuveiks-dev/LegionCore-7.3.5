#include "NativeHost.inc"
std::function<void(uint32,ObjectGuid)> onMessage;
ObjectGuid acknowledged;
void InstanceScript::SetGuidData(uint32 id,ObjectGuid guid){assert(onMessage);onMessage(id,guid);}
ObjectGuid InstanceScript::GetGuidData(uint32 id)const{assert(id==208580);return acknowledged;}
#include "Policy.inc"
#include "Actor.inc"
using Grash=npc_grash::npc_grashAI;
struct Run{
 Player owner,neighbor;Creature actor,erunak,adel;CreatureAI content,temporary;GameObject wall;
 std::unique_ptr<Grash> ai;npc_grash script;std::vector<uint32> messages;bool accept=true;
 Run(){scenario=Scenario{};scenario.step=2;manager.active=&scenario;warnings=0;paths.missing.clear();acknowledged={};
  owner.guid={1,1};neighbor.guid={1,2};ObjectAccessor::players={&owner,&neighbor};
  actor.guid={2,362066};actor.dbGuid=362066;actor.health=1000;actor.wall=&wall;actor.erunak=&erunak;actor.adel=&adel;liveMap.lower=&actor;
  erunak.guid={2,362063};erunak.dbGuid=362063;erunak.entry=102826;erunak.primary=&content;
  adel.guid={2,362078};adel.dbGuid=362078;adel.entry=104853;
  ai=std::make_unique<Grash>(&actor);
  onMessage=[&](uint32 id,ObjectGuid guid){messages.push_back(id);
   if(id==208580){assert(guid==owner.guid&&!(ai->_gossipActions&4)&&owner.talk.menu.id==19483&&!owner.talk.menu.items.empty()&&!owner.closed&&actor.flags&&actor.motion.paths.empty());if(accept)acknowledged=guid;}
   else{assert(id==10993810&&guid==erunak.guid&&erunak.motion.paths==std::vector<uint32>{10993810});}
  };
 }
 ~Run(){onMessage={};liveMap.lower=nullptr;ObjectAccessor::players.clear();}
 void Offer(){script.OnGossipHello(&owner,&actor);}
 void Select(){Offer();ai->sGossipSelect(&owner,19483,0);}
};
int main(){unsigned cases=0;
 for(uint32 status:{1u,3u})for(uint32 mode=0;mode<5;++mode){Run r;r.owner.quest.Status=status;
  if(mode){r.erunak.i_disabledAI=mode==4?nullptr:&r.content;r.erunak.primary=mode==1?&r.temporary:mode==2?nullptr:mode==3?&r.content:nullptr;}
  r.Select();assert(r.messages==std::vector<uint32>{208580}&&r.owner.closed==1&&(r.ai->_gossipActions&4));r.ai->UpdateAI(999);assert(r.erunak.motion.paths.empty());r.ai->UpdateAI(1);
  assert(r.messages==std::vector<uint32>({208580,10993810})&&r.actor.motion.paths==std::vector<uint32>{10993810}&&r.adel.motion.paths==std::vector<uint32>{10993810});
  assert(r.content.speech==(mode==4?std::vector<uint32>{}:std::vector<uint32>{9})&&r.temporary.speech.empty());
  r.ai->sGossipSelect(&r.owner,19483,0);r.ai->UpdateAI(5000);assert(r.messages.size()==2&&r.erunak.motion.paths.size()==1&&r.owner.credits.empty()&&r.neighbor.credits.empty());++cases;
 }
 {Run r;r.accept=false;r.Select();assert(r.messages==std::vector<uint32>{208580}&&!r.owner.closed&&r.owner.talk.menu.id==19483&&r.actor.flags&&!(r.ai->_gossipActions&4));r.ai->UpdateAI(5000);assert(r.actor.motion.paths.empty()&&r.erunak.motion.paths.empty());r.accept=true;r.ai->sGossipSelect(&r.owner,19483,0);r.ai->UpdateAI(1000);assert(r.messages==std::vector<uint32>({208580,208580,10993810}));++cases;}
 {Run r;r.actor.erunak=nullptr;r.Select();r.ai->UpdateAI(1000);assert(r.messages==std::vector<uint32>{208580}&&r.actor.motion.paths.size()==1&&r.adel.motion.paths.size()==1);++cases;}
 for(uint32 denial=0;denial<5;++denial){Run r;r.Offer();switch(denial){case 0:r.owner.talk.menu.id=1;break;case 1:r.owner.talk.menu.items[0].IsCoded=true;break;case 2:r.owner.interact=false;break;case 3:r.actor.dbGuid=362092;break;case 4:paths.missing.insert(10993810);break;}r.ai->sGossipSelect(&r.owner,19483,0);r.ai->UpdateAI(1000);assert(r.messages.empty()&&r.actor.motion.paths.empty()&&!r.owner.closed);++cases;}
 for(uint32 mode=0;mode<5;++mode){Run r;scenario.step=1;
  if(mode){r.erunak.i_disabledAI=mode==4?nullptr:&r.content;r.erunak.primary=mode==1?&r.temporary:mode==2?nullptr:mode==3?&r.content:nullptr;}
  r.ai->DoAction(1);r.ai->UpdateAI(1000);assert(r.actor.motion.paths==std::vector<uint32>{10993808}&&r.erunak.motion.paths==std::vector<uint32>{10993809}&&r.messages.empty());assert(r.content.speech==(mode==4?std::vector<uint32>{}:std::vector<uint32>{2})&&r.temporary.speech.empty());++cases;
 }
 std::cout<<"PASS "<<cases<<" current whole Grash raw step2 gossip binds original full GUID before menu/action consumption; refusal retains retry, actual installed Erunak path precedes issue message, absent actor cannot acknowledge, duplicate gossip cannot replay; original/suspended/null speech2/9 guards. Instance admission/controller, movement installation and geometry are controlled services here and covered separately; no client traversal claim.\n";
}
