#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "QuestData.h"
#include "SpellScript.h"
#include <algorithm>
#include <map>
#include <memory>

namespace StormheimFollowup
{
constexpr uint32 ChargeQuest = 38058, RescueQuest = 38059, ClueQuest = 38360;
constexpr uint32 Pack = 180645, ChargedPack = 180860, ActorReady = 38059;

bool Scope(WorldObject* object)
{
    return object && object->IsInWorld() && object->GetMapId() == 1220 && object->GetZoneId() == 7541;
}
bool Active(Player* player, uint32 quest, uint32 team)
{
    return Scope(player) && player->IsAlive() && !player->IsBeingTeleported() &&
        player->GetTeam() == team && player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}
uint32 Value(Player* player, uint32 quest, uint32 entry)
{
    int32 count = player->GetQuestObjectiveData(quest, entry);
    return count < 0 ? 0 : uint32(count);
}
void Credit(Player* player, uint32 id, uint32 entry, uint32 amount, uint32 type = QUEST_OBJECTIVE_MONSTER)
{
    if (player->GetQuestStatus(id) != QUEST_STATUS_INCOMPLETE)
        return;
    if (Quest const* quest = sQuestDataStore->GetQuestTemplate(id))
        for (QuestObjective const& objective : quest->GetObjectives())
            if (objective.Type == type && objective.ObjectID == entry)
            {
                uint32 before = Value(player, id, entry);
                uint32 after = std::min<uint32>(objective.Amount, before + amount);
                if (after <= before) return;
                player->SetQuestObjectiveData(quest, &objective, after);
                player->SendQuestUpdateAddCredit(quest, ObjectGuid::Empty, objective, after);
                if (player->CanCompleteQuest(id)) player->CompleteQuest(id);
                return;
            }
}
void SyncCharge(Player* player)
{
    QuestStatus status = player->GetQuestStatus(ChargeQuest);
    if (!Scope(player) || player->GetTeam() != ALLIANCE ||
        (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE))
    {
        player->RemoveAurasDueToSpell(Pack);
        // Signal Boost uses the charged pack too; preserve it during that quest.
        if (player->GetQuestStatus(38060) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(38060) != QUEST_STATUS_COMPLETE)
            player->RemoveAurasDueToSpell(ChargedPack);
        return;
    }
    uint32 charge = std::min<uint32>(100, Value(player, ChargeQuest, 90903));
    if (charge == 100)
    {
        Credit(player, ChargeQuest, 90866, 1);
        player->RemoveAurasDueToSpell(Pack);
        if (player->IsAlive() && !player->HasAura(ChargedPack)) player->CastSpell(player, ChargedPack, true);
        return;
    }
    if (!player->IsAlive() || player->IsBeingTeleported()) return;
    if (!player->HasAura(Pack) && player->HasAuraType(SPELL_AURA_ENABLE_ALT_POWER)) return;
    if (!player->HasAura(Pack)) player->CastSpell(player, Pack, true);
    if (player->HasAura(Pack)) player->SetPower(POWER_ALTERNATE, charge);
}
void Charge(Player* player, uint32 amount)
{
    if (!Active(player, ChargeQuest, ALLIANCE) || !player->HasAura(Pack)) return;
    Credit(player, ChargeQuest, 90903, amount);
    SyncCharge(player);
}
bool Interact(Player* player, WorldObject* object)
{
    return Scope(player) && Scope(object) && player->IsAlive() && !player->IsBeingTeleported() &&
        !player->GetVehicle() && !player->GetTransport() && player->GetExactDist(object) <= 6.0f &&
        player->InSamePhase(object) && player->canSeeOrDetect(object) && player->IsWithinLOSInMap(object);
}
bool Drake(Creature* creature)
{
    if (!Scope(creature)) return false;
    switch (creature->GetEntry())
    {
        case 90903: case 91486: case 91920: case 91202: case 91239: case 91488: return true;
        default: return false;
    }
}
void DirtyClicks(Player* player)
{
    if (!Scope(player)) return;
    for (uint32 entry : {90948u, 91904u})
    {
        std::list<Creature*> actors;
        GetCreatureListWithEntryInGrid(actors, player, entry, 150.0f);
        for (Creature* actor : actors) actor->ForceValuesUpdateAtIndex(UNIT_FIELD_NPC_FLAGS);
    }
}
}

