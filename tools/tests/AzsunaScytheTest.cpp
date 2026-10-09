#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32 = uint32_t;
using int32 = int32_t;
struct ObjectGuid {
    unsigned value=0;
    void Clear() { value=0; }
    bool operator<(ObjectGuid b) const { return value<b.value; }
    bool operator==(ObjectGuid b) const { return value==b.value; }
};
using GuidList=std::vector<ObjectGuid>;
enum { QUEST_STATUS_INCOMPLETE=1, QUEST_STATUS_COMPLETE=2, QUEST_OBJECTIVE_GAMEOBJECT=2,
    TEMPSUMMON_TIMED_DESPAWN=1, REACT_PASSIVE=0, REACT_AGGRESSIVE=1, UNIT_FIELD_FLAGS=0,
    UNIT_FLAG_NON_ATTACKABLE=1, UNIT_FLAG_IMMUNE_TO_PC=2, UNIT_FLAG_NOT_SELECTABLE=4, UNIT_FLAG_IMMUNE_TO_NPC=8 };
struct Position { float x,y,z,o; };
struct Player;
struct CreatureAI;
struct TempSummon;
struct Unit {
    ObjectGuid guid; Position pos{-154.62f,6906.46f,13.2f,0}; bool alive=true; unsigned map=1220;
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsAlive() const { return alive; }
    float GetDistance(Position p) { return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z)); }
    bool IsWithinDistInMap(Unit* u,float range) { return u && map==u->map && GetDistance(u->pos)<=range; }
};
struct SpellInfo { unsigned Id; };
struct Motion { void MovePoint(unsigned,Position) {} };
struct Creature:Unit {
    unsigned entry=0; ObjectGuid owner,viewer; bool removed=false; CreatureAI* ai=nullptr; Motion motion;
    virtual TempSummon* ToTempSummon() { return nullptr; }
    void DespawnOrUnsummon() { removed=true; }
    void AddPlayerInPersonnalVisibilityList(ObjectGuid id) { viewer=id; }
    void SetReactState(int) {} void setFaction(int) {} void RemoveFlag(int,int) {}
    Motion* GetMotionMaster() { return &motion; }
    CreatureAI* AI() { return ai; }
    void CastSpell(Unit*,unsigned,bool);
};
struct TempSummon:Creature {
    TempSummon* ToTempSummon() override { return this; }
    ObjectGuid GetSummonerGUID() { return owner; }
};
struct GameObject:Unit { unsigned entry=0; unsigned GetEntry() { return entry; } Position GetPosition() { return pos; } };
static std::map<ObjectGuid,TempSummon> creatures;
static std::map<ObjectGuid,GameObject> objects;
static std::map<ObjectGuid,Player*> players;
static unsigned serial=100;
static bool castHit=true;
struct Menu { void SendCloseGossip() {} };
struct Player:Unit {
    Player() { guid={++serial}; players[guid]=this; }
    ~Player() { players.erase(guid); }
    int status=QUEST_STATUS_INCOMPLETE; bool spawnFailure=false, los=true;
    std::map<unsigned,unsigned> objectives; std::map<unsigned,GuidList> summons;
    std::set<unsigned> auras; std::vector<unsigned> credits; Menu menu; Menu* PlayerTalkClass=&menu;
    Player* ToPlayer() override { return this; }
    unsigned GetMapId() { return map; }
    int GetQuestStatus(unsigned) { return status; }
    unsigned GetQuestObjectiveData(unsigned,unsigned obj) { return objectives[obj]; }
    GuidList* GetSummonList(unsigned entry) { return &summons[entry]; }
    bool HasAura(unsigned id) { return auras.count(id)!=0; }
    void RemoveAurasDueToSpell(unsigned id) { auras.erase(id); }
    void AddAura(unsigned id,Player*) { auras.insert(id); }
    void KilledMonsterCredit(unsigned obj) { credits.push_back(obj); objectives[obj]=1; }
    void QuestObjectiveSatisfy(unsigned obj,unsigned,unsigned) { KilledMonsterCredit(obj); }
    bool IsWithinLOSInMap(Creature*) { return los; }
    Creature* FindNearestCreature(unsigned entry,float distance) {
        for(auto& pair:creatures) if(pair.second.entry==entry && !pair.second.removed && IsWithinDistInMap(&pair.second,distance)) return &pair.second;
        return nullptr;
    }
    TempSummon* SummonCreature(unsigned entry,Position pos,int,unsigned,int,ObjectGuid viewer) {
        if(spawnFailure) return nullptr;
        ObjectGuid id{++serial}; auto& c=creatures[id]; c.entry=entry;c.pos=pos;c.map=map;c.guid=id;c.owner=guid;c.viewer=viewer;summons[entry].push_back(id);return &c;
    }
};
struct CreatureAI {
    virtual ~CreatureAI()=default;
    virtual void IsSummonedBy(Unit*) {} virtual void UpdateAI(unsigned) {} virtual void SetGUID(ObjectGuid const&,int32) {}
    virtual void SpellHitTarget(Unit*,SpellInfo const*) {} virtual void Talk(unsigned,ObjectGuid) {}
};
struct ScriptedAI:CreatureAI { Creature* me; ScriptedAI(Creature* c):me(c) { c->ai=this; } };
void Creature::CastSpell(Unit* target,unsigned id,bool) { if(castHit && ai) { SpellInfo spell{id};ai->SpellHitTarget(target,&spell); } }
struct CreatureScript {
    CreatureScript(char const*) {} virtual ~CreatureScript()=default;
    virtual bool OnGossipSelect(Player*,Creature*,uint32,uint32) { return false; }
    virtual CreatureAI* GetAI(Creature*) const { return nullptr; }
};
struct GameObjectScript {
    GameObjectScript(char const*) {} virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*) { return false; }
};
struct PlayerScript {
    PlayerScript(char const*) {} virtual ~PlayerScript()=default;
    virtual void OnLogout(Player*) {} virtual void OnMapChanged(Player*) {} virtual void OnUpdate(Player*,uint32) {}
};
struct ObjectAccessor {
    static Creature* GetCreature(Unit& u,ObjectGuid id) { auto i=creatures.find(id);return i==creatures.end() || i->second.removed || i->second.map!=u.map?nullptr:&i->second; }
    static GameObject* GetGameObject(Unit& u,ObjectGuid id) { auto i=objects.find(id);return i==objects.end() || i->second.map!=u.map?nullptr:&i->second; }
    static Player* GetPlayer(Unit& u,ObjectGuid id) { auto i=players.find(id);return i==players.end() || i->second->map!=u.map?nullptr:i->second; }
};

