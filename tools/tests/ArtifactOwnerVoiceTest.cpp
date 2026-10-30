#define main ExistingHalakPresentationEntryPoint
#include "HalakPresentationTest.cpp"
#undef main
static bool Selected(uint32 entry,uint32 id){return (entry==98760&&id==6)||((entry==100937||entry==100938)&&id==1)||((entry==102244||entry==102245||entry==102304)&&id==0)||((entry==102245||entry==102304||entry==109397)&&id==1)||(entry==102362&&id==2);}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==4);Texts(argv[3]);Player owner,other;Creature pet,guardian,charmed,npc,npcOwned;pet.entry=9001;guardian.entry=9002;charmed.entry=9003;npc.entry=9004;npcOwned.entry=9005;pet.owner=guardian.owner=&owner;charmed.charmer=&owner;npcOwned.owner=&npc;
 unsigned sites=0;
 for(uint32 entry:{98760u,100937u,100938u,102244u,102245u,102304u,102362u,109397u}){
  auto old=Read(argv[1],entry),fixed=Read(argv[2],entry);assert(old.size()==fixed.size());
  for(auto const& row:fixed)if(Selected(entry,row.event_id)){
   ++sites;assert(row.actionType==1&&row.targetType==130&&row.target.playerDistance.dist==70);
   auto kind=SMART_EVENT(row.type);assert(kind==SMART_EVENT_AGGRO||kind==SMART_EVENT_DEATH);
   for(Unit* invoker:{static_cast<Unit*>(&owner),static_cast<Unit*>(&pet),static_cast<Unit*>(&guardian),static_cast<Unit*>(&charmed),static_cast<Unit*>(&npc),static_cast<Unit*>(&npcOwned),static_cast<Unit*>(nullptr)}){
    std::vector<Call> baseline;uint32 previousCredit=owner.criteria;Clear();auto before=Script(old,entry,invoker);before.ProcessEventsFor(kind,invoker);baseline=before.calls;uint32 creditDelta=owner.criteria-previousCredit;
    bool oldOwn=!IsCreature(invoker);assert(chat.requests.size()==unsigned(oldOwn));if(oldOwn)assert(chat.requests[0].speaker==entry&&chat.requests[0].group==row.parameter);
    previousCredit=owner.criteria;Clear();auto after=Script(fixed,entry,invoker);after.ProcessEventsFor(kind,invoker);assert(after.calls==baseline&&owner.criteria-previousCredit==creditDelta&&!after.errors);
    assert(chat.requests.size()==1&&chat.requests[0].speaker==entry&&chat.requests[0].group==row.parameter&&after.GetLastInvoker()==invoker);
    assert(after.mTextGUID.unit==GetScriptCreditPlayer(invoker)); // preserve class/name/whisper context for the actual owner
    // A repeated native event has the same non-speech service requests and
    // original once/repeat semantics, not a new progression/credit source.
    Clear();before.ProcessEventsFor(kind,invoker);auto repeatCalls=before.calls;Clear();after.ProcessEventsFor(kind,invoker);assert(after.calls==repeatCalls&&!after.errors);
   }
   for(unsigned mode=0;mode<7;++mode){Clear();auto s=Script(fixed,entry,&pet);owner.available=true;owner.world=true;owner.map=&firstMap;owner.phase=true;owner.distance=1;s.actor.scope=false;
    switch(mode){case 0:owner.available=false;break;case 1:owner.world=false;break;case 2:owner.map=&secondMap;break;case 3:owner.phase=false;break;case 4:owner.distance=70.01f;break;case 5:s.actor.scope=true;s.actor.scopeAllows=false;break;case 6:s.actor.world=false;break;}
    s.ProcessEventsFor(kind,&pet);if(mode==6)assert(chat.requests.empty());else assert(chat.requests.size()==1&&chat.requests[0].speaker==entry&&!s.mTextGUID.unit);
   }
   owner.available=true;owner.world=true;owner.map=&firstMap;owner.phase=true;owner.distance=70;Clear();{auto s=Script(fixed,entry,&guardian);s.ProcessEventsFor(kind,&guardian);assert(s.mTextGUID.unit==&owner&&chat.requests.size()==1);}owner.distance=1;
   for(unsigned mode=0;mode<3;++mode){Clear();auto s=Script(fixed,entry,&pet);conditions.allow=mode!=0;s.actor.expired=mode==1;if(mode==2)s.me=nullptr;s.ProcessEventsFor(kind,&pet);assert(chat.requests.empty());}conditions.allow=true;
   Clear();{pet.owner=&other;auto s=Script(fixed,entry,&pet);s.ProcessEventsFor(kind,&pet);assert(s.mTextGUID.unit==&other&&chat.requests.size()==1);pet.owner=&owner;}
  }
 }
 assert(sites==10);
 std::puts("PASS native root/action/TALK/owner130/SELF-victim-invoker/chat admission on10 actual SQL lines: pet/guardian/charm keeps own NPC speaker and true Player text GUID; Player/NPC/null/missing owner/map/phase/world/range70/scope/repeat/fresh direct death preserved. Full recorded linked service/credit requests match old SQL; recipient spell/AI world effects/timers and client voice remain separate.");
}
