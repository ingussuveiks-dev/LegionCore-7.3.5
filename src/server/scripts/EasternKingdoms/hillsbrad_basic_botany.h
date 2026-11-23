#ifndef hillsbrad_basic_botany_h__
#define hillsbrad_basic_botany_h__

#include "BrazieBotany.h"
#include "DB2Stores.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MoveSpline.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "WorldSession.h"

namespace HillsbradBasicBotany
{
using namespace BrazieBotany;

constexpr uint32 EmptySpot = 49190;
constexpr uint32 SolarPower = 49194;
constexpr uint32 Spitter = 49200;
constexpr uint32 Zombie = 49209;
constexpr uint32 IntroduceBotany = 49189;
constexpr uint32 PlantSpitter = 91649;
constexpr uint32 GainSolarPower = 91687;
constexpr uint32 SunSphere = 91707;
constexpr uint32 VenomSpit = 91748;
constexpr uint32 AddSolarPower = 94068;
constexpr uint8 Rows = 5;
constexpr uint8 Columns = 8;
constexpr uint8 CenterRow = 2;
constexpr float PlotSpacing = 8.0f;
constexpr float FirstColumnX = -108.0f;
constexpr float FirstRowY = 236.0f;
constexpr uint32 Retry = GOSSIP_ACTION_INFO_DEF + 1;

class npc_brazie_basic_botany : public CreatureScript
{
public:
    npc_brazie_basic_botany() : CreatureScript("npc_brazie_basic_botany") { }

    bool OnQuestAccept(Player* player, Creature* giver, Quest const* quest) override
    {
        if (quest->GetQuestId() == BasicBotany && CanStart(player, giver))
            player->CastSpell(player, SummonBoard, true);
        return false;
    }

    bool OnGossipHello(Player* player, Creature* giver) override
    {
        if (!CanStart(player, giver))
            return false;
        player->PlayerTalkClass->ClearMenus();
        player->PrepareQuestMenu(giver->GetGUID());
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I'm ready to try Basic Botany again.", GOSSIP_SENDER_MAIN, Retry);
        player->PlayerTalkClass->SendGossipMenu(player->GetGossipTextId(giver), giver->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* giver, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN || action != Retry || !CanStart(player, giver))
            return false;
        player->PlayerTalkClass->SendCloseGossip();
        player->CastSpell(player, SummonBoard, true);
        return true;
    }

private:
    static bool CanStart(Player* player, Creature* giver)
    {
        return giver->GetDBTableGUIDLow() == GiverSpawn && giver->GetMapId() == BrazieBotany::Map && giver->IsAlive() &&
            player->IsAlive() && !player->GetVehicle() && !player->isInCombat() && !player->isInFlight() &&
            player->IsWithinDistInMap(giver, INTERACTION_DISTANCE) && player->IsWithinLOSInMap(giver) &&
            player->GetQuestStatus(BasicBotany) == QUEST_STATUS_INCOMPLETE;
    }
};

struct Plot
{
    ObjectGuid Spot;
    ObjectGuid Plant;
    ObjectGuid Model;
};

class npc_brazie_basic_botany_board : public ScriptedAI
{
public:
    explicit npc_brazie_basic_botany_board(Creature* creature) : ScriptedAI(creature), _summons(creature) { }

    void OnCharmed(bool /*apply*/) override { }

    void IsSummonedBy(Unit* summoner) override
    {
        auto scope = me->GetQuestObjectScope().lock();
        if (!scope || !summoner || !summoner->IsPlayer() || scope->Owner != summoner->GetGUID() || !me->GetVehicleKit())
        {
            me->DespawnOrUnsummon(1);
            return;
        }
        _owner = summoner->GetGUID();
        me->setFaction(summoner->getFaction());
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPower(POWER_ENERGY, 0);
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (seat != 0 || passenger->GetGUID() != _owner)
            return;
        if (!apply)
        {
            _left = true;
            passenger->RemoveAurasDueToSpell(SummonBoard);
        }
        else if (Player* player = passenger->ToPlayer())
            player->SetClientControl(me, false);
    }

