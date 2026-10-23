// Real native Socket/MessageBuffer and QueryCallback/AsyncCallbackProcessor;
// database results are controlled futures, not a live account/database session.
#include <boost/asio.hpp>
#include "Socket.h"
#include "QueryCallback.h"
#include "AsyncCallbackProcessor.h"
#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>
using boost::asio::ip::tcp;
using namespace std::chrono_literals;
struct FakeStatement {void setString(int,std::string const&) {}};
constexpr int LOGIN_SEL_IP_INFO=0;
struct FakeDatabase {
 FakeStatement stmt;
 unsigned syncCalls=0;
 std::vector<std::shared_ptr<std::promise<PreparedQueryResult>>> pending;
 FakeStatement* GetPreparedStatement(int){return &stmt;}
 PreparedQueryResult Query(FakeStatement*){++syncCalls;return {};}
 QueryCallback AsyncQuery(FakeStatement*){
  auto p=std::make_shared<std::promise<PreparedQueryResult>>();pending.push_back(p);return QueryCallback(p->get_future());
 }
 void Complete(bool banned){
  PreparedQueryResult result;
  // An opaque non-null result is sufficient: the production query returns ban
  // rows, not TrinityCore's differently shaped country/ban-status projection.
  if(banned)result=PreparedQueryResult(reinterpret_cast<PreparedResultSet*>(new int(1)),[](PreparedResultSet* p){delete reinterpret_cast<int*>(p);});
  pending.back()->set_value(result);
 }
} LoginDatabase;
struct WorldSocket:Socket<WorldSocket> {
 using Socket::Socket;
 static std::string const ServerConnectionInitialize,ClientConnectionInitialize;
 QueryCallbackProcessor _queryProcessor;
 MessageBuffer _packetBuffer;
 unsigned reads=0,writes=0,logs=0;
 std::thread::id readThread,writeThread;
 std::promise<void> received;
 void LogInitializationState(char const*){++logs;}
 void Start()override;
 void CheckIpCallback(PreparedQueryResult);
 void ReadHandler()override{}
 void InitializeHandler(boost::system::error_code const& error,std::size_t bytes){
  if(error)return;
  auto& data=GetReadBuffer();data.WriteCompleted(bytes);
  if(data.GetActiveSize()<ClientConnectionInitialize.size()+1){Socket::AsyncReadWithCallback(&WorldSocket::InitializeHandler);return;}
  assert(std::string(reinterpret_cast<char const*>(data.GetReadPointer()),data.GetActiveSize())==ClientConnectionInitialize+"\n");
  received.set_value();
 }
 void AsyncReadWithCallback(void(WorldSocket::*callback)(boost::system::error_code const&,std::size_t)){
  ++reads;readThread=std::this_thread::get_id();Socket::AsyncReadWithCallback(callback);
 }
 void QueuePacket(MessageBuffer&& buffer){++writes;writeThread=std::this_thread::get_id();Socket::QueuePacket(std::move(buffer));}
};
std::string const WorldSocket::ServerConnectionInitialize="WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT";
std::string const WorldSocket::ClientConnectionInitialize="WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER";
#define LoginDatabasePreparedStatement FakeStatement
#include "Start.inc"
#ifndef EXPECT_BASELINE
#include "CheckIp.inc"
#else
void WorldSocket::CheckIpCallback(PreparedQueryResult){}
#endif
#undef LoginDatabasePreparedStatement

static void Run(int mode)
{
 LoginDatabase.syncCalls=0;LoginDatabase.pending.clear();
 boost::asio::io_context network,clientContext;
 auto work=boost::asio::make_work_guard(network);
 tcp::acceptor acceptor(clientContext,{boost::asio::ip::address_v4::loopback(),0});
 tcp::socket client(clientContext);client.connect(acceptor.local_endpoint());
 tcp::socket accepted(network);acceptor.accept(accepted);
 auto server=std::make_shared<WorldSocket>(std::move(accepted));
 auto received=server->received.get_future();
 std::thread worker([&]{network.run();});
 auto pump=[&]{std::promise<void> done;auto ready=done.get_future();boost::asio::post(network,[&]{server->_queryProcessor.ProcessReadyCallbacks();done.set_value();});assert(ready.wait_for(2s)==std::future_status::ready);};
 server->Start(); // Same caller as SocketMgr: an acceptor thread, not worker.
#ifdef EXPECT_BASELINE
 assert(LoginDatabase.syncCalls==1&&server->reads==1&&server->writes==1);
 assert(server->readThread==std::this_thread::get_id()&&server->writeThread!=worker.get_id());
#else
 assert(LoginDatabase.syncCalls==0&&LoginDatabase.pending.size()==1&&server->reads==0&&server->writes==0);
 pump();assert(server->reads==0&&server->writes==0); // Not-yet-ready DB result.
 if(mode==2)server->AbortSocket();
 LoginDatabase.Complete(mode==1);pump();pump();
 if(mode){assert(!server->IsOpen()&&server->reads==0&&server->writes==0);}
 else{
  assert(server->reads==1&&server->writes==1);
  assert(server->readThread==worker.get_id()&&server->writeThread==worker.get_id());
 }
#endif
 if(mode==0){
  client.non_blocking(true);std::string greeting;auto until=std::chrono::steady_clock::now()+2s;
  while(greeting.size()<48&&std::chrono::steady_clock::now()<until){char bytes[48];boost::system::error_code error;auto n=client.read_some(boost::asio::buffer(bytes,48-greeting.size()),error);if(n)greeting.append(bytes,n);if(error&&error!=boost::asio::error::would_block&&error!=boost::asio::error::try_again)assert(false);std::this_thread::yield();}
  assert(greeting==WorldSocket::ServerConnectionInitialize+"\n");
  client.non_blocking(false);boost::asio::write(client,boost::asio::buffer(WorldSocket::ClientConnectionInitialize+"\n"));
  assert(received.wait_for(2s)==std::future_status::ready);
 }
 boost::system::error_code ignored;client.close(ignored);
 std::promise<void> closed;auto finished=closed.get_future();boost::asio::post(network,[&]{server->AbortSocket();closed.set_value();});assert(finished.wait_for(2s)==std::future_status::ready);
 work.reset();worker.join();
}
int main(){
#ifdef EXPECT_BASELINE
 Run(0);std::cout<<"PASS baseline reproduction: production Start issues native read/write on acceptor thread; 48-byte loopback exchange can still succeed, so this does not reproduce the user's intermittent silence.\n";
#else
 for(int i=0;i<16;++i)Run(0);Run(1);Run(2);
 std::cout<<"PASS: production startup defers native read/write to socket worker via real ready-future dispatcher; 16 exact 48-byte loopback exchanges, pending query, ban and close-before-callback gates. Original client silence is not reproduced by this fixture.\n";
#endif
}
