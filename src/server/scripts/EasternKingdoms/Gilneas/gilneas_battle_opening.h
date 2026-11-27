#ifndef GILNEAS_BATTLE_OPENING_H
#define GILNEAS_BATTLE_OPENING_H

#include "CreatureTextMgr.h"
#include "DB2Stores.h"
#include "GossipDef.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestObjectScope.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "TemporarySummon.h"
#include <span>

namespace GilneasBattle
{
enum : uint32
{
    Quest = 24904,
    Prerequisite = 24676,
    MapId = 654,
    Outpost = 4842,
    BattlePhase = 69485,
    KrennanSpawn = 149660,
    Menu = 11061,
    MenuText = 15377,
    Liam = 38218,
    Almyra = 38466,
    Myriam = 38465,
    Militia = 38221,
    Crossbowman = 38210,
    SoldierAura = 72069,
    Shoot = 6660,
    MarchPoint = 2100,
    Lifetime = 8 * MINUTE * IN_MILLISECONDS,
    SpeechGroup = 30,
};

enum class Stage : uint8 { Speech = 1, March, Assault, FirstWaveCleared };

constexpr uint32 Leaders[] = { Liam, Almyra, Myriam };
Position const LeaderStarts[] = {
    { -1408.661f, 1260.017f, 36.51123f, 1.797689f },
    { -1401.306f, 1251.913f, 36.51123f, 2.024582f },
    { -1412.951f, 1248.698f, 36.51123f, 1.762783f },
};

// The opening march uses the three distinct positions at the Merchant Square
// end of the bridge. The remaining first-wave points lead through the square.
Position const LiamMarch[] = {
    { -1437.231934f, 1371.772461f, 35.555882f }, { -1443.534302f, 1384.691772f, 35.555882f },
    { -1449.132690f, 1392.529663f, 35.555882f }, { -1458.036621f, 1394.339233f, 35.555882f },
    { -1471.946533f, 1396.155151f, 35.555882f }, { -1486.640137f, 1396.526489f, 35.556095f },
    { -1498.510620f, 1396.715576f, 35.576904f }, { -1501.070557f, 1389.945435f, 35.556374f },
};
Position const AlmyraMarch[] = {
    { -1437.389526f, 1393.817383f, 35.555603f }, { -1437.941650f, 1404.163208f, 35.555603f },
    { -1438.317627f, 1412.905151f, 35.555603f }, { -1447.051880f, 1417.736816f, 35.555603f },
    { -1454.315796f, 1423.748657f, 35.555603f }, { -1461.147217f, 1428.819946f, 35.555603f },
    { -1469.281494f, 1430.770996f, 35.555603f }, { -1477.451416f, 1432.033936f, 35.555603f },
    { -1489.788818f, 1433.395752f, 35.667927f },
};
Position const MyriamMarch[] = {
    { -1438.375122f, 1357.833984f, 35.555393f }, { -1450.322998f, 1356.605225f, 35.555393f },
    { -1464.221680f, 1353.181885f, 35.555931f }, { -1478.260376f, 1351.074341f, 35.555931f },
    { -1491.284058f, 1348.592896f, 35.555931f }, { -1502.223022f, 1347.533447f, 35.555931f },
    { -1503.380859f, 1362.383057f, 35.555931f }, { -1503.456055f, 1372.238770f, 35.555931f },
};

Position const CrossbowPositions[] = {
    { -1440.31f, 1425.90f, 35.6393f, 5.41052f }, { -1443.06f, 1428.06f, 35.6393f, 5.41052f },
    { -1443.30f, 1424.79f, 35.6392f, 5.41052f }, { -1461.44f, 1400.33f, 35.6392f, 5.41052f },
    { -1464.05f, 1397.30f, 35.6392f, 5.46288f }, { -1466.62f, 1393.85f, 35.6392f, 5.48033f },
    { -1470.96f, 1435.55f, 35.8477f, 5.28835f }, { -1475.77f, 1430.84f, 35.6392f, 5.42797f },
    { -1479.81f, 1426.92f, 35.6392f, 5.56760f }, { -1492.65f, 1430.91f, 35.6392f, 5.41052f },
};

bool HasBattle(Player const* player)
{
    return player && player->IsInWorld() && player->IsAlive() && !player->IsBeingTeleported() &&
        player->getRace() == RACE_WORGEN && player->GetMapId() == MapId &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE && player->GetQuestRewardStatus(Prerequisite) &&
        player->HasAura(BattlePhase);
}

bool SameAttempt(Creature const* actor, Unit const* other)
{
    auto scope = actor->GetQuestObjectScope().lock();
    if (!scope || actor->IsQuestObjectExpired() || !other || !actor->InSamePhase(other))
        return false;

    if (other->GetQuestObjectScope().lock() == scope)
        return true;

    Player const* player = other->GetCharmerOrOwnerPlayerOrPlayerItself();
    return HasBattle(player) && player->GetGUID() == scope->Owner;
}

// Each moving leader confirms the real per-point spline arrival before the
// encounter consumes that point. A missing callback cannot finish a wave.
struct BattleLeaderAI : ScriptedAI
{
    explicit BattleLeaderAI(Creature* actor) : ScriptedAI(actor) { }

