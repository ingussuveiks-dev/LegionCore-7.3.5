// Compile the complete production CPP, public header and actual STL.
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include "FunctionProcessorActual.inc"

static void Require(bool ok,char const* text){if(!ok)throw std::runtime_error(text);}
struct Queue : FunctionProcessor { std::recursive_mutex& Mutex(){return m_queue_lock;} };

static void LockAdmission()
{
    Queue queue;
    std::atomic<bool> entering{false},done{false};
    bool escaped=false;unsigned callbacks=0;
    std::unique_lock<std::recursive_mutex> producer(queue.Mutex());
    std::thread consumer([&]{entering=true;queue.AddFunctionsFromQueue();done=true;});
    while(!entering.load())std::this_thread::yield();
    // The old empty fast path escapes the held producer lock. This bounded
    // probe complements the source/linked dominance check, not a race detector.
    auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(200);
    while(!done.load()&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
    escaped=done.load();
    queue.AddDelayedEvent(0,[&]{++callbacks;});
    producer.unlock();consumer.join();queue.Update(0);
    Require(!escaped,"empty queue admission escaped producer mutex");
    Require(callbacks==1,"pending handoff callback lost");
}

static void ConcurrentHandoff()
{
    constexpr unsigned producers=4,perProducer=2500;
    for(unsigned round=0;round<4;++round)
    {
        FunctionProcessor queue;
        std::array<unsigned,producers> next{};
        std::vector<unsigned> seen(producers*perProducer,0);
        std::atomic<unsigned> completedProducers{0};
        std::atomic<bool> start{false};
        unsigned executed=0;bool ordered=true;
        std::vector<std::thread> workers;
        for(unsigned p=0;p<producers;++p)workers.emplace_back([&,p]{
            while(!start.load())std::this_thread::yield();
            for(unsigned i=0;i<perProducer;++i)
            {
                queue.AddDelayedEvent(0,[&,p,i]{
                    ++seen[p*perProducer+i];++executed;
                    if(next[p]++!=i)ordered=false;
                });
                if(i%31==0)std::this_thread::yield();
            }
            ++completedProducers;
        });
        start=true;
        // Only this consumer updates the active map. Producers touch only
        // the locked pending queue and the atomic clock, matching live use.
        while(completedProducers.load()!=producers){queue.Update(0);std::this_thread::yield();}
        for(auto& worker:workers)worker.join();
        queue.Update(0);queue.Update(0);
        Require(executed==producers*perProducer,"concurrent handoff lost callbacks");
        for(auto n:seen)Require(n==1,"handoff callback not exactly once");
        Require(ordered,"same-producer order changed");
        Require(queue.Empty()&&queue.SizeQueue()==0,"drained queue retains work");
    }
}

static void CancellationBoundary()
{
    FunctionProcessor queue;unsigned old=0,fresh=0;
    std::thread before([&]{for(unsigned i=0;i<1000;++i)queue.AddDelayedEvent(0,[&]{++old;});});
    before.join();queue.KillAllFunctions();
    std::thread after([&]{for(unsigned i=0;i<1000;++i)queue.AddDelayedEvent(0,[&]{++fresh;});});
    after.join();queue.Update(0);queue.Update(0);
    Require(old==0&&fresh==1000,"producer cancellation boundary merged generations");
}

int main(int argc,char** argv)
{
    try
    {
        Require(argc==2,"case required");std::string name=argv[1];
        if(name=="lock")LockAdmission();
        else if(name=="stress")ConcurrentHandoff();
        else if(name=="cancel")CancellationBoundary();
        else throw std::runtime_error("unknown case");
        std::cout<<"PASS actual FunctionProcessor handoff "<<name<<'\n';return 0;
    }
    catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}
}
