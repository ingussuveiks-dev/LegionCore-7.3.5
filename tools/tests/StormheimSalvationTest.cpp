#include <crtdbg.h>
#include <fstream>
#include <sstream>
#include <algorithm>
constexpr int TYPEID_UNIT=0,TYPEID_ITEM=1,TYPEID_CONTAINER=2,TYPEID_PLAYER=4,TYPEID_GAMEOBJECT=5;
constexpr int GAMEOBJECT_TYPE_QUESTGIVER=2,INVALIDREASON_QUEST_ALREADY_ON=1,DISABLE_TYPE_QUEST=0;
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128,MAX_KILL_CREDIT=2,QUEST_OBJECTIVE_FLAG_TRACKED_ON_MINIMAP=1;
constexpr int UNIT_FIELD_NPC_FLAGS2=16,UNIT_FIELD_FLAGS_2=17,UNIT_FLAG2_ALLOW_ENEMY_INTERACT=0x4000;
constexpr int CREATURE_TYPEFLAGS_GHOST=2,CREATURE_TYPEFLAGS_DEAD_INTERACT=0x80,REP_UNFRIENDLY=1;
constexpr float INTERACTION_DISTANCE=5;
#include "SalvationFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
namespace DisableMgr{bool IsDisabledFor(int,uint32,Player*){return false;}}
struct FactionEntry{bool CanHaveReputation()const{return false;}};
struct FactionTemplateEntry{uint32 Faction=148;};
struct FactionStore{FactionEntry row;FactionEntry* LookupEntry(uint32){return &row;}} sFactionStore;
struct FactionTemplateStore{FactionTemplateEntry row;FactionTemplateEntry* LookupEntry(uint32){return &row;}} sFactionTemplateStore;
namespace ObjectAccessor{Object* GetObjectByTypeMask(Player& p,ObjectGuid g,int){return GetCreature(p,g);}}
constexpr int TYPEMASK_UNIT=1,TYPEMASK_GAMEOBJECT=2;
struct Item:WorldObject{struct Template{uint32 GetMaxCount(){return 0;}} info;Template* GetTemplate(){return &info;}uint32 GetBagSlot(){return 0;}uint32 GetSlot(){return 0;}};
struct ScriptMgr{template<class T>void OnQuestAccept(Player*,T*,Quest const*){}} scriptMgr;auto sScriptMgr=&scriptMgr;
#include "CanTake.inc"
#include "Status.inc"
#include "Accept.inc"
#include "Interact.inc"
bool WorldObject::hasInvolvedQuest(uint32 q){return GetEntry()==97988&&(q==40005||q==40072);}
namespace WorldPackets{namespace Quest{struct QuestGiverRequestReward{ObjectGuid QuestGiverGUID;uint32 QuestID;};}}
struct WorldSession{Player* _player;void HandleQuestGiverRequestReward(WorldPackets::Quest::QuestGiverRequestReward&);};
#include "RequestReward.inc"

#include "NativeKilledMonster.inc"
struct KillRewarder {Group* _group=nullptr;Unit* _victim=nullptr;void _RewardKillCredit(Player*);void _RewardPlayer(Player* p,bool){_RewardKillCredit(p);}void SelectGroup(){bool isDungeon=false;
#include "NativeGroupSelection.inc"
}};
#include "NativeKillReward.inc"
using Difficulty=unsigned;
struct CreatureSpell {std::vector<Difficulty> Difficulties;bool CanUseInDifficulty(Difficulty)const;};
#include "NativeSpellDifficulty.inc"
std::pair<Player*,bool> ResolveKiller(Unit& attacker,Unit* victim)
{
    auto GetCharmerOrOwnerPlayerOrPlayerItself=[&]{return attacker.GetCharmerOrOwnerPlayerOrPlayerItself();};
#include "KillOwner.inc"
    return {player,isRewardAllowed};
}
void Deliver(Player& p){auto events=std::move(p.events);p.events.clear();for(auto& f:events)f();}

