#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using uint32 = std::uint32_t;
using int32 = std::int32_t;
enum MovementGeneratorType { IDLE_MOTION_TYPE = 0, WAYPOINT_MOTION_TYPE = 2 };
constexpr unsigned MOTION_SLOT_IDLE = 0;
constexpr unsigned UNIT_STATE_NOT_MOVE = 1;
struct Creature;
struct Player;
struct Unit
{
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
    bool IsPlayer() { return !ToCreature(); }
    bool stopped = true;
    unsigned stops = 0;
    bool IsStopped() { return stopped; }
    void StopMoving() { stopped = true; ++stops; }
};
struct Creature : Unit
{
    Creature* ToCreature() override { return this; }
    uint32 guid = 0, entry = 0, path = 0;
    MovementGeneratorType movement = IDLE_MOTION_TYPE;
    bool controlled = false, blocked = false, casting = false;
    bool GetPlayerMovingMe() { return controlled; }
    MovementGeneratorType GetDefaultMovementType() { return movement; }
    uint32 GetWaypointPath() { return path; }
    uint32 GetEntry() { return entry; }
    uint32 GetGUIDLow() { return guid; }
    char const* GetName() { return "native initial-motion fixture"; }
    bool HasUnitState(unsigned state) { assert(state == UNIT_STATE_NOT_MOVE); return blocked; }
    bool IsMovementPreventedByCasting() { return casting; }
};
struct Player : Unit { };
struct MovementGenerator
{
    virtual ~MovementGenerator() = default;
    virtual void Initialize(Unit&) = 0;
    virtual MovementGeneratorType Type() const = 0;
};
struct IdleMovementGenerator : MovementGenerator
{
    void Initialize(Unit&) override;
    void Reset(Unit&);
    MovementGeneratorType Type() const override { return IDLE_MOTION_TYPE; }
};
using WaypointPath = std::vector<unsigned>;
struct WaypointManager
{
    std::map<uint32, WaypointPath> ordinary, scripted;
    unsigned ordinaryReads = 0, scriptedReads = 0;
    WaypointPath const* GetPath(uint32 id)
    {
        ++ordinaryReads;
        auto found = ordinary.find(id);
        return found == ordinary.end() ? nullptr : &found->second;
    }
    WaypointPath const* GetPathScript(uint32 id)
    {
        ++scriptedReads;
        auto found = scripted.find(id);
        return found == scripted.end() ? nullptr : &found->second;
    }
} waypointManager;
auto* sWaypointMgr = &waypointManager;
unsigned warnings = 0, starts = 0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_DEBUG(...) static_cast<void>(0)
#define ASSERT_NOTNULL(value) (assert(value), value)
#include "MotionTimer.inc"
template<class Actor> struct WaypointMovementGenerator;
template<> struct WaypointMovementGenerator<Creature> : MovementGenerator
{
    TimeTrackerSmall _nextMoveTime;
    bool _isArrivalDone;
    uint32 _pathId;
    bool _repeating, _goBack;
    float _randomMoveX, _randomMoveY;
    bool _forceTPToStart, _recalculateSpeed, _loadedFromDB, _stalled;
    int32 _pauseTime;
    WaypointPath const* _path = nullptr;
#include "WaypointConstructor.inc"
    void LoadPath(Creature&);
    void DoInitialize(Creature&);
    bool CanMove(Creature&);
    bool StartMoveNow(Creature&) { ++starts; return true; }
    void Initialize(Unit& owner) override { DoInitialize(*owner.ToCreature()); }
    MovementGeneratorType Type() const override { return WAYPOINT_MOTION_TYPE; }
};
template<> struct WaypointMovementGenerator<Player> : MovementGenerator
{
    explicit WaypointMovementGenerator(uint32) { }
    void Initialize(Unit&) override { assert(false); }
    MovementGeneratorType Type() const override { return WAYPOINT_MOTION_TYPE; }
};
struct MovementFactory
{
    MovementGeneratorType type;
    MovementGenerator* Create(Unit*)
    {
        if (type == IDLE_MOTION_TYPE)
            return new IdleMovementGenerator;
        return new WaypointMovementGenerator<Creature>;
    }
};
struct Registry
{
    MovementFactory idle{IDLE_MOTION_TYPE}, waypoint{WAYPOINT_MOTION_TYPE};
    MovementFactory* GetRegistryItem(MovementGeneratorType type)
    {
        assert(type == IDLE_MOTION_TYPE || type == WAYPOINT_MOTION_TYPE);
        return type == IDLE_MOTION_TYPE ? &idle : &waypoint;
    }
} registry;
auto* sMovementGeneratorRegistry = &registry;
namespace FactorySelector
{
#include "SelectMovement.inc"
}
struct MotionMaster
{
    Unit* _owner;
    std::unique_ptr<MovementGenerator> selected;
    explicit MotionMaster(Unit* owner) : _owner(owner) { }
    void Mutate(MovementGenerator* generator, unsigned slot)
    {
        assert(slot == MOTION_SLOT_IDLE);
        selected.reset(generator);
        selected->Initialize(*_owner);
    }
    void InitDefault();
    void MovePath(uint32, bool, float = 0, float = 0);
};
#include "InitialMotionMethods.inc"

