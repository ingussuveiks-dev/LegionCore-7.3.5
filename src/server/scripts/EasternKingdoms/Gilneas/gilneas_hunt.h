#ifndef GILNEAS_HUNT_H
#define GILNEAS_HUNT_H

#include "CreatureTextMgr.h"
#include "DB2Stores.h"
#include "GossipDef.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "TemporarySummon.h"

namespace GilneasHunt
{
constexpr uint32 Quest = 24902;
constexpr uint32 City = 654;
constexpr uint32 Lorna = 38611;
constexpr uint64 LornaSpawn = 149746;
constexpr uint32 Tobias = 38507;
constexpr uint32 General = 38617;
constexpr uint32 Sylvanas = 38530;
constexpr uint32 Warhowl = 38533;
constexpr uint32 Crenshaw = 38537;
constexpr uint32 CallTobias = 72471;
constexpr uint32 SummonTobias = 72470;
constexpr uint32 Hide = 72476;
constexpr uint32 CallScene = 72239;
constexpr uint32 Menu = 24902;
constexpr uint32 Lifetime = 10 * MINUTE * IN_MILLISECONDS;

enum class Moment : uint8 { Walk, Jump, Gate, Wall, Overhear, Water, Hideout };

struct Stop
{
    Position position;
    Moment moment = Moment::Walk;
};

// The five historical paths plus the three jumps and the final water hideout.
Stop const Route[] =
{
    {{ -1623.235f, 1617.035f, 19.97187f }}, {{ -1624.289f, 1605.623f, 20.74269f }},
    {{ -1601.925f, 1607.654f, 20.11606f }, Moment::Jump},
    {{ -1589.44f, 1607.08f, 21.5989f }}, {{ -1588.55f, 1606.71f, 21.5989f }, Moment::Gate},
    {{ -1586.86f, 1612.12f, 21.5938f }}, {{ -1583.96f, 1617.84f, 20.6476f }},
    {{ -1570.64f, 1618.98f, 20.6031f }}, {{ -1553.42f, 1620.73f, 20.7936f }, Moment::Wall},
    {{ -1548.65f, 1618.41f, 23.1788f }, Moment::Jump}, {{ -1546.69f, 1616.66f, 20.5702f }},
    {{ -1514.13f, 1599.58f, 20.4857f }}, {{ -1500.37f, 1582.93f, 20.4857f }},
    {{ -1496.07f, 1573.47f, 20.4857f }}, {{ -1491.17f, 1575.8f, 20.4857f }, Moment::Overhear},
    {{ -1491.75f, 1577.63f, 20.4869f }}, {{ -1508.6f, 1585.36f, 21.1645f }},
    {{ -1513.0f, 1586.07f, 22.9593f }}, {{ -1518.9f, 1586.12f, 25.3814f }},
    {{ -1528.26f, 1585.74f, 26.5368f }}, {{ -1535.76f, 1582.02f, 26.9762f }},
    {{ -1537.27f, 1580.22f, 27.704f }}, {{ -1539.09f, 1578.09f, 28.5757f }},
    {{ -1541.38f, 1575.43f, 29.2073f }}, {{ -1549.64f, 1567.13f, 29.2208f }},
    {{ -1555.67f, 1570.57f, 29.191f }}, {{ -1567.43f, 1559.65f, 29.1903f }},
    {{ -1565.01f, 1552.39f, 29.2225f }}, {{ -1571.41f, 1545.11f, 29.2021f }},
    {{ -1583.74f, 1534.37f, 29.2224f }}, {{ -1590.25f, 1528.49f, 29.2297f }},
    {{ -1598.51f, 1526.85f, 29.2366f }}, {{ -1612.35f, 1533.04f, 29.23f }, Moment::Water},
    {{ -1614.5f, 1533.9f, 27.26f }, Moment::Jump}, {{ -1613.95f, 1536.012f, 27.4f, 5.6f }, Moment::Hideout},
};

struct Speaker
{
    uint32 entry;
    uint32 spell;
    Position start;
    Position destination;
};

Speaker const Speakers[] =
{
    { Warhowl, 72236, { -1566.053f, 1557.191f, 29.36808f, 4.273f }, { -1593.35f, 1531.32f, 29.24f } },
    { Sylvanas, 72238, { -1567.477f, 1554.569f, 29.36808f, 0.88f }, { -1600.11f, 1518.04f, 29.24f, 5.6f } },
    { Crenshaw, 72245, { -1566.795f, 1555.300f, 29.36808f, 3.275f }, { -1587.55f, 1519.32f, 29.24f } },
};

struct Exchange
{
    uint8 speaker;
    uint8 group;
    uint32 pause;
};

Exchange const Dialogue[] =
{
    { 0, 10, 10000 }, { 1, 10, 8000 }, { 0, 11, 11000 }, { 1, 11, 14000 },
    { 0, 12, 8000 }, { 1, 12, 5000 }, { 2, 10, 10000 }, { 1, 13, 11000 },
};

bool IsActive(Player const* player)
{
    return player && player->IsAlive() && player->getRace() == RACE_WORGEN && player->GetMapId() == City &&
        player->GetQuestRewardStatus(24904) && player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE;
}

bool IsLorna(Creature const* actor)
{
    return actor && actor->GetEntry() == Lorna && actor->GetDBTableGUIDLow() == LornaSpawn && actor->IsAlive();
}

void RestoreLegacy(Player* player)
{
    if (player->getRace() == RACE_WORGEN && player->GetQuestStatus(Quest) == QUEST_STATUS_COMPLETE &&
        !player->GetQuestRewardStatus(Quest) && player->GetQuestObjectiveData(Quest, Sylvanas) < 1)
        player->IncompleteQuest(Quest);
}

struct HuntAI : ScriptedAI
{
    explicit HuntAI(Creature* actor) : ScriptedAI(actor) { }

