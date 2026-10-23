#include "TitanstrikeWorkshopFixture.inc"
enum class HighGuid { Conversation };
struct Generator {uint64_t Generate(){return ++serial;}} generator;
struct ObjectMgr {template<HighGuid> Generator* GetGenerator(){return &generator;}} objectMgr;
ObjectMgr* sObjectMgr=&objectMgr;
bool failConversation=false;int conversations=0;
struct Conversation {bool CreateConversation(uint64_t,uint32 id,Creature* actor,void*,Creature&){assert(id==9002055&&actor->entry==106376);if(failConversation)return false;++conversations;return true;}};
#include "TitanstrikeWorkshopActor.inc"
int main(){
 std::array<unsigned,3> order{{0,1,2}};unsigned permutations=0;
 do {
  Creature base(105016);base.reactivating=true;npc_titanstrike_xr947 ai(&base);scenario.step=3;
  Creature tank(106352),left(106354),right(106354),unrelated(106354);
  Creature* actors[3]={&tank,&left,&right};unsigned spawns[3]={382302,382300,382303};
  for(unsigned i=0;i<3;++i){actors[i]->dbguid=spawns[i];actors[i]->flags=UNIT_FLAG_NOT_SELECTABLE;actors[i]->x=base.x+float(i?25:43);}
  unrelated.alive=false;unrelated.dbguid=99;
  ai.SetData(2,2);ai.UpdateAI(500);assert(!ai.functional&&ai.calls.empty()); // no boot, no progression
  ai.SetData(1,1);ai.SetData(1,1);assert(ai.calls.size()==1);ai.UpdateAI(500);assert(base.activationCasts==1&&!ai.functional);
  ai.UpdateAI(500);assert(base.activationCasts==2); // simulate failed first activation: retry
  for(auto c:actors)c->flags=0;
  for(unsigned i=0;i<3;++i){actors[order[i]]->alive=false;ai.UpdateAI(500);assert(ai.functional==(i==2));if(i<2){actors[order[i]]->visible=false;ai.UpdateAI(500);assert(!ai.functional);}}
  assert(!base.reactivating&&ai.calls.back()==std::make_pair(2u,2u));auto calls=ai.calls.size();ai.UpdateAI(500);assert(ai.calls.size()==calls);
  assert(ai.imageGuid.IsEmpty());ai.SetData(3,3);failEntry=106376;ai.UpdateAI(500);assert(ai.imageGuid.IsEmpty());failEntry=0;
  ai.UpdateAI(500);auto image=ai.imageGuid;assert(!image.IsEmpty()&&objects[image]->entry==106376);auto summons=base.spawns;
  failConversation=true;ai.UpdateAI(20000);assert(!ai.dialogue);failConversation=false;int before=conversations;ai.UpdateAI(500);assert(ai.dialogue&&conversations==before+1);
  ai.SetData(3,3);ai.UpdateAI(500);assert(base.spawns==summons&&ai.imageGuid==image&&conversations==before+1);
  ++permutations;
 }while(std::next_permutation(order.begin(),order.end()));assert(permutations==6);
 // Unloaded/missing required actor cannot count as dead; wrong scenario cannot
 // start the workshop. Pet/guardian distance is irrelevant to observed deaths.
 Creature base(105016);npc_titanstrike_xr947 ai(&base);scenario.id=909;ai.SetData(1,1);ai.UpdateAI(500);assert(!ai.booted&&!ai.functional);
 scenario.id=1068;ai.SetData(1,1);ai.UpdateAI(500);assert(ai.booted&&!ai.functional);
 std::cout<<"PASS: actual XR9-47 callbacks: six kill orders, distant tank, boot/stale/duplicate gates, missing actors, failed activation/summon/conversation retries and single console/hologram handoff\n";
}
