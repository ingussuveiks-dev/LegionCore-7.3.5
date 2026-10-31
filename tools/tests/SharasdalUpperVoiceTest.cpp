#include "VoiceHost.inc"
static void Load(char const* path,bool repaired){
 std::ifstream input(path);assert(input);std::string text;store={};store.identity=1868;
 while(std::getline(input,text)){std::istringstream row(text);std::string kind;row>>kind;
  if(kind=="line"){ConversationData value{};assert(row>>value.entry>>value.id>>value.textId>>value.unk1>>value.unk2>>value.idx);assert(value.entry==1868);store.lines.push_back(value);}
  else if(kind=="creature"){ConversationCreature value{};assert(row>>value.entry>>value.id>>value.creatureId>>value.creatureGuid>>value.unk1>>value.unk2>>value.duration);assert(value.entry==1868);store.creatures.push_back(value);}else assert(false);
 }
 uint32 sum=0;for(auto const& actor:store.creatures)sum+=actor.duration;
 assert(store.actors.empty()&&store.creatures.size()==2&&store.lines.size()==(repaired?5u:2u)&&sum==(repaired?42816u:15600u));
}
static Conversation* Create(WorldObject& caster){
 live.clear();auto value=new Conversation;
 if(!value->CreateConversation(1,1868,&caster,nullptr,caster.GetPosition())){delete value;return nullptr;}return value;
}
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);unsigned cases=0;
 WorldObject owner,peer,unrelated;peer.guid=43;unrelated.guid=44;
 Creature erunak{102826},adelee{104853};owner.nearby={{102826,&erunak},{104853,&adelee}};
 Load(argv[1],false);currentTime=100;auto original=Create(owner);
 assert(original&&original->lifetime==20600&&original->lines.size()==2&&original->lines.at(0).id==3961&&original->lines.at(1).id==3962&&!original->lines.count(2));++cases;
 Load(argv[2],true);auto repairedDefinition=store;
 // Explicit counterfactual: adding the missing lines without extending the
 // actor budget still cuts off the real continuation. Not an ordinary run.
 store.creatures[1].duration=7800;currentTime=100;auto shortBudget=Create(owner);
 assert(shortBudget&&shortBudget->lines.size()==5&&!shortBudget->CanNeverSee2(&owner));currentTime+=20601;shortBudget->Update(1);assert(shortBudget->removed&&store.lines.back().textId>20600);++cases;
 store=repairedDefinition;currentTime=100;auto fixed=Create(owner);
 assert(fixed&&fixed->lifetime==47816&&fixed->deadline==47816&&fixed->creatures.size()==2&&fixed->lines.size()==5);
 assert(fixed->personal==std::set<uint32>{42}&&!fixed->CanNeverSee2(&owner));
 for(auto const& line:store.lines){auto const& wire=fixed->lines.at(line.idx);assert(wire.id==line.id&&wire.textId==line.textId&&wire.unk1==line.unk1&&wire.unk2==line.unk2&&wire.unk2<2);}
 currentTime=100+20601;fixed->Update(1);assert(!fixed->removed);currentTime=100+42816;fixed->Update(1);assert(!fixed->removed);
 currentTime=100+47816;fixed->Update(1);assert(!fixed->removed&&!fixed->CanNeverSee2(&owner));++currentTime;fixed->Update(1);assert(fixed->removed&&fixed->CanNeverSee2(&owner));++cases;
 for(uint32 missing:{104853u,102826u}){auto saved=owner.nearby[missing];owner.nearby[missing]=nullptr;assert(!Create(owner));owner.nearby[missing]=saved;++cases;}
 for(auto actor:{&adelee,&erunak}){actor->alive=false;assert(!Create(owner));actor->alive=true;++cases;}
 store.lines.clear();assert(!Create(owner));store=repairedDefinition;++cases;
 currentTime=100;auto expired=Create(owner);assert(expired);expired->questExpired=true;expired->Update(1);assert(expired->removed);++cases;
 GroupReference second{&peer,nullptr},first{&owner,&second};Group group{&first};owner.group=&group;
 currentTime=100;auto shared=Create(owner);assert(shared&&shared->personal==std::set<uint32>({42,43})&&!shared->personal.count(unrelated.guid));
 assert(!shared->CanNeverSee2(&peer));currentTime+=100000;shared->Update(1);assert(!shared->removed);assert(!shared->CanNeverSee2(&owner));currentTime+=47817;shared->Update(1);assert(shared->removed);owner.group=nullptr;++cases;
 currentTime=100;auto replay=Create(owner);assert(replay&&!replay->CanNeverSee2(&owner)&&replay->lines.size()==5&&replay->lifetime==47816);++cases;
 std::cout<<"PASS "<<cases<<" native Conversation payload/lifetime/per-view/Update cases under ASAN: original two-line payload lacks three native continuation slots, explicit lines-only counterfactual20600ms truncates; repaired five exact slots/two actors/native47816ms covers42816+5000. Original line fields, personal/group-first clocks, dead/missing actors, empty data, quest expiry and replay retained. Factory/registry/player caller are controlled; current production upper NPC caller/admission/world expiry and actual client playback/full1066 remain OPEN.\n";
}
