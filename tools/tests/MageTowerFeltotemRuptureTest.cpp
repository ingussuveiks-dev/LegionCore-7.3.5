#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

struct Position
{
    float x, y, z, angle = 0.0f;
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float GetOrientation() const { return angle; }
};

#include "FeltotemRupture.inc"

int main()
{
    unsigned checks = 0;
    for (float orientation : { 0.0f, 0.3f, 0.78539816f, 1.5707963f })
    {
        Position const origin{ 5207.0f, 5057.0f, 626.0f, orientation };
        for (float sign : { -1.0f, 1.0f })
        {
            auto point = [&](float x, float y, float z)
            {
                float const cs = std::cos(orientation), sn = std::sin(orientation);
                return Position{ origin.x + sign * (x * cs - y * sn), origin.y + sign * (x * sn + y * cs), origin.z + z };
            };
            for (auto const& local : std::array<std::array<float, 3>, 5>{{ {0,0,0}, {7.4f,1.2f,0},
                {1.2f,7.4f,0}, {0,2,2.4f}, {7.0f,0,-2.4f} }})
            {
                assert(IsInRuptureCross(origin, point(local[0], local[1], local[2])));
                ++checks;
            }
            for (auto const& local : std::array<std::array<float, 3>, 5>{{ {7.6f,0,0}, {0,7.6f,0},
                {1.35f,1.35f,0}, {0,0,2.6f}, {6.0f,0,-2.6f} }})
            {
                assert(!IsInRuptureCross(origin, point(local[0], local[1], local[2])));
                ++checks;
            }
        }
    }
    std::cout << "PASS " << checks << " boundaries in all arms, diagonals, rotations, and other heights.\n";
}
