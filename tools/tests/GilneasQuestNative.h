#pragma once

Position ReadGilneasPoint(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct GilneasActor : Actor
{
    GilneasActor(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(), ReadGilneasPoint(row + 2))
    {
        m_DBTableGuid = row[0].GetUInt32();
        SetPhaseMask(row[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish the original Gilneas actor");
    }

    ~GilneasActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void MoveGilneasPlayer(LootPlayer& player, Position const& point)
{
    SupplyMove(player, point);
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    player.UpdateZoneDependentAuras(player.GetZoneId());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void OpenGilneasQuestChest(LootPlayer& player, Supply& chest)
{
    constexpr float HalfTurn = 3.14159265358979f;
    for (float offset : {0.0f, HalfTurn / 2.0f, -HalfTurn / 2.0f, HalfTurn})
    {
        float facing = chest.GetOrientation() + offset;
        Position approach{chest.GetPositionX() - 1.5f * std::cos(facing), chest.GetPositionY() - 1.5f * std::sin(facing),
            chest.GetPositionZ(), Position::NormalizeOrientation(facing)};
        MoveGilneasPlayer(player, approach);
        if (player.IsWithinLOSInMap(&chest))
            break;
    }
    OpenSupply(player, chest);
    std::cout << "CASE " << chest.GetEntry() << " ready=" << chest.isSpawned() << " type=" << uint32(chest.GetGoType())
        << " state=" << uint32(chest.getLootState()) << " allowed=" << chest.ActivateToQuest(&player) << " distance=" << player.GetDistance(&chest)
        << " LOS=" << player.IsWithinLOSInMap(&chest) << " lootGUID=" << (player.GetLootGUID() == chest.GetGUID()) << ' ' << chest.loot.items.size() << '\n';
}
