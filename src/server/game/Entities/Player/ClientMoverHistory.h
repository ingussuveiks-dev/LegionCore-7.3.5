#ifndef CLIENT_MOVER_HISTORY_H
#define CLIENT_MOVER_HISTORY_H

#include <cstdint>
#include <deque>

// CMSG_SET_ACTIVE_MOVER carries no sequence number. During a control handoff
// its acknowledgements can arrive after the server has issued the next mover.
// Retain only recent, actually issued commands; this never grants authority.
template<class Guid>
class ClientMoverHistory
{
public:
    void Record(Guid guid, std::uint32_t now)
    {
        Expire(now);
        _commands.push_back({guid, now});
        if (_commands.size() > 16)
            _commands.pop_front();
    }

    bool Contains(Guid guid, std::uint32_t now)
    {
        Expire(now);
        for (auto const& command : _commands)
            if (command.guid == guid)
                return true;
        return false;
    }

private:
    void Expire(std::uint32_t now)
    {
        while (!_commands.empty() && std::uint32_t(now - _commands.front().time) > 5000)
            _commands.pop_front();
    }
    struct Command { Guid guid; std::uint32_t time; };
    std::deque<Command> _commands;
};
#endif
