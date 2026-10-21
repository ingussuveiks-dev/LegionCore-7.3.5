#include "WorldSocketDiagnostics.h"
#ifdef _WIN32
#include <mstcpip.h>
#endif
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>
#ifdef _WIN32
using WorldSocketDiagnostics::TcpInfo;
static_assert(sizeof(TcpInfo)==sizeof(TCP_INFO_v0));
#define CHECK_OFFSET(field) static_assert(offsetof(TcpInfo,field)==offsetof(TCP_INFO_v0,field))
CHECK_OFFSET(State);CHECK_OFFSET(ConnectionTimeMs);CHECK_OFFSET(TimestampsEnabled);
CHECK_OFFSET(RttUs);CHECK_OFFSET(BytesInFlight);CHECK_OFFSET(SndWnd);CHECK_OFFSET(RcvWnd);
CHECK_OFFSET(BytesOut);CHECK_OFFSET(BytesIn);CHECK_OFFSET(BytesRetrans);CHECK_OFFSET(SynRetrans);
#endif
int main(){
#ifdef _WIN32
 using boost::asio::ip::tcp;
 boost::asio::io_context context;
 tcp::acceptor acceptor(context,{boost::asio::ip::address_v4::loopback(),0});
 tcp::socket peer(context);peer.connect(acceptor.local_endpoint());
 tcp::socket server(context);acceptor.accept(server);
 std::string greeting="WORLD OF WARCRAFT CONNECTION - SERVER TO CLIENT\n";
 auto sent=server.send(boost::asio::buffer(greeting));
 char bytes[128];auto received=peer.receive(boost::asio::buffer(bytes));
 if(sent!=greeting.size()||received!=sent)return 1;
 TcpInfo info{};int error=0;bool acknowledged=false;
 for(int i=0;i<100;++i){
  if(!WorldSocketDiagnostics::Query(server,info,error)){std::cerr<<"TCP query failed: "<<error<<'\n';return 2;}
  if(info.BytesOut>=sent&&info.BytesIn==0&&info.BytesInFlight==0){acknowledged=true;break;}
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 if(!acknowledged)return 3;
 std::cout<<"PASS: native TCP reports sent "<<info.BytesOut<<", received "<<info.BytesIn<<", unacknowledged "<<info.BytesInFlight<<" before client initializer\n";
 peer.send(boost::asio::buffer(greeting));server.receive(boost::asio::buffer(bytes));
 if(!WorldSocketDiagnostics::Query(server,info,error)||info.BytesIn<sent)return 4;
 server.close();if(WorldSocketDiagnostics::Query(server,info,error))return 5;
 std::cout<<"PASS: SDK ABI matches, peer response changes native receive count, and a closed socket query fails safely\n";
#endif
}
