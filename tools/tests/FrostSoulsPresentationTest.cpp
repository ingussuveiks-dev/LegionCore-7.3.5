// Reuse the native extracted dispatcher/owner/conversation/TALK host. Its Halak
// entry point remains separately tested; these cases use real three-soul rows.
#define main HalakPresentationEntryPoint
#include "HalakPresentationTest.cpp"
#undef main

static uint32 StoryFor(uint32 entry){return entry==93948?740:(entry==93950?742:743);}
static void Soul(std::vector<SmartScriptHolder> const& rows,uint32 entry,Unit* invoker,bool fixed){
 Clear();auto s=Script(rows,entry,invoker);s.ProcessEventsFor(SMART_EVENT_AGGRO,invoker);
 Player* recipient=fixed?GetScriptCreditPlayer(invoker):(invoker?invoker->ToPlayer():nullptr);
 assert(stories.size()==unsigned(recipient!=nullptr));if(recipient)assert(stories[0].id==StoryFor(entry)&&stories[0].player==recipient);
 bool own=fixed||!IsCreature(invoker);assert(chat.requests.size()==unsigned(own));if(own)assert(chat.requests[0].speaker==entry&&chat.requests[0].group==0);
 auto count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_AGGRO,invoker);assert(s.calls.size()==count);
 s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);uint32 casts=entry==93948?3:2;assert(s.calls.size()==count+casts+(entry==93948?1:0)&&!s.errors);
 if(entry==93948){assert(chat.requests.size()==(own?2u:0u));if(own)assert(chat.requests.back().speaker==entry&&chat.requests.back().group==1);assert(s.calls[count].parameter==81140&&s.calls[count+1].parameter==192647&&s.calls[count+2].parameter==19135&&s.calls[count+3].action==1);}
 if(entry==93950)assert(s.calls[count].parameter==200671&&s.calls[count+1].parameter==200679);
 if(entry==93951)assert(s.calls[count].parameter==79971&&s.calls[count+1].parameter==79970);
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);assert(s.calls.size()==count+2&&!s.errors); // ordinary repeats, Baelgun linked once line does not repeat
 s.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(chat.requests.size()==(own?(entry==93948?3u:2u):0u));if(own)assert(chat.requests.back().speaker==entry&&chat.requests.back().group==(entry==93948?2u:1u));
 count=s.calls.size();s.ProcessEventsFor(SMART_EVENT_DEATH,invoker);assert(s.calls.size()==count&&!s.errors);
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);Player owner;Creature pet,guardian,charmed,npc,npcOwned;pet.entry=9001;guardian.entry=9002;charmed.entry=9003;npc.entry=9004;npcOwned.entry=9005;pet.owner=guardian.owner=&owner;charmed.charmer=&owner;npcOwned.owner=&npc;
 for(uint32 entry:{93948u,93950u,93951u}){
  auto old=Read(argv[1],entry),fixed=Read(argv[2],entry);assert(old.size()==(entry==93948?8:5)&&fixed.size()==old.size());
  for(Unit* actor:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&npc),static_cast<Unit*>(&npcOwned),static_cast<Unit*>(nullptr)}){Soul(old,entry,actor,false);Soul(fixed,entry,actor,true);}
  for(Unit* killer:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&npc),static_cast<Unit*>(nullptr)}){Clear();auto s=Script(fixed,entry,killer);s.ProcessEventsFor(SMART_EVENT_DEATH,killer);assert(stories.empty()&&s.calls.size()==1&&chat.requests.size()==1&&chat.requests[0].speaker==entry);}
  for(unsigned mode=0;mode<7;++mode){Clear();Player eligible;Creature cause;cause.owner=&eligible;auto s=Script(fixed,entry,&cause);if(mode==0)eligible.world=false;if(mode==1)eligible.map=&secondMap;if(mode==2)eligible.phase=false;if(mode==3)eligible.distance=71;if(mode==4)eligible.available=false;if(mode==5){s.actor.scope=true;s.actor.scopeAllows=false;}if(mode==6)s.actor.world=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&cause);assert(stories.empty());}
  Clear();{auto s=Script(fixed,entry,&guardian);owner.distance=70;s.ProcessEventsFor(SMART_EVENT_AGGRO,&guardian);assert(stories.size()==1&&stories[0].player==&owner);owner.distance=1;}
  Clear();{auto s=Script(fixed,entry,&pet);failConversation=true;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty()&&chat.requests.size()==1);failConversation=false;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty());}
  for(unsigned mode=0;mode<3;++mode){Clear();auto s=Script(fixed,entry,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(SMART_EVENT_AGGRO,&pet);assert(stories.empty()&&chat.requests.empty());}conditions.allow=true;
 }
 std::puts("PASS native Baelgun/Magroth/Gavinrad: real740/742/743 conversations reach player/pet/guardian/charmer owner; seven own TALK groups and Baelgun valid cast->LINK speech retained, no repeats; raw old pet failure, ownerless/null/range70/map/phase/world/scope/one-hit death/missing actors covered. Original failed conversation once semantics and independent combat requests unchanged. Creation/chat/spells/timer scheduling controlled; actual client/full Frost separate.");
}
