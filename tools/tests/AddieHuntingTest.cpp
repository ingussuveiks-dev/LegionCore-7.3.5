#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;
using SpellEffIndex=int;
struct DB2Manager{static bool IsInArea(int area,int parent){return area==parent;}};
struct ObjectGuid{int value=0;bool operator<(ObjectGuid b)const{return value<b.value;}static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
enum {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED,EFFECT_0=0,UNIT_FIELD_CREATED_BY_SPELL=4,PROC_FLAG_KILL=2,PROC_FLAG_NONE=0,PROC_EX_NONE=0,SPELL_SCHOOL_MASK_NORMAL=1,SPELL_DIRECT_DAMAGE=0};
enum{QUEST_OBJECTIVE_FLAG_OPTIONAL=4,QUEST_OBJECTIVE_FLAG_HIDE_ITEM_GAINS=16,QUEST_OBJECTIVE_FLAG_PART_OF_PROGRESS_BAR=64,QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT=2};
using SpellSchoolMask=int;struct AuraEffect{};
struct QuestObjective{int8 StorageIndex;int ObjectID,Amount;uint32 Flags=0;};
struct Quest{std::vector<QuestObjective> objectives{{0,97441,38,28},{1,99034,1},{2,97354,1,2},{3,99935,1,2},{4,99936,1,2},{5,97441,56,28},{6,97441,70,28}};auto const& GetObjectives()const{return objectives;}bool HasSpecialFlag(int flag)const{return flag==2;}} quest;
struct QuestStore{Quest const* GetQuestTemplate(uint32 id){assert(id==40216);return &quest;}} store;auto sQuestDataStore=&store;
struct Player;
struct Unit{int entry=0,map=1220;bool creature=true;float distance=0;ObjectGuid guid{1};Player* player=nullptr;bool pet=false,totem=false;Unit* owner=nullptr;
 int area=7732;int GetAreaId(){return area;}
 bool IsCreature(){return creature;}Player* ToPlayer(){return player;}int GetEntry(){return entry;}int GetMapId(){return map;}ObjectGuid GetGUID(){return guid;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself(){return player?player:owner?owner->ToPlayer():nullptr;}
 bool IsWithinDistInMap(Unit* v,float max){return map==v->map&&v->distance<=max;}
 bool isPet(){return pet;}bool isTotem(){return totem;}Unit* GetOwner(){return owner;}
 void ProcDamageAndSpell(Unit*,int,int,int,void*);
};
struct Creature:Unit{uint32 created=188720;uint32 GetUInt32Value(int){return created;}} companion;
struct Player:Unit{int status=QUEST_STATUS_INCOMPLETE,completed=0,packets=0,ownerKillProcs=0;std::map<int,int> counters{{1,1}};Creature* minion=&companion;
 int summons=0;void CastSpell(Player*,int,bool){++summons;}
 Player(){player=this;creature=false;}
 int GetQuestStatus(uint32){return status;}bool HasAura(uint32){return status==QUEST_STATUS_INCOMPLETE||status==QUEST_STATUS_COMPLETE;}
 int GetQuestObjectiveData(uint32,int id){for(auto const& o:quest.objectives)if(o.ObjectID==id)return counters[o.StorageIndex];return 0;}
 int GetQuestObjectiveData(Quest const*,int8 slot){return counters[slot];}
 void SetQuestObjectiveData(Quest const*,QuestObjective const* o,int count){counters[o->StorageIndex]=count;}
 void SendQuestUpdateAddCredit(Quest const*,ObjectGuid,QuestObjective const&,int){++packets;}
 void KilledMonsterCredit(int id){for(auto const& o:quest.objectives)if(o.ObjectID==id){assert(!counters[o.StorageIndex]);counters[o.StorageIndex]=1;return;}}
 bool HasQuestObjectiveComplete(Quest const*,QuestObjective const& o){return counters[o.StorageIndex]>=o.Amount;}
 bool CanCompleteQuest(uint32){auto qInfo=&quest;struct Status{int Status;}state{status};auto q_status=&state;
#include "EventGate.inc"
 return counters[1]&&counters[2]&&counters[3]&&counters[4];}
 void CompleteQuest(uint32){++completed;status=QUEST_STATUS_COMPLETE;}
 Creature* GetMinionByEntry(uint32 id){assert(id==99040);return minion;}
};
struct Effect{int BasePoints=10;};struct SpellInfo{Effect effect;Effect* Effects[1]{&effect};struct Options{uint32 ProcTypeMask=0;bool IsProcAura=false;}options;Options* GetAuraOptions(){return &options;}Options const* GetAuraOptions()const{return &options;};struct Misc{struct Data{int SchoolMask=1;}MiscData;}misc;Misc* GetMisc(int){return &misc;}};
struct Spells{SpellInfo small,large;Spells(){small.effect.BasePoints=5;}SpellInfo const* GetSpellInfo(uint32 id){return id==196679?&small:&large;}} spells;auto sSpellMgr=&spells;
#include "Hunting.inc"
struct ProcEventInfo{Unit* target;uint32 mask=PROC_FLAG_KILL;Unit* GetActionTarget(){return target;}uint32 GetTypeMask(){return mask;}};
struct Aura{Player* owner;std::set<ObjectGuid> credited;uint32 retry=2000;bool removed=false;void PreventDefaultAction(){}Unit* GetTarget(){return owner;}Unit* GetUnitOwner(){return owner;}void Remove(){removed=true;}
#include "Proc.inc"
#include "Update.inc"
};
struct SkillCast{Player* target;Unit* caster;SpellInfo info;bool prevented=false;
 SkillCast(Player* p,Unit* unit):target(p),caster(unit){}
 void PreventHitDefaultEffect(int){prevented=true;}Unit* GetHitUnit(){return target;}Unit* GetCaster(){return caster;}SpellInfo* GetSpellInfo(){return &info;}
#include "Credit.inc"
};
struct DamageInfo{DamageInfo(Unit*,Unit*,int,SpellInfo const*,int,int,int){}};
Aura* active=nullptr;
void Unit::ProcDamageAndSpell(Unit* victim,int mask,int,int,void*){assert(player&&mask==PROC_FLAG_KILL);++player->ownerKillProcs;ProcEventInfo event{victim};active->Proc(nullptr,event);}
struct Killer:Unit{int m_spawnMode=0;void Kill(Unit* victim,SpellInfo* spellProto=nullptr){
#include "PetKill.inc"
}};
bool GenericCredit(uint32 questid,uint32 entry,uint32 real_entry){for(int i=0;i<1;++i){
#include "GenericCredit.inc"
 return true;}return false;}
#define TC_LOG_ERROR(...) ((void)0)
bool ProcAdmission(){SpellInfo native;SpellInfo const* spell=&native;struct Proc{uint32 procFlags=2;}spe;int customProc=0;for(int i=0;i<1;++i){
#include "ProcAdmission.inc"
 }return native.options.IsProcAura&&customProc==1;}
int main(){
 assert(ProcAdmission());assert(!GenericCredit(40216,97441,97441));assert(GenericCredit(40216,97354,97354)&&GenericCredit(1,97441,97441));
 Player eventGate;eventGate.counters={{0,38},{1,1},{2,1},{3,1},{4,1},{5,56},{6,70}};
 assert(!eventGate.CanCompleteQuest(40216));AddieHunting::Advance(&eventGate,0);assert(eventGate.completed==1);
 Player p;AddieHunting::Advance(&p,37);assert(p.counters[0]==37&&!p.counters[2]);
 AddieHunting::Advance(&p,1);assert(p.counters[0]==38&&p.counters[2]&&!p.counters[3]);
 AddieHunting::Advance(&p,55);assert(p.counters[5]==55&&!p.counters[3]);
 Player reentry=p;assert(!reentry.CanCompleteQuest(40216));AddieHunting::Advance(&reentry,71);assert(reentry.counters[5]==56&&reentry.counters[6]==70&&reentry.completed==1);
 AddieHunting::Advance(&reentry,1000);assert(reentry.completed==1);
 Player noTalk;noTalk.counters[1]=0;AddieHunting::Advance(&noTalk,999);assert(!noTalk.packets);
 for(int state:{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED}){Player q;q.status=state;AddieHunting::Advance(&q,100);assert(!q.packets);Aura aura{&q};aura.Update(100);assert(aura.removed==(state!=QUEST_STATUS_COMPLETE));}
 Player q;Aura aura{&q};active=&aura;Unit victim;victim.entry=94151;victim.guid={99};
 Killer pet;pet.pet=true;pet.owner=&q;pet.Kill(&victim);assert(q.ownerKillProcs==1&&q.counters[0]==10);
 pet.Kill(&victim);assert(q.counters[0]==10); // owner/native proc duplicates cannot double-credit
 for(int mode=0;mode<5;++mode){Unit invalid=victim;invalid.guid={100+mode};invalid.entry=mode==0?9999:94151;invalid.map=mode==1?1:1220;invalid.distance=mode==2?101.f:0.f;
  q.minion=mode==3?nullptr:&companion;ProcEventInfo event{&invalid,mode==4?0u:PROC_FLAG_KILL};aura.Proc(nullptr,event);assert(q.counters[0]==10);}
 q.minion=&companion;
 Unit doe;doe.entry=94238;doe.guid={200};q.counters[0]=37;ProcEventInfo deer{&doe};aura.Proc(nullptr,deer);assert(q.counters[2]&&q.counters[5]==0);
 Player trap;SkillCast cast{&trap,&trap};cast.info.effect.BasePoints=2;cast.Credit(0);assert(cast.prevented&&trap.counters[0]==2);
 Player stranger;cast.caster=&stranger;cast.Credit(0);assert(trap.counters[0]==2);
 cast.caster=&trap;trap.area=1;cast.Credit(0);assert(trap.counters[0]==2);
 Player retry;retry.minion=nullptr;Aura resume{&retry};resume.Update(2000);assert(retry.summons==1);
 std::cout<<"PASS: native thresholds, persistence, exact stage credit, pet-owner kill dispatch, duplicates, native skill spell quantity/ownership, retries, unrelated/out-of-range kills and cleanup\n";
}
