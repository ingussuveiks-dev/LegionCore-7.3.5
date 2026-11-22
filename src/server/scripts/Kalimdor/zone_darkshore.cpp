/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Darkshore
SD%Complete: 0
SDComment: Quest support:
SDCategory: Darkshore
EndScriptData */

/* ContentData
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ScriptedFollowerAI.h"
#include "SpellScript.h"

class spell_darkshore_buried_artifact_detector : public SpellScript
{
    PrepareSpellScript(spell_darkshore_buried_artifact_detector);

    void SelectNearestMarker(std::list<WorldObject*>& targets)
    {
        if (targets.size() < 2)
            return;

        auto caster = GetCaster();
        auto nearest = *std::min_element(targets.begin(), targets.end(), [caster](WorldObject const* first, WorldObject const* second)
        {
            return caster->GetExactDistSq(first) < caster->GetExactDistSq(second);
        });
        targets.clear();
        targets.push_back(nearest);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkshore_buried_artifact_detector::SelectNearestMarker,
            EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
    }
};

void AddSC_darkshore()
{
    RegisterSpellScript(spell_darkshore_buried_artifact_detector);
}
