#ifndef hillsbrad_basic_botany_h__
#define hillsbrad_basic_botany_h__

#include "BrazieBotany.h"
#include "ChatTextBuilder.h"
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
constexpr uint32 Pumpkin = 49474;
constexpr uint32 Zombie = 49209;
constexpr uint32 Ghoul = 49232;
constexpr uint32 Aberration = 49485;
constexpr uint32 Abomination = 49694;
constexpr uint32 Warden = 49819;
constexpr uint32 Obelisk = 49911;
constexpr uint32 Fertilizer = 50067;
constexpr uint32 Mower = 49192;
constexpr uint32 IntroduceBotany = 49189;
constexpr uint32 FlowerWave = 49542;
constexpr uint32 GrainsWave = 49553;
constexpr uint32 PumpkinWave = 49561;
constexpr uint32 WardenVictory = 49627;
constexpr uint32 GardenWave = 49626;
constexpr uint32 PlantSpitter = 91649;
constexpr uint32 PlantSunflower = 91646;
constexpr uint32 PlantFreezya = 92440;
constexpr uint32 PickFreezyaSeeds = 92434;
constexpr uint32 PlantRocknut = 91704;
constexpr uint32 PlantStrangler = 91710;
constexpr uint32 StrangleTarget = 91786;
constexpr uint32 Strangle = 91791;
constexpr uint32 PlantPumpkin = 92157;
constexpr uint32 PumpkinExplosion = 92159;
constexpr uint32 PumpkinFuse = 92825;
constexpr uint32 SummonWarden = 92767;
constexpr uint32 WardenHealthBar = 92598;
constexpr uint32 WardenMutagen = 92964;
constexpr uint32 SummonObelisk = 93253;
constexpr uint32 ObeliskVisual = 92871;
constexpr uint32 CreateFertilizer = 93102;
constexpr uint32 FertilizerVisual = 93101;
constexpr uint32 SummonMower = 91680;
constexpr uint32 MowerStartup = 93969;
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
    {PlantStrangler, Strangler, StranglerModel, 1936},
    {PlantPumpkin, Pumpkin, PumpkinBombModel, 484}
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

constexpr SpawnTime PumpkinFirstWave[] =
{
    {17000, 2, Ghoul}, {34000, 0, Zombie}, {44000, 3, Ghoul}, {52500, 1, Zombie}, {62000, 4, Aberration},
    {85000, 0, Aberration}, {85000, 1, Ghoul}, {85000, 2, Ghoul}, {85000, 3, Zombie}, {85000, 4, Zombie},
    {93000, 1, Zombie}, {93000, 2, Aberration}, {93000, 3, Ghoul}
};

constexpr SpawnTime PumpkinFinalWave[] =
{
    {23000, 1, Ghoul}, {29000, 4, Aberration}, {39000, 3, Ghoul},
    {55000, 0, Abomination}, {55000, 2, Abomination}, {55000, 4, Abomination},
    {56000, 1, Aberration}, {56000, 3, Aberration}, {60000, 2, Ghoul}, {62000, 4, Zombie},
    {66000, 0, Ghoul}, {66000, 1, Zombie}, {66000, 3, Ghoul}
};

constexpr SpawnTime WardenArrival[] =
{
    {17000, 2, Ghoul}, {27000, 0, Zombie}, {35000, 3, Ghoul}, {41000, 1, Zombie}, {47000, 4, Aberration},
    {53000, 2, Warden}, {60000, 2, Ghoul}, {66000, 0, Zombie}, {71000, 1, Aberration}, {78000, 3, Ghoul},
    {83000, 4, Zombie}, {88000, 2, Abomination}, {95000, 0, Ghoul}, {102000, 4, Aberration},
    {110000, 3, Ghoul}, {119000, 1, Abomination}, {128000, 4, Zombie}, {135000, 2, Aberration},
    {143000, 0, Ghoul}, {155000, 3, Abomination}, {169000, 2, Ghoul}, {181000, 4, Aberration}
};