#include "Salvation.inc"
void Setup(Player& p,Creature& c,bool returning)
{
    p.Accept(40005);if(returning)p.quests[40005].Status=QUEST_STATUS_COMPLETE;
    p.menu.menu.menu=returning?18852:18843;
    c.entry=returning?98190:97986;c.dbGuid=returning?341218:338656;c.guid={c.dbGuid};c.fields[UNIT_FIELD_NPC_FLAGS]=1;
    static_cast<Position&>(c)=returning?StormheimSalvation::Arrival:StormheimSalvation::Departure;
    static_cast<Position&>(p)=c;creatures[c.guid]=&c;
}
int main(int argc,char** argv)
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    assert(argc==2);std::ifstream input(argv[1]);assert(input);std::string line;unsigned objectives=0;
    while(std::getline(input,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="quest"){uint32 id;in>>id;auto& q=store.quests[id];q.id=id;in>>q.flags>>q.AllowableRaces>>q.NextQuestIdChain>>q.PrevQuestID>>q.NextQuestID>>q.ExclusiveGroup>>q.special;if(q.PrevQuestID)q.prevQuests={q.PrevQuestID};if(q.ExclusiveGroup)store.mExclusiveQuestGroups.emplace(q.ExclusiveGroup,id);}
        if(kind=="objective"){uint32 id,quest;int index;QuestObjective o;in>>id>>quest>>o.Type>>o.ObjectID>>o.Amount>>index>>o.Flags;o.StorageIndex=int8(index);store.quests[quest].objectives.push_back(o);++objectives;}
        if(kind=="starter"){uint32 actor,quest;in>>actor>>quest;store.starters.emplace(actor,quest);}
    }
    auto& q=store.quests[40005];q.special|=QUEST_SPECIAL_FLAGS_KILL;
    assert(objectives==2&&StormheimSalvation::Definition());
    for(auto team:{ALLIANCE,HORDE}){
        Player p;p.team=team;p.race=team==ALLIANCE?1:2;
        assert(!p.CanTakeQuest(&store.quests[40003],false)&&!p.CanTakeQuest(&store.quests[40004],false));
        p.Accept(40002);p.CompleteQuest(40002);assert(!p.CanTakeQuest(&store.quests[40004],false));
        p.m_RewardedQuests.insert(40002);assert(p.CanTakeQuest(&store.quests[40003],false)&&p.CanTakeQuest(&store.quests[40004],false));
        for(unsigned first=0;first<4;++first)for(unsigned second=0;second<4;++second){
            Player candidate;candidate.team=team;candidate.race=p.race;
            unsigned states[2]={first,second},ids[2]={40003,40004};
            for(unsigned i=0;i<2;++i){if(states[i])candidate.Accept(ids[i]);if(states[i]>=2)candidate.CompleteQuest(ids[i]);if(states[i]==3)candidate.m_RewardedQuests.insert(ids[i]);}
            assert(candidate.CanTakeQuest(&q,false)==(first==3&&second==3));
        }
        for(bool returning:{false,true})for(int mode=0;mode<25;++mode){
            Player caller;caller.team=team;caller.race=p.race;Creature c;Setup(caller,c,returning);npc_stormheim_salvation_vethir ai(&c);
            unsigned sender=returning?18852:18843,action=0;
            switch(mode){
             case 1:caller.quests.clear();break;case 2:caller.quests[40005].Status=returning?QUEST_STATUS_INCOMPLETE:QUEST_STATUS_COMPLETE;break;
             case 3:caller.alive=false;break;case 4:c.alive=false;break;case 5:caller.world=false;break;case 6:c.world=false;break;
             case 7:caller.teleport=true;break;case 8:caller.combat=true;break;case 9:caller.flying=true;break;case 10:caller.vehicle=true;break;case 11:caller.transport=true;break;
             case 12:caller.phase=false;break;case 13:caller.visible=false;break;case 14:caller.los=false;break;case 15:caller.map=1;break;case 16:c.zone=1;break;
             case 17:c.x+=2;break;case 18:c.fields[UNIT_FIELD_NPC_FLAGS]=0;break;case 19:caller.menu.menu.item=false;break;case 20:caller.menu.menu.menu=1;break;
             case 21:sender=1;break;case 22:action=1;break;case 23:caller.phases.insert(8306);break;case 24:caller.x+=30;break;
            }
            ai.sGossipSelect(&caller,sender,action);assert(caller.teleportAttempts==unsigned(mode==0)&&!caller.packets&&!caller.completions);
            if(!mode){auto const& target=returning?StormheimSalvation::Return:StormheimSalvation::Arrival;assert(caller.destination.GetExactDist(&target)<0.001f&&caller.teleports==1);ai.sGossipSelect(&caller,sender,action);assert(caller.teleportAttempts==1);caller.teleport=false;ai.sGossipSelect(&caller,sender,action);assert(caller.teleportAttempts==1);}
            creatures.erase(c.guid);
        }
        for(bool returning:{false,true}){
            Player caller;Creature c;Setup(caller,c,returning);npc_stormheim_salvation_vethir ai(&c);caller.teleportSuccess=false;
            ai.sGossipSelect(&caller,returning?18852:18843,0);assert(caller.teleportAttempts==1&&!caller.teleports&&!caller.packets);caller.teleportSuccess=true;
            ai.sGossipSelect(&caller,returning?18852:18843,0);assert(caller.teleports==1);creatures.erase(c.guid);
        }
        for(bool petBlow:{false,true}){
            Player fighter;fighter.team=team;fighter.race=p.race;fighter.m_RewardedQuests={40003,40004};assert(fighter.CanTakeQuest(&q,false));
            Creature departure;Setup(fighter,departure,false);npc_stormheim_salvation_vethir departureAI(&departure);
            if(petBlow)fighter.KilledMonsterCredit(103766);assert(!fighter.CanCompleteQuest(40005));
            auto before=fighter.packets;departureAI.sGossipSelect(&fighter,18843,0);assert(fighter.teleports==1&&fighter.packets==before&&!fighter.GetQuestObjectiveData(40005,92307));fighter.teleport=false;
            Creature boss;boss.entry=92307;boss.data.Entry=92307;boss.guid={291417};creatures[boss.guid]=&boss;
            Unit pet;pet.ownerPlayer=&fighter;auto killer=ResolveKiller(petBlow?pet:static_cast<Unit&>(fighter),&boss);assert(killer.first==&fighter&&killer.second);
            KillRewarder reward;reward._victim=&boss;reward._RewardKillCredit(killer.first);assert(fighter.delays.back()==50&&!fighter.GetQuestObjectiveData(40005,92307));Deliver(fighter);
            assert(fighter.GetQuestObjectiveData(40005,92307)==1&&fighter.CanRewardQuest(&q,false)&&fighter.completions==1);auto packets=fighter.packets;fighter.KilledMonster(&boss.data,boss.guid);assert(fighter.packets==packets);
            Creature returning;returning.entry=98190;returning.dbGuid=341218;returning.guid={341218};returning.fields[UNIT_FIELD_NPC_FLAGS]=1;static_cast<Position&>(returning)=StormheimSalvation::Arrival;creatures[returning.guid]=&returning;
            static_cast<Position&>(fighter)=returning;fighter.menu.menu.menu=18852;npc_stormheim_salvation_vethir returnAI(&returning);returnAI.sGossipSelect(&fighter,18852,0);assert(fighter.teleports==2&&fighter.packets==packets);fighter.teleport=false;
            Creature havi;havi.entry=97988;havi.guid={338669};havi.fields[UNIT_FIELD_NPC_FLAGS]=2;havi.x=2497.89f;havi.y=956.938f;havi.z=241.529f;creatures[havi.guid]=&havi;
            WorldSession session{&fighter};WorldPackets::Quest::QuestGiverRequestReward request{havi.guid,40005};session.HandleQuestGiverRequestReward(request);assert(fighter.menu.offers==1);
            assert(fighter.GetNextQuest(havi.guid,&q)==&store.quests[40072]);assert(!fighter.CanTakeQuest(&store.quests[40072],false));fighter.quests.clear();fighter.m_RewardedQuests.insert(40005);assert(fighter.CanTakeQuest(&store.quests[40072],false));
            creatures.erase(havi.guid);creatures.erase(boss.guid);creatures.erase(departure.guid);creatures.erase(returning.guid);
        }
        {Player fighter;fighter.Accept(40005);fighter.KilledMonsterCredit(92307);assert(fighter.completions==1&&fighter.GetQuestObjectiveData(40005,103766)==0);} // Optional click credit cannot gate the real kill.
    }
    {Player p;Creature c;Setup(p,c,false);npc_stormheim_salvation_vethir ai(&c);c.dbGuid=999;ai.sGossipSelect(&p,1,1);assert(ai.delegated==1&&!p.teleports);}
    {Player p;Creature c;Setup(p,c,false);npc_stormheim_salvation_vethir ai(&c);q.objectives[0].Amount=2;ai.sGossipSelect(&p,18843,0);assert(!p.teleports);q.objectives[0].Amount=1;}
    std::cout<<"PASS: production two-way Vethir guards; native each-from-all prerequisite states, player/pet kill callback/completion, optional counter independence, Havi reward and40072 handoff. Both factions, stale menu/state,25 travel branches, duplicate/failed teleport and other spawn delegation; fixture world/storage/teleport/clock/network, client pending.\n";
}
