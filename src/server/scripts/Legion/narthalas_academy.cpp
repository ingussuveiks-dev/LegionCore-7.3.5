#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "AreaTriggerAI.h"
#include "ScriptedGossip.h"
#include "SpellMgr.h"
#include "GameObject.h"
#include <set>

namespace Narthalas
{
bool Active(Player* player, uint32 quest)
{
    return player && player->IsAlive() && player->GetMapId() == 1220 &&
        player->GetZoneId() == 7334 && player->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE;
}

Player* Owner(Creature* creature)
{
    if (TempSummon* summon = creature->ToTempSummon())
        if (Unit* owner = summon->GetSummoner())
            return owner->ToPlayer();
    return nullptr;
}

bool HasEscort(Player* player)
{
    for (ObjectGuid const& guid : *player->GetSummonList(88889))
        if (Creature* escort = ObjectAccessor::GetCreature(*player, guid))
            if (escort->IsAlive() && Owner(escort) == player)
                return true;
    return false;
}

bool WandLesson(Player* player)
{
    // Existing classroom and instructor position, including its lower floor.
    Position const classroom = {201.533f,6470.87f,-52.8034f,0};
    return (Active(player, 42370) || Active(player, 44784)) &&
        player->GetDistance(classroom) < 60.0f &&
        std::abs(player->GetPositionZ() - classroom.GetPositionZ()) < 8.0f;
}

uint32 NextRune(Player* player)
{
    for (uint32 i = 0; i < 3; ++i)
        if (!player->GetQuestObjectiveData(37729, 89655 + i)) return i;
    return 3;
}
bool RuneLesson(Player* player)
{
    // SceneScriptText -> Path 13470 -> Location 116720, not a guessed origin.
    Position const origin = {200.607635f,6451.277832f,-53.777821f,0};
    return Active(player, 37729) && player->GetDistance(origin) < 35.0f &&
        std::abs(player->GetPositionZ() - origin.GetPositionZ()) < 8.0f;
}
void ClearRunes(Player* player)
{
    for (uint32 spell : {179151u,179152u,179153u})
        if (player->HasAura(spell)) player->RemoveAurasDueToSpell(spell);
}
void StartRune(Player* player)
{
    if (!RuneLesson(player)) return;
    ClearRunes(player);
    uint32 next = NextRune(player);
    if (next < 3) player->CastSpell(player, 179151 + next, true);
}

uint32 const Books[] = {137423,137422,137426};
uint32 const Drawings[] = {107301,107300,107299};
uint32 NextBook(Player* player)
{
    for (uint32 i = 0; i < 3; ++i)
        if (!player->GetQuestObjectiveData(42371, Drawings[i])) return i;
    return 3;
}
Creature* OwnedDrawing(Player* player, uint32 entry)
{
    for (ObjectGuid const& guid : *player->GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            if (creature->IsAlive() && Owner(creature) == player) return creature;
    return nullptr;
}

// The ten existing world.waypoints rows for Farondis 88889.
Position const Walk[] = {
    {-124.78f,6411.12f,27.63f,0}, {-120.03f,6403.82f,27.63f,0},
    {-125.44f,6391.57f,27.64f,0}, {-119.14f,6380.18f,19.25f,0},
    {-106.40f,6378.95f,11.03f,0}, {-95.31f,6391.12f,6.80f,0},
    {-76.10f,6388.81f,1.82f,0}, {-79.87f,6274.28f,1.70f,0},
    {-66.09f,6252.56f,3.30f,0}, {-39.66f,6260.80f,8.57f,0}
};
}

class spell_narthalas_start_walk : public SpellScript
{
    PrepareSpellScript(spell_narthalas_start_walk);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!Narthalas::Active(player, 37467) || player->GetQuestObjectiveData(37467, 88746) ||
            Narthalas::HasEscort(player))
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
        Creature* giver = player->FindNearestCreature(88867, 6.0f);
        return giver && player->IsWithinLOSInMap(giver) ? SPELL_CAST_OK : SPELL_FAILED_OUT_OF_RANGE;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_narthalas_start_walk::Check); }
};

struct npc_narthalas_farondis_walk : public ScriptedAI
{
    npc_narthalas_farondis_walk(Creature* creature) : ScriptedAI(creature) { }
    uint32 point = 0, lifetime = 0;
    bool moving = false, finished = false;

    void IsSummonedBy(Unit* summoner) override
    {
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!Narthalas::Active(player, 37467))
        {
            me->DespawnOrUnsummon();
            return;
        }
        me->AddPlayerInPersonnalVisibilityList(player->GetGUID());
        me->SetReactState(REACT_PASSIVE);
        Talk(0);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE || !moving || id != point + 1)
            return;
        moving = false;
        ++point;
        if (point == 6) Talk(2);
    }

    void UpdateAI(uint32 diff) override
    {
        Player* player = Narthalas::Owner(me);
        lifetime += diff;
        if (!Narthalas::Active(player, 37467) || lifetime >= 600000)
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (finished || !player->IsWithinDistInMap(me, 25.0f))
            return;
        if (point == 10)
        {
            if (me->GetDistance(Narthalas::Walk[9]) < 3.0f && player->IsWithinLOSInMap(me))
            {
                finished = true;
                Talk(3);
                player->KilledMonsterCredit(88746, me->GetGUID());
                me->DespawnOrUnsummon(4000);
            }
        }
        else if (!moving)
        {
            moving = true;
            me->GetMotionMaster()->MovePoint(point + 1, Narthalas::Walk[point]);
        }
    }
};