constexpr SpawnTime GardenFirstWave[] =
{
    {17000, 2, Ghoul}, {30000, 1, Zombie}, {38000, 3, Ghoul}, {49000, 0, Ghoul}, {57000, 4, Aberration},
    {74000, 2, Aberration}, {74000, 3, Ghoul}, {74000, 4, Ghoul}, {78000, 0, Zombie}, {78000, 1, Ghoul},
    {83000, 0, Ghoul}, {83000, 2, Zombie}, {87000, 1, Aberration}, {87000, 3, Zombie}, {87000, 4, Zombie}
};

constexpr SpawnTime GardenFinalWave[] =
{
    {21000, 1, Ghoul}, {27000, 3, Aberration}, {34000, 0, Zombie},
    {51000, 0, Abomination}, {51000, 1, Ghoul}, {51000, 2, Aberration}, {51000, 3, Zombie}, {51000, 4, Abomination},
    {59000, 0, Zombie}, {59000, 1, Aberration}, {59000, 2, Abomination}, {59000, 3, Ghoul}, {59000, 4, Ghoul},
    {69000, 0, Ghoul}, {69000, 2, Ghoul}, {69000, 4, Zombie}
};

struct DefenseWave
{
    SpawnTime const* Spawns;
    size_t Count;
    size_t Warning;
};

template <size_t Count>
constexpr DefenseWave MakeWave(SpawnTime const (&spawns)[Count], size_t warning)
{
    return {spawns, Count, warning};
}

inline uint32 WaveCredit(uint32 quest)
{
    switch (quest)
    {
        case GhoulsHateMyGrains: return GrainsWave;
        case PumpkinBombLesson: return PumpkinWave;
        case TendingTheGarden: return GardenWave;
        default: return 0;
    }
}

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
    uint32 NextAction = 0;
};

struct LaneMower
{
    ObjectGuid Actor;
    float LastX = 0.0f;
    bool Running = false;
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
        if (_quest != BasicBotany && _quest != FlowerPower)
        {
            me->m_templateSpells[2] = PlantFreezya;
            me->m_templateSpells[3] = PlantRocknut;
        }
        if (uint32 credit = WaveCredit(_quest))
            _secondWave = summoner->ToPlayer()->GetQuestObjectiveData(_quest, credit) > 0;
        if (_quest == PumpkinBombLesson || _quest == LawnOfTheDead || _quest == TendingTheGarden)
        {
            me->m_templateSpells[4] = PlantStrangler;
            me->m_templateSpells[5] = PlantPumpkin;
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
        else if (summon->GetEntry() == Fertilizer)
            summon->CastSpell(summon, FertilizerVisual, true);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        _summons.Despawn(summon);
        if (_seeds == summon->GetGUID())
            _seeds.Clear();
        if (_fertilizer == summon->GetGUID())
            _fertilizer.Clear();
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (!_enemies.erase(summon->GetGUID()))
            return;
        if (summon->GetGUID() == _warden)
        {
            _wardenDefeated = true;
            if (Player* player = Rider())
                WardenLine(*summon, *player, 49987);
            return;
        }
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
            Notify(*player, _quest == LawnOfTheDead ? 49984 : _quest == BasicBotany ? 49559 : 49566);
        }
        if (_wardenDefeated)
        {
            player->KilledMonsterCredit(WardenVictory);
            Finish();
            return;
        }
        if (diff >= _sunTimer)
        {
            _sunTimer = _elapsed < _fertilizedUntil ? 1000 : 2000;
            constexpr uint8 sunColumns[] = {2, 5, 3, 6, 1, 4, 7, 0};
            CreateSolarPower(Ground(CenterRow, sunColumns[_sunCursor++ % Columns]));
        }
        else
            _sunTimer -= diff;

