// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef SMARTDOOR_CONNECTION_HPP
#define SMARTDOOR_CONNECTION_HPP

#include <string>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include "smartdoor_handler.hpp"

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::connection_hdl connection_hdl;
typedef client::connection_ptr connection_ptr;

// The SmartDoorConnection is responsible for the WebSocket connection to
// standalone SmartDoor SUT.
class SmartDoorConnection {
public:
    SmartDoorConnection(std::string uri);
    ~SmartDoorConnection();

    void connect();
    void close(int code, std::string message);
    void send(std::string);
    void register_handler(SmartDoorHandler* handler_ptr);

private:
    void on_socket_init(connection_hdl hdl);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_fail(connection_hdl hdl);
    void on_message(connection_hdl hdl, message_ptr msg);

private:
    client m_endpoint;
    websocketpp::connection_hdl m_hdl;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    SmartDoorHandler* handler_ptr;
    std::string server_uri;
};

#endif // SMARTDOOR_CONNECTION_HPP
