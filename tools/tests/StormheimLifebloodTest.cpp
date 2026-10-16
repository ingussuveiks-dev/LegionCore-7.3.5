#include <crtdbg.h>
#include <cstdlib>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstring>
constexpr int QUEST_SPECIAL_FLAGS_SPEAKTO=128, GAMEOBJECT_TYPE_QUESTGIVER=2;
#include "LifebloodFixture.inc"
#include "NativeGossipUse.inc"
#include "Talked.inc"
using ScriptedAI=SmartAI;
using GuidSet=std::set<ObjectGuid>;
constexpr int EO_FLAG_ONE_ACTION=1;
constexpr double M_PI=3.141592653589793;
struct EventObjectTemplate {uint32 SpellID=241140,WorldSafeLocID=0,Flags=0;float radius=2;};
struct EventObject:WorldObject {
    uint64 dbGuid=400325;uint64 GetDBTableGUIDLow(){return dbGuid;}
    EventObjectTemplate data;EventObjectTemplate* eventTemplate=&data;
    bool m_isUpdate=false;GuidSet affectedPlayers,affectedPermamentPlayers;
    float GetDistance(WorldObject* o){return Position::GetExactDist(o);}
    void GetPlayerListInGrid(std::list<Player*>& list,float radius){for(auto const& pair:players)if(pair.second->map==map&&pair.second->GetExactDist(this)<=radius)list.push_back(pair.second);}
    void Update(uint32);bool ActivatedForPlayer(Player*)const;
};
// Native Update also compiles the unused safe-location branch.
struct WorldSafeLocsEntry {uint32 MapID=0;struct {float X=0,Y=0,Z=0,O=0;} Loc;};
struct SafeLocs {WorldSafeLocsEntry* LookupEntry(uint32){return nullptr;}} sWorldSafeLocsStore;
struct EventObjectScript {explicit EventObjectScript(char const*){}virtual bool IsTriggerMeets(Player*,EventObject*){return true;}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnLogin(Player*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
#include "lifeblood.inc"
#include "helheim_bargain.inc"
void Creature::GetPlayerListInGrid(std::list<Player*>& list,float radius)
{for(auto const& pair:players)if(pair.second->map==map&&pair.second->GetExactDist(this)<=radius)list.push_back(pair.second);}
struct ScriptManager {
    bool legacy=false;eventobject_stormheim_lifeblood_return gate;
    bool IsEOMeetConditions(Player* p,EventObject* e){return legacy||gate.IsTriggerMeets(p,e);}
    void OnEventObject(Player* p,EventObject* e,bool enter){gate.OnTrigger(p,e,enter);}
} manager;auto sScriptMgr=&manager;
#include "NativeEventUpdate.inc"
#include "NativeEventActivated.inc"
#include "NativeConversationStructs.inc"
struct ConversationStore {
    std::map<uint32,std::vector<ConversationData>> lines;
    std::map<uint32,std::vector<ConversationActor>> actors;
    auto GetConversationData(uint32 id){return &lines[id];}
    std::vector<ConversationCreature> const* GetConversationCreature(uint32){return nullptr;}
    auto GetConversationActor(uint32 id){return &actors[id];}
} conversationStore;auto sConversationDataStore=&conversationStore;
struct ConversationDynamicFieldActors {uint32 actorId,creatureId,displayId,unk1,unk2,unk3;};
struct ConversationDynamicFieldCreatures {uint64 GuidLow,GuidHight;uint32 unk1,unk2;};
struct ConversationDynamicFieldLines {uint32 id,textId,unk1,unk2;};
struct Conversation;
std::vector<std::unique_ptr<Conversation>> conversations;bool failConversation=false;
struct Conversation {
    uint32 entry=0,duration=0;ObjectGuid owner;
    std::map<uint8,ConversationDynamicFieldActors> actors;
    std::map<uint8,ConversationDynamicFieldLines> lines;
    float GetPositionX(){return 0;}float GetPositionY(){return 0;}
    void SetActor(ConversationDynamicFieldActors* p,uint8 i,bool){actors[i]=*p;}
    void SetLine(ConversationDynamicFieldLines* p,uint8 i,bool){lines[i]=*p;}
    void SetCreature(ConversationDynamicFieldCreatures*,uint8,bool){assert(false);}
    bool GenerateDynamicFieldData(uint32,WorldObject*,uint32&);
    bool CreateConversation(uint64,uint32 id,Unit* caster,SpellInfo const*,Position const& point){
        assert(caster&&caster->ToPlayer()&&point.GetExactDist(&caster->GetPosition())<0.1f);
        if(failConversation||!GenerateDynamicFieldData(id,caster,duration))return false;
        owner=caster->GetGUID();entry=id;conversations.emplace_back(this);return true;
    }
};
#include "NativeConversationData.inc"
struct Generator {uint64 serial=0;uint64 Generate(){return ++serial;}} generator;
struct ObjectManager {template<HighGuid>Generator* GetGenerator(){return &generator;}} objectMgr;auto sObjectMgr=&objectMgr;
constexpr int SPELL_EFFECT_HANDLE_HIT=1;
struct Spell {
    int effectHandleMode=SPELL_EFFECT_HANDLE_HIT;Unit* caster=nullptr;
    struct {bool HasDst(){return false;}} m_targets;
    WorldObject* destTarget=nullptr;SpellInfo info;
    Unit* GetCaster(){return caster;}SpellInfo* GetSpellInfo(){return &info;}
    void EffectSummonConversation(SpellEffIndex);
};
#include "NativeConversationSpell.inc"
using namespace StormheimLifeblood;
uint64 serial=10000;
GameObject altar;
std::unique_ptr<go_stormheim_helheim_bargain> altarAI;
struct Visit {
    Player p;player_stormheim_lifeblood script;
    bool failSummon=false;
    std::vector<std::unique_ptr<Creature>> actors;
    std::vector<std::unique_ptr<npc_stormheim_lifeblood_ashildir>> ais;
    Visit(uint32 team=ALLIANCE){
        p.guid={++serial};p.map=1463;p.zone=7588;p.team=team;p.race=team==ALLIANCE?1:2;players[p.guid]=&p;
        p.Accept(StormheimLifeblood::QuestId);static_cast<Position&>(p)=StormheimLifeblood::Altar;
        p.castTargetHook=[&](Unit*,uint32 id){if(p.castResult)return p.castResult;
            if(id>=212916&&id<=212918){Spell spell;spell.caster=&p;spell.info.effect.MiscValue=id-212916+2257;spell.EffectSummonConversation(0);}return 0;};
        p.judgmentSummon=[&](uint32 id,Position const& point,ObjectGuid viewer)->Creature*{
            assert(id==97558&&viewer==p.guid);if(failSummon)return nullptr;
            auto c=std::make_unique<Creature>();c->entry=id;c->guid={++serial};c->map=1463;c->zone=7588;static_cast<Position&>(*c)=point;
            creatures[c->guid]=c.get();auto ai=std::make_unique<npc_stormheim_lifeblood_ashildir>(c.get());
            auto result=c.get();actors.push_back(std::move(c));ais.push_back(std::move(ai));result->AI()->IsSummonedBy(&p);return result;
        };
    }
    ~Visit(){script.OnLogout(&p);for(auto const& c:actors)creatures.erase(c->guid);players.erase(p.guid);}
    void Tick(uint32 diff=500){script.OnUpdate(&p,diff);}
    Creature* Spirit(){auto it=runs.find(p.guid);return it==runs.end()?nullptr:ObjectAccessor::GetCreature(p,it->second.spirit);}
    void Offer(){altarAI->GossipUse(&p);p.menu.menu.menu=18790;p.menu.menu.item=true;}
    void Select(){altarAI->GossipSelect(&p,18790,1);}
    void Begin(){auto packets=p.packets,teleports=p.teleports;Tick();Offer();Select();assert(runs[p.guid].stage==1&&p.packets==packets&&p.teleports==teleports);}
    void Depart(){Tick(8823);assert(runs[p.guid].stage==2);Tick(4978);assert(runs[p.guid].stage==3);Tick(6396);}
    void Arrive(bool ack=true){p.map=1220;p.zone=7541;static_cast<Position&>(p)=Landing;script.OnMapChanged(&p);p.teleport=!ack;Tick();}
};
int main(int argc,char** argv)
{
    assert(argc==2);std::ifstream data(argv[1]);std::string line;
    while(std::getline(data,line)){
        std::istringstream in(line);std::string kind;in>>kind;
        if(kind=="native-actor"){ConversationActor row{};in>>row.entry>>row.id>>row.actorId>>row.creatureId>>row.displayId>>row.unk1>>row.unk2>>row.unk3>>row.duration;conversationStore.actors[row.entry].push_back(row);}
        if(kind=="line"){ConversationData row{};in>>row.entry>>row.id>>row.idx>>row.textId>>row.unk1>>row.unk2;conversationStore.lines[row.entry].push_back(row);}
    }
    for(uint32 id=2257;id<=2259;++id)assert(conversationStore.actors[id].size()==1&&conversationStore.lines[id].size()==1);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    auto& q=store.quests[QuestId];q.id=QuestId;q.special=QUEST_SPECIAL_FLAGS_KILL;q.objectives={{0,ExitCredit,1,0,0}};
    altar.entry=244559;altar.dbGuid=127713;altar.guid={60};altar.map=1463;altar.zone=7588;altar.info.type=2;
    static_cast<Position&>(altar)=StormheimLifeblood::Altar;objects[altar.guid]=&altar;altarAI=std::make_unique<go_stormheim_helheim_bargain>(&altar);
    for(auto team:{ALLIANCE,HORDE}){
        Visit v(team);assert(!v.p.CanCompleteQuest(QuestId)&&!v.p.CanRewardQuest(&q,false));v.Begin();
        auto actor=v.Spirit();assert(actor&&actor->personal==v.p.guid&&actor->bound&&actor->motion.target==&v.p&&actor->react==REACT_PASSIVE);
        assert(actor->talkGroups==std::vector<uint8>{3});
        v.Tick(8822);assert(runs[v.p.guid].stage==1);v.Tick(1);assert(actor->talkGroups==std::vector<uint8>{3}&&actor->motion.idles==1&&actor->casts==std::vector<uint32>{216818});
        v.Tick(4978);v.Tick(6396);assert(!actor->world&&v.p.teleports==1&&!v.p.completions&&v.p.casts==std::vector<uint32>({212916,212917,212918}));
        auto n=conversations.size();assert(n>=3);
        for(uint32 i=0;i<3;++i){auto& c=*conversations[n-3+i];assert(c.entry==2257+i&&c.owner==v.p.guid&&c.actors.size()==1&&c.lines.size()==1&&c.actors[0].creatureId==91387&&c.actors[0].displayId==65043&&c.lines[0].id==4753+i);assert(c.duration==conversationStore.actors[2257+i][0].duration);}
        assert(conversations[n-3]->actors[0].actorId==53867&&conversations[n-2]->actors[0].actorId==50552&&conversations[n-1]->actors[0].actorId==50552);
        v.Arrive(false);assert(!v.p.packets&&runs[v.p.guid].stage==4);v.p.teleport=false;v.Tick();
        assert(v.p.packets==1&&v.p.completions==1&&v.p.GetQuestObjectiveData(QuestId,ExitCredit)==1&&v.p.CanRewardQuest(&q,false));
        v.Tick();v.Select();assert(v.p.packets==1&&v.p.teleports==1&&runs.find(v.p.guid)==runs.end());
    }
    {Visit v;v.Select();assert(v.p.casts.empty());v.Offer();v.p.menu.menu.item=false;v.Select();assert(v.p.casts.empty());v.Offer();v.p.menu.menu.menu=123;v.Select();assert(v.p.casts.empty());}
    {Visit v;v.Offer();v.p.Accept(QuestId);v.Select();assert(v.p.casts.empty());v.Offer();v.Select();assert(runs[v.p.guid].stage==1);v.Select();v.Offer();v.Select();assert(v.p.casts.size()==1);}
    {Visit a,b;GroupReference last{&b.p,nullptr},first{&a.p,&last};Group group{&first};a.p.group=b.p.group=&group;
        for(auto p:{&a.p,&b.p}){p->KilledMonsterCredit(ExitCredit);p->TalkedToCreature(ExitCredit,{});p->RewardPlayerAndGroupAtEvent(ExitCredit,&altar);}assert(!a.p.packets&&!b.p.packets);
        a.Begin();b.Tick();assert(a.Spirit()!=b.Spirit());a.Depart();a.Arrive();assert(a.p.completions==1&&!b.p.completions&&b.Spirit()->world);b.Begin();b.Depart();b.Arrive();assert(b.p.completions==1);}
    {Visit v;Creature alias;alias.entry=123;alias.guid={999};creatures[alias.guid]=&alias;v.p.KilledMonsterCredit(ExitCredit,alias.guid);assert(!v.p.packets);creatures.erase(alias.guid);
        auto& other=store.quests[999];other.id=999;other.special=QUEST_SPECIAL_FLAGS_KILL;other.objectives={{0,ExitCredit,1,0,0}};v.p.Accept(999);v.p.KilledMonsterCredit(ExitCredit);assert(v.p.GetQuestObjectiveData(999,ExitCredit)==1&&!v.p.GetQuestObjectiveData(QuestId,ExitCredit));}
    for(int invalid=0;invalid<15;++invalid){Visit v;v.Tick();v.Offer();
        if(invalid==0)v.p.alive=false;if(invalid==1)v.p.map=1220;if(invalid==2)v.p.zone=1;if(invalid==3)v.p.teleport=true;
        if(invalid==4)v.p.vehicle=true;if(invalid==5)v.p.transport=true;if(invalid==6)v.p.combat=true;if(invalid==7)v.p.flying=true;
        if(invalid==8)v.p.phase=false;if(invalid==9)v.p.los=false;if(invalid==10)v.p.visible=false;if(invalid==11)v.p.x+=20;
        if(invalid==12)v.p.phases.insert(8306);if(invalid==13)v.p.quests.erase(QuestId);if(invalid==14)altar.flags=GO_FLAG_NOT_SELECTABLE;
        v.Select();assert(v.p.casts.empty()&&!v.p.teleports&&!v.p.packets);altar.flags=0;
    }
    {Visit v;v.Begin();v.p.x+=20;v.Tick();assert(!runs.count(v.p.guid)&&!v.p.teleports);static_cast<Position&>(v.p)=StormheimLifeblood::Altar;v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);}
    {Visit v;v.Begin();v.p.Accept(QuestId);v.Tick();assert(runs[v.p.guid].stage==0&&!v.p.teleports);v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);}
    {Visit v;v.Begin();v.p.teleportResult=false;v.Depart();assert(!v.p.packets&&!runs.count(v.p.guid));v.p.teleportResult=true;v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);}
    {Visit v;v.Begin();v.Depart();v.Tick(120000);assert(!v.p.completions&&runs[v.p.guid].stage==4);v.Arrive();assert(v.p.completions==1);} // slow loading retains the validated request
    {Visit v;v.Begin();v.Depart();v.Tick(120000);v.script.OnLogout(&v.p);assert(!runs.count(v.p.guid)&&!v.p.packets);} // session timeout/logout cleans a stalled transfer
    {Visit v;v.Begin();v.Depart();v.p.teleportPoint.x+=20;v.Arrive();assert(!v.p.packets);}
    {Visit v;v.Begin();v.Depart();v.p.teleport=false;v.Tick();assert(!v.p.packets&&!runs.count(v.p.guid));}
    {Visit v;v.Begin();v.Depart();v.p.map=1220;static_cast<Position&>(v.p)=Landing;v.p.x+=20;v.p.teleport=false;v.Tick();assert(!v.p.packets);}
    {Visit v;v.Begin();v.Depart();v.p.Accept(QuestId);v.Arrive();assert(!v.p.packets);}
    {Visit v;v.Begin();v.Depart();v.script.OnLogout(&v.p);v.Arrive();assert(!v.p.packets);} // no invented exact-stage crash recovery
    {Visit v;v.p.map=1220;static_cast<Position&>(v.p)=Landing;v.script.OnLogin(&v.p);assert(v.p.teleport&&v.p.teleportMap==1463&&v.p.teleportPoint.GetExactDist(&StormheimLifeblood::Altar)<0.1f&&!v.p.packets);
        v.p.map=1463;static_cast<Position&>(v.p)=StormheimLifeblood::Altar;v.p.teleport=false;v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);} // interrupted saved destination restarts the interaction
    for(int skip=0;skip<7;++skip){Visit v;v.p.map=1220;static_cast<Position&>(v.p)=Landing;
        if(skip==0)v.p.quests[QuestId].Status=QUEST_STATUS_COMPLETE;if(skip==1){v.p.quests.erase(QuestId);v.p.m_RewardedQuests.insert(QuestId);}
        if(skip==2)v.p.x+=20;if(skip==3)v.p.phases.insert(8306);if(skip==4)v.p.transport=true;if(skip==5)v.p.vehicle=true;if(skip==6)v.p.teleport=true;
        v.script.OnLogin(&v.p);assert(!v.p.teleports&&!v.p.packets);}
    {Visit v;v.failSummon=true;v.p.castResult=SPELL_FAILED_BAD_TARGETS;v.Begin();assert(!v.Spirit());v.Depart();v.Arrive();assert(v.p.completions==1);} // all optional failures
    {Visit v;failConversation=true;v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);failConversation=false;}
    {Visit v;auto actors=conversationStore.actors[2258];conversationStore.actors[2258].clear();v.Begin();v.Depart();v.Arrive();assert(v.p.completions==1);conversationStore.actors[2258]=actors;} // native builder rejection cannot block exit
    {Visit v;v.failSummon=true;v.Tick();assert(!v.Spirit());v.failSummon=false;v.Tick();assert(v.Spirit());auto actor=v.Spirit();actor->alive=false;v.Tick();assert(!actor->world&&v.Spirit()!=actor&&v.Spirit()->talkGroups.empty());}
    {Visit v;v.Tick();auto actor=v.Spirit();v.p.map=1220;v.script.OnMapChanged(&v.p);assert(actor->world);actor->AI()->UpdateAI(1);assert(!actor->world);} // no cross-map raw access
    {Visit v;v.Tick();auto actor=v.Spirit();v.p.CancelQuestObjects(QuestId);v.p.quests.erase(QuestId);v.Tick();assert(!actor->world&&!runs.count(v.p.guid));}
    {Visit v;v.p.phases.insert(8306);v.Tick();assert(!v.Spirit());Creature foreign;foreign.entry=97558;npc_stormheim_lifeblood_ashildir ai(&foreign);ai.IsSummonedBy(&v.p);ai.UpdateAI(10);assert(foreign.world&&ai.delegated==2);}
    {Visit v;v.p.roots[{192471,v.p.guid.value}]={};v.p.roots[{192471,999}]={};v.script.OnLogin(&v.p);assert(!v.p.GetAura(192471,v.p.guid)&&v.p.GetAura(192471,999));v.Tick();assert(v.p.casts.empty());}
    {Visit v;q.objectives[0].Flags=1;v.Tick();v.Offer();v.Select();assert(!v.Spirit()&&v.p.casts.empty());q.objectives[0].Flags=0;}
    // Reproduce the original template cast before SmartAI's dead source22
    // condition, then execute the full native EventObject update with the hook.
    {Visit v;EventObject e;e.entry=333;e.guid={77};e.map=1463;e.zone=7588;static_cast<Position&>(e)=ReturnGate;static_cast<Position&>(v.p)=ReturnGate;
        manager.legacy=true;e.Update(1);assert(v.p.casts==std::vector<uint32>{241140});manager.legacy=false;e.affectedPlayers.clear();v.p.casts.clear();
        e.Update(1);assert(v.p.casts.empty()&&e.affectedPlayers.empty());v.p.quests[QuestId].Status=QUEST_STATUS_COMPLETE;e.Update(1);assert(v.p.casts.empty());
        v.p.m_RewardedQuests.insert(QuestId);v.p.phases.insert(8306);e.Update(1);assert(v.p.casts.empty());v.p.phases.clear();
        e.dbGuid=123;e.Update(1);assert(v.p.casts.empty());e.dbGuid=400325;v.p.combat=true;e.Update(1);assert(v.p.casts.empty());v.p.combat=false;
        e.Update(1);e.Update(1);assert(v.p.casts==std::vector<uint32>{241140}&&!v.p.packets&&e.affectedPlayers.count(v.p.guid));}
    {Visit v;auto old=altarAI->delegated;altarAI->GossipSelect(&v.p,123,0);assert(altarAI->delegated==old+1);old=altarAI->delegated;v.Select();assert(altarAI->delegated==old);} // actual integration consumes exit1
    std::cout<<"PASS: production39855 current menu/acceptance/altar, owner-only follower/retry/cleanup, complete native line durations and independent cast/summon failures, acknowledged individual arrival/failed transfer/slow loading/logout/cancellation, native generic/group guards and native EventObject pre-cast gate. Teleport, motion, casts and networking are fixtures; client story/arrival/turn-in checks pending.\n";
}