int main(int argc, char** argv)
{
    assert(argc == 2);
    std::ifstream input(argv[1]);
    assert(input.good());
    unsigned baselineRows = 0, fixedRows = 0;
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream fields(line);
        std::string label;
        fields >> label;
        if (label == "path")
        {
            uint32 path = 0, count = 0;
            fields >> path >> count;
            assert(fields && count);
            waypointManager.scripted[path] = WaypointPath(count);
            continue;
        }
        if (label != "before" && label != "after")
            continue;
        Creature actor;
        unsigned movement = 0;
        fields >> actor.guid >> actor.entry >> movement >> actor.path;
        assert(fields && actor.path == 0);
        actor.movement = static_cast<MovementGeneratorType>(movement);
        unsigned previousWarnings = warnings, previousStarts = starts;
        MotionMaster motion(&actor);
        motion.InitDefault();
        if (label == "before")
        {
            ++baselineRows;
            assert(movement == 2 && warnings == previousWarnings + 1 && starts == previousStarts);
            assert(motion.selected->Type() == WAYPOINT_MOTION_TYPE);
        }
        else
        {
            ++fixedRows;
            assert(movement == 0 && warnings == previousWarnings && starts == previousStarts);
            assert(motion.selected->Type() == IDLE_MOTION_TYPE);
            actor.stopped = false;
            motion.InitDefault();
            assert(actor.stopped && actor.stops == 1 && warnings == previousWarnings);
        }
    }
    assert(baselineRows == 7 && fixedRows == 7);
    assert(warnings == 7 && starts == 0 && waypointManager.ordinaryReads == 7 && waypointManager.scriptedReads == 0);
    Creature actor;
    MotionMaster motion(&actor);
    motion.InitDefault();
    for (uint32 path : {10993804u, 10993805u, 10993806u, 10993807u})
    {
        motion.MovePath(path, false);
        auto* waypoint = dynamic_cast<WaypointMovementGenerator<Creature>*>(motion.selected.get());
        assert(waypoint && waypoint->_pathId == path && waypoint->_path && !waypoint->_path->empty() && !waypoint->_repeating);
        assert(waypoint->_nextMoveTime.GetExpiry() == 3000 && !waypoint->CanMove(actor));
        waypoint->_nextMoveTime.Update(2999);
        assert(!waypoint->CanMove(actor));
        waypoint->_nextMoveTime.Update(1);
        assert(waypoint->CanMove(actor));
        actor.blocked = true;
        assert(!waypoint->CanMove(actor));
        actor.blocked = false;
        actor.casting = true;
        assert(!waypoint->CanMove(actor));
        actor.casting = false;
    }
    assert(starts == 0 && warnings == 7 && waypointManager.scriptedReads == 4);
    motion.MovePath(0, false);
    assert(starts == 0 && warnings == 7);
    motion.MovePath(123456789, false);
    assert(warnings == 8 && starts == 0);
    waypointManager.ordinary[42] = WaypointPath(3);
    actor.movement = WAYPOINT_MOTION_TYPE;
    actor.path = 42;
    motion.InitDefault();
    assert(starts == 0 && warnings == 8);
    auto* ordinary = dynamic_cast<WaypointMovementGenerator<Creature>*>(motion.selected.get());
    assert(ordinary && ordinary->_path == &waypointManager.ordinary[42]);
    actor.controlled = true;
    motion.InitDefault();
    assert(motion.selected->Type() == IDLE_MOTION_TYPE && starts == 0 && warnings == 8);
    Player player;
    MotionMaster playerMotion(&player);
    playerMotion.InitDefault();
    assert(playerMotion.selected->Type() == IDLE_MOTION_TYPE);
    std::cout << "PASS seven actual-row missing-default baselines/fixed idle/reinitialization; four explicit script paths/native 3000ms admission timer/stun/cast gates; missing-path diagnostics; valid ordinary path; controlled/player idle. Controlled registry, no actual spline/grid/client traversal.\n";
}
