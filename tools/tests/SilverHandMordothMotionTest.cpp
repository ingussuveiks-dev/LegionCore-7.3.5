#define SILVER_HAND_MOTION_EMBEDDED
#include "SilverHandInitialMotionTest.cpp"

constexpr unsigned MOTION_SLOT_ACTIVE = 1;
constexpr unsigned UNIT_FIELD_FLAGS = 1;
constexpr unsigned UNIT_FLAG_PLAYER_CONTROLLED = 0x01000000;
constexpr unsigned SMART_EVENT_REACHED_HOME = 21;
constexpr auto POINT_MOTION_TYPE = static_cast<MovementGeneratorType>(8);
struct SmartCreature;
struct CreatureGroup
{
    Creature* leader = nullptr;
    bool formed = false;
    Creature* getLeader() { return leader; }
    bool isFormed() { return formed; }
};
struct SmartMotion
{
    MovementGeneratorType idle = IDLE_MOTION_TYPE, active = IDLE_MOTION_TYPE;
    unsigned chases = 0, paths = 0, idles = 0;
    Unit* target = nullptr;
    MovementGeneratorType GetMotionSlotType(unsigned slot) { return slot == MOTION_SLOT_IDLE ? idle : active; }
    void MoveChase(Unit* who) { target = who; ++chases; }
    void MovePath(uint32 path, bool repeat) { assert(path == 42 && repeat); ++paths; }
    void MoveIdle() { ++idles; }
};
struct SmartCreature : Creature
{
    SmartMotion motion;
    CreatureGroup* formation = nullptr;
    unsigned attacks = 0, resumes = 0;
    bool attackSuccess = true;
    bool HasFlag(unsigned field, unsigned flag) { assert(field == UNIT_FIELD_FLAGS && flag == UNIT_FLAG_PLAYER_CONTROLLED); return controlled; }
    bool Attack(Unit* who, bool melee) { assert(who && melee); ++attacks; return attackSuccess; }
    SmartMotion* GetMotionMaster() { return &motion; }
    CreatureGroup* GetFormation() { return formation; }
    int GetPosition() { return 123; }
    void ResumeMovement() { ++resumes; }
};
struct Script
{
    unsigned homes = 0;
    void ProcessEventsFor(unsigned event) { assert(event == SMART_EVENT_REACHED_HOME); ++homes; }
};
struct SmartAI
{
    SmartCreature* me;
    bool mCanCombatMove = true, mRun = false;
    unsigned runs = 0;
    int mLastOOCPos = 0;
    Script script;
    explicit SmartAI(SmartCreature* actor) : me(actor) { }
    Script* GetScript() { return &script; }
    void SetRun(bool value) { assert(value == mRun); ++runs; }
    void AttackStart(Unit*);
    void JustReachedHome();
};
#include "MordothSmartMethods.inc"

int main(int argc, char** argv)
{
    assert(argc == 3 && std::string(argv[2]) == "1");
    assert(RunInitialMotionTest(argc, argv) == 0);
    Player player;
    Creature pet, guardian, defender;
    for (Unit* target : std::vector<Unit*>{&player, &pet, &guardian, &defender})
        for (auto active : {IDLE_MOTION_TYPE, WAYPOINT_MOTION_TYPE, POINT_MOTION_TYPE})
            for (bool running : {false, true})
            {
                SmartCreature actor;
                actor.entry = 106719;
                actor.motion.active = active;
                SmartAI smart(&actor);
                smart.mRun = running;
                smart.AttackStart(target);
                assert(actor.attacks == 1 && actor.motion.chases == 1 && actor.motion.target == target);
                assert(actor.stops == (active == IDLE_MOTION_TYPE ? 0u : 1u));
                assert(smart.runs == 1 && smart.mLastOOCPos == actor.GetPosition());
                smart.JustReachedHome();
                assert(smart.script.homes == 1 && actor.resumes == 1 && actor.motion.paths == 0);
            }
    SmartCreature actor;
    SmartAI smart(&actor);
    smart.AttackStart(nullptr);
    assert(actor.attacks == 0 && actor.motion.chases == 0 && smart.mLastOOCPos == 0);
    actor.attackSuccess = false;
    smart.AttackStart(&player);
    assert(actor.attacks == 1 && actor.motion.chases == 0 && smart.mLastOOCPos == 0);
    actor.attackSuccess = true;
    smart.mCanCombatMove = false;
    smart.AttackStart(&defender);
    assert(actor.attacks == 2 && actor.motion.chases == 0 && smart.runs == 0 && smart.mLastOOCPos == 123);
    actor.controlled = true;
    smart.mCanCombatMove = true;
    smart.AttackStart(&player);
    assert(actor.attacks == 3 && actor.motion.chases == 0 && smart.runs == 0);
    smart.AttackStart(nullptr);
    assert(actor.attacks == 3);
    actor.controlled = false;
    actor.path = 42;
    smart.JustReachedHome();
    assert(actor.motion.paths == 1 && actor.resumes == 0);
    actor.motion.idle = WAYPOINT_MOTION_TYPE;
    smart.JustReachedHome();
    assert(actor.motion.paths == 1 && actor.resumes == 1);
    CreatureGroup formation;
    formation.leader = &defender;
    formation.formed = true;
    actor.formation = &formation;
    smart.JustReachedHome();
    assert(actor.motion.idles == 1 && actor.motion.paths == 1 && actor.resumes == 1 && smart.script.homes == 3);
    std::cout << "PASS native SmartAI AttackStart/home branches retain combat chase independently of idle default; target-type labels, null/failure, charmed/no-combat-move, path and formed-follower guards. Controlled attack/movement services, not live pet/combat/navigation proof.\n";
}
