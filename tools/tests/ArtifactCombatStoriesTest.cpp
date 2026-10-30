// Actual native owner/target/206/205 dispatch shared with presentation hosts.
#define main HalakPresentationEntryPoint
#include "HalakPresentationTest.cpp"
#undef main

static void StoryEncounter(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* cause,bool fixed){
 Clear();auto s=Script(rows,entry,cause);Player* owner=GetScriptCreditPlayer(cause);if(owner){owner->criteria=0;owner->lastAsset=0;owner->criteriaInvoker=nullptr;}
 s.ProcessEventsFor(SMART_EVENT_AGGRO,cause);
 bool selected=fixed?owner!=nullptr:IsPlayer(cause);assert(stories.size()==unsigned(selected));if(selected)assert(stories[0].id==(entry==101114?1331u:1667u)&&stories[0].player==owner);
 if(owner){assert(owner->criteria==unsigned(entry==101114));if(entry==101114)assert(owner->lastAsset==49022&&owner->criteriaInvoker==cause);}
 auto count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_AGGRO,cause);assert(s.calls.size()==count&&stories.size()==unsigned(selected));
 s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);assert(!s.errors&&s.calls.size()==count+(entry==101114?1u:3u));
 if(entry==101114)assert(s.calls.back().parameter==206944);else assert(s.calls[count].parameter==209854&&s.calls[count+1].parameter==209855&&s.calls[count+2].action==28&&s.calls[count+2].parameter==209811);
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);assert(s.calls.size()==count+(entry==101114?1u:2u));
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(s.calls.size()==count+1&&!s.errors);
 if(entry==101114){assert(stories.size()==(selected?2u:0u));if(selected)assert(stories.back().id==1336&&stories.back().player==owner);if(owner)assert(owner->criteria==1);}
 else {assert(stories.size()==unsigned(selected)&&s.calls.back().action==45&&s.calls.back().parameter==1);auto const& death=rows[2];assert(death.targetType==19&&death.target.playerDistance.dist==(entry==105799?104832u:106699u));}
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,cause);assert(s.calls.size()==count+(entry==101114?0u:1u)); // native Scion repeated data send retained; receiver once guards unchanged
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);Player owner,other;Creature pet,guardian,charmed,npc,npcOwned;pet.entry=9001;guardian.entry=9002;charmed.entry=9003;npc.entry=9004;npcOwned.entry=9005;pet.owner=guardian.owner=&owner;charmed.charmer=&owner;npcOwned.owner=&npc;
 for(uint32 entry:{101114u,105799u,106700u}){
  auto original=Read(argv[1],entry),fixed=Read(argv[2],entry);assert(original.size()==(entry==101114?4:5)&&fixed.size()==original.size());
  for(Unit* cause:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&npc),static_cast<Unit*>(&npcOwned),static_cast<Unit*>(nullptr)}){StoryEncounter(original,entry,cause,false);StoryEncounter(fixed,entry,cause,true);}
  for(unsigned mode=0;mode<7;++mode){Clear();Player player;Creature trigger;trigger.owner=&player;auto s=Script(fixed,entry,&trigger);if(mode==0)player.world=false;if(mode==1)player.map=&secondMap;if(mode==2)player.phase=false;if(mode==3)player.distance=71;if(mode==4)player.available=false;if(mode==5){s.actor.scope=true;s.actor.scopeAllows=false;}if(mode==6)s.actor.world=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&trigger);assert(stories.empty());}
  Clear();{auto s=Script(fixed,entry,&pet);owner.distance=70;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.size()==1&&stories[0].player==&owner);owner.distance=1;}
  for(unsigned mode=0;mode<3;++mode){Clear();auto s=Script(fixed,entry,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty());}conditions.allow=true;
  Clear();{auto s=Script(fixed,entry,&pet);failConversation=true;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty());failConversation=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty());} // native once after failed creation, no invented recovery
 }
 auto fixed=Read(argv[2],101114);
 for(Unit* killer:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){Clear();owner.criteria=0;auto s=Script(fixed,101114,killer);s.ProcessEventsFor(SMART_EVENT_DEATH,killer);assert(stories.size()==unsigned(GetScriptCreditPlayer(killer)!=nullptr)&&!owner.criteria);if(!stories.empty())assert(stories[0].id==1336&&stories[0].player==&owner);}
 Clear();{owner.criteria=other.criteria=0;Creature peer;peer.owner=&other;auto s=Script(fixed,101114,&pet);s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);s.ProcessEventsFor(SMART_EVENT_DEATH,&peer);assert(stories.size()==2&&stories[0].player==&owner&&stories[1].player==&other&&owner.criteria==1&&!other.criteria);}
 std::puts("PASS real native206 conversations1331/1336/1667 for actual Player/pet/guardian/charm; original raw pet failure, no nearest stranger, scope/world/map/phase/range70/once/missing actor/one-hit death and different group killer covered. Actual native205 still requests49022 exactly once for eligible aggro owner with original invoker, never added on death. Original casts/aura removal and both Azuregos data1 recipients/repeat flags unchanged. Progress storage, world/cast/payload rendering controlled; failed story native once-on-attempt retained; full encounter/client separate.");
 return 0;
}
