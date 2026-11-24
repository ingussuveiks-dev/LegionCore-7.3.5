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
#include "PassiveAI.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"
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
constexpr uint32 Sunflower = 49193;
constexpr uint32 Freezya = 49618;
constexpr uint32 FreezyaSeeds = 49604;
constexpr uint32 Rocknut = 49205;
constexpr uint32 Strangler = 49207;
constexpr uint32 StranglerSeeds = 49560;
constexpr uint32 Zombie = 49209;
constexpr uint32 Ghoul = 49232;
constexpr uint32 Aberration = 49485;
constexpr uint32 IntroduceBotany = 49189;
constexpr uint32 FlowerWave = 49542;
constexpr uint32 GrainsWave = 49553;
constexpr uint32 PlantSpitter = 91649;
constexpr uint32 PlantSunflower = 91646;
constexpr uint32 PlantFreezya = 92440;
constexpr uint32 PickFreezyaSeeds = 92434;
constexpr uint32 PlantRocknut = 91704;
constexpr uint32 PlantStrangler = 91710;
constexpr uint32 StrangleTarget = 91786;
constexpr uint32 Strangle = 91791;
constexpr uint32 GainSolarPower = 91687;
constexpr uint32 SunSphere = 91707;
constexpr uint32 SeedSparkles = 92406;
constexpr uint32 VenomSpit = 91748;
constexpr uint32 FreezyaBlast = 92441;
constexpr uint32 AddSolarPower = 94068;
constexpr uint8 Rows = 5;
constexpr uint8 Columns = 8;
constexpr uint8 CenterRow = 2;
constexpr float PlotSpacing = 8.0f;
constexpr float FirstColumnX = -108.0f;
constexpr float FirstRowY = 236.0f;
constexpr uint32 Retry = GOSSIP_ACTION_INFO_DEF + 1;

struct PlantDefinition
{
    uint32 Spell;
    uint32 Actor;
    uint32 Model;
    uint32 Health;
};

constexpr PlantDefinition Plants[] =
{
    {PlantSpitter, Spitter, SpitterModel, 360},
    {PlantSunflower, Sunflower, SunflowerModel, 484},
    {PlantFreezya, Freezya, FreezyaModel, 484},
    {PlantRocknut, Rocknut, RocknutModel, 2420},
    {PlantStrangler, Strangler, StranglerModel, 1936}
};

struct SpawnTime
{
    uint32 Delay;
    uint8 Row;
    uint32 Entry;
};

constexpr SpawnTime TutorialWave[] =
{
    {4000, CenterRow, Zombie}, {12000, CenterRow, Zombie}, {20000, CenterRow, Zombie}, {30000, CenterRow, Zombie},
    {32000, CenterRow, Zombie}, {34000, CenterRow, Zombie}, {36000, CenterRow, Zombie}, {38000, CenterRow, Zombie}
};

// Flower Power leaves the outer rows safe. The middle rows introduce ghouls
// after the first zombies, followed by one large mixed wave.
constexpr SpawnTime FlowerPowerWave[] =
{
    {26000, 2, Zombie}, {37000, 1, Zombie}, {48000, 3, Zombie}, {57000, 2, Zombie},
    {70000, 1, Ghoul}, {78000, 3, Zombie}, {84000, 3, Ghoul}, {92000, 2, Ghoul},
    {112000, 1, Zombie}, {112000, 2, Zombie}, {112000, 3, Zombie}, {116000, 2, Ghoul},
    {120000, 1, Ghoul}, {120000, 3, Ghoul}, {124000, 2, Zombie}, {128000, 1, Zombie}, {128000, 3, Zombie}
};

constexpr SpawnTime GrainsFirstWave[] =
{
    {17000, 2, Ghoul}, {35000, 1, Zombie}, {47000, 3, Zombie}, {56000, 0, Ghoul}, {67000, 4, Zombie},
    {90000, 0, Zombie}, {90000, 1, Ghoul}, {90000, 2, Ghoul}, {90000, 3, Zombie}, {90000, 4, Ghoul},
    {96000, 0, Ghoul}, {96000, 2, Zombie}, {96000, 4, Zombie}
};

