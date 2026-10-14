#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using uint8=uint8_t;using SpellCastResult=int;using SkillType=int;using LockType=int;
enum {ALLIANCE=469,HORDE=67,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,QUEST_OBJECTIVE_GAMEOBJECT=2,QUEST_OBJECTIVE_FLAG_OPTIONAL=4,
 GAMEOBJECT_TYPE_GOOBER=10,GAMEOBJECT_FIELD_FLAGS=1,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=16,GO_JUST_DEACTIVATED=3,
 SKILL_NONE=0,SKILL_LOCKPICKING=1,SKILL_HERBALISM=2,SKILL_MINING=3,SKILL_FISHING=4,SKILL_INSCRIPTION=5,SKILL_ARCHAEOLOGY=6,
 LOCKTYPE_PICKLOCK=1,LOCKTYPE_HERBALISM=2,LOCKTYPE_MINING=3,LOCKTYPE_FISHING=4,LOCKTYPE_INSCRIPTION=5,LOCKTYPE_ARCHAELOGY=6,
 LOCK_KEY_ITEM=1,LOCK_KEY_SKILL=2,LOCK_KEY_SPELL=3,MAX_LOCK_CASE=8,TYPEID_PLAYER=4,TARGET_GAMEOBJECT_ITEM_TARGET=23,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_LOW_CASTLEVEL=2,SPELL_FAILED_CUSTOM_ERROR=3,
 ARMY_UNIT_BERSERK=0,SPELL_CUSTOM_ERROR_REQUIRES_2_WITHERED_BERSERKER=1,SPELL_CUSTOM_ERROR_REQUIRES_AT_LEAST_10_WITHERED=2,
 LOOT_SKINNING=1,ITEM_FIELD_DYNAMIC_FLAGS=1,ITEM_FLAG_UNLOCKED=1,ITEM_CHANGED=1};
