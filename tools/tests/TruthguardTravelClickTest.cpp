#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using uint32 = uint32_t;
using uint64 = uint64_t;
using uint8 = uint8_t;
using int8 = int8_t;
struct ObjectGuid
{
    uint64 _high=0,_low=0;
    ObjectGuid(uint32 low=0):_low(low) {}
    ObjectGuid(uint64 high,uint64 low):_high(high),_low(low) {}
    bool operator<(ObjectGuid const&) const;
    bool operator==(ObjectGuid const& other) const { return _high==other._high && _low==other._low; }
};
#include "GuidCompare.inc"
using QuestStatus = uint32;
#include "NativeCastResults.inc"
#include "NativeData.inc"
enum { NPC_HIPPOGRYPH_DALARAN=105883, NPC_HIPPOGRYPH_SCENARIO=106203, NPC_HIPPOGRYPH_DALARAN_SUM=105886, NPC_HIPPOGRYPH_SCENARIO_SUM=106205,
    SPELL_SUMMON_HIPPO=209884, SPELL_SUMMON_HIPPO_INST=210358,
    QUEST_STATUS_NONE=0, QUEST_STATUS_COMPLETE=1, QUEST_STATUS_INCOMPLETE=3, UNIT_FIELD_NPC_FLAGS=0, UNIT_NPC_FLAG_SPELLCLICK=16777216,
    NPC_CLICK_CAST_CASTER_CLICKER=1, NPC_CLICK_CAST_TARGET_CLICKER=2, NPC_CLICK_CAST_ORIG_CASTER_OWNER=4,
    MAX_SPELL_EFFECTS=32, SPELL_AURA_CONTROL_VEHICLE=236, SPELLVALUE_BASE_POINT0=0, MAX_EFFECT_MASK=0xffffffff };
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_ERROR(...) assert(false)
uint32 SpellValueMod(uint32 value) { return value; }
struct Player;
struct Creature;
struct TempSummon;
struct SpellInfo
{
    uint32 identity=0,EffectMask=0;
    struct Effect { uint32 ApplyAuraName=0; } effect;
    Effect const* GetEffect(uint32,uint32) const { return &effect; }
};
struct InstanceScript { uint32 event=0; uint32 GetData(uint32 identity) { assert(identity==DATA_YRGRIM_EVENT); return event; } } scenarioInstance;
struct Cast { uint32 spell; void* target; bool triggered; };
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    virtual TempSummon* ToTempSummon() { return nullptr; }
    uint32 entry=0,identity=17,map=1,phase=1;
    uint64 guidHigh=0;
    Unit* vehicle=nullptr;
    SpellCastResult result=SPELL_CAST_OK;
    std::function<void()> castHook;
    std::vector<Cast> casts;
    struct Vehicle { uint32 GetCreatureEntry() const { assert(false); return 0; } };
    Vehicle* GetVehicleKit() { return nullptr; }
    uint32 GetEntry() const { return entry; }
    ObjectGuid GetGUID() const { return {guidHigh,identity}; }
    ObjectGuid GetOwnerGUID() const { return 0; }
    uint32 GetSpawnMode() const { return 0; }
    bool IsInMap(Unit const* other) const { return map==other->map; }
    bool InSamePhase(Unit const* other) const { return phase==other->phase; }
    Unit* GetVehicleBase() const { return vehicle; }
    bool HandleSpellClick(Unit*,int8=-1);
    SpellCastResult CastSpell(Unit* target,uint32 spell,bool triggered) { casts.push_back({spell,target,triggered}); if(castHook) castHook(); return result; }
    void CastSpell(Unit* target,SpellInfo const* spell,bool triggered,void*,void*,ObjectGuid) { CastSpell(target,spell->identity,triggered); }
    void CastCustomSpell(uint32,uint32,int,Unit*,bool,void*,void*,ObjectGuid) { assert(false); }
};
struct OutdoorPvP { void HandleSpellClick(Player*,Unit*) { assert(false); } };
struct Player : Unit
{
    uint32 quest=0,team=0;
    bool hasInstance=false;
    bool alive=true,rewarded=false;
    Player* ToPlayer() override { return this; }
    QuestStatus GetQuestStatus(uint32 questId) const { assert(questId==42017); return quest; }
    InstanceScript* GetInstanceScript() { return hasInstance?&scenarioInstance:nullptr; }
    uint32 GetGUIDLow() const { return identity; }
    bool IsAlive() const { return alive; }
    bool GetQuestRewardStatus(uint32 questId) const { assert(questId==42017); return rewarded; }
    OutdoorPvP* GetOutdoorPvP() { return nullptr; }
    bool canSeeSpellClickOn(Creature const*) const;
};
struct BaseAI { virtual ~BaseAI()=default; virtual void OnSpellClick(Unit*)=0; virtual void Reset()=0; };
struct Creature : Unit
{
    BaseAI* ai=nullptr;
    bool summoned=false,IsAIEnabled=true;
    uint32 hides=0;
    std::vector<ObjectGuid> hidden;
    Creature* ToCreature() override { return this; }
    BaseAI* AI() { return ai; }
    bool isSummon() const { return summoned; }
    bool HasFlag(uint32,uint32 flag) const { assert(flag==UNIT_NPC_FLAG_SPELLCLICK); return true; }
    void DestroyForPlayer(Player* player) { ++hides; hidden.push_back(player->GetGUID()); }
};
struct TempSummon : Creature
{
    Unit* summoner=nullptr;
    TempSummon* ToTempSummon() override { return summoned?this:nullptr; }
    Unit* GetSummoner() const { return summoner; }
};
namespace ObjectAccessor
{
    std::map<ObjectGuid,Player*> players;
    Player* GetPlayer(Creature const&,ObjectGuid identity) { auto found=players.find(identity); return found==players.end()?nullptr:found->second; }
}
struct Script : BaseAI
{
    Creature* me;
    explicit Script(Creature* actor):me(actor) { actor->ai=this; }
#include "BoardingRequests.inc"
#include "Summon.inc"
#include "Click.inc"
#include "Reset.inc"
};
struct Aura { template<class... Values> static Aura* TryRefreshStackOrCreate(Values...) { assert(false); return nullptr; } void ApplyForTargets() { assert(false); } };
using ConditionList=std::vector<int>;
struct ConditionSourceInfo { ConditionSourceInfo(Unit*,Unit*) {} };
struct ConditionMgr
{
    ConditionList GetConditionsForSpellClickEvent(uint32,uint32) const { return {}; }
    bool IsObjectMeetToConditions(ConditionSourceInfo&,ConditionList const& rows) const { assert(rows.empty()); return true; }
} conditions;
auto* sConditionMgr=&conditions;
struct SpellClickInfo { uint32 spellId=0,castFlags=0; bool IsFitToRequirements(Unit const*,Unit const*) const { return true; } };
using SpellClickInfoContainer=std::multimap<uint32,SpellClickInfo>;
using SpellClickInfoMapBounds=std::pair<SpellClickInfoContainer::const_iterator,SpellClickInfoContainer::const_iterator>;
struct ObjectMgr
{
    SpellClickInfoContainer clicks;
    SpellClickInfoMapBounds GetSpellClickInfoMapBounds(uint32 identity) const { return clicks.equal_range(identity); }
} templates;
auto* sObjectMgr=&templates;
struct SpellMgr
{
    SpellInfo spell;
    SpellInfo const* GetSpellInfo(uint32 identity) { spell.identity=identity; return &spell; }
} spells;
auto* sSpellMgr=&spells;
#include "Visibility.inc"
#include "Dispatch.inc"
int main(int argc,char** argv)
{
    assert(argc==2);
    std::ifstream input(argv[1]);
    uint32 entry,spell,flags,user,add;
    while(input>>entry>>spell>>flags>>user>>add)
    {
        assert(user==0 && add==1 && flags==1);
        templates.clicks.emplace(entry,SpellClickInfo{spell,flags});
    }
    assert(input.eof());
#ifdef BASELINE
    assert(templates.clicks.size()==2);
#else
    assert(templates.clicks.empty());
#endif
    uint32 cases=0;
    for(uint32 actorEntry : {105883u,106203u})
        for(uint32 status : {0u,1u,3u,5u,6u})
            for(uint32 event=0;event<9;++event)
                for(bool present : {false,true})
                    for(bool summoned : {false,true})
                        for(uint32 kind=0;kind<3;++kind)
                            for(uint32 team : {0u,1u})
                                for(SpellCastResult result : {SPELL_CAST_OK,SPELL_FAILED_ERROR,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_DONT_REPORT})
                                {
                                    Player player; Unit pet,guardian;
                                    player.quest=status; player.hasInstance=present; player.team=team; scenarioInstance.event=event;
                                    Unit* clicker=kind==0?static_cast<Unit*>(&player):kind==1?&pet:&guardian;
                                    clicker->result=result;
                                    Creature actor; actor.entry=actorEntry; actor.summoned=summoned; Script script(&actor);
                                    bool admitted=kind==0 && !summoned && (actorEntry==105883?status==3:present && event==7);
                                    warnings=0;
                                    assert(player.canSeeSpellClickOn(&actor));
                                    bool handled=actor.HandleSpellClick(clicker);
#ifdef BASELINE
                                    assert(handled && clicker->casts.size()==1u+uint32(admitted));
                                    assert(clicker->casts.front().target==&actor && !clicker->casts.front().triggered);
                                    assert(actor.hides==uint32(admitted) && warnings==0);
#else
                                    assert(!handled && clicker->casts.size()==uint32(admitted));
#ifdef BOARDING_BASELINE
                                    assert(actor.hides==uint32(admitted && result==SPELL_CAST_OK));
#else
                                    assert(actor.hides==0);
                                    assert(script.boardingRequests.size()==uint32(admitted && result==SPELL_CAST_OK));
#endif
                                    assert(warnings==uint32(admitted && result!=SPELL_CAST_OK));
#endif
                                    if(admitted) { auto const& cast=clicker->casts.back(); assert(cast.target==&player && cast.triggered && cast.spell==(actorEntry==105883?209884u:210358u)); }
                                    ++cases;
                                }
#ifndef BASELINE
    for(uint32 actorEntry : {105883u,106203u})
    {
        Player player; player.quest=3; player.hasInstance=true; scenarioInstance.event=7;
        Creature actor; actor.entry=actorEntry; Script script(&actor);
        player.result=SPELL_FAILED_DONT_REPORT;
        actor.HandleSpellClick(&player); assert(actor.hides==0 && player.casts.size()==1);
        player.quest=0; player.hasInstance=false;
        actor.HandleSpellClick(&player); assert(actor.hides==0 && player.casts.size()==1);
        player.quest=3; player.hasInstance=true; player.result=SPELL_CAST_OK;
        actor.HandleSpellClick(&player); assert(player.casts.size()==2);
#ifdef BOARDING_BASELINE
        assert(actor.hides==1);
#else
        assert(actor.hides==0 && script.boardingRequests.size()==1);
#endif
    }
#endif
#if !defined(BASELINE) && !defined(BOARDING_BASELINE)
    uint32 boardingCases=0;
    for(uint32 actorEntry : {105883u,106203u})
        for(uint32 variant=0;variant<16;++variant)
            for(uint32 team : {0u,1u})
            {
                Player player,stranger; player.quest=3; player.hasInstance=true; player.team=team; stranger.identity=18;
                scenarioInstance.event=7;
                Creature actor; actor.entry=actorEntry; Script script(&actor);
                TempSummon bird; bird.summoned=true; bird.entry=actorEntry==105883?105886:106205; bird.summoner=&player;
                ObjectAccessor::players={{17,&player},{18,&stranger}};
                player.castHook=[&] { actor.HandleSpellClick(&player); };
                actor.HandleSpellClick(&player);
                player.castHook={};
                assert(player.casts.size()==1 && actor.hides==0 && script.boardingRequests.size()==1);
                script.Reset();
                actor.HandleSpellClick(&player); assert(player.casts.size()==1);
                player.vehicle=&bird;
                switch(variant)
                {
                    case 1: player.vehicle=nullptr; break;
                    case 2: bird.summoner=&stranger; break;
                    case 3: bird.summoner=nullptr; break;
                    case 4: bird.summoned=false; break;
                    case 5: bird.entry=actorEntry==105883?106205:105886; break;
                    case 6: bird.map=2; break;
                    case 7: bird.phase=2; break;
                    case 8: ObjectAccessor::players.clear(); break;
                    case 9: player.map=2; break;
                    case 10: player.phase=2; break;
                    case 11: player.alive=false; break;
                    case 12: player.quest=0; break;
                    case 13: player.quest=5; break;
                    case 14: player.rewarded=true; break;
                    case 15: player.quest=1; break;
                }
                script.UpdateBoardingRequests(0);
                bool boarded=variant==0 || variant==15;
                assert(actor.hides==uint32(boarded));
                script.UpdateBoardingRequests(0xffffffff);
                assert(actor.hides==uint32(boarded) && script.boardingRequests.empty());
                script.UpdateBoardingRequests(5000); assert(actor.hides==uint32(boarded));
                ++boardingCases;
            }
    for(uint32 actorEntry : {105883u,106203u})
    {
        Player player,stranger; player.quest=stranger.quest=3; player.hasInstance=stranger.hasInstance=true; stranger.identity=18;
        scenarioInstance.event=7;
        Creature actor; actor.entry=actorEntry; Script script(&actor);
        ObjectAccessor::players={{17,&player},{18,&stranger}};
        actor.HandleSpellClick(&player); actor.HandleSpellClick(&stranger);
        assert(script.boardingRequests.size()==2 && actor.hides==0);
        script.UpdateBoardingRequests(4999); actor.HandleSpellClick(&player); assert(player.casts.size()==1);
        TempSummon bird; bird.summoned=true; bird.entry=actorEntry==105883?105886:106205; bird.summoner=&stranger; stranger.vehicle=&bird;
        script.UpdateBoardingRequests(1);
        assert(script.boardingRequests.empty() && actor.hidden==std::vector<ObjectGuid>{18});
        actor.HandleSpellClick(&player); assert(player.casts.size()==2 && actor.hides==1);
        actor.HandleSpellClick(&stranger); assert(stranger.casts.size()==1);
        player.vehicle=&bird; bird.summoner=&player;
        script.UpdateBoardingRequests(5000);
        assert(actor.hidden==std::vector<ObjectGuid>({18,17}) && script.boardingRequests.empty());
        player.vehicle=nullptr;
        actor.HandleSpellClick(&player); assert(player.casts.size()==3);
        player.quest=0; script.UpdateBoardingRequests(0); assert(script.boardingRequests.empty());
        player.quest=3; actor.HandleSpellClick(&player); assert(player.casts.size()==4 && actor.hides==2);
        {
            Creature reentry; reentry.entry=actorEntry; Script fresh(&reentry);
            reentry.HandleSpellClick(&player);
            assert(player.casts.size()==5 && fresh.boardingRequests.size()==1 && reentry.hides==0);
        }
        {
            Creature shared; shared.entry=actorEntry; Script separate(&shared);
            Player first,second; first.quest=second.quest=3; first.hasInstance=second.hasInstance=true;
            first.guidHigh=1; second.guidHigh=2;
            ObjectAccessor::players={{first.GetGUID(),&first},{second.GetGUID(),&second}};
            shared.HandleSpellClick(&first); shared.HandleSpellClick(&second);
            assert(separate.boardingRequests.size()==2 && first.casts.size()==1 && second.casts.size()==1);
            TempSummon owned; owned.summoned=true; owned.entry=bird.entry; owned.summoner=&second; second.vehicle=&owned;
            separate.UpdateBoardingRequests(0);
            assert(shared.hidden==std::vector<ObjectGuid>{second.GetGUID()} && separate.boardingRequests.size()==1);
        }
        ++boardingCases;
    }
    std::cout << "PASS: " << boardingCases << " boarding identity/revocation/timeout/group-label cases; ";
#endif
    std::cout << "PASS: " << cases << " extracted native dispatch/visibility and actual AI admission cases; ";
#ifdef BASELINE
    std::cout << "pinned edca8e77 pre-AI rows cast for ineligible/nonplayer clicks and double-cast eligible clicks; rejected AI summons still hide source.\n";
#elif defined(BOARDING_BASELINE)
    std::cout << "pinned643bda5a accepts a summon request without a spawned/boarded actor yet immediately hides its source; no live vehicle/client proof.\n";
#else
    std::cout << "pending requests suppress reentrant/repeated clicks, only actual matching owned vehicle acknowledgment hides source; rejected or timed-out requests retain retry. Native HandleSpellClick false return still dispatches AI; no live spawn, boarding, group/faction gameplay or client rendering proof.\n";
#endif
}
