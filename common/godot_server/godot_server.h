/**
 * @file    godot_server.h
 * @brief   Defines functions for publishing to/from godot
 */

#ifndef _GODOT_SERVER_H
#define _GODOT_SERVER_H

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <set>
#include <mutex>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <json.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;            // from <nlohman/json.hpp>

class GODOT_SESSION;

class GODOT_SERVER : public std::enable_shared_from_this<GODOT_SERVER>
{
public:
    /**
     * Establish a connection
     *
     * @param address the ip address
     * @param port the port
     */
    GODOT_SERVER( const std::string& address, const unsigned short port );

    /**
     * Get the current output directory (i.e. last received output directory
     * from Godot). If we haven't gotten one, returns empty string.
     */
    std::string GetGLTFOutputDir();

    /**
     * Send file-ready message
     */
    void Send( const json& msg );

    void run_forever();

private :

    std::string outDir_;
    net::io_context ioc_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::mutex open_sessions_lock_;
    std::vector<std::weak_ptr<GODOT_SESSION>> open_sessions_;

    // (async) Attempt handshake with client. Calls on_accept on success
    void do_accept();

    // (async) Callback for connection established. Calls do_accept when done.
    void on_accept( boost::beast::error_code ec);

    // (async) Called when a session closes
    void on_close(GODOT_SESSION&& s);
};

#endif // GODOT_SERVER_H