struct ObjectGuid {uint32 value=0;bool operator==(ObjectGuid r)const{return value==r.value;}};
struct Player;struct GameObject;struct GameObjectAI;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}bool IsCreature(){return false;}Unit* ToUnit(){return this;}struct Threat {Unit* getHostilTarget(){return nullptr;}} threat;Threat& getThreatManager(){return threat;}bool isPet(){return false;}Unit* GetOwner(){return nullptr;}uint32 GetTypeId(){return ToPlayer()?TYPEID_PLAYER:0;}bool HasAura(uint32){return false;}};
struct QuestObjective {uint32 Type=2,ObjectID=251257,StorageIndex=0,Amount=7,Flags=0;};
struct Quest {uint32 id=38410;std::vector<QuestObjective> objectives{{}};uint32 GetQuestId()const{return id;}auto const& GetObjectives()const{return objectives;}};
struct Store {std::map<uint32,Quest> quests;Quest const* GetQuestTemplate(uint32 q){auto i=quests.find(q);return i==quests.end()?nullptr:&i->second;}} store;auto sQuestDataStore=&store;
struct Player:Unit {uint32 team=ALLIANCE,updates=0,completions=0;bool world=true,alive=true,teleport=false,vehicle=false,transport=false,phase=true,visible=true,los=true,range=true;ObjectGuid guid{1};std::map<uint32,int> statuses;std::map<uint32,uint32> counts;
 Player* ToPlayer()override{return this;}uint32 GetTeam(){return team;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}bool IsWithinDistInMap(GameObject*,float){return range;}bool IsWithinLOSInMap(GameObject*){return los;}ObjectGuid GetGUID(){return guid;}
 int GetQuestStatus(uint32 q){return statuses[q];}uint32 GetQuestObjectiveData(uint32 q,uint32){return counts[q];}void SetQuestObjectiveData(Quest const* q,QuestObjective const*,uint32 n){counts[q->id]=n;}void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,uint32){++updates;}bool CanCompleteQuest(uint32 q){return counts[q]>=7;}void CompleteQuest(uint32 q){statuses[q]=QUEST_STATUS_COMPLETE;++completions;}void SetQuestStatus(uint32 q,int s){statuses[q]=s;}
 uint32 GetSkillValue(int){return 0;}bool IsInArmyTraining(){return false;}struct Army {std::vector<int> currentUnits[1];} armyTrainingInfo;uint32 GetAuraCount(uint32){return 0;}
};
struct Template {uint32 type=10,flags=262144;struct Goober {uint32 open=43;} goober;};
struct GameObject {uint32 entry=251257,map=1220,zone=7541,flags=0,state=0,nativeFallback=0,scriptId=1;uint32 GetScriptId(){return scriptId;}bool world=true,spawned=true,expired=false,hidden=false;ObjectGuid guid{10};Template info;::GameObjectAI* ai=nullptr;void Use(Unit*);
 bool IsInWorld(){return world;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return map;}uint32 GetZoneId(){return zone;}bool isSpawned(){return spawned;}bool IsQuestObjectExpired(){return expired;}bool HasFlag(uint32,uint32 mask){return (flags&mask)!=0;}uint32 GetGoType(){return info.type;}Template* GetGOInfo(){return &info;}bool ShouldHideFor(ObjectGuid){return hidden;}float GetInteractionDistance(){return 5;}
 ObjectGuid GetGUID(){return guid;}void SetFlag(uint32,uint32 flag){flags|=flag;}void SetLootState(uint32 s,Unit*){state=s;}auto AI()->::GameObjectAI*{return ai;}
};
struct GameObjectAI {GameObject* go;explicit GameObjectAI(GameObject* g):go(g){g->ai=this;}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}};
struct GameObjectScript {explicit GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual bool OnGossipHello(Player*,GameObject*){return false;}virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual ~PlayerScript()=default;virtual void OnLogin(Player*){}};
#include "Impalement.inc"
using QuestStatus=int;
constexpr int CONDITION_QUESTTAKEN=9;
bool Taken(Player* object,uint32 ConditionValue1){bool condMeets=false;switch(CONDITION_QUESTTAKEN){
#include "QuestTaken.inc"
}return condMeets;}
bool Phase(Player* player){return (Taken(player,38410)||Taken(player,38615))&&!Taken(player,41451);}
struct ScriptManager {bool enabled=true;bool OnGossipHello(Player* player,GameObject* object){go_stormheim_impalement script;return enabled&&script.OnGossipHello(player,object);}} scriptManager;auto sScriptMgr=&scriptManager;
#include "UsePrefix.inc"
 ++nativeFallback;
}
struct LockEntry {uint32 Type[8]{0,2},Index[8]{0,13},Skill[8]{};};
struct LockStore {LockEntry value;LockEntry const* LookupEntry(uint32 id){return id==43?&value:nullptr;}} locks;auto& sLockStore=locks;
struct Item {uint32 GetEntry(){return 0;}void SetFlag(uint32,uint32){}void SetState(uint32,Player*){}Player* GetOwner(){return nullptr;}};
struct Effect {int MiscValue=13;struct Target {uint32 GetTarget(){return 23;}} TargetA,TargetB;int CalcValue(Unit*,void*){return 0;}};
struct SpellInfo {uint32 Id=6478;Effect effect;Effect* GetEffect(uint32,uint32){return &effect;}};
struct Spell {Unit* m_caster;SpellInfo info;SpellInfo* m_spellInfo=&info;uint32 m_diffMode=0;Item* m_CastItem=nullptr;uint32 m_customError=0;GameObject* gameObjTarget;Item* itemTarget=nullptr;uint32 sends=0,failed=0;
 Spell(Unit* p,GameObject* g):m_caster(p),gameObjTarget(g){}SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);void Complete();void SendCastResult(int){++failed;}void SendLoot(ObjectGuid,int){++sends;}
};
struct ObjectMgr {std::string GetScriptName(uint32 id){return id==1?"go_stormheim_impalement":"custom";}} objectMgr;auto sObjectMgr=&objectMgr;
#include "CanOpenLock.inc"
void Spell::Complete(){Player* player=m_caster->ToPlayer();if(!player)return;uint32 lockId=gameObjTarget->GetGOInfo()->goober.open;uint32 effIndex=0;ObjectGuid guid=gameObjTarget->GetGUID();
#include "OpenLockCompletion.inc"
}
struct Run {Player p;GameObject object;go_stormheim_impalement_ai ai;go_stormheim_impalement script;Run(uint32 team=ALLIANCE):ai(&object){p.team=team;p.statuses[team==ALLIANCE?38410:38615]=QUEST_STATUS_INCOMPLETE;}uint32 Quest(){return p.team==ALLIANCE?38410:38615;}void Open(){Spell s(&p,&object);s.Complete();assert(s.sends==0);}void Respawn(){object.flags=0;object.state=0;object.spawned=true;}};
int main(){store.quests[38410].id=38410;store.quests[38615].id=38615;
 for(uint32 team:{ALLIANCE,HORDE}){Run r(team);scriptManager.enabled=false;r.object.Use(&r.p);assert(r.object.nativeFallback==1);scriptManager.enabled=true;r.object.Use(&r.p);assert(r.object.nativeFallback==1&&r.p.counts[r.Quest()]==0);assert(r.script.OnGossipHello(&r.p,&r.object));r.ai.GossipHello(&r.p,false);assert(r.p.counts[r.Quest()]==0&&r.object.state==0);
  Spell wrong(&r.p,&r.object);wrong.info.effect.MiscValue=5;wrong.Complete();assert(wrong.failed==1&&r.p.counts[r.Quest()]==0);
  for(uint32 i=0;i<7;++i){r.Respawn();r.object.guid={100+i};r.Open();assert(r.p.counts[r.Quest()]==i+1&&r.object.state==GO_JUST_DEACTIVATED);r.Open();assert(r.p.counts[r.Quest()]==i+1);}assert(r.p.updates==7&&r.p.completions==1);r.Respawn();r.Open();assert(r.p.counts[r.Quest()]==7);
 }
 for(int f=0;f<14;++f){Run r;if(f==0)r.p.world=false;if(f==1)r.p.alive=false;if(f==2)r.p.teleport=true;if(f==3)r.p.vehicle=true;if(f==4)r.p.phase=false;if(f==5)r.p.visible=false;if(f==6)r.p.range=false;if(f==7)r.p.los=false;if(f==8)r.object.spawned=false;if(f==9)r.object.hidden=true;if(f==10)r.object.flags=GO_FLAG_NOT_SELECTABLE;if(f==11)r.p.statuses[38410]=QUEST_STATUS_NONE;if(f==12)r.p.transport=true;if(f==13)r.object.expired=true;r.Open();assert(!r.p.counts[38410]&&!r.object.state);}
 {Run a,b;Spell first(&a.p,&a.object);first.Complete();Spell second(&b.p,&a.object);second.Complete();assert(a.p.counts[38410]==1&&!b.p.counts[38410]);a.Respawn();second.Complete();assert(b.p.counts[38410]==1);}
 {Run r;r.Open();r.p.world=false;r.Open();assert(r.p.counts[38410]==1);r.p.world=true;r.Open();assert(r.p.counts[38410]==1);r.Respawn();r.Open();assert(r.p.counts[38410]==2);r.p.statuses[38410]=QUEST_STATUS_NONE;r.Open();assert(r.p.counts[38410]==2);r.p.statuses[38410]=QUEST_STATUS_INCOMPLETE;r.p.counts[38410]=0;r.Respawn();r.Open();assert(r.p.counts[38410]==1);}
 {Run r;r.object.map=1;Spell s(&r.p,&r.object);s.Complete();assert(s.sends==1&&!r.script.OnGossipHello(&r.p,&r.object)&&!r.ai.GossipHello(&r.p,true));}
 {Run r;r.object.scriptId=2;Spell s(&r.p,&r.object);s.Complete();assert(s.sends==1&&!r.p.counts[38410]&&!r.object.state);}
 {Run r;r.p.statuses[38410]=QUEST_STATUS_COMPLETE;StormheimImpalement::RepairLegacy(&r.p);assert(r.p.statuses[38410]==QUEST_STATUS_INCOMPLETE);r.p.counts[38410]=7;r.p.statuses[38410]=QUEST_STATUS_COMPLETE;StormheimImpalement::RepairLegacy(&r.p);assert(r.p.statuses[38410]==QUEST_STATUS_COMPLETE);}
 {Player p;assert(!Phase(&p));for(auto q:{38410u,38615u}){p.statuses.clear();p.statuses[q]=QUEST_STATUS_INCOMPLETE;assert(Phase(&p));p.statuses[41451]=QUEST_STATUS_INCOMPLETE;assert(!Phase(&p));p.statuses[41451]=QUEST_STATUS_NONE;p.statuses[q]=QUEST_STATUS_COMPLETE;assert(!Phase(&p));}}
 std::cout<<"PASS: production native lock43 completion gate and launcher AI; wrong lock type, direct/report use, seven counts, same-spawn duplicate, shared respawn, both factions, failures, re-entry/retry and unrelated-map delegation.\n";
}
