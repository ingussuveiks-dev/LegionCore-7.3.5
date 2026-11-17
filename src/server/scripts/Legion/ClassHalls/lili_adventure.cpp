#include "ScriptMgr.h"
#include "SmartAI.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestObjectScope.h"
#include "TemporarySummon.h"
#include "CreatureTextMgr.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "DB2Stores.h"
#include "Log.h"
#include <map>

namespace LiliAdventure
{
constexpr uint32 QuestId = 40633;
Position const Door = {745.349f, 3664.7365f, 143.2642f, 0.0f};
Position const Outside = {734.5573f, 3631.977f, 142.6309f, 4.7f};
Position const CraneStart = {730.0816f, 3616.991f, 147.0f, 1.487782f};
Position const CraneLand = {733.8f, 3630.5f, 142.6309f, 1.487782f};

bool Eligible(Player* player)
{
    return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
        !player->HasPendingTeleport() && player->getClass() == CLASS_MONK && player->GetMapId() == 1514 &&
        !player->GetQuestRewardStatus(QuestId) &&
        (player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE);
}

class AdventureAI : public SmartAI
{
public:
    explicit AdventureAI(Creature* actor) : SmartAI(actor) { }
    ObjectGuid owner, source, originalFather, father, crane;
    std::map<ObjectGuid, ObjectGuid> scenes;
    uint32 step = 0, timer = 0, remaining = 180000, moveWait = 0, moveId = 0, boardWait = 0;
    bool privateScene = false, stopped = false, boarded = false, landing = false;
    Creature* Lili() const { return me; }

