#define main PreservationMain
#include "TuureFirstFightTest.cpp"
#undef main
#include <string>
using SpellCastResult=uint32;using SkillType=int;using LockType=int;
enum {GAMEOBJECT_FIELD_FLAGS=17,GO_FLAG_NOT_SELECTABLE=16,GO_FLAG_IN_USE=32,GAMEOBJECT_TYPE_GOOBER=10,
 GO_READY=1,GO_ACTIVATED=2,GO_STATE_READY=1,GO_STATE_ACTIVE=0,IN_MILLISECONDS=1000,
 SPELL_CAST_OK=0,SPELL_FAILED_BAD_TARGETS=1,SPELL_FAILED_LOW_CASTLEVEL=2,SPELL_FAILED_CUSTOM_ERROR=3,
 SKILL_NONE=0,SKILL_LOCKPICKING=1,SKILL_HERBALISM=2,SKILL_MINING=3,SKILL_FISHING=4,SKILL_INSCRIPTION=5,SKILL_ARCHAEOLOGY=6,
 LOCKTYPE_PICKLOCK=1,LOCKTYPE_HERBALISM=2,LOCKTYPE_MINING=3,LOCKTYPE_FISHING=19,LOCKTYPE_INSCRIPTION=20,LOCKTYPE_ARCHAELOGY=22,
 LOCK_KEY_ITEM=1,LOCK_KEY_SKILL=2,LOCK_KEY_SPELL=3,MAX_LOCK_CASE=8,TYPEID_PLAYER=4,TARGET_GAMEOBJECT_ITEM_TARGET=23,
 ARMY_UNIT_BERSERK=0,SPELL_CUSTOM_ERROR_REQUIRES_2_WITHERED_BERSERKER=1,SPELL_CUSTOM_ERROR_REQUIRES_AT_LEAST_10_WITHERED=2};
