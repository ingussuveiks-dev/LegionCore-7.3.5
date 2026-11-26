#ifndef BATTLE_PET_STABLE_SERVICE_H
#define BATTLE_PET_STABLE_SERVICE_H

#include "Define.h"

namespace BattlePetStableService
{
    constexpr uint32 Revive = 133994;

    struct Service
    {
        uint32 Entry;
        uint32 Map;
        uint32 Menu;
        uint32 Option;
        uint32 Quest = 0;
        uint32 Credit = 0;
        uint32 AlternateQuest = 0;
    };

    // The tutorial quest credits the stable master named in its quest text.
    // The pet battle trainers retain their paid service for the player's journal.
    constexpr Service Services[] =
    {
        {  6749,   0,   9821, 2, 31309, 64320 }, // Erma
        {  9980,   0,   9821, 2, 31549, 65020 }, // Shelby Stoneflint
        {  9987,   1,   9821, 2, 31572, 65180 }, // Shoja'my
        { 10050,   1,   9821, 2, 31574, 65184 }, // Seikwa
        { 10051,   1,   9821, 2, 31553, 65041, 31554 }, // Seriadne
        { 10055,   0, 100550, 1, 31577, 65193 }, // Morganus
        { 10085,   1,   9821, 2, 31583, 65199 }, // Jaelysia
        { 11069,   0,   9821, 2, 31592, 65214 }, // Jenova Stoneshield
        { 16185, 530,   9821, 2, 31580, 65198 }, // Anathos
        { 17485, 530,   9821, 2, 31568, 65179 }, // Esbina
        { 45789,   1,  12106, 2, 31586, 65200 }, // Bezzil
        { 47764,   1,  14991, 1, 31589, 65212 }, // Murog
        { 63014,   0,  66420, 0 }, // Marcus Jensen
        { 63061,   1,  66420, 0 }, // Narzak
        { 63067,   1,  66420, 0 }, // Naleen
        { 63070,   1,  66420, 0 }, // Valeena
        { 63073,   0,  66420, 0 }, // Ansel Fincap
        { 63075,   0,  66420, 0 }, // Grady Bannson
        { 63077, 530,  66420, 0 }, // Lehna
        { 63080, 530,  66420, 0 }, // Jarson Everlong
        { 63083,   1,  66420, 0 }, // Will Larsons
        { 63086,   1,  66420, 0 }, // Matty
        { 63596,   0,  14991, 1 }, // Audrey Burnhep
        { 63626,   1,  14991, 1 }, // Varzok
    };

    inline Service const* Find(uint32 entry, uint32 map)
    {
        for (Service const& service : Services)
            if (service.Entry == entry && service.Map == map)
                return &service;

        return nullptr;
    }
}

#endif