    void JustSummoned(Creature* summon) override
    {
        _summons.Summon(summon);
        summon->SetReactState(REACT_PASSIVE);
        if (summon->GetEntry() == SolarPower)
            summon->CastSpell(summon, SunSphere, true);
    }

    void SummonedCreatureDespawn(Creature* summon) override { _summons.Despawn(summon); }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon->GetEntry() == Zombie && _zombies.erase(summon->GetGUID()))
            ++_defeated;
    }

    Player* Rider() const
    {
        Vehicle* kit = me->GetVehicleKit();
        Unit* rider = kit ? kit->GetPassenger(0) : nullptr;
        Player* player = rider ? rider->ToPlayer() : nullptr;
        return player && player->GetGUID() == _owner && player->IsAlive() && !player->IsBeingTeleported() &&
            player->GetQuestStatus(BasicBotany) == QUEST_STATUS_INCOMPLETE && !me->IsQuestObjectExpired() ? player : nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        Player* player = Rider();
        if (_left || diff >= _remaining || (!player && _started))
        {
            Finish();
            return;
        }
        _remaining -= diff;
        if (!player)
            return;
        if (!_started)
        {
            _started = true;
            _remaining = 5 * MINUTE * IN_MILLISECONDS;
            if (!BuildPlots())
            {
                Finish();
                return;
            }
            Notify(*player, 49559);
        }
        if (diff >= _sunTimer)
        {
            _sunTimer = 2000;
            constexpr uint8 sunColumns[] = {2, 5, 3, 6, 1, 4, 7, 0};
            Position sun = Ground(CenterRow, sunColumns[_sunCursor++ % Columns]);
            sun.m_positionZ += 1.0f;
            me->SummonCreature(SolarPower, sun, TEMPSUMMON_TIMED_DESPAWN, 10000);
        }
        else
            _sunTimer -= diff;

        if (_defendedLane)
            UpdateTutorial(*player, diff);
    }

    Plot* FindEmptyPlot(Position const* destination)
    {
        if (!_started || !destination || !Rider())
            return nullptr;
        for (auto& row : _plots)
            for (Plot& plot : row)
                if (!plot.Plant)
                    if (Creature* spot = ObjectAccessor::GetCreature(*me, plot.Spot))
                        if (std::abs(spot->GetPositionX() - destination->GetPositionX()) < PlotSpacing * 0.5f &&
                            std::abs(spot->GetPositionY() - destination->GetPositionY()) < PlotSpacing * 0.5f &&
                            std::abs(spot->GetPositionZ() - destination->GetPositionZ()) < 5.0f)
                            return &plot;
        return nullptr;
    }

    bool PlaceSpitter(Plot& plot)
    {
        Creature* spot = ObjectAccessor::GetCreature(*me, plot.Spot);
        if (!spot || plot.Plant || !Rider())
            return false;
        TempSummon* plant = me->SummonCreature(Spitter, spot->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
        if (!plant)
            return false;
        GameObject* model = me->SummonGameObject(SpitterModel, spot->GetPosition(), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0);
        if (!model)
        {
            plant->DespawnOrUnsummon(1);
            return false;
        }
        plant->setFaction(2263);
        plant->SetMaxHealth(360);
        plant->SetHealth(360);
        plot.Plant = plant->GetGUID();
        plot.Model = model->GetGUID();
        if (std::abs(spot->GetPositionY() - Ground(CenterRow, 0).GetPositionY()) < 0.5f)
            _defendedLane = true;
        return true;
    }

    bool CollectSolarPower(Creature& orb, Player& player)
    {
        if (&player != Rider() || std::find(_summons.begin(), _summons.end(), orb.GetGUID()) == _summons.end() || orb.IsDespawn() ||
            !_spentSolar.insert(orb.GetGUID()).second)
            return false;
        if (orb.CastSpell(me, AddSolarPower, true) == SPELL_CAST_OK)
            return true;
        _spentSolar.erase(orb.GetGUID());
        return false;
    }

    bool IsTutorialZombie(WorldObject* object) const { return _zombies.count(object->GetGUID()) != 0; }

private:
    static void Notify(Player& player, uint32 textId)
    {
        if (BroadcastTextEntry const* text = sBroadcastTextStore.LookupEntry(textId))
            player.GetSession()->SendNotification("%s", DB2Manager::GetBroadcastTextValue(text, player.GetSession()->GetSessionDbLocaleIndex()));
    }

    Position Ground(uint8 row, uint8 column) const
    {
        float x = FirstColumnX + column * PlotSpacing;
        float y = FirstRowY - row * PlotSpacing;
        float z = me->GetMap()->GetHeight(x, y, 60.0f);
        return {x, y, z, 0.0f};
    }

    bool BuildPlots()
    {
        for (uint8 row = 0; row < Rows; ++row)
            for (uint8 column = 0; column < Columns; ++column)
            {
                Position position = Ground(row, column);
                if (position.GetPositionZ() <= INVALID_HEIGHT)
                    return false;
                TempSummon* spot = me->SummonCreature(EmptySpot, position, TEMPSUMMON_MANUAL_DESPAWN);
                if (!spot)
                    return false;
                spot->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                _plots[row][column].Spot = spot->GetGUID();
            }
        return true;
    }

    void UpdateTutorial(Player& player, uint32 diff)
    {
        // Three examples introduce the lane, followed by the tutorial's five-zombie wave.
        constexpr uint32 waves[] = {4000, 12000, 20000, 30000, 32000, 34000, 36000, 38000};
        _elapsed += diff;
        if (_published < std::size(waves) && _elapsed >= waves[_published])
        {
            Position start = Ground(CenterRow, Columns - 1);
            start.m_positionX += PlotSpacing;
            if (TempSummon* zombie = me->SummonCreature(Zombie, start, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2000))
            {
                zombie->SetMaxHealth(390);
                zombie->SetHealth(390);
                zombie->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
                zombie->GetMotionMaster()->MovePoint(1, Ground(CenterRow, 0), false, 1.5f);
                _zombies.insert(zombie->GetGUID());
                if (_published == 1)
                    Notify(player, 49558);
                else if (_published == 3)
                    Notify(player, 49409);
                ++_published;
            }
        }
        if (_defeated == std::size(waves))
        {
            player.KilledMonsterCredit(IntroduceBotany);
            Notify(player, 49552);
            Finish();
            return;
        }
        if (diff < _attackTimer)
        {
            _attackTimer -= diff;
            return;
        }
        _attackTimer = 1000;
        for (Plot& plot : _plots[CenterRow])
        {
            if (!plot.Plant)
                continue;
            if (Creature* plant = ObjectAccessor::GetCreature(*me, plot.Plant); plant && plant->IsAlive())
            {
                plant->CastSpell(plant, VenomSpit, true);
                continue;
            }
            if (GameObject* model = me->GetMap()->GetGameObject(plot.Model))
                model->Delete();
            plot.Plant.Clear();
            plot.Model.Clear();
        }
        for (ObjectGuid guid : _zombies)
            if (Creature* zombie = ObjectAccessor::GetCreature(*me, guid); zombie && zombie->IsAlive())
            {
                if (zombie->GetPositionX() <= FirstColumnX + 0.75f)
                {
                    Finish();
                    return;
                }
                UpdateZombie(*zombie);
            }
    }

    void UpdateZombie(Creature& zombie)
    {
        for (Plot const& plot : _plots[CenterRow])
        {
            Creature* plant = ObjectAccessor::GetCreature(*me, plot.Plant);
            if (!plant || !plant->IsAlive() || !zombie.IsWithinMeleeRange(plant))
                continue;
            if (!zombie.movespline->Finalized())
                zombie.StopMoving();
            zombie.AttackerStateUpdate(plant);
            return;
        }
        if (zombie.movespline->Finalized())
            zombie.GetMotionMaster()->MovePoint(1, Ground(CenterRow, 0), false, 1.5f);
    }

    void Finish()
    {
        _left = true;
        if (Vehicle* kit = me->GetVehicleKit())
            if (Unit* player = kit->GetPassenger(0); player && player->GetGUID() == _owner)
                player->ExitVehicle();
        if (Player* player = ObjectAccessor::GetPlayer(*me, _owner))
            player->RemoveAurasDueToSpell(SummonBoard);
        for (auto& row : _plots)
            for (Plot& plot : row)
                if (GameObject* model = me->GetMap()->GetGameObject(plot.Model))
                    model->Delete();
        _summons.DespawnAll();
        me->DespawnOrUnsummon(1);
    }

    SummonList _summons;
    ObjectGuid _owner;
    std::array<std::array<Plot, Columns>, Rows> _plots{};
    GuidUnorderedSet _zombies;
    GuidUnorderedSet _spentSolar;
    uint32 _remaining = 10000;
    uint32 _sunTimer = 0;
    uint32 _elapsed = 0;
    uint32 _attackTimer = 1000;
    uint8 _sunCursor = 0;
    size_t _published = 0;
    size_t _defeated = 0;
    bool _left = false;
    bool _started = false;
    bool _defendedLane = false;
};

