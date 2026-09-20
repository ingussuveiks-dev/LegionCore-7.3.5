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

#ifndef DEF_CELESTIAL_TOURNAMENT
#define DEF_CELESTIAL_TOURNAMENT

enum CelestialTournamentSpells
{
    SPELL_BATTLE_PET_CHALLENGE = 143999
};

enum CelestialTournamentData
{
    DATA_CHAMPIONS_DEFEATED,
    DATA_CELESTIALS_DEFEATED,
    DATA_TOURNAMENT_COMPLETE,
};

enum CelestialTournamentCreatures
{
    NPC_WRATHION                 = 71924,
    NPC_LOREWALKER_CHO           = 71926,
    NPC_CHEN_STORMSTOUT          = 71927,
    NPC_SULLY_MCLEARY            = 71929,
    NPC_SHADEMASTER_KIRYN        = 71930,
    NPC_TARAN_ZHU                = 71931,
    NPC_WISE_MARI                = 71932,
    NPC_BLINGTRON_4000           = 71933,
    NPC_DR_ION_GOLDBLOOM         = 71934,
    NPC_XU_FU                    = 72009,
    NPC_CHI_CHI                  = 72285,
    NPC_ZAO                      = 72290,
    NPC_YULA                     = 72291,
    NPC_TOURNAMENT_KILL_CREDIT   = 73159,
};

enum CelestialTournamentScenario
{
    SCENARIO_CELESTIAL_TOURNAMENT = 257,
    CRITERIA_TREE_CHAMPIONS        = 33303,
    CRITERIA_TREE_XU_FU            = 33332,
    CRITERIA_TREE_YULA             = 33333,
    CRITERIA_TREE_ZAO              = 33334,
    CRITERIA_TREE_CHI_CHI          = 33335,
};

#endif
