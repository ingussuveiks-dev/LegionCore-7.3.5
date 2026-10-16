#include <crtdbg.h>
#include <fstream>
#include <sstream>
struct NativeEvents;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,GAMEOBJECT_TYPE_QUESTGIVER=2;
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_GAMEOBJECT=5;
constexpr int MAX_KILL_CREDIT=2,SMART_EVENT_DEATH=6,SMART_ACTION_SUMMON_CONVERSATION=206;
constexpr int INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int SMART_ESCORT_ESCORTING=1,DONE=2,ENCOUNTER_FRAME_DISENGAGE=3,ENCOUNTER_FRAME_INSTANCE_END=4;
struct DeathInstance {void SetBossState(int,int){}void SaveToDB(){}template<class T>void SendEncounterUnit(int,T*){}};
#include "VictoryFixture.inc"
namespace DisableMgr {bool IsDisabledFor(int,uint32,Player*){return false;}}
#include "CanTake.inc"
#include "Status.inc"
#include "Talked.inc"
#include "NativeKilledMonster.inc"
#include "NativeConversationStructs.inc"
struct ConversationStore {
    std::vector<ConversationData> lines;std::vector<ConversationActor> actors;
    auto GetConversationData(uint32 id){assert(id==2256);return &lines;}
    std::vector<ConversationCreature> const* GetConversationCreature(uint32 id){assert(id==2256);return nullptr;}
    auto GetConversationActor(uint32 id){assert(id==2256);return &actors;}
} conversationStore;auto sConversationDataStore=&conversationStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct Conversation;std::vector<Conversation*> created;bool failConversation=false;uint32 attempts=0;
struct Conversation {
    std::map<uint8,ConversationDynamicFieldActors> actors;std::map<uint8,ConversationDynamicFieldLines> lines;Player* owner=nullptr;uint32 duration=0;
    float GetPositionX(){return 0;}float GetPositionY(){return 0;}void SetActor(ConversationDynamicFieldActors* p,uint8 i,bool){actors[i]=*p;}void SetLine(ConversationDynamicFieldLines* p,uint8 i,bool){lines[i]=*p;}void SetCreature(ConversationDynamicFieldCreatures*,uint8,bool){assert(false);}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool CreateConversation(uint64,uint32 id,Unit* caster,SpellInfo const*,Position const&){++attempts;assert(caster&&caster->ToPlayer());owner=caster->ToPlayer();if(failConversation||!GenerateDynamicFieldData(id,caster,duration))return false;created.push_back(this);return true;}
};
#include "NativeConversationData.inc"
struct Generator {uint64 id=0;uint64 Generate(){return ++id;}} generator;
struct ObjectMgrFixture {template<HighGuid>Generator* GetGenerator(){return &generator;}std::string GetScriptName(uint32 id){return id==2?"npc_stormheim_geir":"custom";}} objectMgr;auto sObjectMgr=&objectMgr;
using ObjectList=std::vector<WorldObject*>;
struct Event {struct {struct {uint32 id=2256,targetX=0,targetY=0,targetZ=0;} conversation;} action;};
struct NativeEvents {
    WorldObject* base=nullptr;uint32 callbacks=0;Unit* last=nullptr;Event e;
    WorldObject* GetBaseObject(){return base;}
    ObjectList* GetTargets(Event const&,Unit* unit){return new ObjectList{unit};} // Installed target7.
    void ProcessEventsFor(uint32 type,Unit* unit){assert(type==SMART_EVENT_DEATH);++callbacks;last=unit;switch(SMART_ACTION_SUMMON_CONVERSATION){
#include "NativeConversationAction.inc"
    }}
} nativeEvents;
NativeEvents* SmartAI::GetScript(){return &nativeEvents;}
#include "NativeSmartDeath.inc"
#include "Geir.inc"
struct KillRewarder {Group* _group=nullptr;Unit* _victim=nullptr;void _RewardKillCredit(Player*);void _RewardPlayer(Player* p,bool){_RewardKillCredit(p);}void SelectGroup(){bool isDungeon=false;
#include "NativeGroupSelection.inc"
}};
#include "NativeKillReward.inc"
std::pair<Player*,bool> ResolveKiller(Unit& attacker,Unit* victim)
{
    auto GetCharmerOrOwnerPlayerOrPlayerItself=[&]{return attacker.GetCharmerOrOwnerPlayerOrPlayerItself();};
#include "KillOwner.inc"
    return {player,isRewardAllowed};
}
void CreditAfter50(Player& p){auto events=std::move(p.events);p.events.clear();for(auto& f:events)f();}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);assert(argc==2);
    std::ifstream input(argv[1]);std::string line;uint32 sai=0;
    while(std::getline(input,line)){std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="actor"){ConversationActor a{};in>>a.entry>>a.id>>a.actorId>>a.creatureId>>a.displayId>>a.unk1>>a.unk2>>a.unk3>>a.duration;conversationStore.actors.push_back(a);}
        if(kind=="line"){ConversationData d{};in>>d.entry>>d.id>>d.idx>>d.textId>>d.unk1>>d.unk2;conversationStore.lines.push_back(d);}
        if(kind=="death"){uint32 event,action,id,target;in>>event>>action>>id>>target;assert(event==6&&action==206&&id==2256&&target==7);++sai;}
    }
    assert(sai==1&&conversationStore.actors.size()==1&&conversationStore.lines.size()==1);
    auto& q=store.quests[39853];q.id=39853;q.special=QUEST_SPECIAL_FLAGS_KILL;q.PrevQuestID=39850;q.prevQuests={39850};q.NextQuestID=39855;q.NextQuestIdChain=39855;q.objectives={{0,91948,1,0,1}};
    for(uint32 id:{39850u,39851u}){auto& p=store.quests[id];p.id=id;p.ExclusiveGroup=-39850;store.mExclusiveQuestGroups.emplace(-39850,id);}
    auto& next=store.quests[39855];next.id=39855;next.PrevQuestID=39853;next.prevQuests={39853};next.objectives={{0,97487,1,0,0}};store.starters.emplace(97480,39855);store.starters.emplace(97558,39855);
    Creature geir;geir.entry=91948;geir.dbGuid=11546719;geir.guid={11546719};geir.map=1463;geir.zone=7588;geir.home={209.854f,587.158f,-4.0533f,1.39828f};static_cast<Position&>(geir)=geir.home;creatures[geir.guid]=&geir;nativeEvents.base=&geir;npc_stormheim_geir ai(&geir);
    Creature giver;giver.guid={97480};giver.entry=97480;creatures[giver.guid]=&giver;
    KillRewarder reward;reward._victim=&geir;
    for(auto team:{ALLIANCE,HORDE})for(bool petKill:{false,true})for(int failure=0;failure<3;++failure){
        Player p;p.guid={1000+team+uint32(petKill)*10+uint32(failure)};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;static_cast<Position&>(p)=geir.home;
        assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39850);assert(!p.CanTakeQuest(&q,false));p.m_RewardedQuests.insert(39851);assert(p.CanTakeQuest(&q,false));p.Accept(39853);assert(!p.CanCompleteQuest(39853)&&!p.CanRewardQuest(&q,false));
        Unit pet;pet.ownerPlayer=&p;auto& killer=petKill?pet:static_cast<Unit&>(p);auto resolved=ResolveKiller(killer,&geir);assert(resolved.first==&p&&resolved.second);
        reward._RewardKillCredit(&p);assert(p.delays==std::vector<uint64>{50}&&!p.GetQuestObjectiveData(39853,91948));
        auto count=attempts;auto saved=conversationStore.actors;failConversation=failure==1;if(failure==2)conversationStore.actors.clear();
        geir.x+=20;ai.JustDied(&killer);geir.x-=20;assert(attempts==count+1&&nativeEvents.last==&p&&!p.GetQuestObjectiveData(39853,91948)); // Home identity survives combat chasing; credit is still delayed.
        if(!failure){auto c=created.back();assert(c->owner==&p&&c->duration==6422&&c->actors[0].displayId==65043&&c->lines[0].id==4752);}
        conversationStore.actors=saved;CreditAfter50(p);assert(p.GetQuestObjectiveData(39853,91948)==1&&p.completions==1&&p.packets==1&&p.CanRewardQuest(&q,false));
        p.KilledMonster(geir.GetCreatureTemplate(),geir.guid);assert(p.packets==1);assert(!p.CanTakeQuest(&next,false));p.m_RewardedQuests.insert(39853);p.quests.erase(39853);assert(p.CanTakeQuest(&next,false)&&p.GetNextQuest(giver.guid,&q)==&next);
        giver.entry=97558;assert(p.GetNextQuest(giver.guid,&q)==&next);giver.entry=92539;assert(!p.GetNextQuest(giver.guid,&q));giver.entry=97480;
        p.Accept(39855);assert(!p.CanCompleteQuest(39855)&&!p.CanRewardQuest(&next,false));
    }
    failConversation=false;
    {Player owner;owner.map=1463;owner.zone=7588;owner.Accept(39853);Unit pet;pet.ownerPlayer=&owner;SmartAI old(&geir);auto count=attempts;old.JustDied(&pet);assert(attempts==count);geir.ai=&ai;} // Actual previous action206 rejects a pet target.
    for(int boundary=0;boundary<8;++boundary){Player owner;owner.map=1463;owner.zone=7588;owner.Accept(39853);Unit pet;pet.ownerPlayer=&owner;
        if(boundary==0)owner.world=false;if(boundary==1)owner.map=1220;if(boundary==2)owner.phases.insert(8306);if(boundary==3)owner.phase=false;if(boundary==4)owner.rewardRange=false;if(boundary==5)owner.quests.erase(39853);if(boundary==6)geir.dbGuid=12742855;if(boundary==7)geir.home.x+=2;
        auto count=attempts;ai.JustDied(&pet);assert(attempts==count&&nativeEvents.last==&pet);geir.dbGuid=11546719;geir.home={209.854f,587.158f,-4.0533f,1.39828f};}
    {Unit ambient;auto count=attempts;ai.JustDied(&ambient);assert(attempts==count&&nativeEvents.last==&ambient);}
    {Player p;p.Accept(39853);reward._RewardKillCredit(&p);p.quests.erase(39853);CreditAfter50(p);assert(!p.packets&&!p.completions);} // Abandon before delayed native credit.
    {Player killer,nearby,far,released,disconnected;for(auto p:{&killer,&nearby,&far,&released,&disconnected})p->Accept(39853);far.rewardRange=false;released.alive=false;released.corpse=true;disconnected.contact=false;
        GroupReference e{&disconnected,nullptr},d{&released,&e},c{&far,&d},b{&nearby,&c},a{&killer,&b};Group group;group.first=&a;reward._group=&group;reward.SelectGroup();
        for(auto p:{&killer,&nearby,&far,&released,&disconnected})CreditAfter50(*p);assert(killer.completions==1&&nearby.completions==1&&!far.completions&&!released.completions&&!disconnected.completions);reward._group=nullptr;}
    for(auto c:created)delete c;
    std::cout<<"PASS: previous native action206 pet rejection reproduced; production Geir owner handoff -> full native SmartAI death/action206/conversation fields, real50ms kill-credit callback/KilledMonster/completion and both Ashildir39855 relations. Both factions, direct/pet blows, combat displacement, optional failures, scope boundaries, repeat cap and abandon-before-credit passed. World, delayed clock, combat/reward selection and network are fixtures; client checks pending.\n";
}
