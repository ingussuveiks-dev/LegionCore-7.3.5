#ifndef SCENARIO_RESUME_H
#define SCENARIO_RESUME_H

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

// This is an entrance checkpoint, not a serialization of scenario actors.
// The boot time prevents a recycled instance ID from joining another run.
struct ScenarioResume
{
    uint32_t map = 0, scenario = 0, instance = 0;
    uint64_t boot = 0;
    float x = 0, y = 0, z = 0, o = 0;

    bool Read(std::string const& value)
    {
        *this = {};
        std::istringstream in(value);
        in.imbue(std::locale::classic());
        unsigned version = 0;
        if (!(in >> version >> map >> scenario >> instance >> boot >> x >> y >> z >> o) ||
            version != 1 || !scenario || scenario > 65535 || !std::isfinite(x) ||
            !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(o) || !(in >> std::ws).eof())
        {
            *this = {};
            return false;
        }
        return true;
    }

    std::string Write() const
    {
        if (!scenario) return {};
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(9) << "1 " << map << ' ' << scenario << ' ' << instance << ' ' << boot
            << ' ' << x << ' ' << y << ' ' << z << ' ' << o;
        return out.str();
    }
};
#endif
