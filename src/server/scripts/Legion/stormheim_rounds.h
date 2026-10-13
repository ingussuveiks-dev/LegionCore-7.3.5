#ifndef LEGION_STORMHEIM_ROUNDS_H
#define LEGION_STORMHEIM_ROUNDS_H
#include "ObjectGuid.h"
#include <memory>
class Creature;
class Unit;
namespace StormheimRounds
{
struct Route;
class Controller
{
public:
    explicit Controller(Creature* actor) : me(actor) { }
    bool Start(Unit* summoner);
    bool Running() const { return route != nullptr; }
    void Update(uint32 diff);
    void Movement(uint32 type, uint32 id);
    uint32 GetData(uint32 id) const;
    void SetData(uint32 id, uint32 value);
private:
    void Clear();
    Creature* me;
    Route const* route = nullptr;
    ObjectGuid owner;
    std::weak_ptr<uint8> lifetime;
    uint32 part = 0, node = 1, token = 100, speech = 0, talkTimer = 0, moveTimer = 0;
    bool moving = false, arrived = false, pending = false;
};
}
#endif
