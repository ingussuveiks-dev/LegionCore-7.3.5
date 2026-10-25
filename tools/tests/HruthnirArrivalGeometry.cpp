#include <list>
#include <vector>
#include "Position.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

static void Require(bool condition)
{
    if (!condition)
        std::abort();
}

int main()
{
    Position const destination(4720.80f, 347.91f, -37.71f);
    auto admitted = [](Position const& actor)
    {
        return actor.GetExactDistSq(4720.80f, 347.91f, -37.71f) <= 4.0f;
    };
    Require(admitted(destination));
    unsigned checks = 1;
    for (unsigned axis = 0; axis < 3; ++axis)
        for (float direction : {-1.0f, 1.0f})
            for (float distance : {1.99f, 2.0f, 2.01f})
            {
                Position actor(destination);
                if (axis == 0) actor.m_positionX += direction * distance;
                if (axis == 1) actor.m_positionY += direction * distance;
                if (axis == 2) actor.m_positionZ += direction * distance;
                Require(admitted(actor) == (distance <= 2.0f));
                ++checks;
            }
    Position diagonal(destination);
    diagonal.m_positionX += 1.5f;
    diagonal.m_positionY += 1.5f;
    Require(!admitted(diagonal));
    ++checks;
    for (unsigned axis = 0; axis < 3; ++axis)
        for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()})
        {
            Position actor(destination);
            if (axis == 0) actor.m_positionX = invalid;
            if (axis == 1) actor.m_positionY = invalid;
            if (axis == 2) actor.m_positionZ = invalid;
            Require(!admitted(actor));
            ++checks;
        }
    Require(checks == 29);
    std::puts("PASS native Position:29 actual3D squared-distance cases; inclusive2-yard boundary, six axis directions, diagonal and nonfinite rejection. Not a native spline traversal or retail radius measurement.");
}