struct npc_stormheim_followup_actor : SmartAI
{
    explicit npc_stormheim_followup_actor(Creature* creature) : SmartAI(creature) { }
    uint32 releaseTimer = 0, breathTimer = 3000;
    bool Scoped() const { return StormheimFollowup::Scope(me); }
    void Pose()
    {
        auto data = me->GetCreatureTemplate();
        me->SetVisible(true);
        me->SetStandState(UNIT_STAND_STATE_KNEEL);
        // Creature initialization in this core writes all four state values to
        // WORLD_EFFECT_ID. Correct this actor's native visual/animation fields.
        me->SetUInt32Value(UNIT_FIELD_STATE_WORLD_EFFECT_ID, data->StateWorldEffectID);
        me->SetUInt32Value(UNIT_FIELD_STATE_SPELL_VISUAL_ID, data->SpellStateVisualID);
        me->SetUInt32Value(UNIT_FIELD_STATE_ANIM_ID, data->SpellStateAnimID);
        me->SetUInt32Value(UNIT_FIELD_STATE_ANIM_KIT_ID, data->SpellStateAnimKitID);
        me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        me->ForceValuesUpdateAtIndex(UNIT_FIELD_NPC_FLAGS);
    }
    void Reset() override
    {
        SmartAI::Reset();
        releaseTimer = 0; breathTimer = 3000;
        if (Scoped() && me->GetEntry() == 90948) Pose();
    }
    void EnterCombat(Unit* enemy) override
    {
        if (Scoped() && me->GetEntry() == 90903) me->RemoveAurasDueToSpell(133131);
        SmartAI::EnterCombat(enemy);
    }
    uint32 GetData(uint32 id) const override
    {
        if (Scoped() && me->GetEntry() == 90948 && id == StormheimFollowup::ActorReady) return releaseTimer == 0;
        return SmartAI::GetData(id);
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (!Scoped() || me->GetEntry() != 90948 || id != StormheimFollowup::ActorReady)
        { SmartAI::SetData(id, value); return; }
        if (releaseTimer) return;
        releaseTimer = 20000;
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        me->ForceValuesUpdateAtIndex(UNIT_FIELD_NPC_FLAGS);
        me->SetUInt32Value(UNIT_FIELD_STATE_SPELL_VISUAL_ID, 0);
        me->SetUInt32Value(UNIT_FIELD_STATE_ANIM_KIT_ID, 0);
        me->SetStandState(UNIT_STAND_STATE_STAND);
    }
    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (!Scoped() || me->GetEntry() != 90948) SmartAI::SpellHit(caster, spell);
    }
    void UpdateAI(uint32 diff) override
    {
        if (!Scoped()) { SmartAI::UpdateAI(diff); return; }
        if (me->GetEntry() == 90948)
        {
            if (!releaseTimer) return;
            if (releaseTimer <= diff) { releaseTimer = 0; Pose(); }
            else { releaseTimer -= diff; if (releaseTimer <= 15000) me->SetVisible(false); }
            return;
        }
        if (me->GetEntry() != 90903) { SmartAI::UpdateAI(diff); return; }
        if (!UpdateVictim()) return;
        if (breathTimer <= diff)
        {
            if (!me->HasUnitState(UNIT_STATE_CASTING)) { DoCastVictim(183386); breathTimer = 6000; }
        }
        else breathTimer -= diff;
        DoMeleeAttackIfReady();
    }
};

class spell_stormheim_followup_click : public SpellScript
{
    PrepareSpellScript(spell_stormheim_followup_click);
    ObjectGuid actor;
    std::weak_ptr<uint8> lifetime;
    bool captured = false, hit = false, completed = false;
    uint32 QuestId() { return GetSpellInfo()->Id == 180737 ? 38059 : 38360; }
    Creature* Target(Player* player) { return player && captured ? ObjectAccessor::GetCreature(*player, actor) : nullptr; }
    bool Valid(Player* player, Creature* creature)
    {
        using namespace StormheimFollowup;
        bool rescue = GetSpellInfo()->Id == 180737;
        if (!Active(player, QuestId(), rescue ? ALLIANCE : HORDE) || !Interact(player, creature) || !creature->IsAlive() ||
            creature->GetEntry() != (rescue ? 90948u : 91904u) ||
            !creature->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK)) return false;
        return rescue ? creature->AI()->GetData(ActorReady) && Value(player, RescueQuest, 90948) < 6 :
            Value(player, ClueQuest, 241031) && !Value(player, ClueQuest, 91904);
    }
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!StormheimFollowup::Scope(player)) return SPELL_CAST_OK;
        if (!captured)
        {
            // Self-targeted effect 134 clears the explicit NPC target. The
            // engine retains the original clicked target separately.
            Creature* creature = GetOriginalTarget() ? GetOriginalTarget()->ToCreature() : nullptr;
            if (!Valid(player, creature)) return SPELL_FAILED_BAD_TARGETS;
            actor = creature->GetGUID();
            player->AddQuestDelayedEvent(QuestId(), 0, [] { });
            lifetime = player->getQuestStatus(QuestId())->ScriptLifetime;
            captured = true;
        }
        return !lifetime.expired() && Valid(player, Target(player)) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Action(SpellEffIndex)
    {
        Player* player = GetCaster()->ToPlayer();
        if (QuestId() == 38059 ? GetHitCreature() != Target(player) : GetHitPlayer() != player) return;
        if (!lifetime.expired() && Valid(player, Target(player))) hit = true;
    }
    void Suppress(SpellEffIndex index)
    {
        if (StormheimFollowup::Scope(GetCaster()->ToPlayer())) PreventHitDefaultEffect(index);
    }
    void Complete()
    {
        Player* player = GetCaster()->ToPlayer(); Creature* creature = Target(player);
        if (completed || !hit || lifetime.expired() || !Valid(player, creature)) return;
        completed = true;
        if (QuestId() == 38059)
        {
            creature->AI()->SetData(StormheimFollowup::ActorReady, 1);
            StormheimFollowup::Credit(player, 38059, 90948, 1);
            StormheimFollowup::Charge(player, 1); // Native release's energize amount.
            creature->AI()->Talk(0, player->GetGUID());
        }
        else
        {
            StormheimFollowup::Credit(player, 38360, 91904, 1);
            creature->AI()->Talk(3, player->GetGUID());
        }
        StormheimFollowup::DirtyClicks(player);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stormheim_followup_click::Check);
        if (m_scriptSpellId == 180737)
        {
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_click::Action, EFFECT_0, SPELL_EFFECT_DUMMY);
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_click::Suppress, EFFECT_1, SPELL_EFFECT_ENERGIZE);
        }
        else
        {
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_click::Action, EFFECT_0, SPELL_EFFECT_KILL_CREDIT2);
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_click::Suppress, EFFECT_0, SPELL_EFFECT_KILL_CREDIT2);
        }
        AfterCast += SpellCastFn(spell_stormheim_followup_click::Complete);
    }
};