    void MoveTo(Position const& position, uint32 point)
    {
        destination = position;
        expectedPoint = MarchPoint + point;
        movementReported = false;
        me->GetMotionMaster()->MovePoint(expectedPoint, destination);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == expectedPoint)
            movementReported = true;
    }

    bool HasArrived() const { return movementReported && me->GetExactDist(&destination) <= 2.0f; }

    void DamageTaken(Unit*, uint32& damage, DamageEffectType) override { damage = 0; }

    Position destination;
    uint32 expectedPoint = 0;
    bool movementReported = false;
};

struct CrossbowmanAI : ScriptedAI
{
    explicit CrossbowmanAI(Creature* actor) : ScriptedAI(actor) { SetCombatMovement(false); }

    void MoveInLineOfSight(Unit* target) override
    {
        if (!me->getVictim() && SameAttempt(me, target) && me->IsWithinDistInMap(target, 24.0f))
            ScriptedAI::MoveInLineOfSight(target);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        if (!SameAttempt(me, attacker))
            damage = 0;
    }

    void UpdateAI(uint32) override
    {
        if (!UpdateVictim())
            return;
        Unit* victim = me->getVictim();
        if (!SameAttempt(me, victim))
        {
            me->CombatStop();
            return;
        }
        if (me->IsWithinMeleeRange(victim))
            DoMeleeAttackIfReady();
        else
            DoSpellAttackIfReady(Shoot);
    }
};

struct OpeningAI : BattleLeaderAI
{
    explicit OpeningAI(Creature* actor) : BattleLeaderAI(actor) { }

    bool Start(Player& owner)
    {
        me->SetAnimKitId(2409);
        if (me->CastSpell(me, SoldierAura, true) != SPELL_CAST_OK)
            return false;

        leaders[0] = me->GetGUID();
        for (size_t index = 1; index < leaders.size(); ++index)
        {
            auto leader = Summon(Leaders[index], LeaderStarts[index]);
            if (!leader || !leader->AIM_Initialize(new BattleLeaderAI(leader)))
                return false;
            leaders[index] = leader->GetGUID();
        }

        // Use a small vanguard at the already captured Livery placements.
        // Each follows its own leader through the bridge and square.
        Position const militiaStarts[][2] = {
            {{ -1406.21f, 1232.23f, 35.6430f, 1.90241f }, { -1406.07f, 1241.32f, 36.0042f, 1.72788f }},
            {{ -1412.06f, 1229.05f, 35.3626f, 1.90241f }, { -1411.20f, 1235.85f, 35.6430f, 1.85005f }},
            {{ -1402.16f, 1232.82f, 35.6430f, 1.83260f }, { -1402.03f, 1236.74f, 35.6430f, 1.97222f }},
        };
        for (size_t index = 0; index < leaders.size(); ++index)
            for (Position const& position : militiaStarts[index])
            {
                auto fighter = Summon(Militia, position);
                if (!fighter)
                    return false;
                militia.emplace_back(fighter->GetGUID(), index);
            }

        ownerGuid = owner.GetGUID();
        return true;
    }

    uint32 GetData(uint32 id) const override { return id == Quest && !stopping ? uint32(stage) : 0; }

    void Stop()
    {
        if (stopping)
            return;
        stopping = true;
        for (ObjectGuid guid : children)
            if (Creature* child = ObjectAccessor::GetCreature(*me, guid))
                child->DespawnOrUnsummon();
        me->DespawnOrUnsummon();
    }

    void SummonedCreatureDies(Creature* actor, Unit* killer) override
    {
        if (stage == Stage::Assault && SameAttempt(me, killer))
            enemies.erase(actor->GetGUID());
    }

    void JustDied(Unit*) override { Stop(); }

