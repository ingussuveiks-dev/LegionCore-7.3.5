#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

// Track C++ object lifetime before dereferencing the native selected pointer.
// Coordinates and the caller's services are controlled fixtures; the included
// declaration/switch is extracted verbatim from SelectImplicitBetweenTargets.
struct Position
{
    static std::set<Position const*> live;
    float x = 0, y = 0;
    Position(float px = 0, float py = 0) : x(px), y(py) { live.insert(this); }
    Position(Position const& other) : x(other.x), y(other.y) { live.insert(this); }
    ~Position() { live.erase(this); }
};
std::set<Position const*> Position::live;
struct Unit : Position
{
    using Position::Position;
    void GetNearPoint2D(Position& point, float distance, float angle)
    { point.x = x + distance * std::cos(angle); point.y = y + distance * std::sin(angle); }
};
enum Reference { TARGET_REFERENCE_TYPE_NEAR_DEST, TARGET_REFERENCE_TYPE_SRC, TARGET_REFERENCE_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER };
struct TargetType { Reference reference; Reference GetReferenceType() const { return reference; } };
struct Targets
{
    Position source{10, 20}, destination{30, 40};
    Position const* GetSrcPos() const { return &source; }
    Position const* GetDstPos() const { return &destination; }
};
struct ExpiredPosition : std::runtime_error { ExpiredPosition() : std::runtime_error("native center points to an expired Position") {} };
static Position Select(Unit* m_caster, Unit* referer, Targets const& m_targets, TargetType targetType, float dist, float angle)
{
#include "BetweenSelection.inc"
    if (!Position::live.count(center)) throw ExpiredPosition();
    return *center;
}
static void Require(bool ok, char const* message) { if (!ok) throw std::runtime_error(message); }
int main()
{
    try
    {
        Unit caster{6235, 4951}, referer{100, 200}; Targets targets;
        unsigned expired = 0, completed = 0;
        for (float distance : {1.0f, 8.0f, 45.0f, 100.0f})
            for (float angle : {0.0f, 1.0f, 3.1415926f, 4.5f})
            {
                try
                {
                    auto point = Select(&caster, &referer, targets, {TARGET_REFERENCE_TYPE_NEAR_DEST}, distance, angle);
                    Require(std::fabs(point.x - (caster.x + distance * std::cos(angle))) < 0.01f &&
                        std::fabs(point.y - (caster.y + distance * std::sin(angle))) < 0.01f, "Native selection changed near-destination coordinates");
                    ++completed;
                }
                catch (ExpiredPosition const&) { ++expired; }
            }
#ifdef BETWEEN_LIFETIME_BASELINE
        Require(expired == 16 && completed == 0, "Pinned native switch did not reproduce all sixteen expired positions");
        std::cout << "PASS: pinned de86651c native switch selects an expired NEAR_DEST Position in all16 cases.\n";
#else
        Require(expired == 0 && completed == 16, "Native selected position expired before target search");
        std::cout << "PASS: native NEAR_DEST Position stays alive for all16 target-search selections with unchanged coordinates.\n";
#endif
        for (auto reference : {TARGET_REFERENCE_TYPE_SRC, TARGET_REFERENCE_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER})
        {
            auto selected = Select(&caster, &referer, targets, {reference}, 8, 1);
            auto expected = reference == TARGET_REFERENCE_TYPE_SRC ? targets.GetSrcPos() :
                reference == TARGET_REFERENCE_TYPE_DEST ? targets.GetDstPos() : &referer;
            Require(selected.x == expected->x && selected.y == expected->y, "Native source/destination/referer selection changed");
        }
        std::cout << "PASS: existing source/destination/referer pointers remain live and select their original coordinates.\n";
        return 0;
    }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
