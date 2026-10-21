#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;using uint8=uint8_t;using int8=int8_t;
using TempSummonType=int;using SpellValueMod=int;
constexpr int UNIT_MASK_ACCESSORY=1,VEHICLE_SPELL_RIDE_HARDCODED=46598,SPELLVALUE_BASE_POINT0=0;
int errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...)
struct ObjectGuid{static const ObjectGuid Empty;};const ObjectGuid ObjectGuid::Empty{};
struct Map;
struct Unit
{
    Map* map=nullptr;int entry=114970,clicks=0,rideSpell=0,rideSeat=0;Unit* rideTarget=nullptr;bool rideTriggered=true;
    int GetGUIDLow(){return 1;}bool IsCreature(){return true;}Unit* ToCreature(){return this;}
    int GetDBTableGUIDLow(){return 100;}Map* FindMap(){return map;}
    void HandleSpellClick(Unit*,int){++clicks;assert(false&&"Accessory must not invoke player spellclick actions");}
    void CastCustomSpell(int spell,int,int seat,Unit* base,bool triggered)
    {rideSpell=spell;rideSeat=seat;rideTarget=base;rideTriggered=triggered;}
    void EnterVehicle(Unit*,int8,bool fullTriggered=false);
};
struct TempSummon:Unit
{
    int summonType=0,mask=0;
    struct Info{int Pos=0;}transport;
    struct MoveInfo{Info transport;}m_movementInfo;
    void SetTempSummonType(int t){summonType=t;}void AddUnitTypeMask(int m){mask|=m;}
};
struct Map
{
    TempSummon* result=nullptr;int calls=0,lastEntry=0,lastTimer=0,lastSeat=0;Unit* owner=nullptr;
    TempSummon* SummonCreature(int entry,Unit&,void*,int timer,Unit* base,ObjectGuid,int,int seat)
    {++calls;lastEntry=entry;lastTimer=timer;owner=base;lastSeat=seat;return result;}
};
struct VehicleAccessory{int AccessoryEntry=116044;int8 SeatId=1;bool IsMinion=true;uint8 SummonedType=8;uint32 SummonTime=0;int Pos=7;};
struct Vehicle
{
    enum Status{STATUS_NONE,STATUS_INSTALLED,STATUS_UNINSTALLING};Status _status=STATUS_INSTALLED;
    Unit* _me;int recAura=0;
    int GetCreatureEntry(){return _me->entry;}int GetRecAura(){return recAura;}
    void InstallAccessory(VehicleAccessory const*);
};
#include "Enter.inc"
#include "Install.inc"
int main()
{
    Unit base;Map map;TempSummon rider;map.result=&rider;base.map=&map;Vehicle vehicle{Vehicle::STATUS_INSTALLED,&base};
    VehicleAccessory data;
    for(int8 seat=1;seat<=7;++seat)
    {
        data.SeatId=seat;vehicle.InstallAccessory(&data);
        assert(!base.clicks&&rider.rideSpell==46598&&rider.rideSeat==seat+1&&rider.rideTarget==&base&&!rider.rideTriggered);
        assert(map.owner==&base&&map.lastEntry==data.AccessoryEntry&&map.lastSeat==-1);
        assert(rider.summonType==8&&rider.mask&UNIT_MASK_ACCESSORY&&rider.m_movementInfo.transport.Pos==data.Pos);
    }
    vehicle.recAura=123;rider.rideSpell=0;vehicle.InstallAccessory(&data);assert(rider.rideSeat==0&&map.lastSeat==0&&!base.clicks);
    vehicle.recAura=0;data.IsMinion=false;rider.mask=0;vehicle.InstallAccessory(&data);assert(!rider.mask);
    int before=map.calls;vehicle._status=Vehicle::STATUS_UNINSTALLING;vehicle.InstallAccessory(&data);assert(map.calls==before&&errors==1);
    vehicle._status=Vehicle::STATUS_INSTALLED;base.map=nullptr;vehicle.InstallAccessory(&data);assert(map.calls==before);
    base.map=&map;map.result=nullptr;rider.rideSpell=0;vehicle.InstallAccessory(&data);assert(!rider.rideSpell);
    std::cout<<"PASS: native accessory installation and Unit::EnterVehicle use 46598 with each seat+1, preserve forced-aura/owner/minion/offset/summon state, and skip player click actions; uninstall/map/summon failures covered.\n";
}
