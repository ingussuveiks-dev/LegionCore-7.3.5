#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Map.h"
#include "EventObjectData.h"
#include "ConditionMgr.h"

namespace CommandersValor
{
bool IsAltar(uint32 entry)
{
    switch (entry)
    {
        case 124: case 134: case 145: case 147: case 148: case 158:
        case 165: case 167: case 177: case 181: case 198:
            return true;
        default:
            return false;
    }
}

bool Allowed(Player* player)
{
    if (!player || !player->IsInWorld() || player->IsBeingTeleported())
        return false;
    // Use the loaded spawn and its configured radius/phase, not the spell's
    // 20-yard cast range or an entire class-hall zone. Conditions are shared
    // with the actual SmartAI enter event (including the DK OR alternatives).
    for (auto const& pair : sEventObjectDataStore->GetEventObjectDataContainer())
    {
        EventObjectData const& data = pair.second;
        if (!IsAltar(data.id) || data.mapid != player->GetMapId())
            continue;
        EventObject* altar = player->GetMap()->GetEventObject(
            ObjectGuid::Create<HighGuid::EventObject>(data.mapid, data.id, data.guid));
        if (!altar || !altar->eventTemplate || altar->eventTemplate->radius <= 0.0f || !player->InSamePhase(altar) ||
            player->GetDistance(altar) > altar->eventTemplate->radius)
            continue;
        auto conditions = sConditionMgr->GetConditionsForSmartEvent(data.id, 0, 13);
        if (!conditions.empty() && sConditionMgr->IsObjectMeetToConditions(player, altar, conditions))
            return true;
    }
    return false;
}
}

class spell_commanders_valor_location : public SpellScript
{
    PrepareSpellScript(spell_commanders_valor_location);
    SpellCastResult Check()
    {
        return CommandersValor::Allowed(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_NOT_HERE;
    }
    void Register() override { OnCheckCast += SpellCheckCastFn(spell_commanders_valor_location::Check); }
};

class aura_commanders_valor_location : public AuraScript
{
    PrepareAuraScript(aura_commanders_valor_location);
    void Update(uint32)
    {
        if (!CommandersValor::Allowed(GetTarget()->ToPlayer()))
            Remove();
    }
    void Register() override { OnAuraUpdate += AuraUpdateFn(aura_commanders_valor_location::Update); }
};

void AddSC_commanders_valor()
{
    RegisterSpellScript(spell_commanders_valor_location);
    RegisterAuraScript(aura_commanders_valor_location);
}
