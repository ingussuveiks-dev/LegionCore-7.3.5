#pragma once

// The Dalaran click summons with the native198941/properties61. Receipt100111
// belongs to an actual boarded departure, never HandleSpellClick's cast attempt.
namespace TalonclawBoardedDeparture
{
constexpr uint32 QuestId = 40385, Source = 100965, Ride = 100967, Credit = 100111;
constexpr uint32 Transfer = 198950, Receipt = 100111;

bool SourceActor(Creature* source)
{
    return source && source->GetEntry() == Source && source->GetMapId() == 1220 &&
        source->GetDBTableGUIDLow() == 366115;
}

bool Eligible(Player* player)
{
    return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        player->getClass() == CLASS_HUNTER && player->GetMapId() == 1220 &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && !player->GetQuestRewardStatus(QuestId);
}

bool Contact(Player* player, Creature* source, float distance)
{
    return Eligible(player) && SourceActor(source) && source->IsInWorld() && source->IsAlive() &&
        source->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) &&
        player->GetMap() == source->GetMap() && player->InSamePhase(source) &&
        player->canSeeOrDetect(source) && player->IsWithinDistInMap(source, distance) &&
        player->IsWithinLOSInMap(source);
}
}

struct npc_talonclaw_boarded_ride : ScriptedAI
{
    explicit npc_talonclaw_boarded_ride(Creature* creature) : ScriptedAI(creature) { }
    enum class Stage { Unbound, Joining, Boarded, Exiting, Casting, Finished };
    Stage stage = Stage::Unbound;
    ObjectGuid owner, source;
    std::weak_ptr<uint8> lifetime;
    uint32 age = 0, timer = 0, joinAttempts = 0, castAttempts = 0;
    bool boarded = false;