struct GameObjectTemplate{
 uint32 entry=249693;struct {uint32 open=1690,pageID=0,gossipID=0,eventID=0,questID=0,linkedTrap=0,customAnim=0,spell=210480;}goober;
 uint32 GetAutoCloseTime()const{return 3;}
};
struct GameObjectAI{GameObject* go;explicit GameObjectAI(GameObject* p):go(p){}virtual ~GameObjectAI()=default;virtual bool GossipHello(Player*,bool){return false;}virtual bool GossipUse(Player*){return false;}virtual void UpdateAI(uint32){}virtual void Reset(){}};
struct GameObject:Unit{
 uint32 entry=249693,mapId=1489,scriptId=1,flags=0,state=GO_STATE_READY,lootState=GO_READY,uses=0,deletes=0;
 uint64 db=134192,m_cooldownTime=0;bool spawned=true,preDelete=false;GameObjectAI* ai=nullptr;GameObjectTemplate info;
 uint32 GetEntry()const override{return entry;}uint32 GetMapId()const override{return mapId;}uint64 GetDBTableGUIDLow()const{return db;}
 bool IsPreDelete()const override{return preDelete;}bool isSpawned()const{return spawned;}
 uint32 GetScriptId(){return scriptId;}uint32 GetGoType(){return GAMEOBJECT_TYPE_GOOBER;}GameObjectTemplate const* GetGOInfo(){return &info;}
 uint32 GetGoState(){return state;}void SetGoState(uint32 v){state=v;}float GetInteractionDistance(){return 5;}
 bool HasFlag(uint32 field,uint32 mask){assert(field==17);return (flags&mask)!=0;}void SetFlag(uint32 field,uint32 mask){assert(field==17);flags|=mask;}
 void SetLootState(uint32 v,Unit*){lootState=v;}void EventInform(uint32){}uint32 GetGoAnimProgress(){return 0;}void SendCustomAnim(uint32){}void TriggeringLinkedGameObject(uint32,Unit*){}
 GameObjectAI* AI(){return ai;}void Use(Unit* user){
#include "GoUseGate.inc"
 ++uses;Unit* spellCaster=user;uint32 spellId=0;
 switch(GetGoType()){
#include "GooberUse.inc"
 }
 assert(spellId==210480&&!spellCaster); // native dummy payload independently read from26972
 }
 void Delete(){++deletes;world=false;}
};
struct ObjectMgr{std::string GetScriptName(uint32 id){return id==1?"go_tuure_crate":"custom";}} objectMgr;auto sObjectMgr=&objectMgr;
struct LockEntry{uint32 Type[8]{0,2},Index[8]{0,5},Skill[8]{};};
struct LockStore{LockEntry lock;bool present=true;LockEntry const* LookupEntry(uint32 id){return present&&id==1690?&lock:nullptr;}} lockStore;auto& sLockStore=lockStore;
struct Item{uint32 GetEntry(){return 0;}};
struct LockEffect{int MiscValue=5;struct Target{uint32 GetTarget(){return 23;}}TargetA,TargetB;int CalcValue(Unit*,void*){return 0;}};
struct LockSpellInfo{uint32 Id=3365;LockEffect effect;LockEffect* GetEffect(uint32,uint32){return &effect;}};
struct Spell{
 Unit* m_caster;GameObject* gameObjTarget;LockSpellInfo info;LockSpellInfo* m_spellInfo=&info;uint32 m_diffMode=0,m_customError=0,failed=0,sends=0;Item* m_CastItem=nullptr;
 Spell(Unit* p,GameObject* g):m_caster(p),gameObjTarget(g){}
 SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);void SendCastResult(uint32){++failed;}
 void SendLoot(ObjectGuid,uint32){++sends;
#include "GooberLoot.inc"
 assert(false);
 }
 void Complete(){Player* player=m_caster->ToPlayer();if(!player)return;uint32 lockId=gameObjTarget->GetGOInfo()->goober.open;SkillType skillId=0;int32 reqSkillValue=0,skillValue=0;SpellCastResult res=CanOpenLock(0,lockId,skillId,reqSkillValue,skillValue);
#include "LockGate.inc"
#include "CageBridge.inc"
 SendLoot(gameObjTarget->GetGUID(),0);
 }
};
#include "CanOpenLock.inc"
#include "Cage.inc"
struct CageRun:Run{
 GameObject cage;Creature captain;std::unique_ptr<go_tuure_crateAI> ai;
 CageRun(){scenario.step=3;lockStore={};scriptMgr={};cage.map=&map;cage.guid={3,100};mage.dbGuid=361433;mage.flags=4;script.OnCreatureCreate(&mage);captain.map=&map;captain.entry=106107;captain.dbGuid=361436;captain.guid={2,200};captain.flags=15;map.actors.push_back(&captain);script.OnCreatureCreate(&captain);ai=std::make_unique<go_tuure_crateAI>(&cage);cage.ai=ai.get();}
 void Open(){Spell spell(&player,&cage);spell.Complete();assert(!spell.failed);}
};
int main(){
 assert(PreservationMain()==0);
#ifdef CAGE_BASELINE
 CageRun r;r.Open();assert(!r.player.attempts&&scenario.step==3&&!r.cage.deletes&&r.cage.state==GO_STATE_ACTIVE);
 std::cout<<"PASS labelled ebdbfcd6 native goober/lock counterfactual: real completed3365/lock1690 routes through native SendLoot goober Use, never old GossipHello(true); no50633/release. No deliberately null-AI baseline crash claimed.\n";
#else
 for(uint32 status:{1u,3u}){CageRun r;r.player.quest.Status=status;r.Open();assert(scenario.step==4&&scenario.achievement.progress[47561].Counter==1&&r.cage.uses==1&&!r.cage.deletes&&r.cage.state==GO_STATE_ACTIVE);assert(r.mage.flags==0&&r.captain.flags==0&&r.captain.faction==2102&&r.mage.followMoves==1);r.ai->UpdateAI(2999);assert(!r.cage.deletes);r.ai->UpdateAI(1);assert(r.cage.deletes==1);}
 for(int bad=0;bad<23;++bad){CageRun r;switch(bad){
 case 0:r.player.alive=false;break;case 1:r.player.gm=true;break;case 2:r.player.classId=4;break;case 3:r.player.quest.Status=0;break;
 case 4:r.player.rewarded=true;break;case 5:r.player.pending=true;break;case 6:r.player.phase=false;break;case 7:r.player.near=false;break;case 8:r.player.world=false;break;case 9:r.player.contact=false;break;
 case 10:scenario.id=940;break;case 11:scenario.id=972;break;case 12:scenario.id=1145;break;case 13:scenario.id=1146;break;case 14:scenario.step=2;break;
 case 15:r.cage.db=0;break;case 16:r.cage.entry=249694;break;case 17:r.cage.mapId=1220;break;case 18:r.cage.scriptId=2;break;
 case 19:r.cage.spawned=false;break;case 20:r.cage.flags=GO_FLAG_NOT_SELECTABLE;break;case 21:r.cage.preDelete=true;break;case 22:r.cage.world=false;break;
 }r.Open();r.ai->UpdateAI(1000);assert(!r.player.attempts&&!r.cage.deletes&&r.cage.uses==0&&r.captain.flags==15);}
 {CageRun r;r.ai->GossipHello(&r.player,false);r.cage.Use(&r.player);assert(!r.cage.uses&&!r.player.attempts);}
 {CageRun r;r.player.los=false;r.Open();assert(r.player.attempts==1&&r.cage.uses==1);} // native goober requireLOS=0; completed lock hit remains authoritative
 {CageRun r;Spell s(&r.player,&r.cage);s.info.effect.MiscValue=6;s.Complete();assert(s.failed==1&&!r.player.attempts&&!r.cage.uses);}
 {CageRun r;lockStore.present=false;Spell s(&r.player,&r.cage);s.Complete();assert(s.failed==1&&!r.player.attempts);}
 {CageRun r;Spell s(&r.player,&r.cage);s.info.Id=123;s.Complete();assert(!r.player.attempts&&!r.cage.uses);}
 {CageRun r;r.cage.info.goober.open=2127;Spell s(&r.player,&r.cage);s.Complete();assert(!r.player.attempts&&!r.cage.uses);}
 {CageRun r;r.captain.world=false;r.Open();assert(!r.player.attempts&&!r.cage.uses&&warnings==1);r.ai->UpdateAI(1000);assert(!r.player.attempts&&warnings==1);r.captain.world=true;r.ai->UpdateAI(1);assert(scenario.step==4&&r.cage.uses==1);}
 {CageRun r;r.player.rejectCredit=true;r.Open();assert(r.player.attempts==1&&!r.cage.uses&&!r.cage.deletes);r.player.rejectCredit=false;r.ai->UpdateAI(4999);assert(r.player.attempts==1);r.ai->UpdateAI(1);assert(r.player.attempts==2&&r.cage.uses==1);}
 {CageRun r;r.captain.world=false;r.Open();auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.captain.world=true;r.ai->UpdateAI(1);assert(!r.player.attempts&&!r.cage.uses);r.Open();assert(r.player.attempts==1&&r.player.quest.ScriptLifetime!=held);}
 {CageRun r;r.captain.world=false;r.Open();r.ai->UpdateAI(30000);r.captain.world=true;r.ai->UpdateAI(1);assert(!r.player.attempts&&!r.cage.uses);r.Open();assert(r.player.attempts==1);}
 {CageRun r;scriptMgr.intercept=true;r.Open();assert(r.player.attempts==1&&!r.cage.uses&&!r.cage.deletes&&r.captain.flags==15);scriptMgr.intercept=false;r.ai->UpdateAI(5000);assert(r.cage.uses==1&&r.player.attempts==1&&r.captain.flags==0);}
 {CageRun r;r.Open();r.ai=std::make_unique<go_tuure_crateAI>(&r.cage);r.cage.ai=r.ai.get();r.ai->UpdateAI(0);assert(r.player.attempts==1&&r.cage.uses==1&&r.captain.flags==0);}
 {CageRun r;r.captain.world=false;r.Open();r.ai->Reset();r.captain.world=true;r.ai->UpdateAI(0);assert(!r.player.attempts);r.Open();assert(r.player.attempts==1);}
 {CageRun r;r.player.onCredit=[&]{r.captain.world=false;};r.Open();assert(r.player.attempts==1&&!r.cage.uses&&r.captain.flags==15);r.player.onCredit={};r.captain.world=true;r.ai->UpdateAI(1);assert(r.player.attempts==1&&r.cage.uses==1&&r.captain.flags==0);}
 {CageRun r;r.player.onCredit=[&]{r.map.players.clear();};r.Open();assert(r.player.attempts==1&&!r.cage.uses&&r.cage.state==GO_STATE_ACTIVE&&r.captain.flags==0);}
 {CageRun r;scenario.achievement.progress[47561].Counter=1;scenario.step=4;r.player.gm=true;r.ai->UpdateAI(0);assert(!r.player.attempts&&r.cage.state==GO_STATE_ACTIVE&&r.captain.flags==0);}
 {CageRun r;scenario.achievement.progress[47561].Counter=1;scenario.step=5;r.ai->UpdateAI(0);r.ai->UpdateAI(3000);assert(!r.player.attempts&&r.cage.deletes==1&&r.captain.flags==15);}
 {CageRun r;r.captain.guid={9,200};r.script.OnCreatureCreate(&r.captain);r.Open();assert(r.player.attempts==1);}
 std::cout<<"PASS Tuure full cage AI, full native CanOpenLock1690 and exact native post-validation dispatch/SendLoot goober/Use branch: genuine opening,23 denials, no direct/report-use grant, missing actors/5s native retry/30s timeout/retake/earned recovery and native3s visual interval. World/criteria and presentation endpoints are adapters; client opening/cage/Boja/captain pending.\n";
#endif
}