    void UpdateAI(uint32 diff) override
    {
        if (stopping)
            return;

        Player* owner = ObjectAccessor::GetPlayer(*me, ownerGuid);
        if (!HasBattle(owner) || !me->InSamePhase(owner) || !me->IsWithinDistInMap(owner, 100.0f) || life <= diff)
        {
            Stop();
            return;
        }
        life -= diff;

        if (stage == Stage::Speech)
        {
            if (timer > diff)
            {
                timer -= diff;
                return;
            }

            constexpr uint32 speechDelay[] = { 10000, 10000, 11000, 9000, 11000, 5000, 500 };
            if (line < std::size(speechDelay))
            {
                sCreatureTextMgr->SendChat(me, SpeechGroup + line, ownerGuid, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
                timer = speechDelay[line++];
                return;
            }

            for (size_t index = 0; index < militia.size(); ++index)
            {
                auto [guid, leaderIndex] = militia[index];
                Creature* leader = ObjectAccessor::GetCreature(*me, leaders[leaderIndex]);
                if (!leader || !leader->IsAlive())
                {
                    Stop();
                    return;
                }
                if (Creature* fighter = ObjectAccessor::GetCreature(*me, guid))
                    fighter->GetMotionMaster()->MoveFollow(leader, 3.0f, index % 2 == 0 ? 2.5f : 3.8f);
            }
            stage = Stage::March;
            AdvanceLeaders();
            return;
        }

        if (stage == Stage::March && AllAtPoint(0))
        {
            for (Position const& position : CrossbowPositions)
            {
                auto enemy = Summon(Crossbowman, position);
                if (!enemy || !enemy->AIM_Initialize(new CrossbowmanAI(enemy)))
                {
                    Stop();
                    return;
                }
                enemies.insert(enemy->GetGUID());
            }
            stage = Stage::Assault;
        }

        if (stage == Stage::Assault)
        {
            AdvanceLeaders();
            if (stopping)
                return;
            for (ObjectGuid guid : enemies)
            {
                auto enemy = ObjectAccessor::GetCreature(*me, guid);
                // Only the accepted death callback can consume a wave actor.
                // A vanished actor cancels the attempt and leaves it retryable.
                if (!enemy || !enemy->IsAlive() || enemy->IsDespawn())
                {
                    Stop();
                    return;
                }
            }
            SupportAssault(diff);
            if (enemies.empty() && AllAtPoint(std::size(AlmyraMarch) - 1))
                stage = Stage::FirstWaveCleared;
        }
    }

private:
    TempSummon* Summon(uint32 entry, Position const& position)
    {
        auto child = me->GetMap()->SummonCreature(entry, position, sSummonPropertiesStore.LookupEntry(2912), 0, me);
        if (child)
        {
            children.push_back(child->GetGUID());
            child->SetTempSummonType(TEMPSUMMON_MANUAL_DESPAWN);
            child->SetReactState(entry == Crossbowman ? REACT_AGGRESSIVE : REACT_PASSIVE);
            child->setActive(true);
        }
        return child;
    }

    std::span<Position const> Path(size_t index) const
    {
        switch (Leaders[index])
        {
            case Liam: return LiamMarch;
            case Almyra: return AlmyraMarch;
            default: return MyriamMarch;
        }
    }

    bool AllAtPoint(size_t point) const
    {
        for (size_t index = 0; index < leaders.size(); ++index)
        {
            auto actor = ObjectAccessor::GetCreature(*me, leaders[index]);
            if (!actor || !actor->IsAlive())
                return false;
            auto ai = static_cast<BattleLeaderAI*>(actor->AI());
            if (!ai->HasArrived() || ai->expectedPoint < MarchPoint + std::min(point, Path(index).size() - 1))
                return false;
        }
        return true;
    }

    void AdvanceLeaders()
    {
        for (size_t index = 0; index < leaders.size(); ++index)
        {
            auto actor = ObjectAccessor::GetCreature(*me, leaders[index]);
            if (!actor || !actor->IsAlive())
            {
                Stop();
                return;
            }
            auto ai = static_cast<BattleLeaderAI*>(actor->AI());
            if (ai->expectedPoint && !ai->HasArrived())
                continue;
            size_t point = ai->expectedPoint ? ai->expectedPoint - MarchPoint + 1 : 0;
            auto path = Path(index);
            if (point < path.size())
                ai->MoveTo(path[point], uint32(point));
        }
    }

    void SupportAssault(uint32 diff)
    {
        if (volley > diff)
        {
            volley -= diff;
            return;
        }
        volley = 2500;
        for (auto [guid, index] : militia)
        {
            Creature* fighter = ObjectAccessor::GetCreature(*me, guid);
            if (!fighter || !fighter->IsAlive())
                continue;

            Creature* nearest = nullptr;
            float distance = 30.0f;
            for (ObjectGuid enemyGuid : enemies)
                if (Creature* enemy = ObjectAccessor::GetCreature(*me, enemyGuid))
                    if (enemy->IsAlive() && fighter->GetDistance(enemy) < distance && fighter->IsWithinLOSInMap(enemy))
                    {
                        nearest = enemy;
                        distance = fighter->GetDistance(enemy);
                    }
            if (nearest)
                fighter->CastSpell(nearest, Shoot, false);
        }
    }

    std::array<ObjectGuid, 3> leaders;
    std::vector<ObjectGuid> children;
    std::vector<std::pair<ObjectGuid, size_t>> militia;
    GuidUnorderedSet enemies;
    ObjectGuid ownerGuid;
    Stage stage = Stage::Speech;
    uint32 life = Lifetime;
    uint32 timer = 15000;
    uint32 volley = 2500;
    uint8 line = 0;
    bool stopping = false;
};

bool CanStart(Player* player, Creature const* krennan)
{
    if (!HasBattle(player) || krennan->GetDBTableGUIDLow() != KrennanSpawn || !krennan->IsAlive() || !player->InSamePhase(krennan) ||
        player->GetAreaId() != Outpost || !player->IsWithinDistInMap(krennan, INTERACTION_DISTANCE) ||
        !player->IsWithinLOSInMap(krennan))
        return false;

    for (ObjectGuid guid : *player->GetSummonList(Liam))
        if (Creature* actor = ObjectAccessor::GetCreature(*player, guid))
            if (actor->IsAlive() && !actor->IsDespawn() && actor->HasQuestObjectScope() && !actor->IsQuestObjectExpired() &&
                actor->AI()->GetData(Quest))
                return false;
    return true;
}

bool Begin(Player& owner)
{
    auto properties = sSummonPropertiesStore.LookupEntry(2912);
    if (!properties)
        return false;
    auto root = std::make_unique<TempSummon>(properties, &owner, false);
    Position const& position = LeaderStarts[0];
    Map* map = owner.GetMap();
    if (!root->Create(sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate(), map, owner.GetPhaseMask(), Liam, 0,
        owner.GetTeam(), position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), position.GetOrientation()))
        return false;

