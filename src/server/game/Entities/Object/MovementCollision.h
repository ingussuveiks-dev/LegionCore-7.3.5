#ifndef LEGION_MOVEMENT_COLLISION_H
#define LEGION_MOVEMENT_COLLISION_H

class WorldObject;
struct Position;
struct DynamicTreeCallback;

void MovePositionWithCollision(WorldObject& mover, Position& position, float distance, float angle,
    DynamicTreeCallback* query = nullptr, bool measureHeightFromFeet = false);

#endif
