// Exercise the real CosmeticAI queue, not a Unit-queue projection.
#include "ActorQueueLifetimeNative.h"
#include "ScriptedCosmeticAI.h"
struct CosmeticOwnershipWitness : MS::AI::CosmeticAI
{
    explicit CosmeticOwnershipWitness(Creature* actor) : CosmeticAI(actor) { }
    std::weak_ptr<uint8> Queue()
    {
        auto token = std::make_shared<uint8>(0);
        std::weak_ptr<uint8> observer = token;
        AddDelayedEvent(123, [this, token] { Observe::Mark(me); });
        return observer;
    }
};
int main(int argc, char** argv)
{
    try
    {
        InitializeAltheaGuardsNative(argc, argv);
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
        MapInstanced parent(1600,300);
        InstanceMap map(1600,300,9999771,DIFFICULTY_N_SCENARIO,&parent);
        Actor actor(&map,299,11901,{52,803,805,0});
        Require(map.AddToMap(static_cast<Creature*>(&actor)),"insert real ownership witness");
        Observe::Reset();
        auto live = new CosmeticOwnershipWitness(&actor);
        Require(actor.AIM_Initialize(live),"install real CosmeticAI");
        auto first = live->Queue();
        Require(!first.expired(),"AI queue retains the actual callback");
        PumpQueue(actor,1000);
        Require(!Observe::calls && !first.expired(),"Unit queue does not own or dispatch CosmeticAI's callback");
        live->UpdateAI(124);
        Require(Observe::calls==1 && first.expired(),"actual CosmeticAI queue dispatches and releases its callback");
        auto retired = new CosmeticOwnershipWitness(&actor);
        Require(actor.AIM_Initialize(retired),"replace initial AI");
        auto second = retired->Queue();
        uint32 unrelated = 0;
        actor.AddDelayedEvent(123,[&unrelated]{++unrelated;});
        Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"destroy CosmeticAI through real Creature dispatch");
        Require(second.expired(),"AI destruction destroys its queue and callback;there is no surviving raw-this closure");
        PumpQueue(actor,124);
        Require(Observe::calls==1 && unrelated==1,"Unit survives and dispatches its own separate queue");
        CloseAltheaNative();
        std::cout << "PASS real CosmeticAI ownership:AI-owned live dispatch/destructor cancellation,independent surviving Unit queue;delegated CosmeticAI events are not Unit-owned UAF cases\n";
        return 0;
    }
    catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