constexpr SpawnTime GrainsFinalWave[] =
{
    {28000, 1, Zombie}, {38000, 3, Ghoul}, {48000, 0, Ghoul},
    {62000, 0, Zombie}, {62000, 1, Ghoul}, {62000, 2, Aberration}, {62000, 3, Ghoul}, {62000, 4, Zombie},
    {70000, 0, Ghoul}, {70000, 1, Zombie}, {70000, 3, Zombie}, {70000, 4, Ghoul}
};

inline uint32 ActiveQuest(Player const& player)
{
    for (uint32 quest : Quests)
        if (player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE)
            return quest;
    return 0;
}

class npc_brazie_basic_botany : public CreatureScript
{
public:
    npc_brazie_basic_botany() : CreatureScript("npc_brazie_basic_botany") { }

    bool OnQuestAccept(Player* player, Creature* giver, Quest const* quest) override
    {
        if (quest->GetQuestId() == ActiveQuest(*player) && CanStart(player, giver))
            player->CastSpell(player, SummonBoard, true);
        return false;
    }

    bool OnGossipHello(Player* player, Creature* giver) override
    {
        if (!CanStart(player, giver))
            return false;
        player->PlayerTalkClass->ClearMenus();
        player->PrepareQuestMenu(giver->GetGUID());
        player->ADD_GOSSIP_ITEM(GossipOptionNpc::None, "I'm ready to try again.", GOSSIP_SENDER_MAIN, Retry);
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
            ActiveQuest(*player) != 0;
    }
};

