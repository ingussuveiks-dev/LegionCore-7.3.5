// OPEN caller counterfactual, not a repaired-behavior certificate.
#include "Support.inc"
int main()
{
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    std::cout << std::unitbuf;
    for (bool refused : {false, true})
    {
        Run r(0);
        Creature boss{98257};
        boss.i_AI = new CreatureAI(&boss);
        r.actor.nearby.push_back(&boss);
        for (auto actor : r.actor.nearby) actor->nearby = r.actor.nearby;
        Conversation::fails = refused;
        r.actor.AI()->DoAction(true);
        assert(Conversation::created == std::vector<uint32>{1412});
        assert(Conversation::allocated.size() == (refused ? 0u : 1u));
        assert(r.actor.attacks == std::vector<uint32>{98257});
        assert(r.ally.attacks == std::vector<uint32>{98257});
        r.Tick(30000);
        assert(Conversation::created == std::vector<uint32>{1412});
        assert(r.actor.attacks.size() == 1 && r.ally.attacks.size() == 1);
        std::cout << (refused ? "COUNTERFACTUAL OPEN: " : "CONTROL: ")
                  << "current full Ragnvald1412 factory result=" << !refused
                  << "; native attack requests remain once, no conversation retry after30s. "
                  << "Factory/Map/visibility endpoints controlled; no native full-core or client claim.\n";
    }
    std::cout << "OPEN: current caller loses a refused1412; no passing repaired counterpart exists yet. "
                 "Retained handoff tests are separate from fixing factory admission/lifetime/voice sequencing.\n";
}
