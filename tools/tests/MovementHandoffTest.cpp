#include <cassert>
#include <iostream>
#include <string>

struct Guid
{
    unsigned value;
    bool operator==(Guid rhs) const { return value == rhs.value; }
    bool operator!=(Guid rhs) const { return !(*this == rhs); }
    std::string ToString() const { return std::to_string(value); }
};
enum { VEHICLE_SEAT_FLAG_CAN_CONTROL = 0x800 };
struct VehicleSeatEntry { unsigned Flags = 0; };
struct Unit
{
    Guid guid;
    bool inWorld = true;
    bool IsInWorld() const { return inWorld; }
    Guid GetGUID() const { return guid; }
};
struct Player;
struct Vehicle
{
    VehicleSeatEntry seat;
    bool seated = true;
    VehicleSeatEntry const* GetSeatForPassenger(Player*) { return seated ? &seat : nullptr; }
};
struct Player : Unit
{
    Unit* mover = nullptr;
    Unit* vehicleBase = nullptr;
    Vehicle vehicle;
    Unit* GetUnitBeingMoved() { return mover; }
    Unit* GetVehicleBase() { return vehicleBase; }
    Vehicle* GetVehicle() { return &vehicle; }
};
namespace WorldPackets { namespace Movement {
struct SetActiveMover { Guid ActiveMover; };
struct MoveSetActiveMover
{
    Guid MoverGUID;
    MoveSetActiveMover* Write() { return this; }
};
} }
unsigned errors = 0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct WorldSession
{
    Player* player;
    unsigned sent = 0;
    Guid lastSent{0};
    Player* GetPlayer() { return player; }
    void SendPacket(WorldPackets::Movement::MoveSetActiveMover* packet)
    {
        ++sent;
        lastSent = packet->MoverGUID;
    }
    void HandleSetActiveMover(WorldPackets::Movement::SetActiveMover& packet);
};
#include "MovementHandoff.inc"

int main()
{
    Player player;
    player.guid = {1};
    Unit bird{{2}};
    player.mover = player.vehicleBase = &bird;
    WorldSession session{&player};
    WorldPackets::Movement::SetActiveMover oldSelf{{1}};
    session.HandleSetActiveMover(oldSelf);
    assert(session.sent == 1 && session.lastSent == bird.guid && errors == 0);
    assert(player.mover == &bird); // Client cannot override server authority.
    WorldPackets::Movement::SetActiveMover matching{{2}}, unrelated{{3}};
    session.HandleSetActiveMover(matching);
    assert(session.sent == 1 && errors == 0);
    session.HandleSetActiveMover(unrelated);
    assert(session.sent == 1 && errors == 1);
    player.vehicle.seat.Flags = VEHICLE_SEAT_FLAG_CAN_CONTROL;
    session.HandleSetActiveMover(oldSelf);
    assert(session.sent == 1 && errors == 2);
    player.vehicle.seat.Flags = 0;
    player.vehicle.seated = false;
    session.HandleSetActiveMover(oldSelf);
    assert(session.sent == 1 && errors == 3);
    player.vehicleBase = nullptr;
    session.HandleSetActiveMover(oldSelf);
    assert(session.sent == 1 && errors == 4);
    player.inWorld = false;
    session.HandleSetActiveMover(oldSelf);
    assert(session.sent == 1 && errors == 4);
    std::cout << "Scripted mover resync and rejection of unrelated mover GUIDs passed.\n";
}
