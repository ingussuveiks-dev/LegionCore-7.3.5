#include <mutex>
#include <crtdbg.h>
#include <cstdlib>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128;
#include "HelheimDisguiseFixture.inc"
#include "Talked.inc"
struct EventObjectTemplate {uint32 ScriptID=3,SpellID=0,WorldSafeLocID=0;float radius=4;};
struct EventStore {EventObjectTemplate row;EventObjectTemplate const* GetEventObjectTemplate(uint32 id){return id==334?&row:nullptr;}} eventStore;auto sEventObjectDataStore=&eventStore;
struct Templates {CreatureTemplate actor;CreatureTemplate const* GetCreatureTemplate(uint32 id){return id==97419?&actor:nullptr;}std::string GetScriptName(uint32 id){return id==2?"npc_stormheim_helheim_ashildir":id==3?"eventobject_stormheim_helheim_passage":"custom";}} templates;auto sObjectMgr=&templates;
struct EventObject:WorldObject {uint64 dbGuid=400326;uint64 GetDBTableGUIDLow(){return dbGuid;}} exitMarker;
struct EventObjectScript {explicit EventObjectScript(char const*){}virtual bool IsTriggerMeets(Player*,EventObject*){return true;}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
#include "HelheimDisguise.inc"
using namespace StormheimHelheimDisguise;
uint64 nextGuid=80000;
void Apply(Unit* target,uint32 id){target->roots[{id,target->GetGUID().value}]={};if(auto p=target->ToPlayer())p->auras.insert(id);}
struct Visit {
    Player p;player_stormheim_helheim_disguise script;eventobject_stormheim_helheim_passage exit;
    bool failDisguise=false,failReaction=false,failSummon=false,failVisual=false;
    std::vector<std::unique_ptr<Creature>> actors;std::vector<std::unique_ptr<npc_stormheim_helheim_ashildir>> ais;
    Visit(uint32 team=ALLIANCE){
        p.guid={++nextGuid};p.team=team;p.race=team==ALLIANCE?1:2;p.map=1463;p.zone=7588;p.area=7588;players[p.guid]=&p;p.Accept(QuestId);
        p.x=366.422f;p.y=369.746f;p.z=27.5921f;
        p.castTargetHook=[&](Unit* target,uint32 id){if((id==Disguise&&failDisguise)||(id==Reaction&&failReaction))return 1;Apply(target,id);if(id==Disguise&&!failReaction)Apply(target,Reaction);return 0;};
        p.judgmentSummon=[&](uint32 id,Position const& point,ObjectGuid viewer)->Creature*{
            assert(id==97419&&viewer==p.guid);if(failSummon)return nullptr;
            auto c=std::make_unique<Creature>();c->entry=id;c->guid={++nextGuid};c->map=1463;c->zone=7588;static_cast<Position&>(*c)=point;creatures[c->guid]=c.get();
            c->castTargetHook=[&](Unit* target,uint32 id){if(failVisual)return 1;Apply(target,id);return 0;};
            auto ai=std::make_unique<npc_stormheim_helheim_ashildir>(c.get());auto result=c.get();actors.push_back(std::move(c));ais.push_back(std::move(ai));result->AI()->IsSummonedBy(&p);return result;
        };
    }
    ~Visit(){script.OnLogout(&p);for(auto const& c:actors)creatures.erase(c->guid);players.erase(p.guid);}
    void Tick(uint32 n=500){script.OnUpdate(&p,n);}
    void Approach(){p.x=455.686f;p.y=500;p.z=5.13701f;Tick();}
    void Exit(){static_cast<Position&>(p)=ExitPoint;assert(exit.IsTriggerMeets(&p,&exitMarker));exit.OnTrigger(&p,&exitMarker,true);Tick();}
    Creature* Spirit(){auto it=runs.find(p.guid);return it==runs.end()?nullptr:p.GetMap()->GetCreature(it->second.spirit);}
};
int main()
{
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.NextQuestIdChain=NextQuest;q.NextQuestID=NextQuest;q.objectives={{QUEST_OBJECTIVE_MONSTER,91386,1,0,1}};
    auto& next=store.quests[NextQuest];next.id=NextQuest;next.PrevQuestID=QuestId;next.prevQuests={QuestId};store.starters.emplace(97419,NextQuest);
    Creature guard;guard.entry=91386;guard.dbGuid=11546644;guard.guid={100};guard.map=1463;guard.zone=7588;static_cast<Position&>(guard)=GuardPoint;creatures[guard.guid]=&guard;
    exitMarker.entry=334;exitMarker.guid={200};exitMarker.map=1463;exitMarker.zone=7588;static_cast<Position&>(exitMarker)=ExitPoint;eventObjects[exitMarker.guid]=&exitMarker;
    for(auto team:{ALLIANCE,HORDE}){
        Visit v(team);v.Tick();assert(v.Spirit()&&v.Spirit()->personal==v.p.guid&&v.Spirit()->motion.target==&v.p);
        assert(v.p.GetAura(Disguise,v.p.guid)&&v.p.GetAura(Reaction,v.p.guid)&&!v.p.completions);
        v.p.KilledMonsterCredit(91386);v.p.TalkedToCreature(91386,guard.guid);v.p.RewardPlayerAndGroupAtEvent(91386,&guard);assert(!v.p.GetQuestObjectiveData(QuestId,91386));
        v.Approach();assert(runs[v.p.guid].approached&&!v.p.completions);v.p.x=GuardPoint.x;v.p.y=GuardPoint.y;v.Tick();assert(!v.p.completions);
        v.Exit();assert(v.p.completions==1&&v.p.packets==1&&v.p.GetQuestObjectiveData(QuestId,91386)==1&&v.p.CanRewardQuest(&q,false));
        v.exit.OnTrigger(&v.p,&exitMarker,true);v.Tick();assert(v.p.completions==1&&v.p.packets==1);
        auto spirit=v.Spirit();v.p.m_RewardedQuests.insert(QuestId);v.p.quests.erase(QuestId);v.Tick(10000);assert(v.Spirit()==spirit&&spirit->world&&v.p.GetNextQuest(spirit->guid,&q)==&next);
        v.p.Accept(NextQuest);v.Tick();assert(!spirit->world&&!v.p.GetAura(Disguise,v.p.guid)&&!v.p.GetAura(Reaction,v.p.guid));
    }
    {Visit a,b;a.Approach();b.Approach();assert(a.Spirit()!=b.Spirit());a.Exit();assert(a.p.completions==1&&!b.p.completions);b.Exit();assert(b.p.completions==1);}
    {Visit v;v.Tick();v.Exit();assert(!v.p.completions);v.Approach();v.Exit();assert(v.p.completions==1);} // no approach/no port-to-exit credit
    {Visit v;v.failSummon=true;v.failVisual=true;v.Approach();assert(!v.Spirit());v.Exit();assert(v.p.completions==1);v.failSummon=false;v.Tick();assert(v.Spirit());} // optional actor/visual never gate passage
    {Visit v;v.failDisguise=true;v.Approach();v.Exit();assert(!v.p.completions);v.failDisguise=false;v.Tick();assert(!runs[v.p.guid].approached);v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.failReaction=true;v.Approach();v.Exit();assert(!v.p.completions);v.failReaction=false;v.Tick();v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Approach();v.p.roots.erase({Disguise,v.p.guid.value});v.Exit();assert(!v.p.completions&&!runs[v.p.guid].approached);v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Approach();v.p.Accept(QuestId);v.Exit();assert(!v.p.completions);v.Approach();v.Exit();assert(v.p.completions==1);} // new acceptance lifetime
    for(int failure=0;failure<10;++failure){
        Visit v;v.Approach();auto spirit=v.Spirit();
        if(failure==0)v.p.alive=false;if(failure==1)v.p.map=1220;if(failure==2)v.p.zone=1;if(failure==3)v.p.teleport=true;
        if(failure==4)v.p.vehicle=true;if(failure==5)v.p.transport=true;if(failure==6)v.p.quests.erase(QuestId);
        if(failure==7)v.p.phase=false;if(failure==8)v.p.combat=true;
        if(failure==9)v.p.flying=true;
        v.Tick();assert(!v.p.completions);
        if(failure<7||failure==9)assert(!spirit->world&&!v.p.GetAura(Disguise,v.p.guid));
        if(failure==7||failure==8)assert(!runs[v.p.guid].approached);
    }
    {Visit v;guard.dbGuid=12743029;v.Approach();v.Exit();assert(!v.p.completions);guard.dbGuid=11546644;v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Approach();exitMarker.dbGuid=999;static_cast<Position&>(v.p)=ExitPoint;assert(!v.exit.OnTrigger(&v.p,&exitMarker,true)&&!v.p.completions);exitMarker.dbGuid=400326;v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Approach();guard.alive=false;v.Exit();assert(!v.p.completions);guard.alive=true;v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Approach();v.p.los=false; // an already valid approach remains; lack of LOS cannot arm a fresh one
        runs[v.p.guid].approached=false;v.Tick();assert(!runs[v.p.guid].approached);v.p.los=true;v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;v.Tick();auto old=v.Spirit();old->alive=false;v.Tick();assert(!old->world&&v.Spirit()!=old);}
    {Visit v;v.Tick();Creature foreign;foreign.entry=97419;npc_stormheim_helheim_ashildir ai(&foreign);ai.IsSummonedBy(&v.p);assert(!ai.owned&&ai.delegated==1);ai.UpdateAI(1);assert(ai.delegated==2);}
    {Visit v;v.Tick();for(uint32 id:{Disguise,Reaction,192299u})v.p.roots[{id,999}]={};v.script.OnMapChanged(&v.p);for(uint32 id:{Disguise,Reaction,192299u})assert(v.p.GetAura(id,{999})&&!v.p.GetAura(id,v.p.guid));}
    {Visit v;Apply(&v.p,192299);v.script.OnLogin(&v.p);assert(!v.p.GetAura(192299,v.p.guid));v.Approach();v.Exit();auto old=v.Spirit();v.script.OnLogout(&v.p);assert(!old->world);v.script.OnLogin(&v.p);v.Tick();assert(v.Spirit()&&v.p.completions==1&&v.p.CanRewardQuest(&q,false));}
    {Visit v;v.Approach();v.script.OnLogout(&v.p);v.script.OnLogin(&v.p);v.Exit();assert(!v.p.completions);v.Approach();v.Exit();assert(v.p.completions==1);}
    {Visit v;auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{QUEST_OBJECTIVE_MONSTER,91386,1,0,0}};v.p.Accept(999);v.p.KilledMonsterCredit(91386);assert(v.p.GetQuestObjectiveData(999,91386)==1&&!v.p.GetQuestObjectiveData(QuestId,91386));}
    {Visit v;Creature alias;alias.guid={998};alias.entry=123;creatures[alias.guid]=&alias;v.p.KilledMonsterCredit(91386,alias.guid);assert(!v.p.GetQuestObjectiveData(QuestId,91386));creatures.erase(alias.guid);}
    {Visit v;templates.actor.ScriptID=99;v.Tick();assert(!v.Spirit()&&!runs.count(v.p.guid));templates.actor.ScriptID=2;v.Tick();assert(v.Spirit());}
    {Visit v;v.p.phases.insert(8306);Apply(&v.p,Disguise);v.script.OnLogin(&v.p);v.Tick();assert(!runs.count(v.p.guid)&&v.p.casts.empty()&&v.p.GetAura(Disguise,v.p.guid));}
    std::cout<<"PASS: native passage script/Player credit guards, both factions/owners, living exact Guarm approach then exact exit334 with self disguise/reaction, once-only credit, cast/visual/summon retry, cancel/lifetime/death/map/vehicle/phase/combat/re-entry, shared later actors and actual39848 giver retention/cleanup. World/movement/aura application/network are fixtures; client checks remain pending.\n";
}