class spell_narthalas_borrow_robes : public SpellScript
{
    PrepareSpellScript(spell_narthalas_borrow_robes);
    SpellCastResult Check()
    {
        Player* player = GetCaster()->ToPlayer();
        // Effect 157 implicitly targets the caster; InitExplicitTargets removes
        // the redundant clicked unit. Preserve the original spellclick target.
        Creature* student = GetOriginalTarget() ? GetOriginalTarget()->ToCreature() : nullptr;
        if (!Narthalas::Active(player, 37736) || player->HasItemCount(120948, 1) ||
            !student || student->GetEntry() != 89669 || !student->IsAlive())
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
        return player->IsWithinDistInMap(student, 6.0f) && player->IsWithinLOSInMap(student) ?
            SPELL_CAST_OK : SPELL_FAILED_OUT_OF_RANGE;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_narthalas_borrow_robes::Check); }
};

class spell_narthalas_dressing_complete : public SpellScript
{
    PrepareSpellScript(spell_narthalas_dressing_complete);
    void ObsoleteTrigger(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Register() override
    {
        // Native effect 1 references absent 108997. Keep the original reward
        // spell and its dummy/visual effect; do not invent a replacement spell.
        OnEffectLaunch += SpellEffectFn(spell_narthalas_dressing_complete::ObsoleteTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_narthalas_dressing_complete::ObsoleteTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

class player_narthalas_wand : public PlayerScript
{
public:
    player_narthalas_wand() : PlayerScript("player_narthalas_wand") { }
    void Clear(Player* player)
    {
        // Never clear a vehicle/other quest's replacement bar.
        if (player->GetUInt16Value(PLAYER_FIELD_BYTES_5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET) == 711)
            player->SetUInt16Value(PLAYER_FIELD_BYTES_5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET, 0);
        player->RemoveTemporarySpell(212782); // Core preserves permanently known spells.
    }
    void OnLogout(Player* player) override { Clear(player); Narthalas::ClearRunes(player); }
    void OnMapChanged(Player* player) override { OnUpdate(player, 0); }
    void OnUpdate(Player* player, uint32) override
    {
        // Recover pre-migration saves that had all objective counters but were
        // held incomplete by the old EXPLORATION_OR_EVENT flag.
        for (uint32 quest : {42370u,42371u,37729u})
            if (Narthalas::Active(player, quest) && player->CanCompleteQuest(quest))
                player->CompleteQuest(quest);
        if (!Narthalas::RuneLesson(player)) Narthalas::ClearRunes(player);
        uint16 bar = player->GetUInt16Value(PLAYER_FIELD_BYTES_5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET);
        if (Narthalas::WandLesson(player))
        {
            // OverrideSpellData 711 is native and contains only 212782. No
            // surviving aura references it, so own this temporary bar locally.
            if (!bar || bar == 711)
            {
                player->AddTemporarySpell(212782);
                if (!bar) player->SetUInt16Value(PLAYER_FIELD_BYTES_5, PLAYER_BYTES_2_OVERRIDE_SPELLS_UINT16_OFFSET, 711);
            }
        }
        else if (bar == 711 || player->HasSpell(212782))
            Clear(player);
    }
};

class npc_narthalas_instructor : public CreatureScript
{
public:
    npc_narthalas_instructor() : CreatureScript("npc_narthalas_instructor") { }
    bool OnQuestAccept(Player* player, Creature*, Quest const* quest) override
    {
        if (quest->GetQuestId() == 37729) Narthalas::StartRune(player);
        return false;
    }
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->PrepareQuestMenu(creature->GetGUID());
        if (Narthalas::RuneLesson(player) && Narthalas::NextRune(player) < 3)
            player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I am ready to practice the next rune.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->SendCloseGossip();
        if (sender == GOSSIP_SENDER_MAIN && action == GOSSIP_ACTION_INFO_DEF + 1 &&
            player->IsWithinDistInMap(creature, 6.0f) && player->IsWithinLOSInMap(creature))
            Narthalas::StartRune(player);
        return true;
    }
};

class scene_narthalas_rune : public SceneTriggerScript
{
public:
    scene_narthalas_rune() : SceneTriggerScript("scene_narthalas_rune") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string event) override
    {
        if (!scene || scene->MiscValue < 935 || scene->MiscValue > 937) return false;
        uint32 index = scene->MiscValue - 935;
        uint32 spell = 179151 + index;
        if (!Narthalas::RuneLesson(player) || !player->HasAura(spell)) return true;
        // The shipped Lua emits exactly "Credit" after drawing every circle.
        // "complete" is also sent on cancel; it must never give objective credit.
        if (event == "Credit" && Narthalas::NextRune(player) == index)
            player->KilledMonsterCredit(89655 + index);
        else if (event == "complete" && player->GetQuestObjectiveData(37729, 89655 + index))
        {
            player->RemoveAurasDueToSpell(spell);
            Narthalas::StartRune(player);
        }
        return true;
    }
};

class go_narthalas_podium : public GameObjectScript
{
public:
    go_narthalas_podium() : GameObjectScript("go_narthalas_podium") { }
    bool OnGossipHello(Player* player, GameObject* podium) override
    {
        using namespace Narthalas;
        if (!Active(player, 42371) || !player->IsWithinDistInMap(podium, 6.0f) ||
            !player->IsWithinLOSInMap(podium)) return true;
        uint32 stage = NextBook(player);
        if (stage == 3 || !player->HasItemCount(Books[stage], 1) || OwnedDrawing(player, Drawings[stage])) return true;
        // Quest POI points place the combat at the native rune-floor origin.
        Position const floor = {200.607635f,6451.277832f,-53.777821f,0};
        if (Creature* drawing = player->SummonCreature(Drawings[stage], floor,
            TEMPSUMMON_TIMED_DESPAWN, 300000, 0, player->GetGUID()))
        {
            // The native placement spell supplies its credit and book scene.
            // Kill credit remains the core's real death path, never this click.
            player->CastSpell(player, 212912 + stage, true);
        }
        return true; // Consume the original goober's absent spell 212924.
    }
};

struct npc_narthalas_drawing : public ScriptedAI
{
    npc_narthalas_drawing(Creature* creature) : ScriptedAI(creature) { }
    uint32 intro = 6500;
    void IsSummonedBy(Unit*) override
    {
        // SceneScriptText 14125 returns the camera after 3 + 1.5 + 1.5 sec.
        // Do not start a fight while the owner is still watching that scene.
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
    }
    void UpdateAI(uint32 diff) override
    {
        Player* player = Narthalas::Owner(me);
        if (!Narthalas::Active(player, 42371) || !player->IsWithinDistInMap(me, 80.0f))
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (intro)
        {
            if (intro > diff) { intro -= diff; return; }
            intro = 0;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->SetReactState(REACT_AGGRESSIVE);
        }
        if (UpdateVictim()) DoMeleeAttackIfReady();
    }
};

class spell_narthalas_wand : public SpellScript
{
    PrepareSpellScript(spell_narthalas_wand);
    SpellCastResult Check()
    {
        return Narthalas::WandLesson(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }
    void ObsoleteTrigger(SpellEffIndex index) { PreventHitDefaultEffect(index); }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_narthalas_wand::Check);
        // Preserve native effect 0: moving AT 7042 / client entry 11511.
        // Effect 1 instead references removed spell 218570.
        OnEffectLaunch += SpellEffectFn(spell_narthalas_wand::ObsoleteTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
        OnEffectLaunchTarget += SpellEffectFn(spell_narthalas_wand::ObsoleteTrigger, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
    }
};

struct at_narthalas_wand : public AreaTriggerAI
{
    at_narthalas_wand(AreaTrigger* trigger) : AreaTriggerAI(trigger) { }
    std::set<ObjectGuid> hit;
    void OnUpdate(uint32) override
    {
        Player* player = at->GetCaster() ? at->GetCaster()->ToPlayer() : nullptr;
        if (!Narthalas::WandLesson(player)) return;
        // Friendly, non-attackable target dummies are absent from the core's
        // default attackable-unit scan. Use the native moving volume instead.
        std::list<Creature*> targets;
        at->GetCreatureListWithEntryInGrid(targets, 107279, 5.0f);
        at->GetCreatureListWithEntryInGridAppend(targets, 117780, 5.0f);
        for (Creature* target : targets)
            if (target->IsAlive() && at->IsInArea(target) && player->IsWithinLOSInMap(target) &&
                hit.insert(target->GetGUID()).second)
            {
                if (Narthalas::Active(player, 42370)) player->KilledMonsterCredit(107279, target->GetGUID());
                if (Narthalas::Active(player, 44784)) player->KilledMonsterCredit(117780, target->GetGUID());
            }
    }
};

void AddSC_narthalas_academy()
{
    RegisterSpellScript(spell_narthalas_start_walk);
    RegisterSpellScript(spell_narthalas_borrow_robes);
    RegisterSpellScript(spell_narthalas_dressing_complete);
    RegisterCreatureAI(npc_narthalas_farondis_walk);
    new player_narthalas_wand();
    RegisterSpellScript(spell_narthalas_wand);
    RegisterAreaTriggerAI(at_narthalas_wand);
    new npc_narthalas_instructor();
    new scene_narthalas_rune();
    new go_narthalas_podium();
    RegisterCreatureAI(npc_narthalas_drawing);
}
