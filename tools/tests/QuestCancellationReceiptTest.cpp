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
        std::shared_ptr<QuestCancellationReceipt const> ordinary;
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
        scope=std::make_shared<QuestObjectScope>();auto canonical=scope->CancellationReceipt();original=scope;
        auto reentered=std::make_shared<QuestObjectScope>();auto local=reentered->CancellationReceipt();
        Require(reentered->AdoptCancellationReceipt(canonical)&&reentered->CancellationReceipt()==canonical,"Reentry did not retain canonical attempt identity");
        scope.reset();Require(original.expired(),"Adoption retained the old quest scope");
        reentered->Cancel();Require(canonical->load()&&local->load(),"Adopted or previously handed-out receipt missed cancellation");

        scope=std::make_shared<QuestObjectScope>();canonical=scope->CancellationReceipt();
        std::vector<std::shared_ptr<QuestCancellationReceipt const>> views{canonical};
        for(uint32 i=0;i<3;++i){auto nextScope=std::make_shared<QuestObjectScope>();views.push_back(nextScope->CancellationReceipt());
            Require(nextScope->AdoptCancellationReceipt(canonical),"Repeated ordinary reentry refused active attempt");scope=nextScope;}
        scope->Cancel();for(auto const& view:views)Require(view->load(),"Multiple reconnects lost a receipt reader");

        auto fresh=std::make_shared<QuestObjectScope>();auto freshView=fresh->CancellationReceipt();
        Require(!fresh->AdoptCancellationReceipt(canonical)&&!freshView->load(),"Cancelled old run poisoned a fresh task");
        auto other=std::make_shared<QuestObjectScope>();auto otherView=other->CancellationReceipt();fresh->Cancel();
        Require(!fresh->AdoptCancellationReceipt(otherView)&&!otherView->load(),"Already-cancelled scope adopted another active attempt");

        for(uint32 i=0;i<128;++i){auto previous=std::make_shared<QuestObjectScope>();auto receipt=previous->CancellationReceipt();
            auto current=std::make_shared<QuestObjectScope>();auto before=current->CancellationReceipt();
            std::thread cancel([previous](){previous->Cancel();});current->AdoptCancellationReceipt(receipt);cancel.join();
            Require(receipt->load(),"Concurrent old cancellation lost its flag");
            if(current->CancellationReceipt()==receipt)Require(before->load(),"Concurrent adoption lost a mirrored reader");
            current->Cancel();Require(before->load(),"Current cancellation failed after adoption race");}

        views.clear();scope=std::make_shared<QuestObjectScope>();views.push_back(scope->CancellationReceipt());
        for(uint32 i=0;i<1000;++i){auto nextScope=std::make_shared<QuestObjectScope>();auto receipt=nextScope->CancellationReceipt();
            Require(scope->AdoptCancellationReceipt(receipt),"Deep mirror preparation failed");views.push_back(receipt);scope=nextScope;}
        scope->Cancel();for(auto const& view:views)Require(view->load(),"Iterative cancellation lost a deep mirror");
        auto left=std::make_shared<QuestObjectScope>(),right=std::make_shared<QuestObjectScope>();
        auto leftView=left->CancellationReceipt(),rightView=right->CancellationReceipt();
        Require(left->AdoptCancellationReceipt(rightView)&&right->AdoptCancellationReceipt(leftView),"Mirror cycle setup failed");
        left->Cancel();Require(leftView->load()&&rightView->load(),"Mirror cycle failed to cancel finitely");
        std::cout<<"PASS13 actual QuestObjectScope cancellation/ordinary expiry/new attempt/no-retention/idempotence/children/concurrency plus canonical adoption, multiple reconnect mirrors, refusals and iterative deep/cyclic cancellation; header ASAN, not an instrumented whole core.\n";return 0;
    }catch(std::exception const& error){std::cerr<<error.what()<<'\n';return 1;}
}