    root->SetPhaseId(owner.GetPhases(), false);
    owner.BindQuestObject(Quest, root.get());
    if (!root->HasQuestObjectScope())
        return false;

    root->SetHomePosition(position);
    root->InitStats(Lifetime);
    root->SetTempSummonType(TEMPSUMMON_TIMED_DESPAWN);
    root->SetReactState(REACT_PASSIVE);
    if (!map->AddToMap(static_cast<Creature*>(root.get())))
    {
        owner.GetSummonList(Liam)->remove(root->GetGUID());
        return false;
    }

    TempSummon* leader = root.release();
    auto ai = new OpeningAI(leader);
    if (!leader->AIM_Initialize(ai))
    {
        leader->UnSummon();
        return false;
    }
    leader->setActive(true);
    leader->InitSummon();
    if (!ai->Start(owner))
    {
        ai->Stop();
        return false;
    }
    return true;
}

class npc_gilneas_battle_krennan : public CreatureScript
{
public:
    npc_gilneas_battle_krennan() : CreatureScript("npc_gilneas_battle_krennan") { }

    bool OnGossipHello(Player* player, Creature* actor) override
    {
        if (!CanStart(player, actor))
            return false;
        player->PlayerTalkClass->ClearMenus();
        auto& menu = player->PlayerTalkClass->GetGossipMenu();
        menu.SetMenuId(Menu);
        menu.AddMenuItem(Menu, 0, GOSSIP_SENDER_MAIN, Quest);
        player->PlayerTalkClass->SendGossipMenu(MenuText, actor->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player*, Creature* actor, uint32, uint32) override
    {
        return actor->GetDBTableGUIDLow() == KrennanSpawn;
    }

    struct KrennanAI : ScriptedAI
    {
        explicit KrennanAI(Creature* actor) : ScriptedAI(actor) { }

        void sGossipSelect(Player* player, uint32 menuId, uint32 index) override
        {
            auto const& menu = player->PlayerTalkClass->GetGossipMenu();
            if (menuId != Menu || menu.GetMenuId() != Menu || index != 0 || menu.GetMenuItemAction(index) != Quest || !CanStart(player, me))
                return;

            player->PlayerTalkClass->SendCloseGossip();
            player->PlayerTalkClass->ClearMenus();
            if (!Begin(*player))
                TC_LOG_WARN("scripts", "Gilneas battle: could not assemble the opening for player %u", player->GetGUIDLow());
            else
                sCreatureTextMgr->SendChat(me, 0, player->GetGUID(), CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
        }
    };

    CreatureAI* GetAI(Creature* actor) const override { return new KrennanAI(actor); }
};
}

#endif
