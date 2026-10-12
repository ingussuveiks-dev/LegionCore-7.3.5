#include <boost/asio.hpp>
using boost::asio::ip::tcp;
#include "Socket.h"
#include "Timer.h"
#include <iostream>

struct HandshakeSocket : Socket<HandshakeSocket>
{
    using Socket::Socket;
    bool _authed = false;
    uint32 _initializationStart = getMSTime();
    uint32 _type = 0;
    void* _compressionStream = nullptr;
    uint64 _key = 0;
    MessageBuffer _packetBuffer;
    bool* readFinished = nullptr;
    boost::system::error_code* readError = nullptr;
    void Start() override { AsyncReadWithCallback(&HandshakeSocket::ReadDone); }
    void ReadHandler() override { }
    void ReadDone(boost::system::error_code const& error, size_t)
    {
        *readFinished = true;
        *readError = error;
    }
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
        boost::system::error_code error;
        server->readFinished = &completed;
        server->readError = &error;
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
            // A pending read owns the server through shared_from_this. It must
            // finish without requiring the silent peer to close its own socket.
            std::weak_ptr<HandshakeSocket> weak = server;
            server.reset();
            context.run_for(std::chrono::milliseconds(250));
            if (!completed || !error || !weak.expired())
            {
                std::cerr << "FAIL: handshake read/lifetime did not terminate\n";
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
        }
    }
    std::cout << "PASS: real loopback timeout closes the native socket and cancels its pending read; authenticated and younger sockets survive.\n";
}