    static HuntAI* Find(Player& owner)
    {
        for (ObjectGuid guid : *owner.GetSummonList(Tobias))
            if (Creature* actor = owner.GetMap()->GetCreature(guid); actor && actor->IsAlive() && !actor->IsDespawn() &&
                actor->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == SummonTobias && actor->HasQuestObjectScope() && !actor->IsQuestObjectExpired())
                return dynamic_cast<HuntAI*>(actor->AI());
        return nullptr;
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (id == point + 1 && (type == POINT_MOTION_TYPE || type == EFFECT_MOTION_TYPE))
            arrived = true;
    }

    uint32 GetData(uint32 id) const override { return id == Quest ? point : 0; }

    void UpdateAI(uint32 diff) override
    {
        Player* owner = ObjectAccessor::GetPlayer(*me, me->ToTempSummon()->GetSummonerGUID());
        if (finishing)
        {
            Retreat(owner, diff);
            return;
        }
        if (!IsActive(owner) || me->IsQuestObjectExpired() || !me->InSamePhase(owner) || lifetime <= diff)
        {
            End();
            return;
        }
        lifetime -= diff;
        if (delay > diff)
        {
            delay -= diff;
            return;
        }
        delay = 0;
        if (point < std::size(Route))
            Escort(*owner, diff);
        else
            Scene(*owner, diff);
    }

    void PublishSpeakers()
    {
        if (!requestedScene || actors[0] || finishing)
            return;

        for (size_t i = 0; i < std::size(Speakers); ++i)
        {
            Speaker const& speaker = Speakers[i];
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(speaker.spell);
            auto effect = spell ? spell->GetEffect(EFFECT_0) : nullptr;
            auto properties = effect && uint32(effect->MiscValue) == speaker.entry ?
                sSummonPropertiesStore.LookupEntry(effect->MiscValueB) : nullptr;
            Creature* actor = properties ? me->SummonCreature(speaker.entry, speaker.start, ObjectGuid::Empty,
                TEMPSUMMON_TIMED_DESPAWN, Lifetime, speaker.spell, properties) : nullptr;
            if (!actor)
            {
                End();
                return;
            }
            actors[i] = actor->GetGUID();
            Prepare(actor);
            actor->SetWalk(true);
            actor->GetMotionMaster()->MovePoint(1, speaker.destination, false);
        }
    }