// Compile all production handlers, including their ordering and cleanup logic.
#include "../../src/server/scripts/Legion/azsuna_scythe.cpp"

int main()
{
    using namespace AzsunaScythe;
    Player p, stranger;
    player_azsuna_scythe recovery;
    go_azsuna_soul_gem gems;
    npc_azsuna_allari_q37660 guide;
    GameObject& first=objects[{1}];first.guid={1};first.entry=239338;
    GameObject& second=objects[{2}];second.guid={2};second.entry=239332;
    GameObject& enter=objects[{3}];enter.guid={3};enter.entry=237017;
    GameObject& leave=objects[{4}];leave.guid={4};leave.entry=240012;
    p.status=0;recovery.OnUpdate(&p,1000);assert(!OwnedSummon(&p,Allari));
    gems.OnGossipHello(&p,&first);assert(p.credits.empty());
    p.status=QUEST_STATUS_INCOMPLETE;recovery.OnUpdate(&p,1000);
    Creature* allari=OwnedSummon(&p,Allari);assert(allari && allari->viewer==p.guid && p.credits.empty());
    npc_azsuna_allari_q37660::AI ai(allari);ai.IsSummonedBy(&p);
    assert(!guide.OnGossipSelect(&p,allari,0,0)); // actual arrival first
    ai.UpdateAI(1000);assert(Done(&p,Allari));
    assert(!guide.OnGossipSelect(&stranger,allari,0,0));
    assert(!guide.OnGossipSelect(&p,allari,0,1000));
    assert(guide.OnGossipSelect(&p,allari,0,0));assert(!guide.OnGossipSelect(&p,allari,0,0));
    gems.OnGossipHello(&p,&second);gems.OnGossipHello(&p,&enter);assert(!Done(&p,239332) && !Done(&p,237017));
    p.spawnFailure=true;gems.OnGossipHello(&p,&first);assert(!Done(&p,239338));p.spawnFailure=false;
    p.pos.x+=20;gems.OnGossipHello(&p,&first);assert(!Done(&p,239338));p.pos=Meeting;
    castHit=false;gems.OnGossipHello(&p,&first);ai.UpdateAI(6500);assert(Done(&p,239338) && !Done(&p,90402));
    castHit=true;gems.OnGossipHello(&p,&first);unsigned count=p.summons[90402].size();
    gems.OnGossipHello(&p,&first);assert(p.summons[90402].size()==count);
    ai.UpdateAI(6499);assert(!Done(&p,90402));ai.UpdateAI(1);assert(Done(&p,90402));
    gems.OnGossipHello(&p,&second);OwnedSummon(&p,89276)->alive=false;ai.UpdateAI(6500);assert(!Done(&p,89276));
    gems.OnGossipHello(&p,&second);p.pos.x+=70;ai.UpdateAI(6500);assert(!Done(&p,89276));p.pos=Meeting;
    gems.OnGossipHello(&p,&second);ai.UpdateAI(6500);assert(Done(&p,89276));
    assert(!Done(&p,89673) && !Done(&p,89398));
    p.spawnFailure=true;gems.OnGossipHello(&p,&enter);assert(!Done(&p,237017) && !p.HasAura(SoulScreen));p.spawnFailure=false;
    gems.OnGossipHello(&p,&enter);assert(Done(&p,237017) && p.HasAura(SoulScreen) && OwnedSummon(&p,89673));
    count=p.summons[89673].size();gems.OnGossipHello(&p,&enter);assert(p.summons[89673].size()==count);
    gems.OnGossipHello(&p,&leave);assert(!p.HasAura(SoulScreen) && !OwnedSummon(&p,89673) && !Done(&p,89673));
    gems.OnGossipHello(&p,&enter);assert(OwnedSummon(&p,89673));
    // Actual death credit is provided by the engine; it must never come from a
    // timer, spawn, entry, exit or abandonment in these production handlers.
    p.KilledMonsterCredit(89673);OwnedSummon(&p,89673)->alive=false;
    Creature& receiver=creatures[{90}];receiver.guid={90};receiver.entry=89398;receiver.pos=Meeting;
    p.los=false;recovery.OnUpdate(&p,1000);assert(!Done(&p,89398));
    p.los=true;recovery.OnUpdate(&p,1000);assert(Done(&p,89398) && !p.HasAura(SoulScreen));
    Player retry;recovery.OnUpdate(&retry,1000);assert(OwnedSummon(&retry,Allari));
    retry.AddAura(SoulScreen,&retry);retry.alive=false;recovery.OnUpdate(&retry,1000);assert(!retry.HasAura(SoulScreen) && !OwnedSummon(&retry,Allari));
    retry.alive=true;recovery.OnUpdate(&retry,1000);assert(OwnedSummon(&retry,Allari));
    retry.status=0;recovery.OnUpdate(&retry,1000);assert(!OwnedSummon(&retry,Allari));
    retry.status=QUEST_STATUS_INCOMPLETE;recovery.OnUpdate(&retry,1000);recovery.OnLogout(&retry);assert(!OwnedSummon(&retry,Allari));
    recovery.OnUpdate(&retry,1000);assert(OwnedSummon(&retry,Allari));retry.AddAura(SoulScreen,&retry);retry.map=1;recovery.OnMapChanged(&retry);assert(!retry.HasAura(SoulScreen));
    retry.map=1220;retry.pos.x+=200;recovery.OnUpdate(&retry,1000);assert(!OwnedSummon(&retry,Allari));
    std::cout<<"PASS: production Scythe handlers: order, owner, range, actual hit/dialogue, duplicate and failed summons, retry, real-kill boundary, return and cleanup.\n";
}
