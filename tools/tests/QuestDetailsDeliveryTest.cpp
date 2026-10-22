#include <cassert>
#include <iostream>
#include <vector>
struct Packet{int writes=0;std::vector<int> bytes;std::vector<int> const* Write(){++writes;bytes.push_back(42);return &bytes;}};
struct Session{std::vector<std::vector<int>> sent;void SendPacket(std::vector<int> const* p){sent.push_back(*p);}};
struct Player{Session* session;void SendDirectMessage(std::vector<int> const* p){session->SendPacket(p);}};
void Send(Session* _session,Player* player,Packet& packet){(void)_session;
#include "Delivery.inc"
}
int main(){Session session;Player player{&session};Packet packet;Send(&session,&player,packet);
 assert(packet.writes==1&&session.sent.size()==1&&session.sent.front()==std::vector<int>{42});
 std::cout<<"PASS: production quest-details dispatch serializes once and sends one unappended packet\n";
}