    static void Prepare(Creature* actor)
    {
        actor->SetReactState(REACT_PASSIVE);
        actor->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
    }

private:
    static void Say(Creature* speaker, uint8 group, Player const& owner)
    {
        sCreatureTextMgr->SendChat(speaker, group, owner.GetGUID(), CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
    }

    void Escort(Player& owner, uint32 diff)
    {
        Stop const& stop = Route[point];
        if (!moving)
        {
            if (!point)
            {
                if (!me->IsWithinDistInMap(&owner, 20.0f))
                    return;
                Say(me, 10, owner);
            }
            moving = true;
            arrived = false;
            travelLeft = 26000;
            if (stop.moment == Moment::Jump)
                me->GetMotionMaster()->MoveJump(stop.position, 20.0f, point == 2 ? 20.0f : 5.0f, point + 1);
            else
                me->GetMotionMaster()->MovePoint(point + 1, stop.position, point == 0);
            return;
        }
        if (!arrived || me->GetExactDist(stop.position) > 1.5f)
        {
            if (travelLeft <= diff)
                End();
            else
                travelLeft -= diff;
            return;
        }
        if (stop.moment >= Moment::Gate && !me->IsWithinDistInMap(&owner, stop.moment == Moment::Hideout ? 12.0f : 8.0f))
            return;

        switch (stop.moment)
        {
            case Moment::Gate: Say(me, 11, owner); break;
            case Moment::Overhear:
                if (!general)
                {
                    Position const where{ -1476.889f, 1573.613f, 20.56887f, 5.340707f };
                    if (TempSummon* actor = me->SummonCreature(General, where, TEMPSUMMON_TIMED_DESPAWN, 16000))
                    {
                        general = actor->GetGUID();
                        Prepare(actor);
                        Say(actor, 10, owner);
                        delay = 6000;
                        return;
                    }
                    End();
                    return;
                }
                Say(me, 12, owner);
                delay = 6000;
                break;
            case Moment::Water: Say(me, 13, owner); break;
            case Moment::Hideout:
                me->SetFacingTo(stop.position.GetOrientation());
                requestedScene = true;
                me->CastSpell(&owner, Hide, true);
                travelLeft = 25000;
                break;
            default: break;
        }
        ++point;
        moving = false;
    }

    void Scene(Player& owner, uint32 diff)
    {
        if (travelLeft)
        {
            bool reached = true;
            for (size_t i = 0; i < std::size(Speakers); ++i)
            {
                Creature* actor = me->GetMap()->GetCreature(actors[i]);
                reached &= actor && actor->IsAlive() && actor->GetExactDist(Speakers[i].destination) < 1.5f;
            }
            if (!reached)
            {
                if (travelLeft <= diff)
                    End();
                else
                    travelLeft -= diff;
                return;
            }
            travelLeft = 0;
        }
        if (!me->IsWithinDistInMap(&owner, 12.0f))
            return;

        Exchange const& exchange = Dialogue[line];
        Creature* actor = me->GetMap()->GetCreature(actors[exchange.speaker]);
        if (!actor || !actor->IsAlive() || actor->GetQuestObjectScope().lock() != me->GetQuestObjectScope().lock())
        {
            End();
            return;
        }
        Say(actor, exchange.group, owner);
        if (line == 5)
            if (Creature* warhowl = me->GetMap()->GetCreature(actors[0]))
                warhowl->GetMotionMaster()->MovePoint(2, Speakers[0].start, false);
        delay = exchange.pause;
        if (++line == std::size(Dialogue))
        {
            owner.KilledMonsterCredit(Sylvanas, actor->GetGUID());
            finishing = true;
        }
    }

    void Retreat(Player* owner, uint32 diff)
    {
        if (delay > diff)
        {
            delay -= diff;
            return;
        }
        if (retreating)
        {
            End();
            return;
        }
        if (Creature* crenshaw = me->GetMap()->GetCreature(actors[2]))
        {
            if (owner && me->IsWithinDistInMap(owner, 12.0f))
                Say(crenshaw, 11, *owner);
            crenshaw->GetMotionMaster()->MovePoint(2, Speakers[2].start, false);
        }
        if (Creature* queen = me->GetMap()->GetCreature(actors[1]))
            queen->GetMotionMaster()->MovePoint(2, Speakers[1].start, false);
        retreating = true;
        delay = 14000;
    }

    void End()
    {
        Map* map = me->GetMap();
        for (ObjectGuid guid : actors)
            if (Creature* actor = map->GetCreature(guid))
                actor->DespawnOrUnsummon();
        if (Creature* actor = map->GetCreature(general))
            actor->DespawnOrUnsummon();
        me->DespawnOrUnsummon();
    }

    ObjectGuid actors[std::size(Speakers)];
    ObjectGuid general;
    uint32 point = 0;
    uint32 lifetime = Lifetime;
    uint32 delay = 2000;
    uint32 travelLeft = 0;
    size_t line = 0;
    bool moving = false;
    bool arrived = false;
    bool requestedScene = false;
    bool finishing = false;
    bool retreating = false;
};

bool CanCall(Player* player, Creature const* lorna)
{
    return IsActive(player) && IsLorna(lorna) && player->InSamePhase(lorna) &&
        player->IsWithinDistInMap(lorna, INTERACTION_DISTANCE) && player->IsWithinLOSInMap(lorna) && !HuntAI::Find(*player);
}

class spell_gilneas_hunt_tobias : public SpellScript
{
    PrepareSpellScript(spell_gilneas_hunt_tobias);

