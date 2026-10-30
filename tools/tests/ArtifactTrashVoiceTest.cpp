#define main ExistingHalakPresentationEntryPoint
#include "HalakPresentationTest.cpp"
#undef main
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);
 Player player;Creature pet,guardian,charmed,npc,npcOwned;pet.entry=9001;guardian.entry=9002;charmed.entry=9003;npc.entry=9004;npcOwned.entry=9005;pet.owner=guardian.owner=&player;charmed.charmer=&player;npcOwned.owner=&npc;
 for(uint32 entry:{95433u,95434u,97993u,97994u,98892u}){
  bool opening=entry==95433||entry==95434;
  auto event=opening?SMART_EVENT_AGGRO:SMART_EVENT_DEATH;
  auto original=Read(argv[1],entry),fixed=Read(argv[2],entry);assert(original.size()==fixed.size()&&!original.empty());
  for(Unit* invoker:{static_cast<Unit*>(&player),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&npc),static_cast<Unit*>(&npcOwned),static_cast<Unit*>(nullptr)}){
   for(bool repaired:{false,true}){Clear();auto s=Script(repaired?fixed:original,entry,invoker);s.ProcessEventsFor(event,invoker);
    bool own=repaired||!IsCreature(invoker);assert(chat.requests.size()==unsigned(own));if(own)assert(chat.requests[0].speaker==entry&&chat.requests[0].group==0);
    assert(s.calls.size()==1&&s.calls[0].action==1&&!s.errors&&stories.empty()&&!player.criteria&&s.GetLastInvoker()==invoker);
    s.ProcessEventsFor(event,invoker);assert(chat.requests.size()==(own?(opening?1u:2u):0u)); // retain once-on-aggro and original repeatable death
    assert(s.calls.size()==(opening?1u:2u));
   }
  }
  for(unsigned mode=0;mode<3;++mode){Clear();auto s=Script(fixed,entry,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(event,&pet);assert(chat.requests.empty());if(mode<2)assert(s.calls.empty());else assert(s.calls.size()==1);}conditions.allow=true;
  Clear();{auto s=Script(fixed,entry,&guardian);s.actor.world=false;s.ProcessEventsFor(event,&guardian);assert(chat.requests.empty());}
  // A fresh actor/attempt admits its own native event again; no previous pet
  // owner, group/faction proxy, scene, quest or item credit is carried over.
  Clear();{auto s=Script(fixed,entry,nullptr);s.ProcessEventsFor(event);assert(chat.requests.size()==1&&chat.requests[0].speaker==entry&&stories.empty());}
  Clear();{auto s=Script(fixed,entry,&pet);s.ProcessEventsFor(SMART_EVENT_UPDATE_IC);std::vector<uint32> spells;for(auto const& c:s.calls){assert(c.action==11);spells.push_back(c.parameter);}std::vector<uint32> expected;
   if(entry==95433)expected={200663};if(entry==97993)expected={195577,195575};if(entry==97994)expected={187207};if(entry==98892)expected={195584};assert(spells==expected&&!s.errors&&chat.requests.empty());
  }
 }
 std::puts("PASS actual native event/TALK/SELF-victim-invoker/chat admission on5 SQL actors: old pet redirects own lines, repaired source speaks0 for Player/pet/guardian/charm/NPC/null; native aggro once/death repeat/one-hit direct death/new attempt/missing-expired-hidden actor preserved; all5 combat spell requests unchanged. Range delivery/audio/AI combat timers and ordinary death credit remain native/separate client checks.");
}
