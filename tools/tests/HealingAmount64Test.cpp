#include "HealingAmount.h"
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <cassert>
#include <iostream>
using Exact = boost::multiprecision::cpp_bin_float_100;
static uint64 Bound(Exact value)
{
    if (value <= 0) return 0;
    if (value >= Exact(UINT64_MAX)) return UINT64_MAX;
    return value.convert_to<uint64>();
}
int main()
{
    uint32 cases = 0;
    for (uint64 amount : {0ull,1ull,1000ull,2147483647ull,2147483648ull,4294967295ull,1099511627793ull,9223372036854775808ull,18446744073709551615ull})
        for (int64 flat : {INT64_MIN,int64(-1000),int64(-1),int64(0),int64(1),int64(1000),INT64_MAX})
            for (float factor : {-1.f,0.f,std::numeric_limits<float>::denorm_min(),0.5f,1.f,1.01f,2.f,100.f,FLT_MAX})
            {
                Exact sum = Exact(amount) + flat;
                uint64 expected = Bound(sum * Exact(factor));
                if (sum <= 0 || factor <= 0) expected = 0;
                else if (amount <= INT32_MAX && sum <= INT32_MAX)
                {
                    float legacy = float(sum.convert_to<int32>()) * factor;
                    if (legacy < 4294967296.f) expected = uint64(legacy);
                }
                assert(HealingAmount::Apply(amount,flat,factor) == expected); ++cases;
            }
    for (uint64 amount : {0ull,1ull,1000ull,2147483647ull,2147483648ull,1099511627793ull,18446744073709551615ull})
        for (float flat : {-FLT_MAX,-1.5f,-0.5f,0.f,std::numeric_limits<float>::denorm_min(),0.5f,1.5f,FLT_MAX})
            for (float factor : {0.f,0.5f,1.f,1.01f,2.f,FLT_MAX})
            {
                Exact value = (Exact(amount) + Exact(flat)) * Exact(factor);
                uint64 expected = Bound(value);
                if (!flat) expected = HealingAmount::Apply(amount,0,factor);
                else if (amount <= INT32_MAX)
                {
                    float legacy = float(amount + flat) * factor;
                    if (legacy >= 0.f && legacy < 4294967296.f) expected = uint64(legacy);
                }
                assert(HealingAmount::ApplySpellMod(amount,flat,factor) == expected); ++cases;
            }
    for (float amount : {-1.f,0.f,0.5f,1.f,1.5f,2147483648.f,4294967296.f,1099511627776.f,18446744073709551616.f})
    {
        assert(HealingAmount::FromFloat(amount) == Bound(Exact(amount))); ++cases;
    }
    assert(HealingAmount::Add(UINT64_MAX,1)==UINT64_MAX);
    assert(HealingAmount::CombatLogValue(UINT64_MAX)==INT32_MAX);
    assert(HealingAmount::CounterValue(UINT64_MAX)==UINT32_MAX);
    std::cout << "PASS " << cases << " exact healing arithmetic cases: independent333bit floating oracle,legacy ordinary rounding,full64integer/flat/factor boundaries and protocol-only projection\n";
}
