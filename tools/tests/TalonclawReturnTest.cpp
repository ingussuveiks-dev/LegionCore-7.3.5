#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
using uint16 = uint16_t;
using int32 = int32_t;
using int8 = int8_t;
enum { CLASS_HUNTER = 3, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1 };
#include "ObjectiveFlags.inc"
#include "ObjectiveTypes.inc"
struct QuestObjective { uint32 ObjectID; int8 StorageIndex; uint32 Amount = 1, Type = 0; };
struct Quest {
    std::vector<QuestObjective> objectives{{100111, 0}, {100112, 1}, {103798, 2}};
    auto const& GetObjectives() const { return objectives; }
};
struct QuestStore {
    Quest quest;
    Quest const* GetQuestTemplate(uint32 id) const { return id == 40385 ? &quest : nullptr; }
} questStore;
auto* sQuestDataStore = &questStore;
#define TC_LOG_INFO(...) ((void)0)
struct Player {
    bool world = true, alive = true, teleporting = false, rewarded = false, owned = true;
    uint32 playerClass = 3, map = 1220, zone = 7502, status = 3;
    std::vector<uint32> data{1, 1, 0};
    uint32 credits = 0;
    bool IsInWorld() const { return world; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return playerClass; }
    uint32 GetMapId() const { return map; }
    uint32 GetZoneId() const { return zone; }
    bool GetQuestRewardStatus(uint32 id) const { assert(id == 40385); return rewarded; }
    uint32 GetQuestStatus(uint32 id) const { assert(id == 40385); return status; }
    int32 GetQuestObjectiveData(uint32, uint32) const;
    int32 GetQuestObjectiveData(Quest const*, int8 index) const { return data.at(index); }
    bool HasItemCount(uint32 item, uint32 count, bool bank) const { assert(item == 128808 && count == 1 && bank); return owned; }
    uint32 GetGUIDLow() const { return 123; }
    void SetQuestObjectiveData(Quest const*, QuestObjective const* objective, uint32 count) { data.at(objective->StorageIndex) = count; }
    void SendQuestUpdateAddCredit(Quest const*, int, QuestObjective const&, uint32) { ++credits; }
    bool CanCompleteQuest(uint32 id) const {
        assert(id == 40385);
        return std::all_of(questStore.quest.objectives.begin(), questStore.quest.objectives.end(), [&](QuestObjective const& objective) {
            return data.at(objective.StorageIndex) >= objective.Amount;
        });
    }
    void CompleteQuest(uint32 id) { assert(id == 40385); status = 1; }
    void SetQuestStatus(uint32 id, uint32 value) { assert(id == 40385); status = value; }
    void KilledMonsterCredit(uint32 real_entry) {
        Quest const* qInfo = sQuestDataStore->GetQuestTemplate(40385);
        uint32 questid = 40385; uint16 addKillCount = 1; int guid = 0;
        if (status != 3) return;
#include "CreditLoop.inc"
    }
};
#include "ObjectiveLookup.inc"
using SpellEffIndex = int;
enum { SPELL_EFFECT_HANDLE_HIT_TARGET = 1 };
struct Unit {
    Player* player;
    bool IsPlayer() const { return player != nullptr; }
    Player* ToPlayer() const { return player; }
};
struct SpellEffect { uint32 MiscValue = 100112; };
struct SpellInfo { SpellEffect effect; SpellEffect const* GetEffect(int, int) const { return &effect; } };
struct Spell {
    Unit* unitTarget;
    SpellInfo const* m_spellInfo;
    int effectHandleMode = 1, m_diffMode = 0;
    void EffectKillCreditPersonal(SpellEffIndex);
};
#include "PersonalCredit.inc"
struct PlayerScript {
    explicit PlayerScript(char const*) {}
    virtual void OnLogin(Player*) {}
    virtual void OnUpdate(Player*, uint32) {}
};
#include "Return.inc"
int main() {
    static_assert(QUEST_OBJECTIVE_FLAG_SEQUENCED == 2 && QUEST_OBJECTIVE_FLAG_OPTIONAL == 4);
    Player player; player_talonclaw_return hook;
    hook.OnLogin(&player); hook.OnUpdate(&player, 1);
    assert(player.data[2] == 1 && player.credits == 1 && player.status == 1);
    for (int gate = 0; gate < 14; ++gate) {
        Player other;
        switch (gate) {
            case 0: other.world = false; break;
            case 1: other.alive = false; break;
            case 2: other.teleporting = true; break;
            case 3: other.playerClass = 1; break;
            case 4: other.map = 1511; break;
            case 5: other.zone = 9999; break;
            case 6: other.rewarded = true; break;
            case 7: other.status = 0; break;
            case 8: other.status = 5; break;
            case 9: other.data[0] = 0; break;
            case 10: other.data[1] = 0; break;
            case 11: other.data[2] = 1; break;
            case 12: other.owned = false; break;
            case 13: other.status = 1; break;
        }
        hook.OnLogin(&other); hook.OnUpdate(&other, UINT32_MAX); assert(other.credits == 0);
    }
    hook.OnLogin(nullptr); hook.OnUpdate(nullptr, 1);
    {
        Player actor; actor.map = 1511; actor.zone = 9999; actor.data = {1, 0, 0};
        TalonclawReturn::Recover(&actor); assert(actor.data[2] == 0);
        Unit owner{&actor}, pet{nullptr}; SpellInfo info;
        Spell claim{&pet, &info}; claim.EffectKillCreditPersonal(0); assert(actor.data[1] == 0);
        claim.unitTarget = nullptr; claim.EffectKillCreditPersonal(0); assert(actor.data[1] == 0);
        claim.unitTarget = &owner; claim.effectHandleMode = 0;
        claim.EffectKillCreditPersonal(0); assert(actor.data[1] == 0);
        claim.effectHandleMode = 1; claim.EffectKillCreditPersonal(0);
        assert(actor.data[1] == 1 && actor.status == 3);
        TalonclawReturn::Recover(&actor); assert(actor.data[2] == 0);
        actor.map = 1220; actor.zone = 7502; actor.teleporting = true;
        TalonclawReturn::Recover(&actor); assert(actor.data[2] == 0);
        actor.teleporting = false; TalonclawReturn::Recover(&actor);
        assert(actor.data[2] == 1 && actor.credits == 2 && actor.status == 1);
    }
    {
        Player actor; actor.data = {1, 0, 0}; questStore.quest.objectives[1].Amount = 0;
        actor.KilledMonsterCredit(100112);
        assert(actor.data[1] == 0 && actor.credits == 0);
        questStore.quest.objectives[1].Amount = 1;
        actor.KilledMonsterCredit(100112);
        assert(actor.data[1] == 1 && actor.credits == 1);
    }
    std::cout << "PASS: production return hooks/objective lookup and monster-credit loop, persisted claim, actual Dalaran arrival, retry/relogin/invalidation, no shop-only, pet or group-wide return award\n";
}
