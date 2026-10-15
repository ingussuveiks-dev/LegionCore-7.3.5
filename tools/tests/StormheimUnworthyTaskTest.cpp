#include <fstream>
#include <string>
#include <crtdbg.h>
#include <cstdlib>
#include "UnworthyFixture.inc"
#include "QuestStatus.inc"
using Battleground=Player::BG;
enum {CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28};
bool Condition(Player* object,int kind)
{
    bool condMeets=false;uint32 ConditionValue1=39837;
    switch(kind){
#include "QuestConditions.inc"
    }
    return condMeets;
}
bool Caged(Player* p){return Condition(p,CONDITION_QUESTTAKEN);}
bool Freed(Player* p){return Condition(p,CONDITION_QUEST_COMPLETE)||Condition(p,CONDITION_QUESTREWARDED);}

enum {GAMEOBJECT_TYPE_GOOBER=10,GO_ACTIVATED=1,GO_STATE_ACTIVE=0};
int sEventScripts=0,sGameObjectScripts=0;
namespace GameTime {uint32 GetGameTime(){return 0;}}
namespace WorldPackets {namespace GameObject {struct PageText {explicit PageText(ObjectGuid){}int Write(){return 0;}};}}
FakeMap* GameObject::GetMap(){static FakeMap map;return &map;}
// The complete production goober switch branch. Spell dispatch, grids and
// packet transport are fixtures; this does not simulate rendered animation.
void GameObject::Use(Unit* user)
{
    if(IsQuestObjectExpired()||!spawned||HasFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_IN_USE|GO_FLAG_NOT_SELECTABLE))return;
    uint32 spellId=0;Unit* spellCaster=user;
    switch(info.type){
#include "Goober.inc"
    }
    assert(!spellId);(void)spellCaster;
}

using SkillType=int;using LockType=int;
enum {SKILL_NONE=0,SKILL_LOCKPICKING=1,SKILL_HERBALISM=2,SKILL_MINING=3,SKILL_FISHING=4,SKILL_INSCRIPTION=5,SKILL_ARCHAEOLOGY=6,
 LOCKTYPE_PICKLOCK=1,LOCKTYPE_HERBALISM=2,LOCKTYPE_MINING=3,LOCKTYPE_FISHING=19,LOCKTYPE_INSCRIPTION=20,LOCKTYPE_ARCHAELOGY=22,
 LOCK_KEY_ITEM=1,LOCK_KEY_SKILL=2,LOCK_KEY_SPELL=3,MAX_LOCK_CASE=8,TYPEID_PLAYER=4,TARGET_GAMEOBJECT_ITEM_TARGET=23,
 SPELL_FAILED_LOW_CASTLEVEL=2,SPELL_FAILED_CUSTOM_ERROR=3,ARMY_UNIT_BERSERK=0,
 SPELL_CUSTOM_ERROR_REQUIRES_2_WITHERED_BERSERKER=1,SPELL_CUSTOM_ERROR_REQUIRES_AT_LEAST_10_WITHERED=2};
struct LockEntry {uint32 Type[8]{0,2},Index[8]{0,5},Skill[8]{};};
struct LockStore {LockEntry value;LockEntry const* LookupEntry(uint32 id){return id==1691?&value:nullptr;}} locks;auto& sLockStore=locks;
struct Item {uint32 GetEntry(){return 0;}};
struct LockEffect {int MiscValue=5;struct Target {uint32 GetTarget(){return 23;}} TargetA,TargetB;int CalcValue(Unit*,void*){return 0;}};
struct LockSpellInfo {uint32 Id=3365;LockEffect effect;LockEffect* GetEffect(uint32,uint32){return &effect;}};
struct Spell {
    Unit* m_caster;LockSpellInfo info;LockSpellInfo* m_spellInfo=&info;uint32 m_diffMode=0;Item* m_CastItem=nullptr;uint32 m_customError=0;
    SpellCastResult CanOpenLock(uint32,uint32,SkillType&,int32&,int32&);
};
#include "CanOpenLock.inc"

