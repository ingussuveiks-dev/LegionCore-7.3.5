#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>
using uint32=uint32_t; using uint64=uint64_t;
#define TC_LOG_WARN(...) ((void)0)
#define TC_LOG_INFO(...) ((void)0)
constexpr int CONNECTION_TYPE_REALM=0, CONNECTION_TYPE_INSTANCE=1, ERROR_TIMED_OUT=1;
namespace WorldPackets {
namespace Auth {
enum class ConnectToSerial:uint32 { WorldAttempt1=17,WorldAttempt2=35,WorldAttempt3=53,WorldAttempt4=71,WorldAttempt5=89 };
struct ConnectToFailed {ConnectToSerial Serial;};
}
namespace Character {enum class LoginFailureReason {NoWorld};}
}
using Serial=WorldPackets::Auth::ConnectToSerial;
struct WorldSession;
using WorldSessionPtr=std::shared_ptr<WorldSession>;
struct WorldSocket {
    bool open=true, delayedClose=false, error=false, associated=false;
    WorldSessionPtr _worldSession;
    bool IsOpen()const{return open;}
    void SendAuthResponseError(int){error=true;}
    void DelayedCloseSocket(){delayedClose=true;}
    void SetWorldSession(WorldSessionPtr s){_worldSession=s;associated=true;}
    void HandleConnectToFailed(WorldPackets::Auth::ConnectToFailed&);
};
struct WorldSession {
    union ConnectToKey {uint64 Raw;struct{uint32 AccountId;uint32 Key;}Fields;};
    ConnectToKey _instanceConnectKey{1};
    uint32 _instanceConnectSerial=0;
    bool loading=true,player=false,aborted=false,forceExit=false,continued=false;
    uint64 now=0;
    std::multimap<uint64,std::function<void()>> events;
    std::vector<Serial> sent;
    std::shared_ptr<WorldSocket> m_Socket[2]{std::make_shared<WorldSocket>(),nullptr};
    bool PlayerLoading()const{return loading;}
    bool GetPlayer()const{return player;}
    uint32 GetAccountId()const{return 1;}
    uint64 GetConnectToInstanceKey()const{return _instanceConnectKey.Raw;}
    void SetforceExit(){forceExit=true;}
    bool IsforceExit()const{return forceExit;}
    void AddInstanceConnection(std::shared_ptr<WorldSocket> s){m_Socket[1]=s;}
    void HandleContinuePlayerLogin(){continued=true;}
    bool HasInstanceConnection()const;
    void RetryInstanceConnection(Serial);
    void AbortLogin(WorldPackets::Character::LoginFailureReason){aborted=true;loading=false;}
    void AddDelayedEvent(uint64 ms,std::function<void()>&& f){events.emplace(now+ms,std::move(f));}
    void Arm(Serial serial){
#include "InstanceConnectDeadline.inc"
    }
    void SendConnectToInstance(Serial serial){
        sent.push_back(serial);_instanceConnectSerial=uint32(serial);
        _instanceConnectKey.Raw+=uint64(1)<<32;Arm(serial);
    }
    void Advance(uint64 ms){
        now+=ms;
        while(!events.empty() && events.begin()->first<=now){
            auto f=events.begin()->second;events.erase(events.begin());f();
        }
    }
};
#include "HasInstanceConnection.inc"
#include "RetryInstanceConnection.inc"
#include "ClientConnectFailed.inc"
struct World {
    WorldSessionPtr session;
    WorldSessionPtr FindSession(uint32 id){return id==1?session:nullptr;}
    void ProcessLinkInstanceSocket(std::pair<std::weak_ptr<WorldSocket>,uint64>);
};
#include "LinkInstanceSocket.inc"
int main(){
    WorldSession timeout;timeout.SendConnectToInstance(Serial::WorldAttempt1);
    timeout.Advance(11999);assert(timeout.sent.size()==1);
    timeout.Advance(1);assert(timeout.sent.back()==Serial::WorldAttempt2);
    for(int i=0;i<3;++i)timeout.Advance(12000);
    assert(timeout.sent.size()==5 && !timeout.aborted);
    timeout.Advance(12000);assert(timeout.aborted && !timeout.loading);
    timeout.Advance(120000);assert(timeout.sent.size()==5);

    auto s=std::make_shared<WorldSession>();WorldSocket realm;realm._worldSession=s;
    s->SendConnectToInstance(Serial::WorldAttempt1);
    WorldPackets::Auth::ConnectToFailed failed{Serial::WorldAttempt1};
    realm.HandleConnectToFailed(failed);assert(s->sent.size()==1); // network thread only queues
    s->Advance(1);assert(s->sent.back()==Serial::WorldAttempt2);
    realm.HandleConnectToFailed(failed);s->Advance(1);assert(s->sent.size()==2);
    s->Advance(11998);assert(s->sent.size()==2); // stale attempt-1 deadline ignored
    s->m_Socket[1]=std::make_shared<WorldSocket>();s->Advance(1);assert(s->sent.size()==2);
    s->Advance(120000);assert(!s->aborted);

    for(int mode=0;mode<3;++mode){
        WorldSession cancelled;cancelled.SendConnectToInstance(Serial::WorldAttempt1);
        if(mode==0)cancelled.loading=false;
        if(mode==1)cancelled.player=true;
        if(mode==2)cancelled.m_Socket[0]->open=false;
        cancelled.Advance(12000);assert(cancelled.sent.size()==1 && !cancelled.aborted);
    }

    World world;world.session=std::make_shared<WorldSession>();
    world.session->SendConnectToInstance(Serial::WorldAttempt1);
    uint64 oldKey=world.session->GetConnectToInstanceKey();
    world.session->Advance(12000);
    auto stale=std::make_shared<WorldSocket>();world.ProcessLinkInstanceSocket({stale,oldKey});
    assert(stale->delayedClose && !stale->associated && !world.session->forceExit);
    auto valid=std::make_shared<WorldSocket>();
    world.ProcessLinkInstanceSocket({valid,world.session->GetConnectToInstanceKey()});
    assert(valid->associated && world.session->continued);
    auto duplicate=std::make_shared<WorldSocket>();
    world.ProcessLinkInstanceSocket({duplicate,world.session->GetConnectToInstanceKey()});
    assert(duplicate->delayedClose && !duplicate->associated && !world.session->forceExit);
    assert(world.session->m_Socket[1]==valid);
    world.session->Advance(12000);assert(world.session->sent.size()==2 && !world.session->aborted);
    auto unknown=std::make_shared<WorldSocket>();world.ProcessLinkInstanceSocket({unknown,99});assert(unknown->delayedClose);
    auto closed=std::make_shared<WorldSocket>();closed->open=false;world.ProcessLinkInstanceSocket({closed,oldKey});assert(!closed->associated);
    World exiting;exiting.session=std::make_shared<WorldSession>();exiting.session->forceExit=true;
    auto late=std::make_shared<WorldSocket>();exiting.ProcessLinkInstanceSocket({late,exiting.session->GetConnectToInstanceKey()});
    assert(late->delayedClose&&!late->associated&&!exiting.session->continued);
    // Break test-only shared ownership cycles.
    valid->_worldSession.reset();
    std::cout<<"PASS: silent-client deadlines, five-attempt bound, queued failures, stale reports, successful/cancelled login and stale/duplicate authenticated sockets\n";
}
