/*
    Monk Specialization: Brewmaster
*/

#include "ScriptMgr.h"
#include "InstanceScript.h"

class instance_temple_of_the_jade_serpent : public InstanceMapScript
{
public:
    instance_temple_of_the_jade_serpent() : InstanceMapScript("instance_temple_of_the_jade_serpent", 1625) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        // Native tree 49567 unlocks Yu'lon's first line/path. Korithis's
        // TEXT_OVER 0 then owns DATA 2/2 and Yu'lon's reply. A raw 32-death
        // callback consumed that one-shot reply before the confrontation.
        return new InstanceScript(map);
    }
};

void AddSC_instance_temple_of_the_jade_serpent()
{
    new instance_temple_of_the_jade_serpent();
}