    bool Public() const { return me->GetMapId() == 1514 && me->GetDBTableGUIDLow() == 372111; }
    Player* Current() const
    {
        Player* player = ObjectAccessor::FindPlayer(owner);
        auto scope = me->GetQuestObjectScope().lock();
        QuestStatusData* status = player ? player->getQuestStatus(QuestId) : nullptr;
        return Eligible(player) && me->IsAlive() && me->IsInMap(player) && scope && !scope->Cancelled &&
            scope->Owner == owner && status && status->ScriptObjects == scope ? player : nullptr;
    }
    Creature* Actor(ObjectGuid guid) const { return ObjectAccessor::GetCreature(*me, guid); }
    void Phase(Creature* actor, Player* player)
    {
        if (actor->GetPhaseMask() != player->GetPhaseMask()) actor->SetPhaseMask(player->GetPhaseMask(), false);
        if (actor->GetPhases() != player->GetPhases()) actor->SetPhaseId(player->GetPhases(), true);
    }
    void Speak(Creature* actor, uint8 group)
    {
        if (actor) sCreatureTextMgr->SendChat(actor, group, owner, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_PERSONAL);
    }
    void Release(ObjectGuid player, ObjectGuid scene, ObjectGuid dad)
    {
        auto found = scenes.find(player);
        if (found == scenes.end() || found->second != scene) return;
        scenes.erase(found);
        me->RemoveFromHideList(player);
        me->UpdateObjectVisibility();
        if (Creature* actor = Actor(dad))
        {
            actor->RemoveFromHideList(player);
            actor->UpdateObjectVisibility();
        }
    }
    void Finish(bool despawn = true)
    {
        if (!privateScene || stopped) return;
        stopped = true;
        if (Creature* root = Actor(source))
            if (auto ai = dynamic_cast<AdventureAI*>(root->AI())) ai->Release(owner, me->GetGUID(), originalFather);
        // Do not cancel the player's quest scope: the real kite can share it.
        me->ExitVehicle();
        if (Creature* actor = Actor(crane)) actor->DespawnOrUnsummon();
        if (Creature* actor = Actor(father)) actor->DespawnOrUnsummon();
        if (despawn) me->DespawnOrUnsummon();
    }
    void Fail(char const* reason)
    {
        TC_LOG_WARN("scripts", "Li Li adventure40633: %s; private scene removed, no objective granted; player %u", reason, uint32(owner.GetCounter()));
        Finish();
    }
    TempSummon* Summon(uint32 entry, Position const& pos, uint32 duration, uint32 spell = 0)
    {
        auto properties = sSummonPropertiesStore.LookupEntry(3545);
        if (!properties) return nullptr;
        // Native personal properties, real summoner list and inherited attempt
        // scope. The explicit viewer is the player, not the NPC summoner GUID.
        TempSummon* actor = me->GetMap()->SummonCreature(entry, pos, properties, duration, me,
            ObjectGuid::Empty, spell, 0, owner);
        if (actor)
        {
            actor->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 0);
            actor->SetReactState(REACT_PASSIVE);
            actor->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
            actor->setActive(true);
            if (Player* player = Current()) Phase(actor, player);
        }
        return actor;
    }
    bool Start(Player* player)
    {
        if (!Public() || !Eligible(player) || !me->IsWithinDistInMap(player, 15.0f) ||
            player->GetQuestObjectiveData(QuestId, 102531) > 0) return false;
        auto found = scenes.find(player->GetGUID());
        if (found != scenes.end())
        {
            Creature* actor = Actor(found->second);
            auto ai = actor ? dynamic_cast<AdventureAI*>(actor->AI()) : nullptr;
            if (ai && !ai->stopped && ai->Current()) return true;
            // Release the old generation before registering another one.
            if (ai) ai->Finish();
            else scenes.erase(found);
        }
        std::list<Creature*> candidates;
        me->GetCreatureListWithEntryInGrid(candidates, 100481, 30.0f);
        Creature* dad = nullptr;
        for (Creature* actor : candidates)
            if (actor->GetDBTableGUIDLow() == 340237 && actor->IsAlive()) { dad = actor; break; }
        if (!dad) return false;
        auto properties = sSummonPropertiesStore.LookupEntry(3545);
        if (!properties) return false;
        TempSummon* copy = me->GetMap()->SummonCreature(100475, me->GetPosition(), properties, 240000,
            me, ObjectGuid::Empty, 0, 0, player->GetGUID());
        auto ai = copy ? dynamic_cast<AdventureAI*>(copy->AI()) : nullptr;
        if (!ai) { if (copy) copy->DespawnOrUnsummon(); return false; }
        player->BindQuestObject(QuestId, copy);
        ai->privateScene = true; ai->owner = player->GetGUID(); ai->source = me->GetGUID();
        ai->originalFather = dad->GetGUID();
        copy->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 0);
        copy->SetReactState(REACT_PASSIVE);
        copy->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        copy->setActive(true); ai->Phase(copy, player);
        TempSummon* parent = ai->Summon(100481, dad->GetPosition(), 240000);
        if (!parent) { ai->Finish(); return false; }
        ai->father = parent->GetGUID();
        scenes[player->GetGUID()] = copy->GetGUID();
        me->AddToHideList(player->GetGUID()); dad->AddToHideList(player->GetGUID());
        me->UpdateObjectVisibility(); dad->UpdateObjectVisibility();
        ai->timer = 2000;
        return true;
    }
    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (Public() && quest && quest->GetQuestId() == QuestId)
        {
            if (!Start(player))
                TC_LOG_WARN("scripts", "Li Li adventure40633: unable to start personal departure; gossip permits retry; player %u", player->GetGUIDLow());
            return;
        }
        SmartAI::sQuestAccept(player, quest);
    }
    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (!privateScene)
        {
            if (auto ai = dynamic_cast<AdventureAI*>(summon->AI())) ai->Finish(false);
            SmartAI::SummonedCreatureDespawn(summon);
        }
        else if (!stopped && (summon->GetGUID() == father || summon->GetGUID() == crane))
        {
            if (summon->GetGUID() == crane) crane.Clear();
            if (summon->GetGUID() == father) father.Clear();
            Finish();
        }
    }
    void Move(uint32 id, Position const& point)
    {
        moveId = id; moveWait = 30000;
        me->SetWalk(false);
        me->GetMotionMaster()->MovePoint(id, point, true);
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        if (!privateScene) { SmartAI::MovementInform(type, id); return; }
        if (stopped || !Current() || type != POINT_MOTION_TYPE || id != moveId) return;
        Position const& dest = id == 1 ? Door : Outside;
        if (me->GetExactDist(&dest) > 2.0f) return;
        moveId = 0; moveWait = 0;
        if (id == 1) Move(2, Outside);
        else { step = 6; timer = 1000; }
    }
    void CraneLanded(ObjectGuid actor)
    {
        if (Current() && actor == crane && step == 7) landing = true;
    }
    void CraneBoarded(ObjectGuid actor)
    {
        Creature* bird = Actor(crane);
        if (Current() && step == 9 && actor == crane && bird && me->IsOnVehicle(bird)) boarded = true;
    }
    void UpdateAI(uint32 diff) override
    {
        if (!privateScene) { SmartAI::UpdateAI(diff); return; }
        if (stopped) return;
        Player* player = Current();
        if (!player || remaining <= diff) { Finish(); return; }
        remaining -= diff;
        Creature* dad = Actor(father);
        if (!dad || !dad->IsAlive()) { Fail("father missing"); return; }
        Phase(me, player); Phase(dad, player);
        if (Creature* bird = Actor(crane)) Phase(bird, player);
        if ((step == 7 && !landing) || (step == 9 && !boarded))
        {
            if (boardWait <= diff) { Fail(step == 7 ? "crane did not land" : "Li Li did not board crane"); return; }
            boardWait -= diff;
        }
        if (moveWait)
        {
            if (moveWait <= diff) Fail("departure movement did not arrive");
            else moveWait -= diff;
            return;
        }
        if (timer > diff) { timer -= diff; return; }
        timer = 0;
        switch (step)
        {
            case 0: me->SetFacingToObject(dad); Speak(me, 0); step = 1; timer = 6000; break;
            case 1: dad->SetFacingToObject(me); Speak(dad, 0); Speak(dad, 1); step = 2; timer = 8000; break;
            case 2: Speak(me, 1); step = 3; timer = 9000; break;
            case 3: Speak(dad, 2); step = 4; timer = 10000; break;
            case 4: Speak(me, 2); step = 5; timer = 1000; break;
            case 5: Move(1, Door); break;
            case 6:
            {
                me->SetFacingTo(Outside.GetOrientation()); Speak(me, 3);
                TempSummon* bird = Summon(100972, CraneStart, 40000, 198964);
                if (!bird || !bird->GetVehicleKit()) { if (bird) bird->DespawnOrUnsummon(); Fail("native crane summon/vehicle unavailable"); return; }
                crane = bird->GetGUID(); bird->SetCanFly(true);
                bird->GetMotionMaster()->MoveLand(3, CraneLand);
                // Let the kite instructions finish before the fish dialogue,
                // even if the crane's landing callback arrives immediately.
                step = 7; timer = 10000; boardWait = 15000; break;
            }
            case 7:
                if (!landing)
                {
                    timer = 200; break;
                }
                Speak(me, 4); step = 8; timer = 6000; break;
            case 8:
                // Feeding gesture, then real native vehicle boarding. No
                // player control, quest credit or travel comes from this bird.
                me->HandleEmoteCommand(EMOTE_ONESHOT_USE_STANDING);
                step = 9; boardWait = 5000; timer = 1000; break;
            case 9:
            {
                Creature* bird = Actor(crane);
                if (!bird) { Fail("crane missing before boarding"); return; }
                if (boarded && me->IsOnVehicle(bird))
                {
                    Speak(me, 5); step = 10; timer = 5000; break;
                }
                me->EnterVehicle(bird, 0); timer = 500; break;
            }
            case 10:
                if (Creature* bird = Actor(crane))
                {
                    if (!me->IsOnVehicle(bird)) { Fail("Li Li left crane before departure"); return; }
                    bird->GetMotionMaster()->MoveTakeoff(4, 730.0f, 3618.0f, 170.0f);
                    step = 11; timer = 15000;
                }
                else Fail("crane missing at departure");
                break;
            case 11: Finish(); break;
            default: Fail("invalid departure state"); break;
        }
    }
};

