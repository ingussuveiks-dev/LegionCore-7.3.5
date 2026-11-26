#pragma once

inline Creature* FindOwnedCartActor(LootPlayer& player, uint32 entry)
{
    std::list<Creature*> actors;
    player.GetCreatureListWithEntryInGrid(actors, entry, 20.0f);
    Creature* result = nullptr;
    for (Creature* actor : actors)
        if (TempSummon* summon = actor->ToTempSummon(); summon && summon->GetSummonerGUID() == player.GetGUID())
        {
            Require(!result, "The cart interaction spawned more than one actor of an owned type");
            result = actor;
        }
    return result;
}

inline void FinishOwnedCartRoute(Map& map, LootPlayer& pupil, Creature& startCart, Creature& startYak)
{
    ObjectGuid const cartGuid = startCart.GetGUID();
    ObjectGuid const yakGuid = startYak.GetGUID();
    Creature* cart = &startCart;
    Creature* yak = &startYak;
    for (uint32 elapsed = 0; elapsed < 360000 && (cart || yak); elapsed += 50)
    {
        if (yak)
        {
            yak->Unit::Update(50);
            yak->AI()->UpdateAI(50);
        }
        if (cart)
            cart->Unit::Update(50);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
        cart = map.GetCreature(cartGuid);
        yak = map.GetCreature(yakGuid);
    }
    Require(!pupil.GetVehicleBase() && !cart && !yak, "The owned cart did not dismount and remove both actors at the route end");
}
