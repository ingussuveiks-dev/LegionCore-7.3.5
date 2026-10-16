// Read native 7.3.5 assets; no world/hotfix/client writes.
#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>

#define STORE(T) DB2Storage<T##Entry> T##s(#T ".db2", T##LoadInfo::Instance()); \
    if (!T##s.Load(argv[1], 0)) { std::cerr << "Cannot load " #T "\n"; return 2; }

int main(int argc, char** argv)
{
    if (argc != 2) return 1;
    STORE(SpellEffect) STORE(SpellMisc) STORE(SpellDuration)
    STORE(SpellXSpellVisual) STORE(Vehicle) STORE(VehicleSeat)
    STORE(CreatureDisplayInfo) STORE(CreatureModelData)
    std::set<uint32> ids{211683,211700,211701,211702,211703,211704,211705,211706,212497};
    std::map<uint32, SpellEffectEntry const*> effects;
    for (auto e : SpellEffects)
        if (ids.count(e->SpellID) && e->EffectIndex == 0 && e->DifficultyID == 0)
        {
            effects[e->SpellID] = e;
            std::cout << "effect " << e->SpellID << " type " << e->Effect << " aura " << e->EffectAura
                << " value " << e->EffectBasePoints << " misc " << e->EffectMiscValue[0]
                << " trigger " << e->EffectTriggerSpell << " period " << e->EffectAuraPeriod
                << " target " << e->ImplicitTarget[0] << '\n';
        }
    assert(effects.size() == ids.size());
    assert(effects[211703]->Effect == 77 && effects[211703]->EffectBasePoints == 211702 && effects[211703]->ImplicitTarget[0] == 38);
    assert(effects[211702]->EffectAura == 236);
    assert(effects[211704]->Effect == 77 && effects[211704]->ImplicitTarget[0] == 1);
    assert(effects[211701]->EffectTriggerSpell == 211700 && effects[211701]->EffectAuraPeriod == 2000);
    assert(effects[211700]->Effect == 61 && effects[211700]->EffectMiscValue[0] == 50917);
    assert(effects[212497]->EffectAura == 226 && effects[212497]->EffectAuraPeriod == 5000);
    for (auto m : SpellMiscs)
        if (ids.count(m->SpellID))
        {
            auto duration = SpellDurations.LookupEntry(m->DurationIndex);
            if (m->SpellID == 211701 || m->SpellID == 211706) assert(duration && duration->Duration == 2000);
            if (m->SpellID == 211705) assert(duration && duration->Duration == 100);
        }
    for (auto v : SpellXSpellVisuals)
        if (ids.count(v->SpellID)) std::cout << "visual " << v->SpellID << ' ' << v->SpellVisualID << '\n';
    auto vehicle = Vehicles.LookupEntry(4746); assert(vehicle);
    auto seat = VehicleSeats.LookupEntry(vehicle->SeatID[0]);
    assert(seat && seat->ID == 16586 && seat->AttachmentID == 4 && seat->PassengerAttachmentID == 19);
    std::cout << "native hand vehicle 4746 seat 16586 attachments 4/19\n";
    for (int id : {74997,69686,69681})
    {
        auto display = CreatureDisplayInfos.LookupEntry(id); assert(display);
        auto model = CreatureModelDatas.LookupEntry(display->ModelID); assert(model);
        std::cout << "model " << id << ' ' << model->FileDataID << " scale " << display->CreatureModelScale << '\n';
    }
    std::cout << "PASS: native reverse ride, passenger cleanup, summon event/tick, teleport durations and hand seat\n";
}
