#include "ScriptMgr.h"
#include "SmartAI.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"

namespace TirisfalWards
{
constexpr uint32 HolyWards = 202014;
constexpr uint32 HolyPulse = 202015;

// Native ritualist104583 death still owns scenario/group/owner credit and
// DATA1/1 selection. This receiver owns only the ward's persistent cosmetic.
// Reconstruct the complete observed cleanse instead of inventing a LINK2.
class WardAI : public SmartAI
{
public:
    explicit WardAI(Creature* actor) : SmartAI(actor) { }
    bool pending = false, confirmed = false;
    uint32 timer = 0, elapsed = 0, attempts = 0;

    bool Scoped() const { return me->GetEntry() == 103151 && me->GetMapId() == 1539; }
    bool Usable() const { return Scoped() && me->IsInWorld() && me->IsAlive() && !me->IsQuestObjectExpired(); }
    void Reset() override
    {
        pending = confirmed = false;
        timer = elapsed = attempts = 0;
        SmartAI::Reset();
    }
    void SetData(uint32 id, uint32 value) override
    {
        if (!Scoped() || id != 1 || value != 1) { SmartAI::SetData(id, value); return; }
        if (!Usable() || pending || (confirmed && me->HasAura(HolyWards))) return;
        pending = true; confirmed = false;
        timer = elapsed = attempts = 0;
        // Preserve the original native UNAURA-all operation, including this
        // particular spawn's Shadow Pool. Do it once, not on every retry.
        me->RemoveAllAuras();
    }
    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff); // Keep the separate summoned/sparkle branch.
        if (!pending) return;
        if (!Usable()) { pending = false; return; }
        if (me->HasAura(HolyWards))
        {
            pending = false; confirmed = true;
            return; // Actual aura application, not cast acceptance, is receipt.
        }
        if (elapsed >= 10000 || diff >= 10000 - elapsed)
        {
            pending = false;
            TC_LOG_ERROR("scripts", "Tirisfal ward %u: native202014 did not apply after %u requests; no progression granted; a new DATA1/1 may retry", me->GetGUIDLow(), attempts);
            return;
        }
        elapsed += diff;
        if (timer > diff) { timer -= diff; return; }
        timer = 1000;
        ++attempts;
        SpellCastResult result = me->CastSpell(me, HolyWards, false);
        if (result != SPELL_CAST_OK && attempts == 1)
            TC_LOG_WARN("scripts", "Tirisfal ward %u: native202014 rejected (%u); bounded cosmetic retry retained", me->GetGUIDLow(), uint32(result));
    }
};
}

class npc_tirisfal_holy_ward : public CreatureScript
{
public:
    npc_tirisfal_holy_ward() : CreatureScript("npc_tirisfal_holy_ward") { }
    CreatureAI* GetAI(Creature* actor) const override { return new TirisfalWards::WardAI(actor); }
};

void AddSC_tirisfal_wards()
{
    new npc_tirisfal_holy_ward();
}