inline npc_brazie_basic_botany_board* GetBoard(Unit* unit)
{
    Creature* board = unit ? unit->ToCreature() : nullptr;
    if (!board || board->GetEntry() != Board || board->GetMapId() != BrazieBotany::Map)
        return nullptr;
    return CAST_AI(npc_brazie_basic_botany_board, board->AI());
}

class spell_brazie_plant_spitter : public SpellScript
{
    PrepareSpellScript(spell_brazie_plant_spitter);

    SpellCastResult CheckPlot()
    {
        auto board = GetBoard(GetCaster());
        return board && board->FindEmptyPlot(GetExplTargetDest()) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void RememberEnergy() { _energyBefore = GetCaster()->GetPower(POWER_ENERGY); }

    void Plant(SpellEffIndex effect)
    {
        PreventHitDefaultEffect(effect);
        auto board = GetBoard(GetCaster());
        if (!board)
            return;
        Plot* plot = board->FindEmptyPlot(GetExplTargetDest());
        if ((!plot || !board->PlaceSpitter(*plot)) && GetCaster()->GetPower(POWER_ENERGY) < _energyBefore)
            GetCaster()->SetPower(POWER_ENERGY, _energyBefore);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_brazie_plant_spitter::CheckPlot);
        BeforeCast += SpellCastFn(spell_brazie_plant_spitter::RememberEnergy);
        OnEffectHit += SpellEffectFn(spell_brazie_plant_spitter::Plant, EFFECT_0, SPELL_EFFECT_TRANS_DOOR);
    }

