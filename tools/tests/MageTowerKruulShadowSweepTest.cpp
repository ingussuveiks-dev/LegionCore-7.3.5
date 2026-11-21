#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using uint32 = std::uint32_t;
constexpr float HighlordsReturnSweepDistance = 120.0f;

struct Position
{
    Position() = default;
    Position(float x, float y, float z, float angle) : X(x), Y(y), Z(z), Angle(angle) {}
    float GetPositionX() const { return X; }
    float GetPositionY() const { return Y; }
    float GetPositionZ() const { return Z; }
    float GetOrientation() const { return Angle; }
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float Angle = 0.0f;
};

#include "KruulShadowSweep.inc"

Position Offset(Position const& origin, float forward, float side, float height = 0.0f)
{
    float const x = std::cos(origin.Angle);
    float const y = std::sin(origin.Angle);
    return { origin.X + forward * x - side * y, origin.Y + forward * y + side * x, origin.Z + height, origin.Angle };
}

int main()
{
    for (uint32 direction = 0; direction < 4; ++direction)
    {
        Position left = GetShadowSweeperStart(direction, -14.0f);
        Position right = GetShadowSweeperStart(direction, 14.0f);
        Position mid = Offset(left, 60.0f, 14.0f);
        assert(std::abs(mid.X - 3988.5f) < 0.01f && std::abs(mid.Y + 2404.0f) < 0.01f);
        assert(std::abs(std::hypot(left.X - right.X, left.Y - right.Y) - 28.0f) < 0.01f);

        Position previous = Offset(left, 45.0f, 0.0f);
        Position current = Offset(left, 50.0f, 0.0f);
        assert(ShadowSweepTouches(previous, current, Offset(left, 47.0f, 12.0f)));
        assert(!ShadowSweepTouches(current, current, Offset(left, 47.0f, 12.0f)));
        assert(!ShadowSweepTouches(previous, current, Offset(left, 47.0f, 13.0f)));
        assert(!ShadowSweepTouches(previous, current, Offset(left, 47.0f, 12.0f, 1.5f)));
        assert(!ShadowSweepTouches(previous, current, Offset(left, 47.0f, 12.0f, -1.5f)));
        assert(!ShadowSweepTouches(previous, current, Offset(left, 44.5f, 0.0f)));
        assert(!ShadowSweepTouches(previous, current, Offset(left, 50.5f, 0.0f)));
        assert(ShadowSweepTouches(current, current, Offset(left, 50.25f, 0.0f)));
        assert(!ShadowSweepTouches(left, Offset(left, 120.0f, 0.0f), mid));
        assert(!ShadowSweepTouches(right, Offset(right, 120.0f, 0.0f), mid));
    }
    std::cout << "PASS four Shadow Sweep directions, both lanes, swept contact, jump height and three-yard gap\n";
}
