#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

struct Position
{
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float x, y, z;
};

#include "AgathaBoulders.inc"

int main()
{
    Position const last{-5.0f, 0.0f, 50.0f}, current{15.0f, 0.0f, 60.0f};
    assert(IsInAgathaBoulderPath(last, current, {5.0f, 4.5f, 55.0f}));
    assert(!IsInAgathaBoulderPath(last, current, {5.0f, 4.51f, 55.0f}));
    assert(!IsInAgathaBoulderPath(last, current, {5.0f, 0.0f, 49.9f}));
    assert(IsInAgathaBoulderPath(last, current, {-9.5f, 0.0f, 50.0f}));
    assert(!IsInAgathaBoulderPath(last, current, {-10.0f, 0.0f, 50.0f}));
    assert(IsInAgathaBoulderPath(last, current, {19.5f, 0.0f, 60.0f}));
    assert(!IsInAgathaBoulderPath(last, current, {20.0f, 0.0f, 60.0f}));
    assert(IsInAgathaBoulderPath(last, current, {0.0f, 0.0f, 52.5f}));
    assert(IsInAgathaBoulderPath(last, last, {-5.0f, 0.0f, 54.5f}));
    assert(!IsInAgathaBoulderPath(last, last, {-5.0f, 0.0f, 54.51f}));
    Position const diagonal{22.0f, 16.0f, 70.0f};
    assert(IsInAgathaBoulderPath(last, diagonal, {8.5f, 8.0f, 60.0f}));
    assert(!IsInAgathaBoulderPath(last, diagonal, {6.0f, 14.0f, 60.0f}));
    std::cout << "PASS swept boulder contact, diagonal and stopped movement, vertical separation and both endpoints.\n";
}