    Player* Owner() const { return ObjectAccessor::GetPlayer(*me, owner); }
    Creature* Source() const { return ObjectAccessor::GetCreature(*me, source); }
    bool Lease(Player* player) const
    {
        QuestStatusData* quest = player ? player->getQuestStatus(TalonclawBoardedDeparture::QuestId) : nullptr;
        return player && player->GetGUID() == owner && quest && !lifetime.expired() &&
            quest->ScriptLifetime == lifetime.lock() && !me->IsQuestObjectExpired() && me->IsAlive() &&
            player->IsInWorld() && player->IsAlive() && player->getClass() == CLASS_HUNTER &&
            player->GetQuestStatus(TalonclawBoardedDeparture::QuestId) == QUEST_STATUS_INCOMPLETE &&
            !player->GetQuestRewardStatus(TalonclawBoardedDeparture::QuestId) && player->GetMap() == me->GetMap() &&
            player->InSamePhase(me) && me->ToTempSummon() &&
            me->ToTempSummon()->GetSummonerGUID() == owner;
    }
    bool Current(Player* player) const { return Lease(player) && TalonclawBoardedDeparture::Eligible(player); }
    void Restore(Player* player)
    {
        if (!player) return;
        if (player->GetUnitBeingMoved() == me) player->SetClientControl(player, true);
        else if (player->GetViewpoint() == me) player->SetViewpoint(me, false);
    }
    void Stop(char const* reason)
    {
        if (stage == Stage::Unbound || stage == Stage::Finished) return;
        stage = Stage::Finished;
        if (Player* player = Owner())
        {
            if (player->GetVehicleBase() == me) player->ExitVehicle();
            Restore(player);
        }
        TC_LOG_WARN("scripts", "Talonclaw boarding: %s; no departure credit, retain source100965 for retry", reason);
        me->DespawnOrUnsummon(1);
    }
    bool ExitReady(Player* player) const
    {
        return Current(player) && boarded && !player->HasPendingTeleport() && !player->GetVehicle() &&
            !player->waitOnSeat && !player->GetTransport() && !player->isInFlight() &&
            player->movespline->Finalized() && TalonclawBoardedDeparture::Contact(player, Source(), 15.0f);
    }
    void Join(Player* player)
    {
        if (player->GetVehicle() || player->waitOnSeat || player->GetTransport() || player->isInFlight()) return;
        Vehicle* kit = me->GetVehicleKit();
        if (!kit || !kit->HasEmptySeat(0)) return;
        ++joinAttempts;
        timer = 5000;
        player->EnterVehicle(me, 0, true);
    }
    void Begin(Player* player, Creature* actor)
    {
        using namespace TalonclawBoardedDeparture;
        if (stage != Stage::Unbound || !Contact(player, actor, INTERACTION_DISTANCE) ||
            player->HasPendingTeleport() || player->GetVehicle() || player->waitOnSeat || player->GetTransport() ||
            me->GetEntry() != Ride || !me->ToTempSummon() ||
            me->ToTempSummon()->GetSummonerGUID() != player->GetGUID() || me->GetMap() != player->GetMap() ||
            !me->IsAlive() || !me->IsInWorld() || !me->GetVehicleKit()) return;
        player->AddQuestDelayedEvent(QuestId, 0, [] { });
        QuestStatusData* quest = player->getQuestStatus(QuestId);
        if (!quest || !quest->ScriptLifetime) return;
        owner = player->GetGUID(); source = actor->GetGUID(); lifetime = quest->ScriptLifetime;
        player->BindQuestObject(QuestId, me);
        stage = Stage::Joining;
        me->SetReactState(REACT_PASSIVE);
        me->setActive(true);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        Join(player);
    }
    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (stage == Stage::Unbound) return;
        Player* player = passenger ? passenger->ToPlayer() : nullptr;
        if (apply)
        {
            if (!player || passenger->GetGUID() != owner || seat != 0 || !Current(player) ||
                player->GetVehicleBase() != me || (stage != Stage::Joining && stage != Stage::Boarded))
            {
                if (passenger && passenger->GetVehicleBase() == me) passenger->ExitVehicle();
                return;
            }
            if (stage == Stage::Joining)
            {
                boarded = true;
                stage = Stage::Boarded;
                timer = 0;
            }
        }
        else if (player && passenger->GetGUID() == owner)
        {
            Restore(player);
            if (stage == Stage::Boarded) Stop("owner dismounted before acknowledged departure");
        }
    }
    bool Awaiting(Player* player) const { return stage == Stage::Casting && ExitReady(player); }
    void Accepted(Player* player)
    {
        using namespace TalonclawBoardedDeparture;
        if (stage != Stage::Casting || !Lease(player) || !player->HasPendingTeleport()) return;
        SpellTargetPosition const* destination = sSpellMgr->GetSpellTargetPosition(Transfer);
        Creature* actor = Source();
        if (!destination || destination->target_mapId != 1511 ||
            player->GetTeleportDest().GetMapId() != destination->target_mapId ||
            player->GetTeleportDest().GetExactDist(destination->target_X, destination->target_Y, destination->target_Z) > 0.1f ||
            !SourceActor(actor) || !actor->IsInWorld() || !actor->IsAlive()) return;
        // Latch before native credit/quest callbacks can re-enter. The source
        // retains its real action33 and source22 admission; no direct grant.
        stage = Stage::Finished;
        if (auto smart = dynamic_cast<SmartAI*>(actor->AI()))
            if (!player->GetQuestObjectiveData(QuestId, Credit))
                smart->GetScript()->ProcessEventsFor(SMART_EVENT_DATA_SET, player, Receipt, 1);
        if (!player->GetQuestObjectiveData(QuestId, Credit))
            TC_LOG_WARN("scripts", "Talonclaw boarding: native accepted transfer did not deliver source100965 receipt100111; investigate source22/data dispatch");
        me->DespawnOrUnsummon(1000);
    }
    void UpdateAI(uint32 diff) override
    {
        if (stage == Stage::Unbound || stage == Stage::Finished) return;
        age = diff >= 30000 - age ? 30000 : age + diff;
        timer = timer > diff ? timer - diff : 0;
        Player* player = Owner();
        if (!Current(player)) { Stop("participant/quest attempt/map changed"); return; }
        if (age >= 30000) { Stop("finite boarding/exit/transfer deadline reached"); return; }
        if (stage == Stage::Joining)
        {
            if (!timer && joinAttempts < 3) Join(player);
            if (!timer && joinAttempts >= 3) Stop("native seat join did not complete after three requests");
        }
        else if (stage == Stage::Boarded)
        {
            if (player->GetVehicleBase() != me) { Stop("boarded passenger no longer occupies ride"); return; }
            stage = Stage::Exiting; timer = 10000;
            player->ExitVehicle();
            Restore(player);
        }
        else if (stage == Stage::Exiting)
        {
            if (ExitReady(player))
            {
                if (castAttempts >= 3) { Stop("native transfer was not accepted after three attempts"); return; }
                Creature* actor = Source();
                auto smart = actor ? dynamic_cast<SmartAI*>(actor->AI()) : nullptr;
                if (!smart) { Stop("original SmartAI source unavailable"); return; }
                ++castAttempts;
                stage = Stage::Casting; timer = 5000;
                // Native action85 retains its actual200ms queue and198950.
                // The spell guard revalidates the lease when that queue fires.
                smart->SmartAI::OnSpellClick(player);
            }
            else if (!timer) Stop("native exit spline did not finish");
        }
        else if (stage == Stage::Casting && !timer)
        {
            if (player->HasPendingTeleport()) { Stop("native transfer chose an unverified destination"); return; }
            stage = Stage::Exiting; timer = 10000;
        }
    }
    void JustDied(Unit*) override { Stop("owned transport died"); }
};

namespace TalonclawBoardedDeparture
{
npc_talonclaw_boarded_ride* Owned(Player* player, bool awaiting = false)
{
    if (!player || !player->IsInWorld()) return nullptr;
    GuidList* summons = player->GetSummonList(Ride);
    if (!summons) return nullptr;
    for (ObjectGuid const& guid : *summons)
        if (Creature* creature = ObjectAccessor::GetCreature(*player, guid))
            if (auto ai = dynamic_cast<npc_talonclaw_boarded_ride*>(creature->AI()))
                if (creature->IsInWorld() && creature->IsAlive() && creature->ToTempSummon() &&
                    creature->ToTempSummon()->GetSummonerGUID() == player->GetGUID() &&
                    (!awaiting || ai->Awaiting(player))) return ai;
    return nullptr;
}
}

