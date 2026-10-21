#ifndef WorldSocketDiagnostics_h__
#define WorldSocketDiagnostics_h__

#include <boost/asio/ip/tcp.hpp>
#include <cstdint>

namespace WorldSocketDiagnostics
{
// TCP_INFO_v0 ABI from mstcpip.h. The server targets Windows 7, whose SDK
// feature guards hide this Windows 10 structure. Querying the optional IOCTL
// at runtime keeps that target and reports an unavailable API without failing
// a connection. The native test compares layout with the SDK declaration.
struct TcpInfo
{
    std::uint32_t State, Mss;
    std::uint64_t ConnectionTimeMs;
    std::uint8_t TimestampsEnabled;
    std::uint32_t RttUs, MinRttUs, BytesInFlight, Cwnd, SndWnd, RcvWnd, RcvBuf;
    std::uint64_t BytesOut, BytesIn;
    std::uint32_t BytesReordered, BytesRetrans, FastRetrans, DupAcksIn, TimeoutEpisodes;
    std::uint8_t SynRetrans;
};

inline bool Query(boost::asio::ip::tcp::socket& socket, TcpInfo& info, int& error)
{
#ifdef _WIN32
    std::uint32_t version = 0;
    DWORD returned = 0;
    int result = WSAIoctl(socket.native_handle(), _WSAIORW(IOC_VENDOR, 39),
        &version, sizeof(version), &info, sizeof(info), &returned, nullptr, nullptr);
    error = result == SOCKET_ERROR ? WSAGetLastError() : 0;
    return result == 0 && returned >= sizeof(info);
#else
    (void)socket;
    (void)info;
    error = 0;
    return false;
#endif
}
}
#endif
