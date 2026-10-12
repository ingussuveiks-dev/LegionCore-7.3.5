#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
using int8 = int8_t;
constexpr int QUEST_STATUS_INCOMPLETE = 3;
#define TC_LOG_INFO(...) ((void)0)
struct Quest {};
struct QuestStore {
    Quest quest;
    bool available = true;
    Quest const* GetQuestTemplate(int id) { assert(id == 40522); return available ? &quest : nullptr; }
} questStore;
auto sQuestDataStore = &questStore;
namespace WorldPackets { namespace Misc {
struct CompleteMovie {};
struct TriggerMovie { uint32 MovieID; void* Write() { return nullptr; } };
}}
struct Player {
    int status = 3, credit = 0, teleports = 0, moviePackets = 0;
    bool watching = false, delay = false, pending = false;
    std::vector<uint32> casts;
    int GetQuestStatus(int id) { assert(id == 40522); return status; }
    int GetQuestObjectiveData(Quest const*, int8 index) { assert(index == 2); return credit; }
    bool isWatchingMovie() const { return watching; }
    void setWatchinMovie(bool value) { watching = value; }
    void SetCanDelayTeleport(bool value) { delay = value; }
    void SendDirectMessage(void*) { ++moviePackets; }
    void SendMovieStart(uint32);
    void CastSpell(Player* target, uint32 id, bool triggered) {
        assert(target == this && triggered);
        casts.push_back(id);
        if (id == 198897) {
            SendMovieStart(474); // Native PLAY_MOVIE effect.
            CastSpell(this, 200282, true); // Verified world linked spell.
        } else if (id == 200282) {
            ++credit;
            if (watching || delay) pending = true;
            else ++teleports;
        }
    }
    void Update() { if (pending && !watching && !delay) { pending = false; ++teleports; } }
};
struct WorldSession {
    Player* player;
    Player* GetPlayer() { return player; }
    void HandleCompleteMovie(WorldPackets::Misc::CompleteMovie&);
};
#include "Funeral.inc"
#include "MovieStart.inc"
#include "MovieComplete.inc"
int main() {
    WorldPackets::Misc::CompleteMovie packet;
    for (int status : {0, 1, 6}) {
        Player p; p.status = status; HordeFuneral::StartMovie(&p); assert(p.casts.empty());
    }
    Player passed; passed.credit = 1;
    HordeFuneral::StartMovie(&passed); assert(passed.casts.empty());
    Player otherMovie; otherMovie.watching = true;
    HordeFuneral::StartMovie(&otherMovie); assert(otherMovie.casts.empty());
    Player missing; questStore.available = false;
    HordeFuneral::StartMovie(&missing); assert(missing.casts.empty());
    questStore.available = true;
    // The client uses the same completion packet on normal finish and skip.
    for (int finishOrSkip = 0; finishOrSkip < 2; ++finishOrSkip) {
        Player p;
        assert(HordeFuneral::CanStart(&p));
        HordeFuneral::StartMovie(&p);
        assert(p.casts == std::vector<uint32>({200252, 198897, 200282}));
        assert(p.moviePackets == 1 && p.teleports == 0 && p.pending);
        p.Update(); assert(p.teleports == 0);
        HordeFuneral::StartMovie(&p); assert(p.moviePackets == 1);
        WorldSession session{&p}; session.HandleCompleteMovie(packet);
        p.Update(); assert(p.teleports == 1 && !p.pending && !p.watching);
        session.HandleCompleteMovie(packet); p.Update(); assert(p.teleports == 1);
        // A recreated NPC cannot replay the movie while this quest is still active.
        HordeFuneral::StartMovie(&p); assert(p.moviePackets == 1);
    }
    WorldSession absent{nullptr}; absent.HandleCompleteMovie(packet);
    std::cout << "PASS: funeral checkpoint, delayed-state recheck, one movie/teleport, completion/skip, duplicate callback and no replay after credit.\n";
}
