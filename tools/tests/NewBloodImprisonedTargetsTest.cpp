#include <cassert>
#include <iostream>
struct WorldObject {};
struct Unit : WorldObject
{
    unsigned entry = 101460, map = 1502;
    unsigned GetEntry() const { return entry; }
    unsigned GetMapId() const { return map; }
};
struct Script
{
    Unit source;
    Unit* GetCaster() { return &source; }
#include "ImprisonedTargets.inc"
};
int main()
{
    Script script;
    WorldObject captive;
    for (unsigned map : {1220u, 1502u, 1522u, 0u})
    {
        script.source.map = map;
        for (unsigned entry : {101460u, 98836u})
        {
            script.source.entry = entry;
            WorldObject* self = &script.source;
            script.FilterCaster(self);
            assert((self == nullptr) == (entry == 101460 && (map == 1220 || map == 1502)));
            // The core invokes this hook for both implicit targets of an effect.
            WorldObject* target = &captive;
            script.FilterCaster(target);
            assert(target == &captive);
            target = nullptr;
            script.FilterCaster(target);
            assert(target == nullptr);
        }
    }
    std::cout << "PASS: production hook keeps captive targets, excludes only ritual jailer self auras, preserves other maps/casters.\n";
}
