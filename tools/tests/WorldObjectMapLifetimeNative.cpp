#include <unordered_set>

#include "DB2Stores.h"
#include "MovementPackets.h"
#include "GameObject.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include "Map.h"

#include <iostream>
#include <memory>
#include <set>

// Inspect the real registry without dereferencing a deleted object.
struct WorldObjectsTag
{
    using Member = std::set<WorldObject*> Map::*;
    friend Member GetMember(WorldObjectsTag);
};

template<class Tag, typename Tag::Member Value>
struct TestAccess
{
    friend typename Tag::Member GetMember(Tag) { return Value; }
};

template struct TestAccess<WorldObjectsTag, &Map::i_worldObjects>;

struct RegisteredWorldObject : WorldObject
{
    RegisteredWorldObject() : WorldObject(true) {}
};

int main(int argc, char** argv)
{
    if (argc != 2 || !sMapStore.Load(argv[1], LOCALE_enUS))
        return 1;

    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}});
    auto map = std::make_unique<Map>(0, 300, 0, DIFFICULTY_NONE);
    Map destination(1, 300, 0, DIFFICULTY_NONE);
    auto worldObjects = GetMember(WorldObjectsTag{});

    GameObject gameObject;
    gameObject.SetMap(map.get());
    // Reproduce the old GameObject switch path's inconsistent registration.
    map->AddWorldObject(&gameObject);
    gameObject.ResetMap();
    if (!(map.get()->*worldObjects).empty())
        return 2;

    {
        GameObject forgotten;
        forgotten.SetMap(map.get());
        map->AddWorldObject(&forgotten);
    }
    if (!(map.get()->*worldObjects).empty())
        return 3;

    RegisteredWorldObject moving;
    moving.SetMap(map.get());
    moving.SetMap(&destination);
    if (!(map.get()->*worldObjects).empty() || (destination.*worldObjects).count(&moving) != 1)
        return 4;
    moving.ResetMap();

    RegisteredWorldObject first, second;
    first.SetMap(map.get());
    second.SetMap(map.get());
    map.reset();
    if (first.FindMap() || second.FindMap() || !(destination.*worldObjects).empty())
        return 5;

    std::cout << "PASS native map registry: changed classification, direct destruction, map transfer and shutdown drain\n";
}
