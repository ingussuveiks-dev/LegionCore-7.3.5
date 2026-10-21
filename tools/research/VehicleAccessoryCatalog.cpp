#include "DB2Store.h"
#include "DB2Structure.h"
#include "DB2LoadInfo.h"
#include <iostream>
#include <set>

int main(int argc,char** argv)
{
    if(argc<2)return 1;
    DB2Storage<SpellEntry> spells("Spell.db2",SpellLoadInfo::Instance());
    DB2Storage<SpellEffectEntry> effects("SpellEffect.db2",SpellEffectLoadInfo::Instance());
    DB2Storage<VehicleEntry> vehicles("Vehicle.db2",VehicleLoadInfo::Instance());
    DB2Storage<VehicleSeatEntry> seats("VehicleSeat.db2",VehicleSeatLoadInfo::Instance());
    if(!spells.Load(argv[1],0)||!effects.Load(argv[1],0)||!vehicles.Load(argv[1],0)||!seats.Load(argv[1],0))return 2;
    std::set<uint32> ids{46598,186608,230986,228951};
    for(auto id:ids)
    {
        auto spell=spells.LookupEntry(id);
        if(!spell)return 3;
        std::cout<<"S\t"<<id<<'\t'<<spell->Name->Str[0]<<'\n';
    }
    for(auto effect:effects)
        if(ids.count(effect->SpellID))
            std::cout<<"E\t"<<effect->SpellID<<'\t'<<effect->DifficultyID<<'\t'<<effect->EffectIndex<<'\t'<<effect->Effect<<'\t'<<effect->EffectAura<<'\t'<<effect->EffectTriggerSpell<<'\t'<<effect->EffectMiscValue[0]<<'\n';
    for(uint32 id:{4397u,5016u})
    {
        auto vehicle=vehicles.LookupEntry(id);
        if(!vehicle)return 4;
        for(unsigned index=0;index<8;++index)
            if(vehicle->SeatID[index])
            {
                auto seat=seats.LookupEntry(vehicle->SeatID[index]);
                if(!seat)return 5;
                std::cout<<"SEAT\t"<<id<<'\t'<<index<<'\t'<<seat->ID<<'\t'<<seat->Flags<<'\t'<<seat->FlagsB<<'\n';
            }
    }
}
