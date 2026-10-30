// MSVC data-member address calculation from the current production headers.
// No Player/WorldSession construction, database access or runtime mutation.
#include <unordered_set>
#include "Player.h"
#include "WorldSession.h"
#include <cstdint>
#include <iostream>
template<class Tag, typename Tag::Type Member> struct AbiAccess {
    friend typename Tag::Type Access(Tag) { return Member; }
};
struct FarTag { using Type=bool Player::*; friend Type Access(FarTag); };
template struct AbiAccess<FarTag,&Player::mSemaphoreTeleport_Far>;
struct SessionTag { using Type=WorldSession* Player::*; friend Type Access(SessionTag); };
template struct AbiAccess<SessionTag,&Player::m_session>;
struct LogoutTag { using Type=bool WorldSession::*; friend Type Access(LogoutTag); };
template struct AbiAccess<LogoutTag,&WorldSession::m_playerLogout>;
template<class C,class M> std::uintptr_t MemberOffset(M C::*member) {
    // Only form a member address under the target compiler's nonvirtual data
    // member ABI. Never dereference or construct the sentinel object.
    auto base=reinterpret_cast<C*>(std::uintptr_t{0x100000});
    return reinterpret_cast<std::uintptr_t>(&(base->*member))-reinterpret_cast<std::uintptr_t>(base);
}
int main() {
    std::cout<<"LOGOUT_ABI far="<<MemberOffset(Access(FarTag{}))
             <<" session="<<MemberOffset(Access(SessionTag{}))
             <<" logout="<<MemberOffset(Access(LogoutTag{}))<<'\n';
}