    int32 _energyBefore = 0;
};

class spell_brazie_collect_solar_power : public SpellScript
{
    PrepareSpellScript(spell_brazie_collect_solar_power);

    void Collect(SpellEffIndex /*effect*/)
    {
        Creature* orb = GetCaster()->ToCreature();
        Player* player = GetHitPlayer();
        if (!orb || orb->GetEntry() != SolarPower || !orb->IsAlive() || !player || !orb->CanTargetQuestObject(player))
            return;
        if (auto board = GetBoard(player->GetVehicleBase()); board && board->CollectSolarPower(*orb, *player))
            orb->DespawnOrUnsummon(1);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_brazie_collect_solar_power::Collect, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_brazie_spitter_venom : public SpellScript
{
    PrepareSpellScript(spell_brazie_spitter_venom);

    void TargetLane(std::list<WorldObject*>& targets)
    {
        TempSummon* plant = GetCaster()->ToTempSummon();
        auto board = plant && plant->GetEntry() == Spitter ? GetBoard(plant->GetSummoner()) : nullptr;
        if (!board)
            return;
        targets.remove_if([&](WorldObject* object)
        {
            return !board->IsTutorialZombie(object) || std::abs(object->GetPositionY() - plant->GetPositionY()) > PlotSpacing * 0.5f;
        });
        if (targets.size() > 1)
        {
            targets.sort([&](WorldObject* first, WorldObject* second) { return plant->GetExactDistSq(first) < plant->GetExactDistSq(second); });
            targets.resize(1);
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_spitter_venom::TargetLane, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
    }
};
}

inline void AddSC_hillsbrad_basic_botany()
{
    using namespace HillsbradBasicBotany;
    new npc_brazie_basic_botany();
    RegisterCreatureAI(npc_brazie_basic_botany_board);
    RegisterSpellScript(spell_brazie_plant_spitter);
    RegisterSpellScript(spell_brazie_collect_solar_power);
    RegisterSpellScript(spell_brazie_spitter_venom);
}

#endif