    void SummonGuide(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        auto owner = GetCaster()->ToPlayer();
        auto lorna = GetOriginalCaster() ? GetOriginalCaster()->ToCreature() : nullptr;
        if (!CanCall(owner, lorna))
            return;
        auto properties = sSummonPropertiesStore.LookupEntry(GetSpellInfo()->GetEffect(index)->MiscValueB);
        if (!properties)
            return;
        Position const start = GetHitDest() ? static_cast<Position const&>(*GetHitDest()) : owner->GetPosition();
        Map* map = owner->GetMap();
        auto guide = std::make_unique<TempSummon>(properties, owner, false);
        if (!guide->Create(sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate(), map, owner->GetPhaseMask(),
            Tobias, 0, owner->GetTeam(), start.GetPositionX(), start.GetPositionY(), start.GetPositionZ(), start.GetOrientation()))
            return;
        guide->SetPhaseId(owner->GetPhases(), false);
        owner->BindQuestObject(Quest, guide.get());
        if (!guide->HasQuestObjectScope())
            return;
        guide->SetHomePosition(start);
        guide->SetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL, SummonTobias);
        HuntAI::Prepare(guide.get());
        guide->InitStats(Lifetime);
        guide->SetTempSummonType(TEMPSUMMON_TIMED_DESPAWN);
        if (!map->AddToMap(static_cast<Creature*>(guide.get())))
        {
            owner->GetSummonList(Tobias)->remove(guide->GetGUID());
            return;
        }
        TempSummon* actor = guide.release();
        if (!actor->AIM_Initialize(new HuntAI(actor)))
        {
            actor->UnSummon();
            return;
        }
        actor->setActive(true);
        actor->InitSummon();
        sCreatureTextMgr->SendChat(lorna, 9, owner->GetGUID(), CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_gilneas_hunt_tobias::SummonGuide, EFFECT_0, SPELL_EFFECT_SUMMON); }
};

class spell_gilneas_hunt_call : public SpellScript
{
    PrepareSpellScript(spell_gilneas_hunt_call);

