#ifndef LEGION_HEALING_AMOUNT_H
#define LEGION_HEALING_AMOUNT_H

#include "Define.h"
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <cmath>
#include <cstring>
#include <limits>

namespace HealingAmount
{
    inline uint64 Add(uint64 left, uint64 right)
    {
        return left + std::min(right, std::numeric_limits<uint64>::max() - left);
    }

    // Integer target/cloud counts must not lose ordinary low bits through float.
    inline uint64 Multiply(uint64 amount, uint64 count)
    {
        if (count && amount > std::numeric_limits<uint64>::max() / count)
            return std::numeric_limits<uint64>::max();
        return amount * count;
    }

    inline uint64 FromFloat(float amount)
    {
        if (!(amount > 0.0f))
            return 0;
        if (double(amount) >= double(std::numeric_limits<uint64>::max()))
            return std::numeric_limits<uint64>::max();
        return uint64(amount);
    }

    // Keep the former rounding for its valid ordinary range. Above that range,
    // multiply by the exact binary32 factor without rounding the integer input.
    inline uint64 Apply(uint64 amount, int64 flat, float factor)
    {
        using boost::multiprecision::int128_t;
        using boost::multiprecision::uint128_t;
        int128_t sum = int128_t(amount) + flat;
        if (sum <= 0 || !(factor > 0.0f))
            return 0;
        if (!std::isfinite(factor))
            return std::numeric_limits<uint64>::max();
        if (amount <= uint64(INT32_MAX) && sum <= INT32_MAX)
        {
            float legacy = float(int32(sum)) * factor;
            if (legacy < 4294967296.0f)
                return uint64(legacy);
        }

        uint32 bits;
        std::memcpy(&bits, &factor, sizeof(bits));
        int exponent = int((bits >> 23) & 255);
        uint32 mantissa = bits & 0x7fffff;
        if (exponent)
            mantissa |= 0x800000;
        exponent = exponent ? exponent - 150 : -149;
        uint128_t product = uint128_t(sum) * mantissa;
        uint128_t maximum = std::numeric_limits<uint64>::max();
        if (exponent >= 0)
        {
            if (exponent >= 64 || product > (maximum >> exponent))
                return std::numeric_limits<uint64>::max();
            product <<= exponent;
        }
        else
            product = -exponent >= 128 ? 0 : product >> -exponent;
        return product > maximum ? std::numeric_limits<uint64>::max() : uint64(product);
    }

    inline int32 CombatLogValue(uint64 amount)
    {
        return int32(std::min<uint64>(amount, INT32_MAX));
    }

    // AchievementMgr's native progress accumulator saturates at UINT32_MAX.
    // Preserve that domain boundary instead of wrapping a wider heal on entry.
    inline uint32 CounterValue(uint64 amount)
    {
        return uint32(std::min<uint64>(amount, UINT32_MAX));
    }

    inline boost::multiprecision::cpp_rational FloatRatio(float value)
    {
        uint32 bits; std::memcpy(&bits, &value, sizeof(bits));
        int exponent = int((bits >> 23) & 255);
        uint32 mantissa = bits & 0x7fffff;
        if (exponent) mantissa |= 0x800000;
        exponent = exponent ? exponent - 150 : -149;
        boost::multiprecision::cpp_rational result(mantissa);
        if (exponent >= 0) result *= boost::multiprecision::cpp_int(1) << exponent;
        else result /= boost::multiprecision::cpp_int(1) << -exponent;
        return bits >> 31 ? -result : result;
    }

    inline uint64 ApplySpellMod(uint64 amount, float flat, float factor)
    {
        if (!flat) return Apply(amount, 0, factor);
        if (!(factor > 0.0f) || std::isnan(flat)) return 0;
        if (!std::isfinite(factor) || !std::isfinite(flat))
            return flat < 0 ? 0 : UINT64_MAX;
        if (amount <= uint64(INT32_MAX))
        {
            float legacy = float(amount + flat) * factor;
            if (legacy >= 0.0f && legacy < 4294967296.0f) return uint64(legacy);
        }
        boost::multiprecision::cpp_rational result =
            (boost::multiprecision::cpp_rational(amount) + FloatRatio(flat)) * FloatRatio(factor);
        if (result <= 0) return 0;
        if (result >= boost::multiprecision::cpp_rational(UINT64_MAX)) return UINT64_MAX;
        return uint64(boost::multiprecision::numerator(result) / boost::multiprecision::denominator(result));
    }
}
#endif