class spell_stormheim_followup_lightning : public SpellScript
{
    PrepareSpellScript(spell_stormheim_followup_lightning);
    void Charge(SpellEffIndex index)
    {
        using namespace StormheimFollowup;
        Player* player = GetHitPlayer();
        if (!Scope(player)) return;
        PreventHitDefaultEffect(index);
        Creature* source = GetOriginalCaster() ? GetOriginalCaster()->ToCreature() : nullptr;
        if (!Drake(source) || !player->InSamePhase(source)) return;
        int32 amount = GetEffectValue();
        if (amount > 0 && amount <= 4) StormheimFollowup::Charge(player, uint32(amount));
    }
    void Register() override
    {
        if (m_scriptSpellId == 183386)
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_lightning::Charge, EFFECT_1, SPELL_EFFECT_ENERGIZE);
        else
            OnEffectHitTarget += SpellEffectFn(spell_stormheim_followup_lightning::Charge, EFFECT_1, SPELL_EFFECT_KILL_CREDIT);
    }
};

class go_stormheim_followup_clue : public GameObjectScript
{
public:
    go_stormheim_followup_clue() : GameObjectScript("go_stormheim_followup_clue") { }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        using namespace StormheimFollowup;
        if (!Scope(object)) return false;
        if (!Active(player, ClueQuest, HORDE) || !Interact(player, object) || !object->isSpawned() ||
            object->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE)) return true;
        uint32 entry = object->GetEntry();
        if (Value(player, ClueQuest, entry)) return true;
        if (entry == 241032 && (!Value(player, ClueQuest, 241031) || !Value(player, ClueQuest, 91904))) return true;
        if (entry != 241031 && entry != 241032) return true;
        Credit(player, ClueQuest, entry, 1, QUEST_OBJECTIVE_GAMEOBJECT);
        DirtyClicks(player);
        return false; // Retain native page text and goober animation/cooldown.
    }
};

class player_stormheim_followup : public PlayerScript
{
public:
    player_stormheim_followup() : PlayerScript("player_stormheim_followup") { }
    struct Watch { uint32 timer = 0; uint64 flags = 0; };
    std::map<ObjectGuid, Watch> watches;
    uint64 Flags(Player* player)
    {
        using namespace StormheimFollowup;
        return uint64(player->GetQuestStatus(RescueQuest)) | (uint64(player->GetQuestStatus(ClueQuest)) << 4) |
            (uint64(Value(player, ClueQuest, 241031) != 0) << 8) | (uint64(Value(player, ClueQuest, 91904) != 0) << 9);
    }
    void OnLogin(Player* player) override { StormheimFollowup::SyncCharge(player); StormheimFollowup::DirtyClicks(player); }
    void OnUpdate(Player* player, uint32 diff) override
    {
        Watch& watch = watches[player->GetGUID()];
        if (watch.timer > diff) { watch.timer -= diff; return; } watch.timer = 500;
        StormheimFollowup::SyncCharge(player);
        uint64 flags = Flags(player);
        if (flags != watch.flags) { watch.flags = flags; StormheimFollowup::DirtyClicks(player); }
    }
    void OnLogout(Player* player) override { watches.erase(player->GetGUID()); }
};

void AddSC_stormheim_followup()
{
    RegisterCreatureAI(npc_stormheim_followup_actor);
    RegisterSpellScript(spell_stormheim_followup_click);
    RegisterSpellScript(spell_stormheim_followup_lightning);
    new go_stormheim_followup_clue();
    new player_stormheim_followup();
}