        if (_quest == FlowerPower)
            UpdateDefense(*player, diff, MakeWave(FlowerPowerWave, 8), FlowerWave);
        else if (_quest == GhoulsHateMyGrains)
            UpdateDefense(*player, diff, _secondWave ? MakeWave(GrainsFinalWave, 3) : MakeWave(GrainsFirstWave, 5), GrainsWave);
        else if (_quest == PumpkinBombLesson)
            UpdateDefense(*player, diff, _secondWave ? MakeWave(PumpkinFinalWave, 3) : MakeWave(PumpkinFirstWave, 5), PumpkinWave);
        else if (_quest == TendingTheGarden)
            UpdateDefense(*player, diff, _secondWave ? MakeWave(GardenFinalWave, 3) : MakeWave(GardenFirstWave, 5), GardenWave);
        else if (_quest == LawnOfTheDead)
        {
            UpdateDefense(*player, diff, MakeWave(WardenArrival, std::size(WardenArrival)), 0);
            if (!_left)
                UpdateWarden(*player);
        }
        else if (_defendedLane)
            UpdateDefense(*player, diff, MakeWave(TutorialWave, 3), IntroduceBotany);
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
        plot.NextAction = _elapsed + (definition.Actor == Pumpkin ? 3000 : 9000);
        if (definition.Actor == Pumpkin)
            plant->CastSpell(plant, PumpkinFuse, true);
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

    bool CollectFertilizer(Creature& fertilizer, Player& player)
    {
        if (_quest != LawnOfTheDead || &player != Rider() || fertilizer.GetGUID() != _fertilizer || fertilizer.IsDespawn())
            return false;
        _fertilizer.Clear();
        _fertilizedUntil = _elapsed + 15000;
        _sunTimer = std::min(_sunTimer, 1000u);
        return true;
    }

    bool IsEnemy(WorldObject const& object) const { return _enemies.count(object.GetGUID()) != 0; }

private:
    static void Notify(Player& player, uint32 textId)
    {
        if (BroadcastTextEntry const* text = sBroadcastTextStore.LookupEntry(textId))
            player.GetSession()->SendNotification("%s", DB2Manager::GetBroadcastTextValue(text, player.GetSession()->GetSessionDbLocaleIndex()));
    }

