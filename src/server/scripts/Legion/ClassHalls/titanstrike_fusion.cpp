#include "ScriptMgr.h"
#include "SmartAI.h"
#include "ConditionMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "GameObject.h"

// The two original panels signal their own core, then Mimiron. Testing for
// absence of the OTHER core loses that handoff when both panels are used
// within SmartAI's five-second despawn interval. Keep actor-owned receipts
// for the current native experiment instead of inferring use from absence.
namespace TitanstrikeFusion
{
enum : uint32 { Map = 1579, Mimiron = 106558, Core = 106613, Bunny = 106925,
    Activated = 0x735001, Ready = 0x735002 };

bool Scoped(Creature const* actor)
{
    return actor && actor->IsInWorld() && actor->IsAlive() && actor->GetMapId() == Map;
}

int Slot(uint32 entry)
{
    return entry == Core ? 0 : entry == Bunny ? 1 : -1;
}

struct MimironAI : SmartAI
{
    explicit MimironAI(Creature* actor) : SmartAI(actor) { }
    ObjectGuid cores[2];
    uint8 receipts = 0;
    bool active = false;
    bool released = false;

    void Clear()
    {
        cores[0].Clear();
        cores[1].Clear();
        receipts = 0;
        active = false;
        released = false;
    }

    void Reset() override
    {
        Clear();
        SmartAI::Reset();
    }

    void sGossipSelect(Player* player, uint32 sender, uint32 action) override
    {
        if (Scoped(me) && me->GetEntry() == Mimiron && sender == 19589 && action == 0 &&
            player && player->GetQuestStatus(42158) == QUEST_STATUS_INCOMPLETE)
        {
            Clear();
            active = true;
        }
        SmartAI::sGossipSelect(player, sender, action);
    }

    void JustSummoned(Creature* summon) override
    {
        if (active && Scoped(me) && summon)
            if (int slot = Slot(summon->GetEntry()); slot >= 0)
                if (TempSummon* temp = summon->ToTempSummon())
                    if (temp->GetSummonerGUID() == me->GetGUID())
                        cores[slot] = summon->GetGUID();
        SmartAI::JustSummoned(summon);
    }

    void SetGUID(ObjectGuid const& guid, int32 id) override
    {
        if (id == Activated && active && !released && Scoped(me))
            if (Creature* core = ObjectAccessor::GetCreature(*me, guid))
                if (int slot = Slot(core->GetEntry()); slot >= 0 && cores[slot] == guid && Scoped(core))
                    if (TempSummon* temp = core->ToTempSummon())
                        if (temp->GetSummonerGUID() == me->GetGUID() && core->AI() && core->AI()->GetData(Activated))
                            receipts |= uint8(1 << slot);
        SmartAI::SetGUID(guid, id);
    }

    uint32 GetData(uint32 id) const override
    {
        if (id == Ready)
            return active && !released && receipts == 3;
        return SmartAI::GetData(id);
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (Scoped(me) && me->GetEntry() == Mimiron && id == 1 && value == 1)
        {
            if (!GetData(Ready))
                return;
            released = true;
        }
        SmartAI::SetData(id, value);
    }
};

struct CoreAI : SmartAI
{
    explicit CoreAI(Creature* actor) : SmartAI(actor) { }
    bool activated = false;

    void Reset() override
    {
        activated = false;
        SmartAI::Reset();
    }

    uint32 GetData(uint32 id) const override
    {
        return id == Activated ? activated : SmartAI::GetData(id);
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (Scoped(me) && Slot(me->GetEntry()) >= 0 && id == 1 && value == 1)
            if (TempSummon* temp = me->ToTempSummon())
                if (Unit* summoner = temp->GetSummoner())
                    if (Creature* owner = summoner->ToCreature())
                        if (Scoped(owner) && owner->GetEntry() == Mimiron && owner->AI())
                        {
                            activated = true;
                            owner->AI()->SetGUID(me->GetGUID(), Activated);
                        }
        SmartAI::SetData(id, value);
    }
};
}

class npc_titanstrike_fusion_mimiron : public CreatureScript
{
public:
    npc_titanstrike_fusion_mimiron() : CreatureScript("npc_titanstrike_fusion_mimiron") { }
    CreatureAI* GetAI(Creature* actor) const override { return new TitanstrikeFusion::MimironAI(actor); }
};

class npc_titanstrike_fusion_core : public CreatureScript
{
public:
    npc_titanstrike_fusion_core() : CreatureScript("npc_titanstrike_fusion_core") { }
    CreatureAI* GetAI(Creature* actor) const override { return new TitanstrikeFusion::CoreAI(actor); }
};

class condition_titanstrike_fusion_ready : public ConditionScript
{
public:
    condition_titanstrike_fusion_ready() : ConditionScript("condition_titanstrike_fusion_ready") { }
    bool OnConditionCheck(Condition*, ConditionSourceInfo& source) override
    {
        GameObject* panel = source.mConditionTargets[1] ? source.mConditionTargets[1]->ToGameObject() : nullptr;
        if (!panel || panel->GetMapId() != TitanstrikeFusion::Map ||
            (panel->GetEntry() != 249765 && panel->GetEntry() != 249773))
            return false;
        Creature* owner = panel->FindNearestCreature(TitanstrikeFusion::Mimiron, 50.0f, true);
        return TitanstrikeFusion::Scoped(owner) && owner->AI() && owner->AI()->GetData(TitanstrikeFusion::Ready);
    }
};

void AddSC_titanstrike_fusion()
{
    new npc_titanstrike_fusion_mimiron();
    new npc_titanstrike_fusion_core();
    new condition_titanstrike_fusion_ready();
}
