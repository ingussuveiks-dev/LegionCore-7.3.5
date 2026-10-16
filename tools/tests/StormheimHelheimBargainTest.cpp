#include <crtdbg.h>
#include <cstdlib>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128, GAMEOBJECT_TYPE_QUESTGIVER=2;
#include "BargainFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
using ScriptedAI=SmartAI;
void Creature::GetPlayerListInGrid(std::list<Player*>& list,float range)
{for(auto const& pair:players)if(pair.second->map==map&&pair.second->GetExactDist(this)<=range)list.push_back(pair.second);}
#include "Bargain.inc"
using namespace StormheimHelheimBargain;
enum class GossipOptionNpc {None,Vendor};
constexpr int GAMEOBJECT_TYPE_GOOBER=10;
bool NativeMenuAllows(GameObject* go,GossipOptionNpc option)
{
    struct {struct {GossipOptionNpc OptionNpc;} second;} row{{option}};auto itr=&row;bool canTalk=true;
#include "NativeGoMenu.inc"
    return canTalk;
}
uint64 guid=1000;
struct Visit {
    Player p;
    Visit(uint32 team=ALLIANCE){p.guid={++guid};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;p.m_RewardedQuests.insert(38347);p.Accept(QuestId);players[p.guid]=&p;}
    ~Visit(){players.erase(p.guid);}
    void At(Position const& point){static_cast<Position&>(p)=point;}
};
void Offer(go_stormheim_helheim_bargain& ai,Visit& v)
{v.At(Altar);ai.GossipUse(&v.p);v.p.menu.menu.menu=18790;v.p.menu.menu.item=true;}
void Select(go_stormheim_helheim_bargain& ai,Visit& v){ai.GossipSelect(&v.p,18790,0);}
int main()
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    store.quests[38347].id=38347;
    auto& q=store.quests[QuestId];q.id=QuestId;q.PrevQuestID=38347;q.prevQuests={38347};q.NextQuestIdChain=39857;q.NextQuestID=39857;q.special=QUEST_SPECIAL_FLAGS_KILL;
    q.objectives={{0,97696,1,0,0},{0,97479,1,2,2}};
    auto& next=store.quests[39857];next.id=39857;next.PrevQuestID=QuestId;next.prevQuests={QuestId};store.starters.emplace(244559,39857);
    Creature marker;marker.entry=97696;marker.dbGuid=338828;marker.guid={50};marker.map=1463;marker.zone=7588;static_cast<Position&>(marker)=Throne;creatures[marker.guid]=&marker;npc_stormheim_helheim_throne throne(&marker);
    GameObject altar;altar.entry=244559;altar.dbGuid=127713;altar.guid={60};altar.guid.high=HighGuid::GameObject;altar.map=1463;altar.zone=7588;altar.info.type=2;static_cast<Position&>(altar)=Altar;objects[altar.guid]=&altar;
    go_stormheim_helheim_bargain ai(&altar);
    assert(NativeMenuAllows(&altar,GossipOptionNpc::None)&&!NativeMenuAllows(&altar,GossipOptionNpc::Vendor));
    altar.info.type=0;assert(!NativeMenuAllows(&altar,GossipOptionNpc::None));altar.info.type=2;
    for(auto team:{ALLIANCE,HORDE}){
        Visit v(team);assert(v.p.CanTakeQuest(&q,false)&&!v.p.CanCompleteQuest(QuestId)&&!v.p.CanRewardQuest(&q,false)&&!v.p.CanTakeQuest(&next,false));
        Offer(ai,v);Select(ai,v);assert(!v.p.GetQuestObjectiveData(QuestId,SpeakCredit)&&!v.p.completions); // speaking cannot precede approach
        v.At(Throne);throne.UpdateAI(500);assert(v.p.GetQuestObjectiveData(QuestId,ApproachCredit)==1&&v.p.packets==1&&!v.p.CanCompleteQuest(QuestId));
        throne.UpdateAI(500);assert(v.p.packets==1);Offer(ai,v);v.p.castResult=1;Select(ai,v);assert(v.p.completions==1&&v.p.packets==2&&v.p.CanRewardQuest(&q,false));
        assert(v.p.casts.size()==1&&v.p.casts[0]==212898&&v.p.menu.closed==1&&!v.p.menu.menu.item);
        Select(ai,v);assert(v.p.casts.size()==1&&v.p.completions==1&&v.p.packets==2);
        v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);assert(v.p.GetNextQuest(altar.guid,&q)==&next&&v.p.CanTakeQuest(&next,false));
    }
    {Visit a,b;a.At(Throne);b.At(Throne);throne.UpdateAI(500);Offer(ai,a);b.At(Altar);b.p.menu.menu.menu=18790;Select(ai,b);assert(!b.p.completions);Select(ai,a);assert(a.p.completions==1&&!b.p.completions);Offer(ai,b);Select(ai,b);assert(b.p.completions==1);}
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group;group.first=&first;a.p.group=&group;b.p.group=&group;
        a.p.RewardPlayerAndGroupAtEvent(ApproachCredit,&a.p);a.p.RewardPlayerAndGroupAtEvent(SpeakCredit,&a.p);assert(!a.p.packets&&!b.p.packets);}
    {Visit v;v.At(Throne);throne.UpdateAI(500);Offer(ai,v);v.p.Accept(QuestId);v.p.SetQuestObjectiveData(&q,&q.objectives[0],1);Select(ai,v);assert(!v.p.completions);Offer(ai,v);Select(ai,v);assert(v.p.completions==1);} // old menu cannot survive reacceptance
    {Visit v;v.At(Throne);throne.UpdateAI(500);Offer(ai,v);v.p.menu.menu.item=false;Select(ai,v);assert(!v.p.completions);Offer(ai,v);v.p.menu.menu.menu=1;Select(ai,v);assert(!v.p.completions);}
    {Visit v;for(uint32 entry:{ApproachCredit,SpeakCredit}){v.p.KilledMonsterCredit(entry);v.p.TalkedToCreature(entry,marker.guid);v.p.RewardPlayerAndGroupAtEvent(entry,&marker);}assert(!v.p.packets);Creature alias;alias.guid={999};alias.entry=123;creatures[alias.guid]=&alias;v.p.KilledMonsterCredit(ApproachCredit,alias.guid);v.p.KilledMonsterCredit(SpeakCredit,alias.guid);assert(!v.p.packets);creatures.erase(alias.guid);}
    for(int invalid=0;invalid<15;++invalid){
        Visit v;v.At(Throne);throne.UpdateAI(500);Offer(ai,v);
        if(invalid==0)v.p.alive=false;if(invalid==1)v.p.map=1220;if(invalid==2)v.p.zone=1;if(invalid==3)v.p.teleport=true;
        if(invalid==4)v.p.vehicle=true;if(invalid==5)v.p.transport=true;if(invalid==6)v.p.combat=true;if(invalid==7)v.p.flying=true;
        if(invalid==8)v.p.phase=false;if(invalid==9)v.p.los=false;if(invalid==10)v.p.visible=false;if(invalid==11)v.p.x+=20;
        if(invalid==12)v.p.phases.insert(8306);if(invalid==13)v.p.quests.erase(QuestId);if(invalid==14)altar.flags=GO_FLAG_NOT_SELECTABLE;
        Select(ai,v);assert(!v.p.completions&&v.p.casts.empty());altar.flags=0;
    }
    {Visit v;v.At(Throne);v.p.combat=true;throne.UpdateAI(500);assert(!v.p.packets);v.p.combat=false;throne.UpdateAI(500);assert(v.p.packets==1);} // stationary retry
    {Visit v;v.At(Throne);marker.dbGuid=12743040;throne.UpdateAI(500);assert(!v.p.packets);marker.dbGuid=340857;throne.UpdateAI(500);assert(v.p.packets==1);marker.dbGuid=338828;throne.UpdateAI(500);assert(v.p.packets==1);}
    {Visit v;v.At(Throne);marker.alive=false;throne.UpdateAI(500);assert(!v.p.packets);marker.alive=true;v.p.phase=false;throne.UpdateAI(500);assert(!v.p.packets);v.p.phase=true;throne.UpdateAI(500);Offer(ai,v);altar.dbGuid=25210114;Select(ai,v);assert(!v.p.completions);altar.dbGuid=127713;Select(ai,v);assert(v.p.completions==1);}
    {Visit v;v.p.SetQuestObjectiveData(&q,&q.objectives[0],1);Offer(ai,v);Select(ai,v);assert(v.p.completions==1);} // restored approach counter needs a fresh actual menu
    {Visit v;v.At(Throne);q.objectives[0].Flags=1;throne.UpdateAI(500);assert(!v.p.packets);q.objectives[0].Flags=0;throne.UpdateAI(500);assert(v.p.packets==1);}
    {Visit v;auto delegated=ai.delegated;ai.GossipSelect(&v.p,18790,1);ai.GossipSelect(&v.p,555,0);assert(ai.delegated==delegated+2&&!v.p.packets);} // Lifeblood link seam is stubbed here; real option1 interception is in Test-StormheimLifeblood.
    {Visit v;auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,ApproachCredit,1,0,0}};v.p.Accept(999);v.p.KilledMonsterCredit(ApproachCredit);assert(v.p.GetQuestObjectiveData(999,ApproachCredit)==1&&!v.p.GetQuestObjectiveData(QuestId,ApproachCredit));}
    std::cout<<"PASS: production bargain approach and actual altar menu/lifetime selection, two required native objectives, both factions/owners, one-shot and alias/group/talk guards, stationary retry, failure/cancel/reentry boundaries, optional conversation failure, native next39857 and unrelated option delegation (Lifeblood link seam stubbed here). World, gossip preparation, casts and networking are fixtures; client checks pending.\n";
}
