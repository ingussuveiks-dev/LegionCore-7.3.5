#include <boost/asio.hpp>
using boost::asio::ip::tcp;
#include "Socket.h"
#include "Timer.h"
#include <iostream>
#include <mutex>

struct TestWorldSession
{
    uint32 GetAccountId() const { return 2; }
    std::string GetPlayerName() const { return "fixture"; }
};
using WorldSessionPtr = std::shared_ptr<TestWorldSession>;

struct HandshakeSocket : Socket<HandshakeSocket>
{
    using Socket::Socket;
    bool _authed = false;
    uint32 _initializationStart = getMSTime();
    uint32 _type = 0;
    void* _compressionStream = nullptr;
    uint64 _key = 0;
    uint32 _accountId = 0;
    std::mutex _worldSessionLock;
    WorldSessionPtr _worldSession;
    enum class ReadDataHandlerResult { Ok, Error };
    ReadDataHandlerResult CheckDuplicateAuth()
    {
        std::unique_lock<std::mutex> sessionGuard(_worldSessionLock, std::defer_lock);
#include "DuplicateAUTH_SESSION.inc"
        return ReadDataHandlerResult::Ok;
    }
    ReadDataHandlerResult CheckDuplicateContinuedAuth()
    {
        std::unique_lock<std::mutex> sessionGuard(_worldSessionLock, std::defer_lock);
#include "DuplicateAUTH_CONTINUED_SESSION.inc"
        return ReadDataHandlerResult::Ok;
    }
    void SetWorldSession(WorldSessionPtr session)
    {
#include "SetWorldSession.inc"
    }
    uint32 stateLogs = 0;
    uint32* externalStateLogs = nullptr;
    MessageBuffer _packetBuffer;
    bool* readFinished = nullptr;
    boost::system::error_code* readError = nullptr;
    void Start() override { AsyncReadWithCallback(&HandshakeSocket::ReadDone); }
    void ReadHandler() override { }
    void ReadDone(boost::system::error_code const& error, size_t)
    {
        *readFinished = true;
        *readError = error;
#include "InitializerError.inc"
    }
    void LogInitializationState(char const*) { ++stateLogs; if (externalStateLogs) ++*externalStateLogs; }
    bool Update() override
    {
#include "HandshakeDeadline.inc"
        return true;
    }
    bool NativeOpen() { return underlying_stream().is_open(); }
};

int main()
{
    for (int mode = 0; mode != 3; ++mode)
    {
        boost::asio::io_context context;
        tcp::acceptor acceptor(context, {boost::asio::ip::address_v4::loopback(), 0});
        tcp::socket client(context);
        client.connect(acceptor.local_endpoint());
        tcp::socket accepted(context);
        acceptor.accept(accepted);
        auto server = std::make_shared<HandshakeSocket>(std::move(accepted));
        bool completed = false;
        uint32 stateLogs = 0;
        boost::system::error_code error;
        server->readFinished = &completed;
        server->readError = &error;
        server->externalStateLogs = &stateLogs;
        server->Start();
        server->_authed = mode == 1;
        if (mode != 2)
            server->_initializationStart = getMSTime() - 10001;
        bool keep = server->Update();
        if (mode == 0)
        {
            if (keep || server->IsOpen() || server->NativeOpen())
            {
                std::cerr << "FAIL: expired handshake still owns an open TCP socket\n";
                return 1;
            }
            if (server->stateLogs != 1) return 5;
            // A pending read owns the server through shared_from_this. It must
            // finish without requiring the silent peer to close its own socket.
            std::weak_ptr<HandshakeSocket> weak = server;
            server.reset();
            context.run_for(std::chrono::milliseconds(250));
            if (!completed || !error || !weak.expired() || stateLogs != 1)
            {
                std::cerr << "FAIL: handshake cleanup completed=" << completed << " error=" << error.value()
                    << " category=" << error.category().name() << " expired=" << weak.expired() << " stateLogs=" << stateLogs << '\n';
                return 2;
            }
            char byte;
            client.non_blocking(true);
            client.read_some(boost::asio::buffer(&byte, 1), error);
            if (error != boost::asio::error::connection_reset)
            {
                std::cerr << "FAIL: silent peer did not receive a TCP reset: " << error.message() << '\n';
                return 4;
            }
        }
        else
        {
            if (!keep || !server->IsOpen() || !server->NativeOpen())
                return 3;
            client.close();
            context.run_for(std::chrono::milliseconds(250));
            if (stateLogs != 1) return 6; // An open peer's EOF is still reported.
        }
    }
    for (int mode = 0; mode != 2; ++mode)
    {
        boost::asio::io_context context;
        tcp::acceptor acceptor(context, {boost::asio::ip::address_v4::loopback(), 0});
        tcp::socket client(context);
        client.connect(acceptor.local_endpoint());
        tcp::socket accepted(context);
        acceptor.accept(accepted);
        auto server = std::make_shared<HandshakeSocket>(std::move(accepted));
        server->_initializationStart = getMSTime() - 60000;
        if (mode == 0)
        {
            server->SetWorldSession(std::make_shared<TestWorldSession>());
            if (!server->_authed || server->_accountId != 2) return 7;
            if (server->CheckDuplicateAuth() != HandshakeSocket::ReadDataHandlerResult::Error ||
                server->CheckDuplicateContinuedAuth() != HandshakeSocket::ReadDataHandlerResult::Error) return 11;
            server->SetWorldSession(nullptr);
            if (server->_worldSession || server->_accountId != 0 || !server->_authed)
            {
                std::cerr << "FAIL: session detach reset a completed socket authentication\n";
                return 8;
            }
            if (server->CheckDuplicateAuth() != HandshakeSocket::ReadDataHandlerResult::Error ||
                server->CheckDuplicateContinuedAuth() != HandshakeSocket::ReadDataHandlerResult::Error) return 12;
            if (!server->Update() || server->stateLogs)
            {
                std::cerr << "FAIL: detached authenticated socket restarted its handshake deadline\n";
                return 9;
            }
        }
        server->CloseSocket();
        if (server->Update() || server->stateLogs)
        {
            std::cerr << "FAIL: already closed socket was processed as an expired handshake\n";
            return 10;
        }
    }
    std::cout << "PASS: real loopback handshake timeout/cancellation, ordinary EOF, authenticated/young sockets, session detach and already closed socket cleanup.\n";
}