class CraneAI : public ScriptedAI
{
public:
    explicit CraneAI(Creature* actor) : ScriptedAI(actor) { }
    AdventureAI* Parent() const
    {
        TempSummon* summon = me->ToTempSummon();
        Creature* parent = summon && summon->GetSummoner() ? summon->GetSummoner()->ToCreature() : nullptr;
        auto ai = parent ? dynamic_cast<AdventureAI*>(parent->AI()) : nullptr;
        return ai && !ai->stopped && ai->privateScene && ai->Current() &&
            me->GetQuestObjectScope().lock() == parent->GetQuestObjectScope().lock() ? ai : nullptr;
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        auto ai = Parent();
        if (ai && id == 3 && type == EFFECT_MOTION_TYPE && me->GetExactDist(&CraneLand) <= 2.0f)
            ai->CraneLanded(me->GetGUID());
        Position const end = {730.0f, 3618.0f, 170.0f, 0.0f};
        if (ai && id == 4 && type == EFFECT_MOTION_TYPE && me->GetExactDist(&end) <= 2.0f) ai->Finish();
    }
    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (!apply) return;
        auto ai = Parent();
        if (!ai || passenger != ai->Lili() || seat != 0) { passenger->ExitVehicle(); return; }
        ai->CraneBoarded(me->GetGUID());
    }
};
}

class npc_lili_adventure : public CreatureScript
{
public:
    npc_lili_adventure() : CreatureScript("npc_lili_adventure") { }
    CreatureAI* GetAI(Creature* actor) const override { return new LiliAdventure::AdventureAI(actor); }
    bool OnGossipHello(Player* player, Creature* actor) override
    {
        auto ai = dynamic_cast<LiliAdventure::AdventureAI*>(actor->AI());
        if (ai && ai->Start(player)) { player->PlayerTalkClass->SendCloseGossip(); return true; }
        return false;
    }
};
class npc_lili_adventure_crane : public CreatureScript
{
public:
    npc_lili_adventure_crane() : CreatureScript("npc_lili_adventure_crane") { }
    CreatureAI* GetAI(Creature* actor) const override { return new LiliAdventure::CraneAI(actor); }
};

void AddSC_lili_adventure()
{
    new npc_lili_adventure();
    new npc_lili_adventure_crane();
}