struct Plot
{
    ObjectGuid Spot;
    ObjectGuid Plant;
    ObjectGuid Model;
    ObjectGuid Grappled;
    uint32 PlantEntry = 0;
    uint32 NextSolar = 0;
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
        _quest = ActiveQuest(*summoner->ToPlayer());
        me->setFaction(summoner->getFaction());
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPower(POWER_ENERGY, 0);
        if (_quest != BasicBotany)
            me->m_templateSpells[1] = PlantSunflower;
        if (_quest == GhoulsHateMyGrains)
        {
            me->m_templateSpells[2] = PlantFreezya;
            me->m_templateSpells[3] = PlantRocknut;
            _secondWave = summoner->ToPlayer()->GetQuestObjectiveData(_quest, GrainsWave) > 0;
        }
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
        else if (summon->GetEntry() == FreezyaSeeds || summon->GetEntry() == StranglerSeeds)
            summon->CastSpell(summon, SeedSparkles, true);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        _summons.Despawn(summon);
        if (_seeds == summon->GetGUID())
            _seeds.Clear();
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (!_enemies.erase(summon->GetGUID()))
            return;
        ++_defeated;
        if (_quest == FlowerPower && _defeated >= 3)
            DropSeeds(FreezyaSeeds, summon->GetPosition());
        else if (_quest == GhoulsHateMyGrains)
        {
            if (_secondWave && _defeated >= 2)
                DropSeeds(StranglerSeeds, summon->GetPosition());
            else if (!_secondWave && _defeated == std::size(GrainsFirstWave))
                _firstWaveLastKill = summon->GetPosition();
        }
    }

    Player* Rider() const
    {
        Vehicle* kit = me->GetVehicleKit();
        Unit* rider = kit ? kit->GetPassenger(0) : nullptr;
        Player* player = rider ? rider->ToPlayer() : nullptr;
        return player && player->GetGUID() == _owner && player->IsAlive() && !player->IsBeingTeleported() &&
            _quest && player->GetQuestStatus(_quest) == QUEST_STATUS_INCOMPLETE && !me->IsQuestObjectExpired() ? player : nullptr;
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
            Notify(*player, _quest == BasicBotany ? 49559 : 49566);
        }
        if (diff >= _sunTimer)
        {
            _sunTimer = 2000;
            constexpr uint8 sunColumns[] = {2, 5, 3, 6, 1, 4, 7, 0};
            CreateSolarPower(Ground(CenterRow, sunColumns[_sunCursor++ % Columns]));
        }
        else
            _sunTimer -= diff;

        if (_quest == FlowerPower)
            UpdateDefense(*player, diff, FlowerPowerWave, std::size(FlowerPowerWave), 8, FlowerWave);
        else if (_quest == GhoulsHateMyGrains)
        {
            if (_secondWave)
                UpdateDefense(*player, diff, GrainsFinalWave, std::size(GrainsFinalWave), 3, GrainsWave);
            else
                UpdateDefense(*player, diff, GrainsFirstWave, std::size(GrainsFirstWave), 5, GrainsWave);
        }
        else if (_defendedLane)
            UpdateDefense(*player, diff, TutorialWave, std::size(TutorialWave), 3, IntroduceBotany);
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

    PlantDefinition const* AllowedPlant(uint32 spell) const
    {
        if (!me->HasSpell(spell))
            return nullptr;
        for (auto const& plant : Plants)
            if (plant.Spell == spell)
                return &plant;
        return nullptr;
    }

    bool PlacePlant(Plot& plot, PlantDefinition const& definition)
    {
        Creature* spot = ObjectAccessor::GetCreature(*me, plot.Spot);
        if (!spot || plot.Plant || !Rider())
            return false;
        TempSummon* plant = me->SummonCreature(definition.Actor, spot->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN);
        if (!plant)
            return false;
        plant->AIM_Initialize(new NullCreatureAI(plant));
        GameObject* model = me->SummonGameObject(definition.Model, spot->GetPosition(), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0);
        if (!model)
        {
            plant->DespawnOrUnsummon(1);
            return false;
        }
        plant->setFaction(2263);
        plant->setRegeneratingHealth(false);
        plant->SetMaxHealth(definition.Health);
        plant->SetHealth(definition.Health);
        plot.Plant = plant->GetGUID();
        plot.Model = model->GetGUID();
        plot.PlantEntry = definition.Actor;
        plot.NextSolar = _elapsed + 9000;
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

    bool CollectSeeds(Creature& seeds, Player& player)
    {
        if (&player != Rider() || _seedPlantUnlocked || seeds.GetGUID() != _seeds || seeds.IsDespawn())
            return false;
        if (_quest == FlowerPower && seeds.GetEntry() == FreezyaSeeds)
            me->m_templateSpells[2] = PlantFreezya;
        else if (_quest == GhoulsHateMyGrains && seeds.GetEntry() == StranglerSeeds)
            me->m_templateSpells[4] = PlantStrangler;
        else
            return false;
        _seedPlantUnlocked = true;
        player.VehicleSpellInitialize();
        return true;
    }

    bool IsEnemy(WorldObject const& object) const { return _enemies.count(object.GetGUID()) != 0; }

private:
    static void Notify(Player& player, uint32 textId)
    {
        if (BroadcastTextEntry const* text = sBroadcastTextStore.LookupEntry(textId))
            player.GetSession()->SendNotification("%s", DB2Manager::GetBroadcastTextValue(text, player.GetSession()->GetSessionDbLocaleIndex()));
    }

    void CreateSolarPower(Position position)
    {
        position.m_positionZ += 1.0f;
        me->SummonCreature(SolarPower, position, TEMPSUMMON_TIMED_DESPAWN, 10000);
    }

    void DropSeeds(uint32 entry, Position const& position)
    {
        if (!_seedPlantUnlocked && !_seeds)
            if (TempSummon* seedBag = me->SummonCreature(entry, position, TEMPSUMMON_MANUAL_DESPAWN))
                _seeds = seedBag->GetGUID();
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

    void UpdateDefense(Player& player, uint32 diff, SpawnTime const* schedule, size_t count, size_t finalWave, uint32 credit)
    {
        _elapsed += diff;
        while (_published < count && _elapsed - _waveStart >= schedule[_published].Delay)
        {
            SpawnTime const& spawn = schedule[_published];
            Position start = Ground(spawn.Row, Columns - 1);
            start.m_positionX += PlotSpacing;
            TempSummon* zombie = me->SummonCreature(spawn.Entry, start, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2000);
            if (!zombie)
                break;
            // The lawn owns movement, melee and spell selection. Ordinary creature
            // AI can evade and reset health when the lawn issues a melee swing.
            zombie->AIM_Initialize(new NullCreatureAI(zombie));
            uint32 health = spawn.Entry == Aberration ? 3388 : spawn.Entry == Ghoul ? 968 : 390;
            zombie->SetMaxHealth(health);
            zombie->SetHealth(health);
            zombie->setRegeneratingHealth(false);
            zombie->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            zombie->SetWalk(true);
            zombie->SetSpeed(MOVE_WALK, spawn.Entry == Zombie ? 0.6f : 0.85f);
            zombie->GetMotionMaster()->MovePoint(1, Ground(spawn.Row, 0), false);
            _enemies.emplace(zombie->GetGUID(), spawn.Row);
            if (_quest == BasicBotany && _published == 1)
                Notify(player, 49558);
            else if (_published == finalWave)
                Notify(player, 49409);
            ++_published;
        }
        if (_defeated == count)
        {
            player.KilledMonsterCredit(credit);
            if (credit == GrainsWave && !_secondWave)
            {
                _secondWave = true;
                _waveStart = _elapsed;
                _published = 0;
                _defeated = 0;
                DropSeeds(StranglerSeeds, _firstWaveLastKill);
                return;
            }
            Notify(player, _quest == BasicBotany ? 49552 : 49551);
            Finish();
            return;
        }
        if (diff < _attackTimer)
        {
            _attackTimer -= diff;
            return;
        }
        _attackTimer = 1000;
        for (auto& row : _plots)
            for (Plot& plot : row)
                UpdatePlant(plot);
        for (auto const& [guid, row] : _enemies)
            if (Creature* zombie = ObjectAccessor::GetCreature(*me, guid); zombie && zombie->IsAlive())
            {
                if (zombie->GetPositionX() <= FirstColumnX + 0.75f)
                {
                    Finish();
                    return;
                }
                UpdateZombie(*zombie, row);
            }
    }

    void UpdatePlant(Plot& plot)
    {
        if (!plot.Plant)
            return;
        Creature* plant = ObjectAccessor::GetCreature(*me, plot.Plant);
        if (!plant || !plant->IsAlive())
        {
            if (GameObject* model = me->GetMap()->GetGameObject(plot.Model))
                model->Delete();
            if (plot.Grappled)
                if (Creature* enemy = ObjectAccessor::GetCreature(*me, plot.Grappled))
                    enemy->RemoveAurasDueToSpell(Strangle, plot.Plant);
            plot.Plant.Clear();
            plot.Model.Clear();
            plot.Grappled.Clear();
            return;
        }
        if (plot.PlantEntry == Spitter)
            plant->CastSpell(plant, VenomSpit, true);
        else if (plot.PlantEntry == Freezya)
            plant->CastSpell(plant, FreezyaBlast, true);
        else if (plot.PlantEntry == Strangler)
            UpdateStrangler(plot, *plant);
        else if (plot.PlantEntry == Sunflower && _elapsed >= plot.NextSolar)
        {
            plot.NextSolar = _elapsed + 9000;
            CreateSolarPower(plant->GetPosition());
        }
    }

    void UpdateStrangler(Plot& plot, Creature& plant)
    {
        if (Creature* held = ObjectAccessor::GetCreature(*me, plot.Grappled); held && held->IsAlive() &&
            held->HasAura(Strangle, plant.GetGUID()))
            return;
        Creature* nearest = nullptr;
        float distance = PlotSpacing + 0.5f;
        for (auto const& [guid, row] : _enemies)
            if (Creature* enemy = ObjectAccessor::GetCreature(*me, guid); enemy && enemy->IsAlive() && !enemy->HasAura(Strangle) &&
                std::abs(enemy->GetPositionY() - plant.GetPositionY()) < PlotSpacing * 0.5f)
            {
                float candidateDistance = plant.GetExactDist2d(enemy);
                if (candidateDistance < distance)
                {
                    distance = candidateDistance;
                    nearest = enemy;
                }
            }
        if (!nearest || plant.CastSpell(nearest, Strangle, true) != SPELL_CAST_OK || !nearest->HasAura(Strangle, plant.GetGUID()))
            return;
        plot.Grappled = nearest->GetGUID();
        plant.CastSpell(nearest, StrangleTarget, true);
    }

    void UpdateZombie(Creature& zombie, uint8 row)
    {
        if (zombie.HasAura(Strangle))
        {
            if (!zombie.movespline->Finalized())
                zombie.StopMoving();
            return;
        }
        for (Plot const& plot : _plots[row])
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
            zombie.GetMotionMaster()->MovePoint(1, Ground(row, 0), false);
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
    ObjectGuid _seeds;
    Position _firstWaveLastKill;
    std::array<std::array<Plot, Columns>, Rows> _plots{};
    std::unordered_map<ObjectGuid, uint8> _enemies;
    GuidUnorderedSet _spentSolar;
    uint32 _quest = 0;
    uint32 _remaining = 10000;
    uint32 _sunTimer = 0;
    uint32 _elapsed = 0;
    uint32 _waveStart = 0;
    uint32 _attackTimer = 1000;
    uint8 _sunCursor = 0;
    size_t _published = 0;
    size_t _defeated = 0;
    bool _left = false;
    bool _started = false;
    bool _defendedLane = false;
    bool _seedPlantUnlocked = false;
    bool _secondWave = false;
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
        return board && board->AllowedPlant(GetSpellInfo()->Id) && board->FindEmptyPlot(GetExplTargetDest()) ?
            SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void RememberEnergy() { _energyBefore = GetCaster()->GetPower(POWER_ENERGY); }

    void Plant(SpellEffIndex effect)
    {
        PreventHitDefaultEffect(effect);
        auto board = GetBoard(GetCaster());
        if (!board)
            return;
        auto plant = board->AllowedPlant(GetSpellInfo()->Id);
        Plot* plot = board->FindEmptyPlot(GetExplTargetDest());
        if ((!plant || !plot || !board->PlacePlant(*plot, *plant)) && GetCaster()->GetPower(POWER_ENERGY) < _energyBefore)
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

class spell_brazie_collect_freezya_seeds : public SpellScript
{
    PrepareSpellScript(spell_brazie_collect_freezya_seeds);

    void Collect(SpellEffIndex /*effect*/)
    {
        Creature* seeds = GetCaster()->ToCreature();
        Player* player = GetHitPlayer();
        if (!seeds || (seeds->GetEntry() != FreezyaSeeds && seeds->GetEntry() != StranglerSeeds) ||
            !seeds->IsAlive() || !player || !seeds->CanTargetQuestObject(player))
            return;
        if (auto board = GetBoard(player->GetVehicleBase()); board && board->CollectSeeds(*seeds, *player))
            seeds->DespawnOrUnsummon(1);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_brazie_collect_freezya_seeds::Collect, EFFECT_1, SPELL_EFFECT_DUMMY);
    }
};

inline void SelectLaneTarget(Unit* caster, std::list<WorldObject*>& targets)
{
    TempSummon* plant = caster->ToTempSummon();
    if (!plant || (plant->GetEntry() != Spitter && plant->GetEntry() != Freezya))
        return;
    auto board = GetBoard(plant->GetSummoner());
    if (!board)
        return;
    targets.remove_if([&](WorldObject* object)
    {
        return !board->IsEnemy(*object) || std::abs(object->GetPositionY() - plant->GetPositionY()) > PlotSpacing * 0.5f;
    });
    if (targets.size() > 1)
    {
        targets.sort([&](WorldObject* first, WorldObject* second) { return plant->GetExactDistSq(first) < plant->GetExactDistSq(second); });
        targets.resize(1);
    }
}

class spell_brazie_spitter_venom : public SpellScript
{
    PrepareSpellScript(spell_brazie_spitter_venom);

    void TargetLane(std::list<WorldObject*>& targets) { SelectLaneTarget(GetCaster(), targets); }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_spitter_venom::TargetLane, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
    }
};

class spell_brazie_freezya_blast : public SpellScript
{
    PrepareSpellScript(spell_brazie_freezya_blast);

    void TargetLane(std::list<WorldObject*>& targets) { SelectLaneTarget(GetCaster(), targets); }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_freezya_blast::TargetLane, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_freezya_blast::TargetLane, EFFECT_1, TARGET_UNIT_CONE_ENEMY_104);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_freezya_blast::TargetLane, EFFECT_2, TARGET_UNIT_CONE_ENEMY_104);
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
    RegisterSpellScript(spell_brazie_collect_freezya_seeds);
    RegisterSpellScript(spell_brazie_spitter_venom);
    RegisterSpellScript(spell_brazie_freezya_blast);
}

#endif
