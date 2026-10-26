#include "OwnerHost.inc"
enum{TYPEID_PLAYER=4,GO_STATE_ACTIVE=1};
struct Generator{uint64 Generate(){return 1;}};
struct ObjectMgr{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} objects;
auto sObjectMgr=&objects;
struct Conversation{
 inline static bool fail=false;inline static uint32 calls=0,id=0;inline static Unit* source=nullptr;inline static std::vector<Conversation*> active;inline static std::function<void()> callback;
 bool CreateConversation(uint64,uint32 value,Unit* target,void*,Unit& actor){assert((value==1890||value==1947)&&target==&actor);++calls;id=value;source=target;if(callback)callback();if(!fail)active.push_back(this);return !fail;}
 static void Clean(){for(auto p:active)delete p;active.clear();calls=id=0;source=nullptr;fail=false;callback={};}
};
struct GameObject{uint32 entry=0,state=0,phase=0,updates=0;void SetGoState(uint32 value){assert(value==GO_STATE_ACTIVE);state=value;++updates;}void SetPhaseMask(uint32 value,bool notify){assert(value==4&&notify);phase=value;}};
struct Creature:Unit{
 bool dead=false,m_IsInKillingProcess=false;std::vector<GameObject*> doors;bool isDead(bool)const{return dead;}
};
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& result,Creature* actor,uint32 entry,float range){assert(range==100);for(auto door:actor->doors)if(door->entry==entry)result.push_back(door);}
struct ScriptedAI{Creature* me;explicit ScriptedAI(Creature* actor):me(actor){}virtual ~ScriptedAI()=default;virtual void JustDied(Unit*){}};
struct Zirus:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "ZirusDeath.inc"
};
struct Akaari:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AkaariDeath.inc"
};
void Dispatch(Creature* victim,Unit* killer,ScriptedAI& ai){
#include "DeathGuard.inc"
 victim->m_IsInKillingProcess=true;ai.JustDied(killer);victim->dead=true;victim->m_IsInKillingProcess=false;
}
int main(){
 Map map,other;Player owner,friendPlayer;owner.map=&map;friendPlayer.map=&map;friendPlayer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&friendPlayer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Unit guardian=pet;Unit charmed=pet;charmed.charmer=friendPlayer.objectGuid;
 CriteriaTreeEntry zirus{47365,29789},akaari{47369,29791},pickup{47371,29792};db2Manager.trees[47364]={&zirus};db2Manager.trees[47368]={&akaari};db2Manager.trees[47370]={&pickup};
 Scenario scenario;scenario.currentTree=47364;assert(scenario.CanUpdateCriteria(47365)&&!scenario.CanUpdateCriteria(47369));scenario.currentTree=47368;assert(scenario.CanUpdateCriteria(47369)&&!scenario.CanUpdateCriteria(47365));
 scenario.currentTree=47370;assert(!scenario.CanUpdateCriteria(47369)&&scenario.CanUpdateCriteria(47371));
#ifdef BASELINE
 for(bool final:{false,true}){
  Creature actor;actor.map=&map;Zirus first(&actor);Akaari last(&actor);ScriptedAI& ai=final?static_cast<ScriptedAI&>(last):first;
  Dispatch(&actor,&pet,ai);assert(owner.criteria.empty()&&Conversation::calls==uint32(!final));Conversation::Clean();
 }
 std::cout<<"PASS pinned728285dc counterfactual: both Zirus50298 and final Akaari50300 lose pet/guardian owner progression; original null dereferences not executed. Expected failing baseline.\n";
#else
 uint32 cases=0;
 for(bool final:{false,true})for(Unit* killer:{static_cast<Unit*>(&owner),&pet,&guardian,&charmed})for(bool fail:{false,true}){
  Creature actor;actor.map=&map;GameObject a{248983},b{248984},c{248910},d{249028},unrelated{999};actor.doors={&a,&b,&c,&d,&unrelated};
  Zirus first(&actor);Akaari last(&actor);ScriptedAI& ai=final?static_cast<ScriptedAI&>(last):first;owner.criteria.clear();friendPlayer.criteria.clear();Conversation::fail=fail;
  bool reentered=false;Conversation::callback=[&](){if(!reentered){reentered=true;Dispatch(&actor,killer,ai);}};
  Dispatch(&actor,killer,ai);Dispatch(&actor,killer,ai);
  Player* recipient=killer==&charmed?&friendPlayer:&owner;Player* untouched=killer==&charmed?&owner:&friendPlayer;
  assert(recipient->criteria==std::vector<uint32>{final?50300u:50298u}&&untouched->criteria.empty());
  assert(reentered&&actor.dead&&Conversation::calls==1&&Conversation::source==recipient&&Conversation::id==(final?1947u:1890u));
  for(auto door:{&a,&b,&c,&d})assert(door->updates==uint32(!final)&&door->state==uint32(!final)&&door->phase==(final?0u:4u));assert(!unrelated.updates);
  Conversation::Clean();++cases;
 }
 for(bool final:{false,true})for(int invalid=0;invalid<6;++invalid){
  Creature actor;actor.map=&map;GameObject door{248983};actor.doors={&door};Zirus first(&actor);Akaari last(&actor);ScriptedAI& ai=final?static_cast<ScriptedAI&>(last):first;
  Unit unowned;unowned.map=&map;Unit* killer=&unowned;owner.criteria.clear();friendPlayer.criteria.clear();
  if(invalid==0)killer=nullptr;if(invalid>=2){killer=&pet;}
  if(invalid==2)pet.owner={1,20};if(invalid==3)pet.map=&other;if(invalid==4)owner.preDelete=true;if(invalid==5)owner.inWorld=false;
  Dispatch(&actor,killer,ai);assert(owner.criteria.empty()&&friendPlayer.criteria.empty());
  assert(Conversation::calls==uint32(!final)&&door.updates==uint32(!final));if(!final)assert(Conversation::source==&actor&&door.phase==4);
  Conversation::Clean();pet.map=&map;pet.owner=owner.objectGuid;owner.preDelete=false;owner.inWorld=true;
 }
 Creature missing;missing.map=&map;Zirus noDoors(&missing);owner.criteria.clear();Conversation::fail=true;Dispatch(&missing,&guardian,noDoors);assert(owner.criteria==std::vector<uint32>{50298});Conversation::Clean();
 std::cout<<"PASS "<<cases<<" native direct/pet/guardian/charmer/death-presentation variants; actual owner/in-map lookup and current scenario tree membership, native repeated/reentrant Kill guard, null/unowned/missing/wrong-map/offline/predelete, failed conversation/absent doors and exact four-door cleanup. No new group grants; full combat/loot/client remain separate.\n";
#endif
 Conversation::Clean();
}
