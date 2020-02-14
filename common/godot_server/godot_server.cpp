/**
 * @file    godot_server.h
 * @brief   Defines functions for publishing to/from godot. Much of this code
 *          was taken from https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/server/async/websocket_server_async.cpp
 */

#include <godot_server.h>
#include <json.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <string>
#include <functional>
#include <algorithm>
#include <thread>
#include <memory>
#include <set>
#include <mutex>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;            // from <nlohman/json.hpp>

// Report a failure
void fail(boost::beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

class GODOT_SESSION : public std::enable_shared_from_this<GODOT_SESSION>
{
    std::shared_ptr<GODOT_SERVER> server_;
    websocket::stream<tcp::socket> ws_;
    net::strand<net::io_context::executor_type> strand_;
    beast::multi_buffer buffer_;
    std::function<void()> on_close_cb_; // on close callback set with set_on_close() method

public:
    // Currently echos back all recieved WebSocket messages
    
    // Take ownership of the socket
    explicit GODOT_SESSION(std::shared_ptr<GODOT_SERVER> server, tcp::socket&& socket)
          : server_(std::move(server)),
            ws_(std::move(socket)), 
            strand_(ws_.get_executor()),
            buffer_(),
            on_close_cb_()
    {
        // std::cout << "[ new client ]" << std::endl;
    }

    ~GODOT_SESSION() {
        // std::cout << "[ end client ]" << std::endl;
        if (on_close_cb_) on_close_cb_();
    }

    // start the asynchronous operation, with callback for onclose
    void run(std::function<void()> func) {
        // TODO(mike) timeouts
        // TODO(mike) decorator to change server of the handshake
        
        // accept the client handshake (bind_executor puts us on the right thread)
        // TODO this probably shouldn't fire if we can't open the connection or something
        on_close_cb_ = func;    
        ws_.async_accept(net::bind_executor(strand_, std::bind(
                        &GODOT_SESSION::on_accept, shared_from_this(), std::placeholders::_1)));
    }

    void run() { run(nullptr); }

    // accepting the client's handshake
    void on_accept(beast::error_code ec)
    {
        if(ec) return fail(ec, "accept");
        // read a message
        do_read();
    }

    void do_read() {
        // read a message into our buffer
        ws_.async_read(buffer_, net::bind_executor(strand_, std::bind(
                        &GODOT_SESSION::on_read, shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2)));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {

        // prevent compiler warnings
        boost::ignore_unused(bytes_transferred);

        if (ec == websocket::error::closed) return; // session closed
        if (ec == net::error::eof) return; // end of stream

        if (ec) {
            fail(ec, "read");
            // TODO(mike) need to delete this somehow???
            ws_.close("error"); // TODO(mike) maybe don't close on all errors?
            return;
        }

        // Echo the message TODO(mike) actually do something here
        std::cout << "RX\t" << beast::buffers(buffer_.data()) << std::endl;

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // wait for the next message
        do_read();
    }

    void send(const json& msg) {
        ws_.text(true);
        ws_.async_write(net::buffer(msg.dump()), net::bind_executor(strand_, std::bind(
                        &GODOT_SESSION::on_write, shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2)));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        std::cout << "TX\t" << beast::buffers(buffer_.data()) << std::endl;
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            fail(ec, "write");
            ws_.close("error"); // TODO(mike) maybe don't close on all errors?
            return;
        }

    }
};

void GODOT_SERVER::do_accept()
{
    acceptor_.async_accept(
        socket_, // puts us on the right thread
        std::bind(&GODOT_SERVER::on_accept, shared_from_this(), // handler
            std::placeholders::_1));
}

void GODOT_SERVER::on_close(GODOT_SESSION&& s) {
    return;
}

// called on a new connection
void GODOT_SERVER::on_accept(boost::beast::error_code ec) {
    if (ec)
        fail(ec, "accept");
    else
    {
        // create a session and run it

        auto sessionptr = std::make_shared<GODOT_SESSION>(
                shared_from_this(),
                std::move(socket_));

        std::lock_guard<std::mutex> lock(open_sessions_lock_);

        sessionptr->run();
                
        open_sessions_.push_back(sessionptr);
    }

    // accept another connection
    do_accept();
}

void GODOT_SERVER::Send(const json& msg) {
    for (auto sessionptr : open_sessions_) {
        if (!sessionptr.expired()) {
            auto sp = sessionptr.lock();
            sp->send(msg);
        }
    }
}

GODOT_SERVER::GODOT_SERVER( const std::string& ip, const unsigned short port ) : 
    outDir_(), ioc_(1), acceptor_(ioc_), socket_(ioc_), open_sessions_lock_(), open_sessions_() // TODO(mike) this socket feels weird
{
    auto const address = boost::asio::ip::make_address( ip );
    tcp::endpoint endpoint(address, port);
    
    // accept incoming connections and launch a session for each one.

    boost::beast::error_code ec;

    // open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        exit(1); // TODO(mike) fail more gracefully
    }

    // allow address reuse (?)
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        exit(1); // TODO(mike) fail more gracefully
    }

    // bind to server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        exit(1); // TODO(mike) fail more gracefully
    }

    // start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        exit(1); // TODO(mike) fail more gracefully
    }
}

void GODOT_SERVER::run_forever() {

    // start listening for the first connection
    // the callback for this calls do_accept again, completing the
    // reconnection loop
    do_accept();

    ioc_.run(); // TODO(mike) what does this do?
}
