// Actual QuestObjectScope header: cancellation survives releasing the scope,
// while a receipt never retains ordinary quest children or their owner scope.
#include "Common.h"
#include <map> // Native ObjectGuid aliases normally receive this from enclosing game headers.
#include "QuestObjectScope.h"
#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include "NativeGuidValues.inc"
static void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    try{
        std::shared_ptr<std::atomic<bool> const> ordinary;
        std::weak_ptr<QuestObjectScope> original;
        {
            auto scope=std::make_shared<QuestObjectScope>();original=scope;ordinary=scope->CancellationReceipt();
            Require(!ordinary->load(std::memory_order_acquire)&&!scope->Cancelled,"Fresh scope was cancelled");
        }
        Require(original.expired()&&!ordinary->load(std::memory_order_acquire),"Ordinary expiry kept scope alive or became cancellation");
        auto scope=std::make_shared<QuestObjectScope>();auto cancelled=scope->CancellationReceipt();
        ObjectGuid first,second;first.SetRawValue(1,101);second.SetRawValue(2,102); // Opaque distinct child identities.
        scope->Register(first);scope->Register(second);auto children=scope->Cancel();
        Require(children.size()==2&&children[0]==first&&children[1]==second&&scope->Cancelled&&cancelled->load(std::memory_order_acquire),"Explicit cancellation lost its children/receipt");
        Require(scope->Cancel().empty(),"Repeated cancellation returned children twice");
        original=scope;scope.reset();Require(original.expired()&&cancelled->load(std::memory_order_acquire),"Cancelled receipt retained scope or lost cancellation");
        scope=std::make_shared<QuestObjectScope>();auto next=scope->CancellationReceipt();
        Require(!next->load(std::memory_order_acquire)&&cancelled->load(std::memory_order_acquire),"New attempt reused cancelled receipt");
        original=scope;next.reset();Require(!original.expired()&&!scope->Cancelled,"Dropping a receipt cancelled the active scope");
        auto concurrent=scope->CancellationReceipt();
        std::thread worker([held=scope](){held->Cancel();});worker.join();scope.reset();
        Require(original.expired()&&concurrent->load(std::memory_order_acquire),"Concurrent cancellation/expiry was lost");
        scope=std::make_shared<QuestObjectScope>();auto empty=scope->CancellationReceipt();
        Require(scope->Cancel().empty()&&empty->load(std::memory_order_acquire),"Empty-scope cancellation was lost");
        std::cout<<"PASS8 actual QuestObjectScope cancellation/ordinary expiry/new attempt/no-retention/idempotence/registered children/concurrent lifetime cases; header ASAN, not an instrumented whole core.\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