    static void WardenLine(Creature& warden, Player& player, uint32 textId)
    {
        Trinity::BroadcastTextBuilder builder(&warden, CHAT_MSG_MONSTER_YELL, textId, &player);
        std::unique_ptr<WorldPackets::Packet> packet(builder(player.GetSession()->GetSessionDbLocaleIndex()));
        player.SendDirectMessage(packet->Write());
        if (auto text = sBroadcastTextStore.LookupEntry(textId); text && text->SoundEntriesID[0])
            warden.PlayDirectSound(text->SoundEntriesID[0], &player);
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

    Position EnemyStart(uint8 row) const
    {
        Position position = Ground(row, Columns - 1);
        position.m_positionX += PlotSpacing;
        return position;
    }

    bool BuildPlots()
    {
        for (uint8 row = 0; row < Rows; ++row)
        {
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
            Position start = Ground(row, 0);
            start.m_positionX -= 5.0f;
            TempSummon* mower = me->SummonCreature(Mower, start, ObjectGuid::Empty, TEMPSUMMON_MANUAL_DESPAWN, 0,
                SummonMower, sSummonPropertiesStore.LookupEntry(3077));
            if (!mower)
                return false;
            mower->AIM_Initialize(new NullCreatureAI(mower));
            mower->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            _mowers[row].Actor = mower->GetGUID();
            _mowers[row].LastX = start.GetPositionX();
        }
        return true;
    }

    bool DefendBreach(uint8 row)
    {
        LaneMower& lane = _mowers[row];
        Creature* mower = ObjectAccessor::GetCreature(*me, lane.Actor);
        if (!mower || !mower->IsAlive())
            return false;
        if (lane.Running)
            return lane.LastX <= FirstColumnX + 3.0f;

        lane.Running = true;
        mower->CastSpell(mower, MowerStartup, true);
        mower->SetWalk(false);
        mower->SetSpeed(MOVE_RUN, 2.0f);
        mower->GetMotionMaster()->MovePoint(1, EnemyStart(row), false);
        return true;
    }

    void SweepLanes()
    {
        for (uint8 row = 0; row < Rows; ++row)
        {
            LaneMower& lane = _mowers[row];
            Creature* mower = lane.Running ? ObjectAccessor::GetCreature(*me, lane.Actor) : nullptr;
            if (!mower)
                continue;
            float currentX = mower->GetPositionX();
            if (currentX <= lane.LastX)
                continue;
            std::vector<ObjectGuid> swept;
            for (auto const& [guid, enemyRow] : _enemies)
                if (enemyRow == row)
                    if (Creature* enemy = ObjectAccessor::GetCreature(*me, guid); enemy && enemy->IsAlive() &&
                        enemy->GetEntry() != Warden && enemy->GetEntry() != Obelisk &&
                        enemy->GetPositionX() >= lane.LastX - 2.0f && enemy->GetPositionX() <= currentX + 2.0f)
                        swept.push_back(guid);
            lane.LastX = currentX;
            // A real death callback removes the enemy from the lawn ledger.
            for (ObjectGuid guid : swept)
                if (Creature* enemy = ObjectAccessor::GetCreature(*me, guid); enemy && enemy->IsAlive())
                    mower->Kill(enemy);
            if (currentX >= EnemyStart(row).GetPositionX() - 0.5f)
            {
                mower->DespawnOrUnsummon(1);
                lane.Actor.Clear();
            }
        }
    }

    Creature* SpawnEnemy(uint32 entry, uint8 row)
    {
        bool warden = entry == Warden;
        bool obelisk = entry == Obelisk;
        Position start = obelisk ? Ground(row, Columns - 2) : EnemyStart(row);
        uint32 spell = warden ? SummonWarden : obelisk ? SummonObelisk : 0;
        TempSummon* enemy = spell ? me->SummonCreature(entry, start, ObjectGuid::Empty, TEMPSUMMON_CORPSE_TIMED_DESPAWN,
            2000, spell, sSummonPropertiesStore.LookupEntry(3077)) :
            me->SummonCreature(entry, start, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2000);
        if (!enemy)
            return nullptr;
        // The lawn owns movement, melee and spell selection for these summons.
        enemy->AIM_Initialize(new NullCreatureAI(enemy));
        uint32 health = warden ? 35000 : obelisk ? 2420 : entry == Abomination ? 7260 :
            entry == Aberration ? 3388 : entry == Ghoul ? 968 : 390;
        enemy->SetMaxHealth(health);
        enemy->SetHealth(health);
        enemy->setRegeneratingHealth(false);
        enemy->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
        enemy->SetWalk(true);
        enemy->SetSpeed(MOVE_WALK, entry == Abomination ? 0.5f : entry == Zombie ? 0.6f : 0.85f);
        _enemies.emplace(enemy->GetGUID(), row);
        if (obelisk)
        {
            enemy->CastSpell(enemy, ObeliskVisual, true);
            enemy->ApplySpellImmune(0, IMMUNITY_ID, VenomSpit, true);
            enemy->ApplySpellImmune(0, IMMUNITY_ID, FreezyaBlast, true);
        }
        else if (!warden)
            enemy->GetMotionMaster()->MovePoint(1, Ground(row, 0), false);
        return enemy;
    }

    void UpdateDefense(Player& player, uint32 diff, DefenseWave wave, uint32 credit)
    {
        _elapsed += diff;
        while (_published < wave.Count && _elapsed - _waveStart >= wave.Spawns[_published].Delay)
        {
            SpawnTime const& spawn = wave.Spawns[_published];
            Creature* enemy = SpawnEnemy(spawn.Entry, spawn.Row);
            if (!enemy)
                break;
            if (spawn.Entry == Warden)
            {
                _warden = enemy->GetGUID();
                _nextWardenCast = _elapsed + 10000;
                me->CastSpell(me, WardenHealthBar, true);
                WardenLine(*enemy, player, 49986);
                Notify(player, 49985);
            }
            else if (_quest == BasicBotany && _published == 1)
                Notify(player, 49558);
            else if (_published == wave.Warning)
                Notify(player, 49409);
            ++_published;
        }
        if (credit && _defeated == wave.Count)
        {
            player.KilledMonsterCredit(credit);
            if (WaveCredit(_quest) && !_secondWave)
            {
                _secondWave = true;
                _waveStart = _elapsed;
                _published = 0;
                _defeated = 0;
                if (_quest == GhoulsHateMyGrains)
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
        SweepLanes();
        for (auto& row : _plots)
            for (Plot& plot : row)
                UpdatePlant(plot);
        for (auto const& [guid, row] : _enemies)
            if (Creature* zombie = ObjectAccessor::GetCreature(*me, guid); zombie && zombie->IsAlive() &&
                zombie->GetEntry() != Obelisk && zombie->GetEntry() != Warden)
            {
                if (zombie->GetPositionX() <= FirstColumnX + 0.75f)
                {
                    if (!DefendBreach(row))
                    {
                        Finish();
                        return;
                    }
                    continue;
                }
                UpdateZombie(*zombie, row);
            }
    }

    void UpdateWarden(Player& player)
    {
        Creature* warden = ObjectAccessor::GetCreature(*me, _warden);
        if (!warden || !warden->IsAlive())
            return;
        me->SetPower(POWER_ALTERNATE, me->GetMaxPower(POWER_ALTERNATE) * warden->GetHealthPct() / 100.0f);
        if (warden->movespline->Finalized())
        {
            warden->GetMotionMaster()->MovePoint(1, EnemyStart(_wardenMovesNorth ? 0 : Rows - 1), false);
            _wardenMovesNorth = !_wardenMovesNorth;
        }
        if (_elapsed < _nextWardenCast)
            return;
        _nextWardenCast = _elapsed + 14000;
        if (_wardenUsesMutagen)
        {
            WardenLine(*warden, player, 49841);
            unsigned poisoned = 0;
            for (uint8 offset = 0; offset < Rows * Columns && poisoned < 2; ++offset)
            {
                _wardenPlot = (_wardenPlot + 7) % (Rows * Columns);
                Plot const& plot = _plots[_wardenPlot / Columns][_wardenPlot % Columns];
                if (Creature* plant = ObjectAccessor::GetCreature(*me, plot.Plant); plant && plant->IsAlive() &&
                    plot.PlantEntry != Pumpkin && !plant->HasAura(WardenMutagen) && warden->AddAura(WardenMutagen, plant))
                    ++poisoned;
            }
            if (!_fertilizer)
                if (TempSummon* fertilizer = me->SummonCreature(Fertilizer, Ground(CenterRow, 2), ObjectGuid::Empty,
                    TEMPSUMMON_TIMED_DESPAWN, 15000, CreateFertilizer, sSummonPropertiesStore.LookupEntry(3048)))
                    _fertilizer = fertilizer->GetGUID();
        }
        else
        {
            for (uint8 row : {_obeliskRow, uint8((_obeliskRow + 2) % Rows)})
            {
                bool occupied = std::any_of(_enemies.begin(), _enemies.end(), [&](auto const& enemy)
                {
                    return enemy.first.GetEntry() == Obelisk && enemy.second == row;
                });
                if (!occupied)
                    SpawnEnemy(Obelisk, row);
            }
            _obeliskRow = (_obeliskRow + 1) % Rows;
        }
        _wardenUsesMutagen = !_wardenUsesMutagen;
    }

    void UpdatePlant(Plot& plot)
    {
        if (!plot.Plant)
            return;
        Creature* plant = ObjectAccessor::GetCreature(*me, plot.Plant);
        bool firedBomb = plant && plant->IsAlive() && plot.PlantEntry == Pumpkin && _elapsed >= plot.NextAction &&
            plant->CastSpell(plant, PumpkinExplosion, true) == SPELL_CAST_OK;
        if (!plant || !plant->IsAlive() || firedBomb)
        {
            if (firedBomb)
                plant->DespawnOrUnsummon(1);
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
        else if (plot.PlantEntry == Sunflower && _elapsed >= plot.NextAction)
        {
            plot.NextAction = _elapsed + (_elapsed < _fertilizedUntil ? 4500 : 9000);
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
        if (zombie.GetEntry() != Abomination && zombie.HasAura(Strangle))
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
    ObjectGuid _warden;
    ObjectGuid _fertilizer;
    Position _firstWaveLastKill;
    std::array<std::array<Plot, Columns>, Rows> _plots{};
    std::array<LaneMower, Rows> _mowers{};
    std::unordered_map<ObjectGuid, uint8> _enemies;
    GuidUnorderedSet _spentSolar;
    uint32 _quest = 0;
    uint32 _remaining = 10000;
    uint32 _sunTimer = 0;
    uint32 _elapsed = 0;
    uint32 _waveStart = 0;
    uint32 _nextWardenCast = 0;
    uint32 _fertilizedUntil = 0;
    uint32 _attackTimer = 1000;
    uint8 _sunCursor = 0;
    uint8 _wardenPlot = 0;
    uint8 _obeliskRow = 0;
    size_t _published = 0;
    size_t _defeated = 0;
    bool _left = false;
    bool _started = false;
    bool _defendedLane = false;
    bool _seedPlantUnlocked = false;
    bool _secondWave = false;
    bool _wardenDefeated = false;
    bool _wardenMovesNorth = true;
    bool _wardenUsesMutagen = true;
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

class spell_brazie_collect_fertilizer : public SpellScript
{
    PrepareSpellScript(spell_brazie_collect_fertilizer);

    void Collect(SpellEffIndex /*effect*/)
    {
        Creature* fertilizer = GetCaster()->ToCreature();
        Player* player = GetHitPlayer();
        if (!fertilizer || fertilizer->GetEntry() != Fertilizer || !fertilizer->IsAlive() || !player || !fertilizer->CanTargetQuestObject(player))
            return;
        if (auto board = GetBoard(player->GetVehicleBase()); board && board->CollectFertilizer(*fertilizer, *player))
            fertilizer->DespawnOrUnsummon(1);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_brazie_collect_fertilizer::Collect, EFFECT_1, SPELL_EFFECT_DUMMY);
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

class spell_brazie_pumpkin_explosion : public SpellScript
{
    PrepareSpellScript(spell_brazie_pumpkin_explosion);

    void SelectPlots(std::list<WorldObject*>& targets, bool center)
    {
        TempSummon* bomb = GetCaster()->ToTempSummon();
        auto board = bomb && bomb->GetEntry() == Pumpkin ? GetBoard(bomb->GetSummoner()) : nullptr;
        if (!board || !board->Rider())
        {
            targets.clear();
            return;
        }
        targets.remove_if([&](WorldObject* target)
        {
            float distance = std::max(std::abs(bomb->GetPositionX() - target->GetPositionX()),
                std::abs(bomb->GetPositionY() - target->GetPositionY()));
            return !board->IsEnemy(*target) || distance > PlotSpacing * 1.5f ||
                (distance <= PlotSpacing * 0.5f) != center;
        });
    }

    void Center(std::list<WorldObject*>& targets) { SelectPlots(targets, true); }
    void Adjacent(std::list<WorldObject*>& targets) { SelectPlots(targets, false); }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_pumpkin_explosion::Center, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_brazie_pumpkin_explosion::Adjacent, EFFECT_1, TARGET_UNIT_DEST_AREA_ENTRY);
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
    RegisterSpellScript(spell_brazie_collect_fertilizer);
    RegisterSpellScript(spell_brazie_spitter_venom);
    RegisterSpellScript(spell_brazie_freezya_blast);
    RegisterSpellScript(spell_brazie_pumpkin_explosion);
}

#endif