    void CallGuide(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        auto lorna = GetCaster()->ToCreature();
        auto owner = GetHitUnit() ? GetHitUnit()->ToPlayer() : nullptr;
        if (CanCall(owner, lorna))
            owner->CastSpell(owner, GetSpellInfo()->GetEffect(index)->TriggerSpell, true, nullptr, nullptr, lorna->GetGUID());
    }

    void Register() override { OnEffectHitTarget += SpellEffectFn(spell_gilneas_hunt_call::CallGuide, EFFECT_0, SPELL_EFFECT_FORCE_CAST); }
};

class spell_gilneas_hunt_scene : public SpellScript
{
    PrepareSpellScript(spell_gilneas_hunt_scene);

    void SummonScene(SpellEffIndex index)
    {
        PreventHitDefaultEffect(index);
        if (index != EFFECT_0)
            return;
        auto actor = GetCaster()->ToTempSummon();
        Player* owner = actor ? ObjectAccessor::GetPlayer(*actor, actor->GetSummonerGUID()) : nullptr;
        if (IsActive(owner))
            if (auto ai = HuntAI::Find(*owner); ai && actor->AI() == ai)
                ai->PublishSpeakers();
    }

    void Register() override
    {
        OnEffectLaunch += SpellEffectFn(spell_gilneas_hunt_scene::SummonScene, EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL_4);
        OnEffectLaunchTarget += SpellEffectFn(spell_gilneas_hunt_scene::SummonScene, EFFECT_ALL, SPELL_EFFECT_TRIGGER_SPELL_4);
    }
};

class npc_gilneas_hunt_lorna : public CreatureScript
{
public:
    npc_gilneas_hunt_lorna() : CreatureScript("npc_gilneas_hunt_lorna") { }

    bool OnQuestAccept(Player* player, Creature* actor, ::Quest const* quest) override
    {
        if (quest->GetQuestId() == Quest && CanCall(player, actor))
            actor->CastSpell(player, CallTobias, true);
        return false;
    }

    bool OnGossipHello(Player* player, Creature* actor) override
    {
        if (!IsLorna(actor))
            return false;
        RestoreLegacy(player);
        if (!CanCall(player, actor))
            return false;
        player->PlayerTalkClass->ClearMenus();
        player->PrepareQuestMenu(actor->GetGUID());
        auto& menu = player->PlayerTalkClass->GetGossipMenu();
        menu.SetMenuId(Menu);
        menu.AddMenuItem(0, GossipOptionNpc::None, "Please call Tobias back.", GOSSIP_SENDER_MAIN, Quest, "", 0);
        player->PlayerTalkClass->SendGossipMenu(player->GetGossipTextId(actor), actor->GetGUID());
        return true;
    }

    struct LornaAI : ScriptedAI
    {
        explicit LornaAI(Creature* actor) : ScriptedAI(actor) { }

        void sGossipSelect(Player* player, uint32 menuId, uint32 index) override
        {
            auto const& menu = player->PlayerTalkClass->GetGossipMenu();
            if (menuId != Menu || menu.GetMenuId() != Menu || index != 0 || menu.GetMenuItemAction(index) != Quest || !CanCall(player, me))
                return;
            player->PlayerTalkClass->SendCloseGossip();
            player->PlayerTalkClass->ClearMenus();
            me->CastSpell(player, CallTobias, true);
        }
    };

    CreatureAI* GetAI(Creature* actor) const override { return new LornaAI(actor); }
};

class player_gilneas_hunt : public PlayerScript
{
public:
    player_gilneas_hunt() : PlayerScript("player_gilneas_hunt") { }
    void OnLogin(Player* player) override { RestoreLegacy(player); }
};
}

#endif
