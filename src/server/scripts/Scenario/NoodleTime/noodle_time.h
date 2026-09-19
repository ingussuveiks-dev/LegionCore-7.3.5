/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEF_NOODLE_TIME
#define DEF_NOODLE_TIME

enum NoodleTimeData
{
    DATA_START_TUTORIAL = 1,
    DATA_COOK_NOODLE_SOUP,
    DATA_COOK_DRAGONS_NEST_SOUP,
    DATA_COOK_PANDAREN_TREASURES_SOUP,
    DATA_CLEAR_CUSTOMERS,
    DATA_CLEAR_FOOD,
    DATA_SELECT_CUSTOMER,
    DATA_SELECT_SEAT,
    DATA_SELECT_FOOD,
    DATA_SERVE_CUSTOMER
};

enum NoodleTimeCreatures
{
    NPC_MERCHANT_MALE                 = 72913,
    NPC_MERCHANT_FEMALE               = 72914,
    NPC_SCHOLAR_MALE                  = 72915,
    NPC_SCHOLAR_FEMALE                = 72916,
    NPC_FARMHAND_MALE                 = 72917,
    NPC_FARMHAND_FEMALE               = 72918,
    NPC_IMPATIENT_MERCHANT_MALE       = 72919,
    NPC_IMPATIENT_MERCHANT_FEMALE     = 72920,
    NPC_IMPATIENT_SCHOLAR_MALE        = 72921,
    NPC_IMPATIENT_SCHOLAR_FEMALE      = 72922,
    NPC_IMPATIENT_FARMHAND_MALE       = 72923,
    NPC_IMPATIENT_FARMHAND_FEMALE     = 72924,
    NPC_HUNGRY_MERCHANT_MALE          = 72925,
    NPC_HUNGRY_MERCHANT_FEMALE        = 72926,
    NPC_SEAT                          = 72928,
    NPC_HUNGRY_SCHOLAR_MALE           = 72948,
    NPC_HUNGRY_SCHOLAR_FEMALE         = 72949,
    NPC_HUNGRY_FARMHAND_MALE          = 72950,
    NPC_HUNGRY_FARMHAND_FEMALE        = 72951,
    NPC_NOODLE_STAND                  = 72952,
    NPC_NOODLE_SOUP                   = 72961,
    NPC_DRAGONS_NEST_NOODLE_SOUP      = 72962,
    NPC_PANDAREN_TREASURES_NOODLE_SOUP = 72963,
    NPC_TOTAL_SCORE                   = 72977,
    NPC_SUNGSHIN_IRONPAW             = 73316,
    NPC_TUTORIAL_CREDIT               = 73396
};

enum NoodleTimeSpells
{
    SPELL_MAKE_NOODLE_SOUP            = 146421,
    SPELL_MAKE_DRAGONS_NEST_SOUP      = 146422,
    SPELL_MAKE_PANDAREN_TREASURES_SOUP = 146423,
    SPELL_NOODLE_STAND                = 146424,
    SPELL_CLEAR_CUSTOMERS             = 146519,
    SPELL_CLEAR_FOOD                  = 146521,
    SPELL_NOODLE_TIME                 = 146528
};

enum NoodleTimeCriteria
{
    CRITERIA_NOODLE_TIME_COMPLETE     = 37569
};

constexpr uint32 MAP_NOODLE_TIME = 1157;
constexpr uint32 SCENARIO_SECRET_INGREDIENT = 269;
constexpr uint32 SCENARIO_NOODLE_TIME = 278;

#endif