struct npc_talonclaw_boarding_source : SmartAI
{
    explicit npc_talonclaw_boarding_source(Creature* creature) : SmartAI(creature) { }
    void OnSpellClick(Unit* clicker) override
    {
        using namespace TalonclawBoardedDeparture;
        if (!SourceActor(me)) { SmartAI::OnSpellClick(clicker); return; }
        Player* player = clicker ? clicker->ToPlayer() : nullptr;
        if (!Contact(player, me, INTERACTION_DISTANCE) || player->HasPendingTeleport()) return;
        // The summon spell owns its actual AfterCast receipt and quest lease.
        // HandleSpellClick calls this even for a rejected cast; never start here.
    }
};

class spell_talonclaw_boarding_summon : public SpellScript
{
    PrepareSpellScript(spell_talonclaw_boarding_summon);
    std::weak_ptr<uint8> lifetime;
    ObjectGuid originalSource;
    bool bound = false;
    bool Load() override
    {
        return GetSpellInfo()->Id == 198941 && TalonclawBoardedDeparture::SourceActor(
            GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr);
    }
    SpellCastResult Check()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        Creature* source = GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr;
        if (!TalonclawBoardedDeparture::Contact(player, source, INTERACTION_DISTANCE) ||
            player->HasPendingTeleport() || player->GetTransport() || player->isInFlight()) return SPELL_FAILED_BAD_TARGETS;
        if (TalonclawBoardedDeparture::Owned(player) || player->GetVehicle() || player->waitOnSeat) return SPELL_FAILED_NOT_READY;
        player->AddQuestDelayedEvent(TalonclawBoardedDeparture::QuestId, 0, [] { });
        QuestStatusData* quest = player->getQuestStatus(TalonclawBoardedDeparture::QuestId);
        if (!quest || !quest->ScriptLifetime) return SPELL_FAILED_BAD_TARGETS;
        if (bound && (originalSource != source->GetGUID() || lifetime.expired() || quest->ScriptLifetime != lifetime.lock()))
            return SPELL_FAILED_BAD_TARGETS;
        originalSource = source->GetGUID(); lifetime = quest->ScriptLifetime; bound = true;
        return SPELL_CAST_OK;
    }
    void After()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        Creature* source = GetExplTargetUnit() ? GetExplTargetUnit()->ToCreature() : nullptr;
        QuestStatusData* quest = player ? player->getQuestStatus(TalonclawBoardedDeparture::QuestId) : nullptr;
        auto ride = TalonclawBoardedDeparture::Owned(player);
        if (!ride) return;
        if (bound && source && source->GetGUID() == originalSource && quest && !lifetime.expired() &&
            quest->ScriptLifetime == lifetime.lock()) ride->Begin(player, source);
        // Check() observed no earlier owned ride. Retire only this native
        // summon if the attempt/source changed before its actual AfterCast.
        if (ride->stage == npc_talonclaw_boarded_ride::Stage::Unbound &&
            ride->me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == 198941)
            ride->me->DespawnOrUnsummon(1);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_talonclaw_boarding_summon::Check);
        AfterCast += SpellCastFn(spell_talonclaw_boarding_summon::After);
    }
};

class spell_talonclaw_boarded_transfer : public SpellScript
{
    PrepareSpellScript(spell_talonclaw_boarded_transfer);
    ObjectGuid ride;
    bool authorized = false;
    npc_talonclaw_boarded_ride* Exact(Player* player)
    {
        Creature* actor = player ? ObjectAccessor::GetCreature(*player, ride) : nullptr;
        return actor ? dynamic_cast<npc_talonclaw_boarded_ride*>(actor->AI()) : nullptr;
    }
    bool Load() override
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        return player && player->GetMapId() == 1220 &&
            (TalonclawBoardedDeparture::Owned(player) || player->GetDistance(-859.443f, 4282.79f, 746.366f) < 100.0f);
    }
    SpellCastResult Check()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        auto actor = TalonclawBoardedDeparture::Owned(player, true);
        if (!actor || (!ride.IsEmpty() && ride != actor->me->GetGUID())) return SPELL_FAILED_BAD_TARGETS;
        ride = actor->me->GetGUID();
        return SPELL_CAST_OK;
    }
    void Guard(SpellEffIndex effect)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        auto actor = Exact(player);
        authorized = GetHitUnit() == player && actor && actor->Awaiting(player);
        if (!authorized) PreventHitDefaultEffect(effect);
    }
    void After()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        // CanContact can become false during a native map change. The before-hit
        // authorization and exact actor/lease survive only this spell instance.
        if (authorized && GetHitUnit() == player)
            if (auto actor = Exact(player)) actor->Accepted(player);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_talonclaw_boarded_transfer::Check);
        OnEffectHitTarget += SpellEffectFn(spell_talonclaw_boarded_transfer::Guard, EFFECT_0, SPELL_EFFECT_TELEPORT);
        AfterHit += SpellHitFn(spell_talonclaw_boarded_transfer::After);
    }
};