int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);uint32 type,lock,quest,close,spell,multi,noAnim;
    input>>type>>lock>>quest>>close>>spell>>multi>>noAnim;
    assert(input&&type==10&&lock==1691&&quest==39837&&close==3000&&!spell&&multi==1&&noAnim==1);
    auto& q=store.quests[39837];q.id=39837;q.PrevQuestID=38818;q.NextQuestID=38324;q.NextQuestIdChain=38339;
    q.special=QUEST_SPECIAL_FLAGS_KILL|QUEST_SPECIAL_FLAGS_CAST;q.objectives={{QUEST_OBJECTIVE_GAMEOBJECT,241778,1,0,1}};
    store.quests[38818].id=38818;
    for(auto id:{38324u,38339u}){auto& next=store.quests[id];next.id=id;next.PrevQuestID=39837;next.NextQuestID=38347;next.ExclusiveGroup=-38324;store.mExclusiveQuestGroups.emplace(-38324,id);store.starters.emplace(91531,id);}
    auto& onward=store.quests[38347];onward.id=38347;onward.PrevQuestID=38324;
    QuestMap _questTemplates;for(auto& entry:store.quests)_questTemplates[entry.first]=&entry.second;
    for(auto const& entry:_questTemplates){auto qinfo=entry.second;
#include "QuestPrevLoader.inc"
    }
    for(auto team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(38818);assert(p.CanTakeQuest(&q,false));p.Accept(39837);
        assert(Caged(&p)&&!Freed(&p));
        assert(!p.CanCompleteQuest(39837)&&!p.CanRewardQuest(&q,false));
        GameObject cage;cage.entry=241778;cage.info.type=type;cage.info.goober.questID=quest;
        // Old door type never enters the native goober credit branch.
        cage.info.type=0;cage.Use(&p);assert(!p.packets);cage.info.type=type;
        Spell opening;opening.m_caster=&p;SkillType skill=0;int32 required=0,value=0;
        // The fixture dispatches Use only after native lock validation.
        // Cast scheduling/interruption and target admission are not modeled.
        assert(opening.CanOpenLock(0,1690,skill,required,value)!=SPELL_CAST_OK);
        opening.info.effect.MiscValue=99;assert(opening.CanOpenLock(0,lock,skill,required,value)!=SPELL_CAST_OK);
        assert(!p.packets&&!cage.animations);opening.info.effect.MiscValue=5;
        assert(opening.CanOpenLock(0,lock,skill,required,value)==SPELL_CAST_OK);
        cage.Use(&p);assert(p.GetQuestObjectiveData(39837,241778)==1&&p.completions==1&&p.packets==1&&cage.animations==1&&p.CanRewardQuest(&q,false));
        assert(!Caged(&p)&&Freed(&p));
        cage.Use(&p);cage.flags=0;cage.Use(&p);assert(p.completions==1&&p.packets==1);
        // The old exploration flag blocks completion even with actual 1/1.
        Player blocked;blocked.Accept(39837);q.special|=QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT;blocked.KillCreditGO(241778,{1});assert(blocked.GetQuestObjectiveData(39837,241778)==1&&!blocked.completions);q.special&=~QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT;
        // Other players require their own click. Completion survives a login;
        // abandoning/reaccepting restarts the native objective at zero.
        Player second;second.Accept(39837);assert(!second.packets);cage.flags=0;cage.Use(&second);assert(second.completions==1);
        Player reentry;reentry.quests=p.quests;reentry.data=p.data;assert(reentry.CanRewardQuest(&q,false));
        p.quests.erase(39837);cage.flags=0;cage.Use(&p);assert(p.packets==1);p.Accept(39837);assert(!p.CanCompleteQuest(39837));
        assert(Caged(&p)&&!Freed(&p));
        GameObject other;other.info.entry=241779;other.info.goober.questID=0;other.Use(&p);assert(p.packets==1);
        Unit pet;pet.ownerPlayer=&p;GameObject petCage;petCage.Use(&pet);assert(p.packets==1);
        GameObject expired;expired.expired=true;expired.Use(&p);assert(p.packets==1);
        cage.flags=0;cage.Use(&p);assert(p.packets==2&&p.completions==2);
        for(auto id:{38324u,38339u})assert(!p.CanTakeQuest(&store.quests[id],false));
        p.m_RewardedQuests.insert(39837);p.quests.erase(39837);
        assert(!Caged(&p)&&Freed(&p));
        for(auto id:{38324u,38339u})assert(p.CanTakeQuest(&store.quests[id],false));
        Creature colborn;colborn.entry=91531;creatures[colborn.guid]=&colborn;
        assert(p.GetNextQuest(colborn.GetGUID(),&q)==&store.quests[38339]);
        // Both negative-exclusive-group tasks are required before38347.
        assert(!p.CanTakeQuest(&onward,false));p.m_RewardedQuests.insert(38324);assert(!p.CanTakeQuest(&onward,false));p.m_RewardedQuests.insert(38339);assert(p.CanTakeQuest(&onward,false));
    }
    std::cout<<"PASS: native lock1691/opening3365, full goober branch, GO credit and completion/reward/parallel handoff for both factions; old door and exploration failures, duplicate/unrelated/pet/retry/reentry boundaries. Cast timing, grids and rendering remain fixture/client limits.\n";
}
